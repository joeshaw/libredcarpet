#include <libredcarpet/rc-line-buf.h>
#include <libredcarpet/rc-line-buf-private.h>

static void rc_line_buf_class_init (RCLineBufClass *klass);
static void rc_line_buf_init       (RCLineBuf *obj);

static GtkObjectClass *rc_line_buf_parent;

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

    if (GTK_OBJECT_CLASS (rc_line_buf_parent)->destroy) {
        (*GTK_OBJECT_CLASS (rc_line_buf_parent)->destroy) (obj);
    }
}

static void
rc_line_buf_class_init (RCLineBufClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *)klass;

    object_class->destroy = rc_line_buf_destroy;

    klass->read_line  = NULL;
    klass->read_done  = NULL;

    rc_line_buf_parent = gtk_type_class (gtk_object_get_type ());

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
    GIOError ret;
    guint bytes_read;
    guint count;
    guint base = 0;
    gchar buf[101];

    if (condition == G_IO_HUP) {
        gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                         RC_LINE_BUF_OK);
        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;
        return (FALSE);
    }

    ret = g_io_channel_read (source, buf, 100, &bytes_read);

    switch (ret) {
    case G_IO_ERROR_AGAIN:
        /* This really shouldn't happen; why on earth would we get called if
           there's no data waiting? */
        return (TRUE);
        break;

    case G_IO_ERROR_INVAL:
    case G_IO_ERROR_UNKNOWN:
        /* Bork bork */
        gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                         RC_LINE_BUF_ERROR);
        /* I don't think I should have to do this, but this is the solution to
           the infamous big bad oh-what-the-fuck bug, so... */
        g_source_remove (line_buf->priv->cb_id);
        line_buf->priv->cb_id = 0;
        return (FALSE);
        break;

    case G_IO_ERROR_NONE:
        if (bytes_read == 0) {
            gtk_signal_emit (GTK_OBJECT (line_buf), signals[READ_DONE],
                             RC_LINE_BUF_OK);
            /* I don't think I should have to do this, but this is the
                   solution to the infamous big bad oh-what-the-fuck bug, so */
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
