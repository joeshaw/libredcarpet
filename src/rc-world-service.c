/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-service.c
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

static void
rc_world_service_class_init (RCWorldServiceClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = rc_world_service_finalize;
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
    g_return_if_fail (scheme && *scheme);
    g_return_if_fail (world_type);

    if (!scheme_handlers) {
        scheme_handlers = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, NULL);
    }

    g_hash_table_insert (scheme_handlers, g_strdup (scheme),
                         GINT_TO_POINTER (world_type));
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
    g_return_val_if_fail (scheme && *scheme, 0);

    if (!scheme_handlers)
        return 0;

    return GPOINTER_TO_INT (g_hash_table_lookup (scheme_handlers, scheme));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

GHashTable *mounts = NULL;

RCWorld *
rc_world_service_mount (const char *url)
{
    char *endptr;
    char *scheme;
    GType world_type;
    RCWorldService *worldserv;
    RCWorldServiceClass *klass;

    g_return_val_if_fail (url && *url, NULL);

    /* Check to see if this service is already mounted */
    if (mounts && g_hash_table_lookup (mounts, url))
        return NULL;

    endptr = strchr (url, ':');

    if (!endptr) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, "Invalid service URI: %s", url);
        return NULL;
    }

    scheme = g_strndup (url, endptr - url);
    world_type = rc_world_service_lookup (scheme);
    g_free (scheme);

    if (!world_type)
        return NULL;

    worldserv = g_object_new (world_type, NULL);
    worldserv->url = g_strdup (url);

    klass = RC_WORLD_SERVICE_GET_CLASS (worldserv);
    if (klass->assemble_fn) {
        if (!klass->assemble_fn (worldserv)) {
            g_object_unref (worldserv);
            return NULL;
        }
    }

    if (!mounts) {
        mounts = g_hash_table_new_full (g_str_hash, g_str_equal,
                                        g_free, g_object_unref);
    }

    g_hash_table_insert (mounts, g_strdup (url), g_object_ref (worldserv));

    return RC_WORLD (worldserv);
}

RCWorld *
rc_world_service_lookup_mount (const char *url)
{
    RCWorldService *worldserv;

    if (!mounts)
        return NULL;

    worldserv = g_hash_table_lookup (mounts, url);

    return RC_WORLD (worldserv);
}

gboolean
rc_world_service_unmount_by_url (const char *url)
{
    RCWorldService *worldserv;

    g_return_val_if_fail (url && *url, FALSE);

    if (!mounts)
        return FALSE;

    worldserv = RC_WORLD_SERVICE (g_hash_table_lookup (mounts, url));

    if (worldserv && worldserv->is_sticky)
        return FALSE;

    return g_hash_table_remove (mounts, url);
}

gboolean
rc_world_service_unmount (RCWorldService *worldserv)
{
    g_return_val_if_fail (RC_IS_WORLD_SERVICE (worldserv), FALSE);

    return rc_world_service_unmount_by_url (worldserv->url);
}

typedef struct {
    RCWorldServiceForeachFn callback;
    gpointer user_data;
} ForeachInfo;

static void
foreach_mount_cb (gpointer key, gpointer value, gpointer user_data)
{
    RCWorldService *worldserv = RC_WORLD_SERVICE (value);
    ForeachInfo *info = user_data;

    info->callback (worldserv, info->user_data);
}

void
rc_world_service_foreach_mount (RCWorldServiceForeachFn callback,
                                gpointer                user_data)
{
    ForeachInfo info = { callback, user_data };

    g_hash_table_foreach (mounts, foreach_mount_cb, &info);
}
