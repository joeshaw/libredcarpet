/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-line-buf.h
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

#ifndef _RC_LINE_BUF_H
#define _RC_LINE_BUF_H

#include <glib-object.h>

#define RC_TYPE_LINE_BUF            (rc_line_buf_get_type ())
#define RC_LINE_BUF(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                     RC_TYPE_LINE_BUF, RCLineBuf))
#define RC_LINE_BUF_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                     RC_TYPE_LINE_BUF, RCLineBufClass))
#define IS_RC_LINE_BUF(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                     RC_TYPE_LINE_BUF))
#define IS_RC_LINE_BUF_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                     RC_TYPE_LINE_BUF))

typedef enum {
    RC_LINE_BUF_OK,
    RC_LINE_BUF_ERROR
} RCLineBufStatus;

typedef struct _RCLineBuf        RCLineBuf;
typedef struct _RCLineBufClass   RCLineBufClass;
typedef struct _RCLineBufPrivate RCLineBufPrivate;

struct _RCLineBuf {
    GObject parent;

    RCLineBufPrivate *priv;
};

struct _RCLineBufClass {
    GObjectClass parent_class;

    /* Signals */

    void (*read_line)(RCLineBuf *line_buf, gchar *line);
    void (*read_done)(RCLineBuf *line_buf, RCLineBufStatus status);
};

GType rc_line_buf_get_type (void);

RCLineBuf *rc_line_buf_new (void);

void rc_line_buf_set_fd (RCLineBuf *line_buf, int fd);

gchar *rc_line_buf_get_buf (RCLineBuf *line_buf);

#endif /* _RC_LINE_BUF_H */
