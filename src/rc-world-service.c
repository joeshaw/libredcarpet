/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-service.c
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

#include <config.h>
#include "rc-world-service.h"

#include "rc-debug.h"

static RCWorldStoreClass *parent_class;

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_service_finalize (GObject *obj)
{
    RCWorldService *worldserv = (RCWorldService *) obj;

    if (worldserv->url)
        g_free (worldserv->url);

    if (worldserv->name)
        g_free (worldserv->name);

    if (worldserv->unique_id)
        g_free (worldserv->unique_id);

    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static RCWorld *
rc_world_service_dup (RCWorld *old_world)
{
    RCWorld *new_world;
    RCWorldService *old_service, *new_service;

    new_world = RC_WORLD_CLASS (parent_class)->dup_fn (old_world);

    old_service = RC_WORLD_SERVICE (old_world);
    new_service = RC_WORLD_SERVICE (new_world);

    new_service->url = g_strdup (old_service->url);
    new_service->name = g_strdup (old_service->name);
    new_service->unique_id = g_strdup (old_service->unique_id);

    new_service->is_sticky = old_service->is_sticky;
    new_service->is_invisible = old_service->is_invisible;
    new_service->is_unsaved = old_service->is_unsaved;
    new_service->is_singleton = old_service->is_singleton;

    return new_world;
}

static void
rc_world_service_class_init (RCWorldServiceClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    RCWorldClass *world_class = (RCWorldClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = rc_world_service_finalize;

    world_class->dup_fn = rc_world_service_dup;
}

static void
rc_world_service_init (RCWorldService *worldserv)
{
}

GType
rc_world_service_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldServiceClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_service_class_init,
            NULL, NULL,
            sizeof (RCWorldService),
            0,
            (GInstanceInitFunc) rc_world_service_init
        };

        type = g_type_register_static (RC_TYPE_WORLD_STORE,
                                       "RCWorldService",
                                       &type_info,
                                       0);
    }

    return type;
}

RCWorld *
rc_world_service_new (void)
{
    RCWorld *world;

    world = g_object_new (RC_TYPE_WORLD_SERVICE, NULL);
    
    return world;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

GHashTable *scheme_handlers = NULL;

void
rc_world_service_register (const char *scheme, GType world_type)
{
    GType *p;

    g_return_if_fail (scheme && *scheme);
    g_return_if_fail (world_type);

    if (!scheme_handlers) {
        scheme_handlers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, g_free);
    }

    p = g_new (GType, 1);
    *p = world_type;

    g_hash_table_insert (scheme_handlers, g_strdup (scheme), p);
}

void
rc_world_service_unregister (const char *scheme)
{
    g_return_if_fail (scheme && *scheme);

    if (!scheme_handlers)
        return;

    g_hash_table_remove (scheme_handlers, scheme);
}

GType
rc_world_service_lookup (const char *scheme)
{
    GType *p;

    g_return_val_if_fail (scheme && *scheme, 0);

    if (!scheme_handlers)
        return 0;

    p = g_hash_table_lookup (scheme_handlers, scheme);
    if (p)
        return *p;

    return 0;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCWorld *
rc_world_service_mount (const char *url, GError **error)
{
    char *endptr;
    char *scheme;
    GType world_type;
    RCWorldService *worldserv;
    RCWorldServiceClass *klass;

    g_return_val_if_fail (url && *url, NULL);

    endptr = strchr (url, ':');

    if (!endptr) {
        g_set_error (error, RC_ERROR, RC_ERROR,
                     "Invalid service URI: %s", url);
        return NULL;
    }

    scheme = g_strndup (url, endptr - url);
    world_type = rc_world_service_lookup (scheme);

    if (!world_type) {
        g_set_error (error, RC_ERROR, RC_ERROR,
                     "Don't know how to handle URI scheme '%s'", scheme);
        g_free (scheme);
        
        return NULL;
    }

    g_free (scheme);

    worldserv = g_object_new (world_type, NULL);
    worldserv->url = g_strdup (url);

    klass = RC_WORLD_SERVICE_GET_CLASS (worldserv);
    if (klass->assemble_fn) {
        if (!klass->assemble_fn (worldserv, error)) {
            g_object_unref (worldserv);
            return NULL;
        }
    }

    return RC_WORLD (worldserv);
}
