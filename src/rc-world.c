/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world.c
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Developed by Jon Trowbridge <trow@ximian.com>
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
#include "rc-world.h"

#include "rc-util.h"
#include "rc-deps-util.h"

typedef struct _RCPackageAndSpec RCPackageAndSpec;
struct _RCPackageAndSpec {
    RCPackage *package;
    RCPackageSpec *spec;
};

typedef struct _RCPackageAndDep RCPackageAndDep;
struct _RCPackageAndDep {
    RCPackage *package;
    RCPackageDep *dep;
};

RCWorld *
rc_get_world (void)
{
    static RCWorld *world = NULL;

    if (world == NULL) 
        world = rc_world_new ();

    return world;
}

RCWorld *
rc_world_new (void)
{
	RCWorld *world = g_new0 (RCWorld, 1);

	world->packages_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	world->provides_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	world->requires_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	
	return world;
}

void
rc_world_free (RCWorld *world)
{
	if (world) {

        /* FIXME: Leaks everything */
		g_free (world);

	}
}

void
rc_world_freeze (RCWorld *world)
{
    g_return_if_fail (world != NULL);

    ++world->freeze_count;

    if (world->freeze_count == 1) {

        g_hash_table_freeze (world->packages_by_name);
        g_hash_table_freeze (world->provides_by_name);
        g_hash_table_freeze (world->requires_by_name);

    }
}

void
rc_world_thaw (RCWorld *world)
{
    g_return_if_fail (world != NULL);
    g_return_if_fail (world->freeze_count > 0);

    --world->freeze_count;
    
    if (world->freeze_count == 0) {

        g_hash_table_thaw (world->packages_by_name);
        g_hash_table_thaw (world->provides_by_name);
        g_hash_table_thaw (world->requires_by_name);

    }
}

void
rc_world_add_package (RCWorld *world, RCPackage *package)
{
    const RCPackageDepSList *iter;
    RCPackageAndSpec *pas;
    RCPackageAndDep *pad;

    g_return_if_fail (world != NULL);
    g_return_if_fail (package != NULL);

    /* Store all of our packages in a hash by name. */

    rc_hash_table_slist_insert (world->packages_by_name,
                                package->spec.name,
                                package);

    /* Store all of the package's provides in a hash by name.
       Packages implicitly provide themselves. */

    pas = g_new0 (RCPackageAndSpec, 1);
    pas->package = package;
    pas->spec = &package->spec;
    package->spec.type = RC_PACKAGE_SPEC_TYPE_PACKAGE;

    rc_hash_table_slist_insert (world->provides_by_name,
                                pas->spec->name,
                                pas);

    for (iter = package->provides; iter != NULL; iter = iter->next) {

        RCPackageDep *dep = (RCPackageDep *) iter->data;

        pas = g_new0 (RCPackageAndSpec, 1);
        pas->package = package;
        pas->spec = &dep->spec;

        rc_hash_table_slist_insert (world->provides_by_name,
                                    pas->spec->name,
                                    pas);
    }

    /* Store all of the package's requires in a hash by name. */

    for (iter = package->requires; iter != NULL; iter = iter->next) {
        RCPackageDep *dep = (RCPackageDep *) iter->data;

        pad = g_new0 (RCPackageAndDep, 1);
        pad->package = package;
        pad->dep = dep;

        rc_hash_table_slist_insert (world->requires_by_name,
                                    dep->spec.name,
                                    pad);
    }

}

