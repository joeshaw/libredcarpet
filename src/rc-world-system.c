/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-system.c
 *
 * Copyright (C) 2002-2003 Ximian, Inc.
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
#include "rc-world-system.h"

#include "rc-debug.h"

static gboolean
rc_world_system_load_packages (RCWorldSystem *system)
{
    RCPackageSList *system_packages = NULL, *iter;
    RCPackman *packman = rc_packman_get_global ();
    gboolean retval = FALSE;
    RCWorldStore *store = RC_WORLD_STORE (system);

    rc_debug (RC_DEBUG_LEVEL_MESSAGE, "Loading system packages");

    rc_world_store_clear (store);

    system_packages = rc_packman_query_all (packman);
    if (rc_packman_get_error (packman)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "System query failed: %s", rc_packman_get_reason (packman));

        goto finished;
    }

    rc_world_store_remove_packages (store, RC_CHANNEL_SYSTEM);

    for (iter = system_packages; iter; iter = iter->next) {
        RCPackage *pkg = iter->data;
        pkg->channel = rc_channel_ref (system->system_channel);
        rc_world_store_add_package (store, pkg);
    }

    rc_debug (RC_DEBUG_LEVEL_MESSAGE, "Done loading system packages");

    retval = TRUE;

 finished:
    rc_package_slist_unref (system_packages);
    g_slist_free (system_packages);

    return retval;
}

static RCPending *
rc_world_system_refresh (RCWorld *world)
{
    rc_world_system_load_packages (RC_WORLD_SYSTEM (world));
    rc_world_refresh_complete (world);
    return NULL;
}

static gboolean
rc_world_system_sync (RCWorld *world, RCChannel *channel)
{
    if (channel == RC_CHANNEL_NON_SYSTEM)
        return TRUE;

    if (rc_packman_is_database_changed (rc_packman_get_global ()))
        return rc_world_system_load_packages (RC_WORLD_SYSTEM (world));

    return TRUE;
}

static gboolean
rc_world_system_can_transact (RCWorld *world, RCPackage *package)
{
    return !rc_package_is_synthetic (package);
}

static gboolean
rc_world_system_transact (RCWorld *world,
                          RCPackageSList *install_packages,
                          RCPackageSList *remove_packages,
                          int flags)
{
    rc_packman_transact (rc_packman_get_global (),
                         install_packages,
                         remove_packages,
                         flags);

    rc_world_refresh (world);

    return TRUE;
}

static gboolean
rc_world_system_assemble (RCWorldService *service)
{
    RCWorldSystem *system = RC_WORLD_SYSTEM (service);

    /* Load the system packages */
    system->error_flag = ! rc_world_system_load_packages (system);

    if (system->error_flag)
        return FALSE;

    service->name = g_strdup ("System");
    service->unique_id = g_strdup ("@system");

    service->is_sticky = TRUE;
    service->is_invisible = TRUE;
    service->is_singleton = TRUE;

    return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static RCWorldServiceClass *parent_class;

static void
rc_world_system_class_init (RCWorldSystemClass *klass)
{
    RCWorldClass *world_class = RC_WORLD_CLASS (klass);
    RCWorldServiceClass *service_class = RC_WORLD_SERVICE_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    /* Set up the vtable */
    world_class->sync_fn         = rc_world_system_sync;
    world_class->refresh_fn      = rc_world_system_refresh;
    world_class->can_transact_fn = rc_world_system_can_transact;
    world_class->transact_fn     = rc_world_system_transact;

    service_class->assemble_fn   = rc_world_system_assemble;
}

static void
rc_world_system_init (RCWorldSystem *system)
{
    system->system_channel = rc_channel_new ("@system",
                                             "System Packages",
                                             "@system",
                                             "System Packages");
    rc_channel_set_system (system->system_channel);
    rc_channel_set_hidden (system->system_channel);

    rc_world_store_add_channel (RC_WORLD_STORE (system),
                                system->system_channel);
}

GType
rc_world_system_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldSystemClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_system_class_init,
            NULL, NULL,
            sizeof (RCWorldSystem),
            0,
            (GInstanceInitFunc) rc_world_system_init
        };

        type = g_type_register_static (RC_TYPE_WORLD_SERVICE,
                                       "RCWorldSystem",
                                       &type_info,
                                       0);
    }

    return type;
}

RCWorld *
rc_world_system_new (void)
{
    RCWorldSystem *system;
    system = g_object_new (RC_TYPE_WORLD_SYSTEM, NULL);

    rc_world_system_assemble ((RCWorldService *) system);

    if (system->error_flag) {
        g_object_unref (system);
        system = NULL;
    }

    return (RCWorld *) system;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_world_system_register_service (void)
{
    rc_world_service_register ("system", RC_TYPE_WORLD_SYSTEM);
}
