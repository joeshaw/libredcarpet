/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debman.h
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

#ifndef _RC_DEBMAN_H
#define _RC_DEBMAN_H

#include <glib-object.h>
#include "rc-packman.h"
#include "rc-debman-private.h"

#define RC_TYPE_DEBMAN            (rc_debman_get_type ())
#define RC_DEBMAN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                   RC_TYPE_DEBMAN, RCDebman))
#define RC_DEBMAN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                   RC_TYPE_DEBMAN, \
                                   RCDebmanClass))
#define IS_RC_DEBMAN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                   RC_TYPE_DEBMAN))
#define IS_RC_DEBMAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                   RC_TYPE_DEBMAN))

typedef struct _RCDebman        RCDebman;
typedef struct _RCDebmanClass   RCDebmanClass;

struct _RCDebman {
    RCPackman parent;

    RCDebmanPrivate *priv;
};

struct _RCDebmanClass {
    RCPackmanClass parent_class;
};

GType rc_debman_get_type (void);

RCDebman *rc_debman_new (void);

#endif /* _RC_DEBMAN_H */