void
rc_world_add_packages_from_slist (RCWorld *world,
                                  RCPackageSList *slist)
{
    g_return_if_fail (world != NULL);
    
    rc_world_freeze (world);
    while (slist) {
        rc_world_add_package (world, (RCPackage *) slist->data);
        slist = slist->next;
    }
    rc_world_thaw (world);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_packages_generic (GSList *slist, RCPackage *package_to_remove, RCChannel *channel)
{
    GSList *orig, *iter;

    /* If both are NULL, remove everything. */
    if (package_to_remove == NULL
        && channel == RC_WORLD_ANY_CHANNEL) {
        for (iter = slist; iter != NULL; iter = iter->next) {
            if (iter->data)
                rc_package_free (iter->data);
        }
        
        g_slist_free (slist);
        return TRUE;
    }

    /* 
       Walk the list, removing links that contain a package in the
       matching channel.  Don't remove the first link in order to avoid
       needing to change the head of the list that is stored in the
       hash --- just set the data to NULL in that case. 
    */
    orig = iter = slist;
    while (iter != NULL) {
        RCPackage *package = iter->data;
        GSList *next = iter->next;
        if (package && 
            ((package_to_remove && package == package_to_remove)
             || (channel && package->channel == channel))) {
            /* FIXME: Why does freeing the package here lead to corruption?
               Where else is it being freed? */
            /* rc_package_free (package); */
            if (iter == slist) {
                iter->data = NULL;
            } else {
                slist = g_slist_remove_link (slist, iter);
                g_slist_free_1 (iter);
            }
        }
        iter = next;
    }

    /* Sanity check: make sure the head of the list didn't change. */
    g_assert (slist == orig);

    /* If this node's list is empty, remove it. */
    if (slist == NULL)
        return TRUE;
    if (slist && slist->data == NULL && slist->next == NULL) {
        g_slist_free_1 (slist);
        return TRUE;
    }

    return FALSE;
}

static gboolean
remove_package_structs_generic (GSList *slist, RCPackage *package_to_remove, RCChannel *channel)
{
    GSList *orig, *iter;

    if (package_to_remove == NULL
        && channel == RC_WORLD_ANY_CHANNEL) {
        for (iter = slist; iter != NULL; iter = iter->next) {
            if (iter->data)
                g_free (iter->data);
        }
        
        g_slist_free (slist);
        return TRUE;
    }

    /* 
       Walk the list, removing links that contain a package in the
       matching channel.  Don't remove the first link in order to avoid
       needing to change the head of the list that is stored in the
       hash --- just set the data to NULL in that case. 
    */
    orig = iter = slist;
    while (iter != NULL) {
        RCPackage **our_struct = iter->data;
        GSList *next = iter->next;
        if (our_struct) {
            RCPackage *package = *our_struct;
            
            if (package && 
                ((package_to_remove && package == package_to_remove)
                 || (channel && package->channel == channel))) {
                g_free (our_struct);
                if (iter == slist) {
                    iter->data = NULL;
                } else {
                    slist = g_slist_remove_link (slist, iter);
                    g_slist_free_1 (iter);
                }
            }
            iter = next;
        }
    }

    /* Sanity check: make sure the head of the list didn't change. */
    g_assert (slist == orig);

    /* If this node's list is empty, remove it. */
    if (slist == NULL)
        return TRUE;
    if (slist && slist->data == NULL && slist->next == NULL) {
        g_slist_free_1 (slist);
        return TRUE;
    }

    return FALSE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_package_cb (gpointer key, gpointer val, gpointer user_data)
{
    GSList *slist = val;
    RCPackage *package = user_data;

    return remove_packages_generic (slist, package, NULL);
}

static gboolean
remove_package_struct_cb (gpointer key, gpointer val, gpointer user_data)
{
    GSList *slist = val;
    RCPackage *package = user_data;

    return remove_package_structs_generic (slist, package, NULL);
}

void
rc_world_remove_package (RCWorld *world,
                          RCPackage *package)
{
    g_return_if_fail (world != NULL);
    g_return_if_fail (package != NULL);

    /* FIXME: This is grossly inefficient */

    g_hash_table_foreach_remove (world->provides_by_name,
                                 remove_package_struct_cb,
                                 package);
    g_hash_table_foreach_remove (world->requires_by_name,
                                 remove_package_struct_cb,
                                 package);
    g_hash_table_foreach_remove (world->packages_by_name,
                                 remove_package_cb,
                                 package);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_package_by_channel_cb (gpointer key, gpointer val, gpointer user_data)
{
    GSList *slist = val;
    RCChannel *channel = user_data;

    return remove_packages_generic (slist, NULL, channel);
}

static gboolean
remove_package_struct_by_channel_cb (gpointer key, gpointer val, gpointer user_data)
{
    GSList *slist = val;
    RCChannel *channel = user_data;

    return remove_package_structs_generic (slist, NULL, channel);
}

void
rc_world_remove_packages (RCWorld *world,
                          RCChannel *channel)
{
    g_return_if_fail (world != NULL);

    g_hash_table_foreach_remove (world->provides_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);
    g_hash_table_foreach_remove (world->requires_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);
    g_hash_table_foreach_remove (world->packages_by_name,
                                 remove_package_by_channel_cb,
                                 channel);
}

RCPackage *
rc_world_find_installed_version (RCWorld *world,
                                 RCPackage *package)
{
    GSList *iter;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    iter = g_hash_table_lookup (world->packages_by_name,
                                package->spec.name);
    
    while (iter != NULL) {
        RCPackage *this_package = iter->data;

        if (this_package && rc_package_is_installed (this_package))
            return this_package;
        
        iter = iter->next;
    }

    return NULL;
}

struct ForeachPackageInfo {
    RCChannel *channel;
    RCPackageFn fn;
    gpointer user_data;
    int count;
};

static void
foreach_package_cb (gpointer key, gpointer val, gpointer user_data)
{
    struct ForeachPackageInfo *info = user_data;
    GSList *slist = val, *iter;

    /* FIXME: we should filter out dup uninstalled packages. */

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackage *package = slist->data;
        if (package && (info->channel == RC_WORLD_ANY_CHANNEL 
                        || package->channel == info->channel)) {
            if (info->fn)
                info->fn (package, info->user_data);
            ++info->count;
        }
    }
}

int
rc_world_foreach_package (RCWorld *world,
                          RCChannel *channel,
                          RCPackageFn fn,
                          gpointer user_data)
{
    struct ForeachPackageInfo info;

    g_return_val_if_fail (world != NULL, -1);
    
    info.channel = channel;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    g_hash_table_foreach (world->packages_by_name,
                          foreach_package_cb, 
                          &info);

    return info.count;
}

int
rc_world_foreach_package_by_name (RCWorld *world,
                                  const char *name,
                                  RCChannel *channel,
                                  RCPackageFn fn,
                                  gpointer user_data)
{
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (name && *name, -1);

    slist = (GSList *) g_hash_table_lookup (world->packages_by_name, name);

    installed = g_hash_table_new (rc_package_spec_hash,
                                  rc_package_spec_equal);

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackage *package = iter->data;
        if (package && rc_package_is_installed (package))
            g_hash_table_insert (installed, & package->spec, package);
    }
    
    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackage *package = iter->data;
        if (package && (channel == RC_WORLD_ANY_CHANNEL || package->channel == channel)) {
            if (rc_package_is_installed (package)
                || g_hash_table_lookup (installed, & package->spec) == NULL) {
                if (fn) 
                    fn (package, user_data);
                ++count;
            }
        }
    }

    g_hash_table_destroy (installed);

    return count;
}

