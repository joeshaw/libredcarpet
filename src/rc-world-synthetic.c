/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-synthetic.c
 *
 * Copyright (C) 2002-2003 Ximian, Inc.
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
#include "rc-world-synthetic.h"

#include "rc-debug.h"
#include "rc-xml.h"
#include "rc-extract-packages.h"

static RCWorldServiceClass *parent_class;

static gboolean
add_synthetic_package_cb (RCPackage *pkg, gpointer user_data)
{
    rc_world_store_add_package ((RCWorldStore *) user_data, pkg);
    return TRUE;
}

static gboolean
rc_world_synthetic_load_packages (RCWorldSynthetic *synth)
{
    gboolean retval = FALSE;
    RCWorldStore *store = RC_WORLD_STORE (synth);
    xmlDoc *doc = NULL;
    xmlNode *root;

    /* If no filename, just silently return. */
    if (synth->database_fn == NULL)
        return TRUE;

    if (! g_file_test (synth->database_fn, G_FILE_TEST_EXISTS)) {
        rc_debug (RC_DEBUG_LEVEL_MESSAGE,
                  "Can't find synthetic package file '%s'",
                  synth->database_fn);
        retval = TRUE; /* File not found is not an error. */
        goto finished;
    }

    rc_debug (RC_DEBUG_LEVEL_MESSAGE, "Loading synthetic packages");

    doc = rc_parse_xml_from_file (synth->database_fn);
    if (doc == NULL) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "Can't read synthetic packages file '%s'",
                  synth->database_fn);
        retval = FALSE;
        goto finished;
    }

    rc_world_store_clear (store);

    root = xmlDocGetRootElement (doc);
    if (root == NULL)
        goto finished;

    rc_extract_packages_from_xml_node (root,
                                       synth->synthetic_channel,
                                       add_synthetic_package_cb,
                                       store);

    rc_debug (RC_DEBUG_LEVEL_MESSAGE, "Done loading synthetic packages");

    retval = TRUE;

 finished:
    if (doc)
        xmlFreeDoc (doc);

    return retval;
}

static gboolean
packages_to_xml_cb (RCPackage *pkg, gpointer user_data)
{
    xmlNode *root = user_data;
    xmlAddChild (root, rc_package_to_xml_node (pkg));
    return TRUE;
}

static gboolean
rc_world_synthetic_save_packages (RCWorldSynthetic *synth)
{
    xmlDoc *doc;
    xmlNode *root;
    FILE *out;

    /* If the filename is NULL, don't save synthetic packages. */
    if (synth->database_fn == NULL)
        return TRUE;

    out = fopen (synth->database_fn, "w");
    if (out == NULL) {
        /* FIXME: better error handling */
        g_warning ("Can't open file '%s' to save synthetic packages",
                   synth->database_fn);
        return FALSE;
    }

    doc = xmlNewDoc ("1.0");
    root = xmlNewNode (NULL, "synthetic_packages");
    xmlDocSetRootElement (doc, root);

    rc_world_foreach_package (RC_WORLD (synth),
                              synth->synthetic_channel,
                              packages_to_xml_cb,
                              root);

    xmlDocDump (out, doc);
    fclose (out);

    return TRUE;
}

static RCPending *
rc_world_synthetic_refresh (RCWorld *world)
{
    rc_world_synthetic_load_packages (RC_WORLD_SYNTHETIC (world));
    rc_world_refresh_complete (world);
    return NULL;
}

static gboolean
rc_world_synthetic_sync (RCWorld *world, RCChannel *channel)
{
    /* Just a no-op. */
    return TRUE;
}

static gboolean
rc_world_synthetic_can_transact (RCWorld *world, RCPackage *package)
{
    return rc_package_is_synthetic (package);
}

