/* rc-line-buf.c
 * Copyright (C) 2000 Helix Code, Inc.
 * Author: Ian Peters <itp@helixcode.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "rc-line-buf.h"
#include "rc-line-buf-private.h"

static void rc_line_buf_class_init (RCLineBufClass *klass);
static void rc_line_buf_init       (RCLineBuf *obj);

static GtkObjectClass *parent_class;

enum SIGNALS {
    READ_LINE,
    READ_DONE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

guint
rc_line_buf_get_type (void)
{
    static guint type = 0;

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

    return (type);
}

static void
rc_line_buf_destroy (GtkObject *obj)
{
    RCLineBuf *line_buf = RC_LINE_BUF (obj);

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
}

static void
rc_line_buf_class_init (RCLineBufClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *)klass;

    object_class->destroy = rc_line_buf_destroy;

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
}

static void
rc_line_buf_init (RCLineBuf *obj)
{
    obj->priv = g_new (RCLineBufPrivate, 1);

    obj->priv->channel = NULL;
    obj->priv->cb_id = 0;
    obj->priv->buf = g_string_new (NULL);
}

static gboolean
rc_line_buf_cb (GIOChannel *source, GIOCondition condition,
                gpointer data)
{
    RCLineBuf *line_buf = (RCLineBuf *)data;
    guint bytes_read;
    guint count;
    guint base = 0;
    gchar buf[101];

    /* Sanity check */
    g_assert (condition & (G_IO_IN | G_IO_HUP));

    /* We're done here */
    if (condition == G_IO_HUP) {
        gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                         RC_LINE_BUF_OK);
        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;
        return (FALSE);
    }

    switch (g_io_channel_read (source, buf, 100, &butes_read)) {
    case G_IO_ERROR_AGAIN:
        /* This really shouldn't happen; why on earth would we get called if
           there's no data waiting? */
        return (TRUE);
        break;

    case G_IO_ERROR_INVAL:
    case G_IO_ERROR_UNKNOWN:
        gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                         RC_LINE_BUF_ERROR);
        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;
        return (FALSE);
        break;

    case G_IO_ERROR_NONE:
        if (bytes_read == 0) {
            gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                             RC_LINE_BUF_OK);
            g_source_remove (line_buf->priv->cb_id);
            line_buf->priv->cb_id = 0;
            return (FALSE);
        }

        buf[bytes_read] = '\0';

        for (count = 0; count < bytes_read; count++) {
            if (buf[count] == '\n')
            {
                buf[count] = '\0';
                line_buf->priv->buf =
                    g_string_append (line_buf->priv->buf, buf + base);
                gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_LINE],
                                 line_buf->priv->buf->str);
                g_string_free (line_buf->priv->buf, TRUE);
                line_buf->priv->buf = g_string_new (NULL);
                base = count + 1;
            }
        }

        line_buf->priv->buf = g_string_append (line_buf->priv->buf,
                                               buf + base);

        return (TRUE);
        break;
    }

    g_assert_not_reached ();

    return (FALSE);
}

RCLineBuf *
rc_line_buf_new (int fd)
{
    RCLineBuf *line_buf =
        RC_LINE_BUF (gtk_type_new (rc_line_buf_get_type ()));

    line_buf->priv->channel = g_io_channel_unix_new (fd);

    line_buf->priv->cb_id = g_io_add_watch (line_buf->priv->channel,
                                            G_IO_IN | G_IO_HUP,
                                            (GIOFunc) rc_line_buf_cb,
                                            (gpointer) line_buf);

    return (line_buf);
}

gchar *
rc_line_buf_get_buf (RCLineBuf *line_buf)
{
    gchar *tmp;

    tmp = line_buf->priv->buf->str;

    g_string_free (line_buf->priv->buf, FALSE);
    line_buf->priv->buf = g_string_new (NULL);

    return (tmp);
}
