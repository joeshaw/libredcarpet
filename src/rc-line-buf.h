#ifndef _RC_LINE_BUF_H
#define _RC_LINE_BUF_H

#include <gtk/gtk.h>

#define RC_TYPE_LINE_BUF            (rc_line_buf_get_type ())
#define RC_LINE_BUF(obj)            (GTK_CHECK_CAST ((obj), \
                                     RC_TYPE_LINE_BUF, RCLineBuf))
#define RC_LINE_BUF_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                     RC_TYPE_LINE_BUF, RCLineBufClass))
#define IS_RC_LINE_BUF(obj)         (GTK_CHECK_TYPE ((obj), \
                                     RC_TYPE_LINE_BUF))
#define IS_RC_LINE_BUF_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
                                     RC_TYPE_LINE_BUF))

typedef enum _RCLineBufStatus RCLineBufStatus;

enum _RCLineBufStatus {
    RC_LINE_BUF_OK,
    RC_LINE_BUF_ERROR
};

typedef struct _RCLineBuf        RCLineBuf;
typedef struct _RCLineBufClass   RCLineBufClass;
typedef struct _RCLineBufPrivate RCLineBufPrivate;

struct _RCLineBuf {
    GtkObject parent;

    RCLineBufPrivate *priv;
};

struct _RCLineBufClass {
    GtkObjectClass parent_class;

    /* Signals */

    void (*read_line)(RCLineBuf *line_buf, gchar *line);
    void (*read_done)(RCLineBuf *line_buf, RCLineBufStatus status);
};

guint rc_line_buf_get_type (void);

RCLineBuf *rc_line_buf_new (int fd);

gchar *rc_line_buf_get_buf (RCLineBuf *line_buf);

#endif /* _RC_LINE_BUF_H */