static gboolean
rc_world_synthetic_transact (RCWorld *world,
                             RCPackageSList *install_packages,
                             RCPackageSList *remove_packages,
                             int flags)
{
    RCPackageSList *iter;
    gboolean retval;

    /* FIXME: don't ignore the flags */

    for (iter = install_packages; iter != NULL; iter = iter->next) {
        RCPackage *pkg = iter->data;
        rc_world_store_add_package (RC_WORLD_STORE (world), pkg);
    }

    for (iter = remove_packages; iter != NULL; iter = iter->next) {
        RCPackage *pkg = iter->data;
        rc_world_store_remove_package (RC_WORLD_STORE (world), pkg);
    }

    retval = rc_world_synthetic_save_packages (RC_WORLD_SYNTHETIC (world));

    return retval;
}

static gboolean
rc_world_synthetic_assemble (RCWorldService *service)
{
    RCWorldSynthetic *synth = RC_WORLD_SYNTHETIC (service);
    char *cptr;

    if (synth->error_flag)
        return FALSE;

    cptr = strchr (service->url, ':');
    if (cptr == NULL)
        return FALSE;
    ++cptr;
    if (! *cptr)
        return FALSE;
    /* We append an extra '/' to the beginning of the file name so
       that something broken like 'synthetic:foo/bar' gets treated
       as '/foo/bar' and isn't relative to the cwd. */
    synth->database_fn = g_strconcat ("/", cptr, NULL);
    
    service->name = g_strdup ("Synthetic");
    service->unique_id = g_strdup ("@synthetic");

    service->is_sticky = TRUE;
    service->is_invisible = TRUE;
    service->is_singleton = TRUE;
    
    rc_world_synthetic_load_packages (synth);

    return TRUE;
}

static void
rc_world_synthetic_finalize (GObject *obj)
{
    RCWorldSynthetic *synth = RC_WORLD_SYNTHETIC (obj);

    g_free (synth->database_fn);
    rc_channel_unref (synth->synthetic_channel);

    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize (obj);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_synthetic_class_init (RCWorldSyntheticClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);
    RCWorldClass *world_class = RC_WORLD_CLASS (klass);
    RCWorldServiceClass *service_class = RC_WORLD_SERVICE_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    /* Set up the vtable */
    world_class->sync_fn         = rc_world_synthetic_sync;
    world_class->refresh_fn      = rc_world_synthetic_refresh;
    world_class->can_transact_fn = rc_world_synthetic_can_transact;
    world_class->transact_fn     = rc_world_synthetic_transact;

    service_class->assemble_fn   = rc_world_synthetic_assemble;
    
    obj_class->finalize          = rc_world_synthetic_finalize;
}

static void
rc_world_synthetic_init (RCWorldSynthetic *synth)
{
    synth->synthetic_channel = rc_channel_new ("@synthetic",
                                               "Synthetic Packages",
                                               "@synthetic",
                                               "Synthetic Packages");
    rc_channel_set_system (synth->synthetic_channel);
    rc_channel_set_hidden (synth->synthetic_channel);

    rc_world_store_add_channel (RC_WORLD_STORE (synth),
                                synth->synthetic_channel);

    /* Load the synthetic packages */
    synth->error_flag = ! rc_world_synthetic_load_packages (synth);
}

GType
rc_world_synthetic_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldSyntheticClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_synthetic_class_init,
            NULL, NULL,
            sizeof (RCWorldSynthetic),
            0,
            (GInstanceInitFunc) rc_world_synthetic_init
        };

        type = g_type_register_static (RC_TYPE_WORLD_SERVICE,
                                       "RCWorldSynthetic",
                                       &type_info,
                                       0);
    }

    return type;
}

RCWorld *
rc_world_synthetic_new (void)
{
    RCWorldSynthetic *synth;
    synth = g_object_new (RC_TYPE_WORLD_SYNTHETIC, NULL);

    rc_world_synthetic_assemble ((RCWorldService *) synth);

    if (synth->error_flag) {
        g_object_unref (synth);
        synth = NULL;
    }

    return (RCWorld *) synth;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_world_synthetic_register_service (void)
{
    rc_world_service_register ("synthetic", RC_TYPE_WORLD_SYNTHETIC);
}
