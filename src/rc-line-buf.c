/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-line-buf.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>
#include "rc-line-buf.h"

#include "rc-marshal.h"
#include "rc-debug.h"
#include "rc-line-buf-private.h"

#define BUF_SIZE 1023

static void rc_line_buf_class_init (RCLineBufClass *klass);
static void rc_line_buf_init       (RCLineBuf *obj);

static GObjectClass *parent_class;

enum SIGNALS {
    READ_LINE,
    READ_DONE,
    LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0 };

/* Object argument ID's */
enum {
    ARG_0,
    ARG_FD,
};

GType
rc_line_buf_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCLineBufClass),
            NULL, NULL,
            (GClassInitFunc) rc_line_buf_class_init,            
            NULL, NULL,
            sizeof (RCLineBuf),
            0,
            (GInstanceInitFunc) rc_line_buf_init
        };

        type = g_type_register_static (G_TYPE_OBJECT,
                                       "RCLineBuf",
                                       &type_info,
                                       0);
    }

    return (type);
} /* rc_line_buf_get_type */

static void
rc_line_buf_finalize (GObject *obj)
{
    RCLineBuf *line_buf;

    line_buf = RC_LINE_BUF (obj);

    if (line_buf->priv->buf) {
        g_string_free (line_buf->priv->buf, TRUE);
    }

    if (line_buf->priv->cb_id) {
        g_source_remove (line_buf->priv->cb_id);
    }

    if (line_buf->priv->channel) {
        g_io_channel_close (line_buf->priv->channel);
        g_io_channel_unref (line_buf->priv->channel);
    }

    g_free (line_buf->priv);

    if (parent_class->finalize)
        parent_class->finalize (obj);
} /* rc_line_buf_finalize */

#if 0
static void
rc_line_buf_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
    RCLineBuf *line_buf;

    line_buf = RC_LINE_BUF (object);

    switch (arg_id) {
    case ARG_FD:
        rc_line_buf_set_fd (line_buf, GTK_VALUE_INT (*arg));
        break;

    default:
        break;
    }
} /* rc_line_buf_set_arg */

static void
rc_line_buf_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
    RCLineBuf *line_buf;

    line_buf = RC_LINE_BUF (object);

    switch (arg_id) {
    case ARG_FD:
        GTK_VALUE_INT (*arg) =
            g_io_channel_unix_get_fd (line_buf->priv->channel);
        break;

    default:
        arg->type = GTK_TYPE_INVALID;
        break;
    }
} /* rc_line_buf_get_arg */
#endif

static void
rc_line_buf_class_init (RCLineBufClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    object_class->finalize = rc_line_buf_finalize;

#if 0
    object_class->get_arg = rc_line_buf_get_arg;
    object_class->set_arg = rc_line_buf_set_arg;
#endif

    klass->read_line  = NULL;
    klass->read_done  = NULL;

    parent_class = g_type_class_peek_parent (klass);

    signals[READ_LINE] =
        g_signal_new ("read_line",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (RCLineBufClass, read_line),
                      NULL, NULL,
                      rc_marshal_VOID__STRING,
                      G_TYPE_NONE, 1,
                      G_TYPE_STRING);

    signals[READ_DONE] =
        g_signal_new ("read_done",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCLineBufClass, read_done),
                      NULL, NULL,
                      rc_marshal_VOID__INT,
                      G_TYPE_NONE, 1,
                      G_TYPE_INT);

#if 0
    gtk_object_add_arg_type ("RCLineBuf::fd",
                             GTK_TYPE_INT,
                             GTK_ARG_READWRITE,
                             ARG_FD);
#endif
} /* rc_line_buf_class_init */

static void
rc_line_buf_init (RCLineBuf *obj)
{
    obj->priv = g_new (RCLineBufPrivate, 1);

    obj->priv->channel = NULL;
    obj->priv->cb_id = 0;
    obj->priv->buf = g_string_new ("");
} /* rc_line_buf_init */