struct ForeachUpgradeInfo {
    RCPackage *original_package;
    RCPackageFn fn;
    gpointer user_data;
    int count;
};

static void
foreach_upgrade_cb (RCPackage *package, gpointer user_data)
{
    struct ForeachUpgradeInfo *info = user_data;

    if (rc_package_spec_compare (info->original_package, package) < 0) {
        if (info->fn)
            info->fn (package, info->user_data);
        ++info->count;
    }
}

int
rc_world_foreach_upgrade (RCWorld *world,
                          RCPackage *package,
                          RCChannel *channel,
                          RCPackageFn fn,
                          gpointer user_data)
{
    struct ForeachUpgradeInfo info;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (package != NULL, -1);

    info.original_package = package;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    rc_world_foreach_package_by_name (world,
                                      package->spec.name,
                                      channel,
                                      foreach_upgrade_cb,
                                      &info);

    return info.count;
}

int
rc_world_foreach_providing_package (RCWorld *world, RCPackageDep *dep,
                                    RCChannel *channel,
                                    RCPackageAndSpecFn fn, gpointer user_data)
{
    GSList *slist, *iter;
    int count = 0;
    GHashTable *installed;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    if (dep->is_or) {
        RCPackageDepSList *deps, *iter;
        deps = rc_dep_string_to_or_dep_slist (dep->spec.name);
        for (iter = deps; iter != NULL; iter = iter->next) {
            count += rc_world_foreach_providing_package (world, iter->data,
                                                         channel, fn, user_data);
        }
        g_slist_free (deps);
        return count;
    }

    slist = (GSList *) g_hash_table_lookup (world->provides_by_name,
                                            dep->spec.name);

    installed = g_hash_table_new (rc_package_spec_hash,
                                  rc_package_spec_equal);

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndSpec *pas = iter->data;
        if (pas && pas->package && rc_package_is_installed (pas->package))
            g_hash_table_insert (installed, & pas->package->spec, pas);
    }

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndSpec *pas = iter->data;

        if (pas
            && (channel == RC_WORLD_ANY_CHANNEL || pas->package->channel == channel)
            && rc_package_dep_verify_relation (dep, pas->spec)) {

            /* If we have multiple identical packages in RCWorld, we want to only
               include the package that is installed and skip the rest. */
            if (rc_package_is_installed (pas->package)
                || g_hash_table_lookup (installed, & pas->package->spec) == NULL) {
                
                if (fn)
                    fn (pas->package, pas->spec, user_data);
                ++count;
            }
        }

        slist = slist->next;
    }

    g_hash_table_destroy (installed);

    return count;
}

