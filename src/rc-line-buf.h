#ifndef _RC_LINE_BUF_H
#define _RC_LINE_BUF_H

#include <gtk/gtk.h>

#define GTK_TYPE_RC_LINE_BUF        (rc_line_buf_get_type ())
#define RC_LINE_BUF(obj)            (GTK_CHECK_CAST ((obj), \
                                     GTK_TYPE_RC_LINE_BUF, RCLineBuf))
#define RC_LINE_BUF_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                     GTK_TYPE_RC_LINE_BUF, RCLineBufClass))
#define IS_RC_LINE_BUF(obj)         (GTK_CHECK_TYPE ((obj), \
                                     GTK_TYPE_RC_LINE_BUF))
#define IS_RC_LINE_BUF_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
                                     GTK_TYPE_RC_LINE_BUF))

typedef enum _RCLineBufStatus RCLineBufStatus;

enum _RCLineBufStatus {
    RC_LINE_BUF_OK,
    RC_LINE_BUF_ERROR
};

typedef struct _RCLineBuf      RCLineBuf;
typedef struct _RCLineBufClass RCLineBufClass;

struct _RCLineBuf {
    GtkObject parent;

    GIOChannel *channel;

    guint cb_id;

    GString *buf;
};

struct _RCLineBufClass {
    GtkObjectClass parent_class;

    /* Signals */

    void (*read_line)(RCLineBuf *lb, gchar *line);
    void (*read_done)(RCLineBuf *lb, RCLineBufStatus status);
};

guint rc_line_buf_get_type (void);

RCLineBuf *rc_line_buf_new (void);

void rc_line_buf_set_fd (RCLineBuf *lb, int fd);

#endif