static gboolean
rc_line_buf_cb (GIOChannel *source, GIOCondition condition,
                gpointer data)
{
    RCLineBuf *line_buf = (RCLineBuf *)data;
    guint bytes_read;
    guint count;
    guint base = 0;
    gchar buf[BUF_SIZE + 1];

    if (condition & (G_IO_IN | G_IO_PRI)) {
        switch (g_io_channel_read (source, buf, BUF_SIZE, &bytes_read)) {

        case G_IO_ERROR_AGAIN:
            /* This really shouldn't happen; why on earth would we get
               called if there's no data waiting? */
            rc_debug (RC_DEBUG_LEVEL_WARNING,
                      "%s: got G_IO_ERROR_AGAIN, bork bork?\n", __FUNCTION__);

            return (TRUE);

        case G_IO_ERROR_INVAL:
        case G_IO_ERROR_UNKNOWN:
            /* Whoops, this is bad */
            rc_debug (RC_DEBUG_LEVEL_ERROR,
                      "%s: got G_IO_ERROR_[INVAL|UNKNOWN], ending read\n",
                      __FUNCTION__);

            g_signal_emit (line_buf, signals[READ_DONE], 0,
                           RC_LINE_BUF_ERROR);

            /* I don't think I should have to do this, but this is the
               solution to the infamous big bad oh-what-the-fuck bug,
               so... */
            g_source_remove (line_buf->priv->cb_id);
            line_buf->priv->cb_id = 0;

            return (FALSE);

        case G_IO_ERROR_NONE:
            if (bytes_read == 0) {
                /* A read of 0 bytes with no error means we're done
                 * here */
#if 0
                rc_debug (RC_DEBUG_LEVEL_DEBUG,
                          "%s: read 0 bytes, we're done here\n", __FUNCTION__);
#endif

                g_signal_emit (line_buf, signals[READ_DONE], 0,
                               RC_LINE_BUF_OK);

                /* I don't think I should have to do this, but this is
                   the solution to the infamous big bad
                   oh-what-the-fuck bug, so */
                g_source_remove (line_buf->priv->cb_id);
                line_buf->priv->cb_id = 0;

                return (FALSE);
            }
#if 0
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "%s: read %d bytes\n",
                      __FUNCTION__, bytes_read);
#endif

            buf[bytes_read] = '\0';

            for (count = 0; count < bytes_read; count++) {
                if (buf[count] == '\n') {
                    buf[count] = '\0';

                    if (count && buf[count - 1] == '\r') {
                        buf[count - 1] = '\0';
                    }

                    line_buf->priv->buf =
                        g_string_append (line_buf->priv->buf, buf + base);

#if 0
                    rc_debug (RC_DEBUG_LEVEL_DEBUG,
                              __FUNCTION__ ": line is \"%s\"\n",
                              line_buf->priv->buf->str);
#endif

                    g_signal_emit (line_buf, signals[READ_LINE], 0,
                                   line_buf->priv->buf->str);

                    g_string_truncate (line_buf->priv->buf, 0);

                    base = count + 1;
                }
            }

            line_buf->priv->buf = g_string_append (line_buf->priv->buf,
                                                   buf + base);

            return (TRUE);
        }
    } else {
        g_signal_emit (line_buf, signals[READ_DONE], 0,
                       RC_LINE_BUF_OK);

        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;

#if 0
        rc_debug (RC_DEBUG_LEVEL_DEBUG, "%s: got G_IO_[(HUP)|(ERR)], we're "
                  "done here\n", __FUNCTION__);
#endif

        return (FALSE);
    }

    g_assert_not_reached ();

    return (FALSE);
} /* rc_line_buf_cb */

RCLineBuf *
rc_line_buf_new ()
{
    RCLineBuf *line_buf;

    line_buf = RC_LINE_BUF (g_object_new (rc_line_buf_get_type (), NULL));

    return (line_buf);
} /* rc_line_buf_new */

void
rc_line_buf_set_fd (RCLineBuf *line_buf, int fd)
{
    if (line_buf->priv->cb_id) {
        g_source_remove (line_buf->priv->cb_id);
    }

    if (line_buf->priv->channel) {
        g_io_channel_close (line_buf->priv->channel);
        g_io_channel_unref (line_buf->priv->channel);
    }

    if (line_buf->priv->buf) {
        g_string_truncate (line_buf->priv->buf, 0);
    }

    line_buf->priv->channel = g_io_channel_unix_new (fd);

    line_buf->priv->cb_id = g_io_add_watch (line_buf->priv->channel,
                                            G_IO_IN | G_IO_HUP | G_IO_ERR,
                                            (GIOFunc) rc_line_buf_cb,
                                            (gpointer) line_buf);
} /* rc_line_buf_set_fd */

gchar *
rc_line_buf_get_buf (RCLineBuf *line_buf)
{
    gchar *tmp;

    tmp = g_strdup (line_buf->priv->buf->str);

    g_string_truncate (line_buf->priv->buf, 0);

    return (tmp);
} /* rc_line_buf_get_buf */
