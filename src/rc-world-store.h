/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-store.h
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

#ifndef __RC_WORLD_STORE_H__
#define __RC_WORLD_STORE_H__

#include "rc-world.h"

#define RC_TYPE_WORLD_STORE            (rc_world_store_get_type ())
#define RC_WORLD_STORE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        RC_TYPE_WORLD_STORE, RCWorldStore))
#define RC_WORLD_STORE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                        RC_TYPE_WORLD_STORE, \
                                        RCWorldStoreClass))
#define RC_IS_WORLD_STORE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        RC_TYPE_WORLD_STORE))
#define RC_IS_WORLD_STORE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                        RC_TYPE_WORLD_STORE))
#define RC_WORLD_STORE_GET_CLASS(obj)  (RC_WORLD_STORE_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCWorldStore      RCWorldStore;
typedef struct _RCWorldStoreClass RCWorldStoreClass;

struct _RCWorldStore {
    RCWorld world;

    int freeze_count;

    GHashTable *packages_by_name;
    GHashTable *provides_by_name;
    GHashTable *requires_by_name;
    GHashTable *children_by_name;
    GHashTable *conflicts_by_name;

    GSList *channels;
    GSList *locks;

    GAllocator *allocator;
};

struct _RCWorldStoreClass {
    RCWorldClass parent_class;
};

GType rc_world_store_get_type (void);

RCWorld *rc_world_store_new (void);

/*** Add/remove packages ***/

gboolean rc_world_store_add_package (RCWorldStore *store,
                                     RCPackage    *package);

void rc_world_store_add_packages_from_slist (RCWorldStore   *store,
                                             RCPackageSList *slist);

void rc_world_store_remove_package (RCWorldStore *store,
                                    RCPackage    *package);

void rc_world_store_remove_packages (RCWorldStore *store,
                                     RCChannel    *channel);

void rc_world_store_clear (RCWorldStore *store);

/*** Channels ***/

void rc_world_store_add_channel (RCWorldStore *store,
                                 RCChannel    *channel);

void rc_world_store_remove_channel (RCWorldStore *store,
                                    RCChannel    *channel);

/*** Locks ***/

void rc_world_store_add_lock (RCWorldStore   *store,
                              RCPackageMatch *lock);

void rc_world_store_remove_lock (RCWorldStore   *store,
                                 RCPackageMatch *lock);

void rc_world_store_clear_locks (RCWorldStore *store);

#endif /* __RC_WORLD_STORE_H__ */

