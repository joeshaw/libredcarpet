/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-multi.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef __RC_WORLD_MULTI_H__
#define __RC_WORLD_MULTI_H__

#include "rc-world.h"
#include "rc-world-service.h"

#define RC_TYPE_WORLD_MULTI            (rc_world_multi_get_type ())
#define RC_WORLD_MULTI(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        RC_TYPE_WORLD_MULTI, RCWorldMulti))
#define RC_WORLD_MULTI_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                        RC_TYPE_WORLD_MULTI, RCWorldMultiClass))
#define RC_IS_WORLD_MULTI(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        RC_TYPE_WORLD_MULTI))
#define RC_IS_WORLD_MULTI_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                        RC_TYPE_WORLD_MULTI))
#define RC_WORLD_MULTI_GET_CLASS(obj)  (RC_WORLD_MULTI_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCWorldMulti      RCWorldMulti;
typedef struct _RCWorldMultiClass RCWorldMultiClass;

struct _RCWorldMulti {
    RCWorld parent;

    GHashTable *mounts;
    GSList *subworlds;

    GSList *subworld_pendings;
};

struct _RCWorldMultiClass {
    RCWorldClass parent_class;
};

GType    rc_world_multi_get_type (void);

RCWorld *rc_world_multi_new (void);

void     rc_world_multi_add_subworld    (RCWorldMulti *, RCWorld *);
void     rc_world_multi_remove_subworld (RCWorldMulti *, RCWorld *);

gint     rc_world_multi_foreach_subworld (RCWorldMulti *,
                                          RCWorldFn callback,
                                          gpointer user_data);

gint     rc_world_multi_foreach_subworld_by_type (RCWorldMulti *,
                                                  GType type,
                                                  RCWorldFn callback,
                                                  gpointer user_data);

RCWorldService *rc_world_multi_lookup_service (RCWorldMulti *,
                                               const char *url);

gboolean rc_world_multi_mount_service (RCWorldMulti *,
                                       const char *url);

#endif /* __RC_WORLD_MULTI_MULTI_H__ */

