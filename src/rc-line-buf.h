/* rc-line-buf.h
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

#ifndef _RC_LINE_BUF_H
#define _RC_LINE_BUF_H

#include <gtk/gtk.h>

#define RC_TYPE_LINE_BUF            (rc_line_buf_get_type ())
#define RC_LINE_BUF(obj)            (GTK_CHECK_CAST ((obj), \
                                     RC_TYPE_LINE_BUF, RCLineBuf))
#define RC_LINE_BUF_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                     RC_TYPE_LINE_BUF, RCLineBufClass))
#define RC_IS_LINE_BUF(obj)         (GTK_CHECK_TYPE ((obj), \
                                     RC_TYPE_LINE_BUF))
#define RC_IS_LINE_BUF_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
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
