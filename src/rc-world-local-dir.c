/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-local_dir.c
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
#include "rc-world-local-dir.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rc-debug.h"
#include "rc-extract-packages.h"

static RCWorldServiceClass *parent_class;

static void
rc_world_local_dir_freeze (RCWorldLocalDir *ldir)
{
    g_return_if_fail (ldir->frozen == FALSE);

    ldir->frozen = TRUE;
}

static void
rc_world_local_dir_thaw (RCWorldLocalDir *ldir)
{
    g_return_if_fail (ldir->frozen == TRUE);

    ldir->frozen = FALSE;
}

static gboolean
add_pkg_cb (RCPackage *pkg, gpointer user_data)
{
    rc_world_store_add_package (RC_WORLD_STORE (user_data), pkg);
    return TRUE;
}

static gboolean
rc_world_local_dir_populate (RCWorldLocalDir *ldir,
                             gboolean populate_only_if_mtime_has_changed)
{
    char *id;
    struct stat statbuf;

    g_assert (RC_IS_WORLD_LOCAL_DIR (ldir));

    rc_world_local_dir_freeze (ldir);

    if (stat (ldir->path, &statbuf)) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, "Can't stat %s", ldir->path);
        return FALSE;
    }

    if (populate_only_if_mtime_has_changed) {
        if (difftime (ldir->mtime, statbuf.st_mtime) == 0) {
            rc_world_local_dir_thaw (ldir);
            return TRUE;
        }

        rc_debug (RC_DEBUG_LEVEL_INFO,
                  "%s appears to have changed; re-scanning", ldir->path);
    }

    if (ldir->channel != NULL) {
        rc_world_store_remove_channel (RC_WORLD_STORE (ldir), ldir->channel);
        rc_channel_unref (ldir->channel);
    }

    id = g_strconcat ("@local|", ldir->path, NULL);

    ldir->channel = rc_channel_new (id,
                                    RC_WORLD_SERVICE (ldir)->name,
                                    ldir->alias,
                                    ldir->description);
    g_free (id);

    rc_world_store_add_channel (RC_WORLD_STORE (ldir), ldir->channel);
    rc_extract_packages_from_directory (ldir->path,
                                        ldir->channel,
                                        rc_packman_get_global (),
                                        ldir->recursive,
                                        add_pkg_cb, ldir);

    ldir->mtime = statbuf.st_mtime;

    rc_world_local_dir_thaw (ldir);

    /* FIXME: actually return whether or not the operation succeeded */
    return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_local_dir_finalize (GObject *obj)
{
    RCWorldLocalDir *ldir = (RCWorldLocalDir *) obj;

    g_free (ldir->path);
    g_free (ldir->alias);
    g_free (ldir->description);
    rc_channel_unref (ldir->channel);

    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static gboolean
rc_world_local_dir_sync_fn (RCWorld *world, RCChannel *channel)
{
    RCWorldLocalDir *ldir = (RCWorldLocalDir *) world;

    /* There is nothing to do if we are only syncing system channels. */
    if (channel == RC_CHANNEL_SYSTEM)
        return TRUE;

    /* 
       Re-populate the dir with the 'only actually populate this
       dir if the mtime has changed' flag set to TRUE.
       This causes us to automatically pick up rpms that have been
       added/removed to the directory.
       
       Of course, it doesn't handle certain cases -- the main one
       being changes to the packages in subdirectories that are
       recursively descended into.  To see those sorts of changes,
       you need to either do a full refresh.  Or just touch any
       file in the directory top-level... RC_RECURSIVE leaps to
       mind as a good candidate.

       We have to check to make sure we're not frozen; populating
       the local store causes the world to sync and this function
       to be called.  Otherwise, we get into a nasty infinite
       recursion.
    */

    if (ldir->frozen)
        return TRUE;
    else
        return rc_world_local_dir_populate (ldir, TRUE);
}

static char *
extract_value (char *token)
{
    char *eq, *value;

    eq = strchr (token, '=');

    if (!eq)
        return NULL;

    /* Move past the equals sign and strip leading whitespace */
    value = g_strchug (eq + 1);

    return g_strdup (value);
}

static gboolean
rc_world_local_dir_assemble_fn (RCWorldService *service, GError **error)
{
    RCWorldLocalDir *ldir = RC_WORLD_LOCAL_DIR (service);
    char *query_part, *path;
    char *name = NULL, *alias = NULL;
    gboolean recursive = FALSE;

    /* Find the path.  The "+ 7" part moves past "file://" */
    query_part = strchr (service->url + 7, '?');

    if (query_part)
        path = g_strndup (service->url + 7, query_part - service->url - 7);
    else
        path = g_strdup (service->url + 7);

    if (!g_file_test (path, G_FILE_TEST_IS_DIR)) {
        g_set_error (error, RC_ERROR, RC_ERROR,
                     "%s does not exist", path);
        g_free (path);
        return FALSE;
    }

    if (query_part) {
        char **tokens, **t;

        /* + 1 to move past the "?" */
        tokens = g_strsplit (query_part + 1, ";", 0);

        for (t = tokens; t && *t; t++) {
            if (g_strncasecmp (*t, "name", 4) == 0) {
                name = extract_value (*t);
            }
            
            if (g_strncasecmp (*t, "alias", 5) == 0) {
                alias = extract_value (*t);
            }

            if (g_strncasecmp (*t, "recursive", 9) == 0) {
                char *tmp = extract_value (*t);

                if (atoi(tmp))
                    recursive = TRUE;

                g_free (tmp);
            }
        }
    }

    if (!name)
        name = g_path_get_basename (path);

    if (!alias)
        alias = g_path_get_basename (path);

    service->name = name;
    service->unique_id = g_strdup (path);

    ldir->path = path;
    ldir->alias = g_path_get_basename (path);
    ldir->description = g_strdup_printf ("Files from %s", path);
    ldir->mtime = 0;
    ldir->recursive = recursive;
    
    rc_world_local_dir_populate (ldir, FALSE);

    return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
set_name_cb (RCChannel *channel, gpointer user_data)
{
    const char *name = user_data;

    rc_channel_set_name (channel, name);

    return TRUE;
}

void
rc_world_local_dir_set_name (RCWorldLocalDir *ldir, const char *name)
{
    g_return_if_fail (RC_IS_WORLD_LOCAL_DIR (ldir));
    g_return_if_fail (name != NULL);

    g_free (RC_WORLD_SERVICE (ldir)->name);

    RC_WORLD_SERVICE (ldir)->name = g_strdup (name);

    rc_world_foreach_channel (RC_WORLD (ldir), set_name_cb, (gpointer) name);
}

static gboolean
set_alias_cb (RCChannel *channel, gpointer user_data)
{
    const char *alias = user_data;

    rc_channel_set_alias (channel, alias);

    return TRUE;
}

void
rc_world_local_dir_set_alias (RCWorldLocalDir *ldir, const char *alias)
{
    g_return_if_fail (RC_IS_WORLD_LOCAL_DIR (ldir));
    g_return_if_fail (alias != NULL);

    g_free (ldir->alias);

    ldir->alias = g_strdup (alias);

    rc_world_foreach_channel (RC_WORLD (ldir), set_alias_cb, (gpointer) alias);
}
       

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_local_dir_class_init (RCWorldLocalDirClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    RCWorldClass *world_class = (RCWorldClass *) klass;
    RCWorldServiceClass *service_class = (RCWorldServiceClass *) klass;

    parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = rc_world_local_dir_finalize;

    world_class->sync_fn         = rc_world_local_dir_sync_fn;
    world_class->can_transact_fn = NULL;
    world_class->transact_fn     = NULL;

    service_class->assemble_fn   = rc_world_local_dir_assemble_fn;
}

static void
rc_world_local_dir_init (RCWorldLocalDir *local_dir)
{

}

GType
rc_world_local_dir_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldLocalDirClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_local_dir_class_init,
            NULL, NULL,
            sizeof (RCWorldLocalDir),
            0,
            (GInstanceInitFunc) rc_world_local_dir_init
        };

        type = g_type_register_static (RC_TYPE_WORLD_SERVICE,
                                       "RCWorldLocalDir",
                                       &type_info,
                                       0);
    }

    return type;
}

RCWorld *
rc_world_local_dir_new (const char *path)
{
    RCWorldLocalDir *ldir = NULL;

    g_return_val_if_fail (path && *path, NULL);

    if (g_file_test (path, G_FILE_TEST_IS_DIR)) {
        ldir = g_object_new (RC_TYPE_WORLD_LOCAL_DIR, NULL);
        ldir->path = g_strdup (path);
        RC_WORLD_SERVICE (ldir)->name = g_path_get_basename (path);
        ldir->alias = g_path_get_basename (path);
        ldir->description = g_strdup_printf ("Files from %s", path);
        ldir->mtime = 0;
        rc_world_local_dir_populate (ldir, FALSE);
    }

    return RC_WORLD (ldir);
}

void
rc_world_local_dir_register_service (void)
{
    rc_world_service_register ("file", RC_TYPE_WORLD_LOCAL_DIR);
}