gboolean
rc_world_check_providing_package (RCWorld *world, RCPackageDep *dep,
                                  RCChannel *channel,
                                  RCPackageAndSpecCheckFn fn, gpointer user_data)
{
    GSList *slist, *iter;
    GHashTable *installed;
    gboolean ret = FALSE;

    g_return_val_if_fail (world != NULL, TRUE);
    g_return_val_if_fail (dep != NULL, TRUE);
    g_return_val_if_fail (fn != NULL, TRUE);

    if (dep->is_or) {
        RCPackageDepSList *deps, *iter;
        gboolean terminated = FALSE;
        deps = rc_dep_string_to_or_dep_slist (dep->spec.name);
        for (iter = deps; iter != NULL && !terminated; iter = iter->next) {
            terminated = rc_world_check_providing_package (world, iter->data,
                                                           channel, fn, user_data);
        }
        rc_package_dep_slist_free (deps);
        return terminated;
    }

    slist = (GSList *) g_hash_table_lookup (world->provides_by_name,
                                            dep->spec.name);

    installed = g_hash_table_new (rc_package_spec_hash,
                                  rc_package_spec_equal);

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndSpec *pas = iter->data;
        if (pas && pas->package && rc_package_is_installed (pas->package))
            g_hash_table_insert (installed, & pas->package->spec, pas);
    }

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndSpec *pas = slist->data;

        if (pas
            && (channel == RC_WORLD_ANY_CHANNEL || pas->package->channel == channel)
            && rc_package_dep_verify_relation (dep, pas->spec)) {

            /* Skip uninstalled dups */
            if (rc_package_is_installed (pas->package)
                || g_hash_table_lookup (installed, & pas->package->spec) == NULL) {

                if (! fn (pas->package, pas->spec, user_data)) {
                    ret = TRUE;
                    break;
                }
            }
        }
    }

    g_hash_table_destroy (installed);

    return ret;
}

int
rc_world_foreach_requiring_package (RCWorld *world, RCPackageSpec *spec,
                                    RCChannel *channel,
                                    RCPackageAndDepFn fn, gpointer user_data)
{
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (spec != NULL, -1);

    slist = (GSList *) g_hash_table_lookup (world->requires_by_name,
                                            spec->name);

    installed = g_hash_table_new (rc_package_spec_hash,
                                  rc_package_spec_equal);

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndDep *pad = iter->data;
        if (pad && pad->package && rc_package_is_installed (pad->package))
            g_hash_table_insert (installed, & pad->package->spec, pad);
    }

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndDep *pad = iter->data;

        if (pad
            && (channel == RC_WORLD_ANY_CHANNEL || pad->package->channel == channel)
            && rc_package_dep_verify_relation (pad->dep, spec)) {

            /* Skip dups if one of them in installed. */
            if (rc_package_is_installed (pad->package)
                || g_hash_table_lookup (installed, & pad->package->spec) == NULL) {

                if (fn)
                    fn (pad->package, pad->dep, user_data);
                ++count;
            }
        }
    }

    g_hash_table_destroy (installed);

    return count;
}

static void
foreach_package_by_name_cb (gpointer key, gpointer val, gpointer user_data)
{
    FILE *out = user_data;
    char *name = key;
    GSList *iter = val;

    fprintf (out, "PKG %s: ", name);
    while (iter) {
        RCPackage *package = iter->data;
        if (package) {
            fprintf (out, rc_package_spec_to_str_static (& package->spec));
            fprintf (out, " ");
        } else {
            fprintf (out, "(null) ");
        }
        iter = iter->next;
    }
    fprintf (out, "\n");
}

static void
foreach_provides_by_name_cb (gpointer key, gpointer val, gpointer user_data)
{
    FILE *out = user_data;
    char *name = key;
    GSList *iter = val;

    fprintf (out, "SPC %s: ", name);
    while (iter) {
        RCPackageAndSpec *pas = iter->data;
        if (pas) {
            fprintf (out, rc_package_spec_to_str_static (& pas->package->spec));
            fprintf (out, "/");
            fprintf (out, rc_package_spec_to_str_static (pas->spec));
            fprintf (out, " ");
        } else {
            fprintf (out, "(null) ");
        }
        iter = iter->next;
    }
    fprintf (out, "\n");
}

static void
foreach_requires_by_name_cb (gpointer key, gpointer val, gpointer user_data)
{
    FILE *out = user_data;
    char *name = key;
    GSList *iter = val;

    fprintf (out, "REQ %s: ", name);
    while (iter) {
        RCPackageAndDep *pad = iter->data;
        if (pad) {
            fprintf (out, rc_package_spec_to_str_static (& pad->package->spec));
            fprintf (out, "/");
            fprintf (out, rc_package_spec_to_str_static (& pad->dep->spec));
            fprintf (out, " ");
        } else {
            fprintf (out, "(null) ");
        }
        iter = iter->next;
    }
    fprintf (out, "\n");
}

void
rc_world_spew (RCWorld *world, FILE *out)
{
    g_hash_table_foreach (world->packages_by_name,
                          foreach_package_by_name_cb,
                          out);

    g_hash_table_foreach (world->provides_by_name,
                          foreach_provides_by_name_cb,
                          out);

    g_hash_table_foreach (world->requires_by_name,
                          foreach_requires_by_name_cb,
                          out);
}
