/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-undump.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifndef __RC_WORLD_UNDUMP_H__
#define __RC_WORLD_UNDUMP_H__

#include "rc-world-store.h"

#define RC_TYPE_WORLD_UNDUMP            (rc_world_undump_get_type ())
#define RC_WORLD_UNDUMP(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                         RC_TYPE_WORLD_UNDUMP, RCWorldUndump))
#define RC_WORLD_UNDUMP_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                         RC_TYPE_WORLD_UNDUMP, \
                                         RCWorldUndumpClass))
#define RC_IS_WORLD_UNDUMP(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                         RC_TYPE_WORLD_UNDUMP))
#define RC_IS_WORLD_UNDUMP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                         RC_TYPE_WORLD_UNDUMP))
#define RC_WORLD_UNDUMP_GET_CLASS(obj)  (RC_WORLD_UNDUMP_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCWorldUndump RCWorldUndump;
typedef struct _RCWorldUndumpClass RCWorldUndumpClass;

struct _RCWorldUndump {
    RCWorldStore parent;

    GSList *subscriptions; /* RCChannel */
};

struct _RCWorldUndumpClass {
    RCWorldStoreClass parent_class;
};

GType rc_world_undump_get_type (void);

RCWorld *rc_world_undump_new  (const char *filename);
void     rc_world_undump_load (RCWorldUndump *, const char *filename);

#endif /* __RC_WORLD_UNDUMP_H__ */

