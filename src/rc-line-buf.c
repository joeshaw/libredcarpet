/* rc-line-buf.c
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#define BUF_SIZE 1023

#include "rc-debug.h"
#include "rc-line-buf.h"
#include "rc-line-buf-private.h"

static void rc_line_buf_class_init (RCLineBufClass *klass);
static void rc_line_buf_init       (RCLineBuf *obj);

static GtkObjectClass *parent_class;

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

guint
rc_line_buf_get_type (void)
{
    static guint type = 0;

    RC_ENTRY;

    if (!type) {
        GtkTypeInfo type_info = {
            "RCLineBuf",
            sizeof (RCLineBuf),
            sizeof (RCLineBufClass),
            (GtkClassInitFunc) rc_line_buf_class_init,
            (GtkObjectInitFunc) rc_line_buf_init,
            (GtkArgSetFunc) NULL,
            (GtkArgGetFunc) NULL,
        };

        type = gtk_type_unique (gtk_object_get_type (), &type_info);
    }

    RC_EXIT;

    return (type);
} /* rc_line_buf_get_type */

static void
rc_line_buf_destroy (GtkObject *obj)
{
    RCLineBuf *line_buf;

    RC_ENTRY;

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

    GTK_OBJECT_CLASS (parent_class)->destroy (obj);

    RC_EXIT;
} /* rc_line_buf_destroy */

static void
rc_line_buf_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
    RCLineBuf *line_buf;

    RC_ENTRY;

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

    RC_ENTRY;

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

static void
rc_line_buf_class_init (RCLineBufClass *klass)
{
    GtkObjectClass *object_class;

    RC_ENTRY;

    object_class = GTK_OBJECT_CLASS (klass);

    object_class->destroy = rc_line_buf_destroy;

    object_class->get_arg = rc_line_buf_get_arg;
    object_class->set_arg = rc_line_buf_set_arg;

    klass->read_line  = NULL;
    klass->read_done  = NULL;

    parent_class = gtk_type_class (gtk_object_get_type ());

    signals[READ_LINE] =
        gtk_signal_new ("read_line",
                        GTK_RUN_FIRST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCLineBufClass, read_line),
                        gtk_marshal_NONE__STRING,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_STRING);

    signals[READ_DONE] =
        gtk_signal_new ("read_done",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCLineBufClass, read_done),
                        gtk_marshal_NONE__ENUM,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_ENUM);

    gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

    gtk_object_add_arg_type ("RCLineBuf::fd",
                             GTK_TYPE_INT,
                             GTK_ARG_READWRITE,
                             ARG_FD);

    RC_EXIT;
} /* rc_line_buf_class_init */

static void
rc_line_buf_init (RCLineBuf *obj)
{
    RC_ENTRY;

    obj->priv = g_new (RCLineBufPrivate, 1);

    obj->priv->channel = NULL;
    obj->priv->cb_id = 0;
    obj->priv->buf = g_string_new ("");

    RC_EXIT;
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

    RC_ENTRY;

    if (condition == G_IO_HUP) {
        gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                         RC_LINE_BUF_OK);

        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;

        rc_debug (RC_DEBUG_LEVEL_DEBUG, "%s: got G_IO_HUP, we're done here\n",
                  __FUNCTION__);

        RC_EXIT;

        return (FALSE);
    }

    switch (g_io_channel_read (source, buf, BUF_SIZE, &bytes_read)) {

    case G_IO_ERROR_AGAIN:
        /* This really shouldn't happen; why on earth would we get called if
           there's no data waiting? */
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "%s: got G_IO_ERROR_AGAIN, bork bork?\n", __FUNCTION__);

        RC_EXIT;

        return (TRUE);

    case G_IO_ERROR_INVAL:
    case G_IO_ERROR_UNKNOWN:
        /* Whoops, this is bad */
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "%s: got G_IO_ERROR_[INVAL|UNKNOWN], ending read\n",
                  __FUNCTION__);

        gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                         RC_LINE_BUF_ERROR);

        /* I don't think I should have to do this, but this is the solution to
           the infamous big bad oh-what-the-fuck bug, so... */
        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;

        RC_EXIT;

        return (FALSE);

    case G_IO_ERROR_NONE:
        if (bytes_read == 0) {
            /* A read of 0 bytes with no error means we're done here */
            rc_debug (RC_DEBUG_LEVEL_DEBUG,
                      "%s: read 0 bytes, we're done here\n", __FUNCTION__);

            gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                             RC_LINE_BUF_OK);

            /* I don't think I should have to do this, but this is the
                   solution to the infamous big bad oh-what-the-fuck bug, so */
            g_source_remove (line_buf->priv->cb_id);
            line_buf->priv->cb_id = 0;

            RC_EXIT;

            return (FALSE);
        }

        rc_debug (RC_DEBUG_LEVEL_DEBUG, "%s: read %d bytes\n", __FUNCTION__,
                  bytes_read);

        buf[bytes_read] = '\0';

        for (count = 0; count < bytes_read; count++) {
            if (buf[count] == '\n') {
                buf[count] = '\0';

                if (buf[count - 1] == '\r') {
                    buf[count - 1] = '\0';
                }

                line_buf->priv->buf =
                    g_string_append (line_buf->priv->buf, buf + base);

                rc_debug (RC_DEBUG_LEVEL_DEBUG,
                          __FUNCTION__ ": line is \"%s\"\n",
                          line_buf->priv->buf->str);

                gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_LINE],
                                 line_buf->priv->buf->str);

                g_string_truncate (line_buf->priv->buf, 0);

                base = count + 1;
            }
        }

        line_buf->priv->buf = g_string_append (line_buf->priv->buf,
                                               buf + base);

        RC_EXIT;

        return (TRUE);
    }

    g_assert_not_reached ();

    RC_EXIT;

    return (FALSE);
} /* rc_line_buf_cb */

RCLineBuf *
rc_line_buf_new ()
{
    RCLineBuf *line_buf;

    RC_ENTRY;

    line_buf = RC_LINE_BUF (gtk_type_new (rc_line_buf_get_type ()));

    RC_EXIT;

    return (line_buf);
} /* rc_line_buf_new */

void
rc_line_buf_set_fd (RCLineBuf *line_buf, int fd)
{
    RC_ENTRY;

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
                                            G_IO_IN | G_IO_HUP,
                                            (GIOFunc) rc_line_buf_cb,
                                            (gpointer) line_buf);

    RC_EXIT;
} /* rc_line_buf_set_fd */

gchar *
rc_line_buf_get_buf (RCLineBuf *line_buf)
{
    gchar *tmp;

    RC_ENTRY;

    tmp = g_strdup (line_buf->priv->buf->str);

    g_string_truncate (line_buf->priv->buf, 0);

    RC_EXIT;

    return (tmp);
} /* rc_line_buf_get_buf */
