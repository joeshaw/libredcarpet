/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-undump.c
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
#include "rc-world-undump.h"

#include "rc-world-store.h"
#include "rc-extract-packages.h"
#include "rc-debug.h"
#include "xml-util.h"

static GObjectClass *parent_class;

static void
rc_world_undump_finalize (GObject *obj)
{
    if (parent_class->finalize)
        parent_class->finalize (obj);
}

static void
rc_world_undump_class_init (RCWorldUndumpClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = rc_world_undump_finalize;
}

static void
rc_world_undump_init (RCWorldUndump *undump)
{

}

GType
rc_world_undump_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldUndumpClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_undump_class_init,
            NULL, NULL,
            sizeof (RCWorldUndump),
            0,
            (GInstanceInitFunc) rc_world_undump_init
        };

        type = g_type_register_static (RC_TYPE_WORLD_STORE,
                                       "RCWorldUndump",
                                       &type_info,
                                       0);
    }

    return type;
}

RCWorld *
rc_world_undump_new (const char *filename)
{
    RCWorld *world = g_object_new (RC_TYPE_WORLD_UNDUMP, NULL);
    rc_world_undump_load ((RCWorldUndump *) world, filename);
    return world;
}

static gboolean
add_channel_cb (RCChannel *channel, gboolean subd, gpointer user_data)
{
    RCWorldStore *store = user_data;
    rc_world_store_add_channel (store, channel);
    rc_world_set_subscription ((RCWorld *) store, channel, subd);
    return TRUE;
}

static gboolean
add_package_cb (RCPackage *pkg, gpointer user_data)
{
    RCWorldStore *store = user_data;
    rc_world_store_add_package (store, pkg);
    return TRUE;
}

void
rc_world_undump_load (RCWorldUndump *undump, const char *filename)
{
    g_return_if_fail (undump != NULL && RC_IS_WORLD_UNDUMP (undump));
    
    rc_world_store_clear ((RCWorldStore *) undump);

    if (filename) {
        rc_extract_packages_from_undump_file (filename,
                                              add_channel_cb,
                                              add_package_cb,
                                              undump);
    }
}
