#include "rc-line-buf.h"

static void rc_line_buf_class_init (RCLineBufClass *klass);
static void rc_line_buf_init       (RCLineBuf *obj);

static GtkObjectClass *rc_line_buf_parent;

enum SIGNALS {
    READ_LINE,
    READ_ERROR,
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
    RCLineBuf *lb = RC_LINE_BUF (obj);

    if (lb->buf) {
        g_string_free (lb->buf, TRUE);
    }

    if (lb->read_id) {
        g_source_remove (lb->read_id);
    }

    if (lb->hup_id) {
        g_source_remove (lb->hup_id);
    }

    if (lb->channel) {
        g_io_channel_close (lb->channel);
        g_io_channel_unref (lb->channel);
    }

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
    klass->read_error = NULL;
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

    signals[READ_ERROR] =
        gtk_signal_new ("read_error",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCLineBufClass, read_error),
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);

    signals[READ_DONE] =
        gtk_signal_new ("read_done",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCLineBufClass, read_done),
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);

    gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);
}

static void
rc_line_buf_init (RCLineBuf *obj)
{
    obj->channel = NULL;
    obj->read_id = 0;
    obj->hup_id = 0;
    obj->buf = NULL;
}

static gboolean
rc_line_buf_read_cb (GIOChannel *source, GIOCondition condition,
                     gpointer data)
{
    RCLineBuf *lb = (RCLineBuf *)data;
    GIOError ret;
    guint bytes_read;
    guint count;
    guint base = 0;
    gchar *buf;

    g_assert (condition == G_IO_IN);

    buf = g_new0 (gchar, 101);

    ret = g_io_channel_read (source, buf, 100, &bytes_read);

    switch (ret) {
    case G_IO_ERROR_AGAIN:
        /* This really shouldn't happen; why on earth would we get called if
           there's no data waiting? */
        g_free (buf);
        return (TRUE);
        break;

    case G_IO_ERROR_INVAL:
    case G_IO_ERROR_UNKNOWN:
        /* Bork bork */
        gtk_signal_emit ((GtkObject *)lb, signals[READ_ERROR]);
        /* I don't think I should have to do this, but this is the solution to
           the infamous big bad oh-what-the-fuck bug, so... */
        g_source_remove (lb->read_id);
        lb->read_id = 0;
        g_free (buf);
        return (FALSE);
        break;

    case G_IO_ERROR_NONE:
        if (bytes_read == 0) {
            gtk_signal_emit ((GtkObject *)lb, signals[READ_DONE]);
            /* I don't think I should have to do this, but this is the
               solution to the infamous big bad oh-what-the-fuck bug, so... */
            g_source_remove (lb->read_id);
            lb->read_id = 0;
            g_free (buf);
            return (FALSE);
        }

        buf[bytes_read + 1] = '\0';

        for (count = 0; count < bytes_read; count++) {
            if (buf[count] == '\n') {
                buf[count] = '\0';
                lb->buf = g_string_append (lb->buf, buf + base);
                gtk_signal_emit ((GtkObject *)lb, signals[READ_LINE],
                                 lb->buf->str);
                g_string_free (lb->buf, TRUE);
                lb->buf = g_string_new (NULL);
                base = count + 1;
            }
        }

        lb->buf = g_string_append (lb->buf, buf + base);

        g_free (buf);

        return (TRUE);
        break;
    }

    g_assert_not_reached ();
    return (FALSE);
}

static gboolean
rc_line_buf_hup_cb (GIOChannel *channel, GIOCondition condition,
                    gpointer data)
{
    RCLineBuf *lb = (RCLineBuf *)data;

    g_assert (condition == G_IO_HUP);

    while (rc_line_buf_read_cb (channel, G_IO_IN, lb));

    gtk_signal_emit ((GtkObject *)lb, signals[READ_DONE]);

    lb->hup_id = 0;
    return (FALSE);
}

RCLineBuf *
rc_line_buf_new ()
{
    RCLineBuf *new =
        RC_LINE_BUF (gtk_type_new (rc_line_buf_get_type ()));

    new->buf = g_string_new (NULL);

    return (new);
}

void
rc_line_buf_set_fd (RCLineBuf *lb, int fd)
{
    lb->channel = g_io_channel_unix_new (fd);

    lb->read_id = g_io_add_watch (lb->channel, G_IO_IN,
                                  (GIOFunc) rc_line_buf_read_cb,
                                  (gpointer) lb);

    lb->hup_id = g_io_add_watch (lb->channel, G_IO_HUP,
                                 (GIOFunc) rc_line_buf_hup_cb,
                                 (gpointer) lb);
}
