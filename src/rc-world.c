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
#include "rc-dep-or.h"

typedef struct _RCPackageAndDep RCPackageAndDep;
struct _RCPackageAndDep {
    RCPackage *package;
    RCPackageDep *dep;
};

static gboolean
channel_match (const RCChannel *a, const RCChannel *b)
{
    if (a == RC_WORLD_ANY_CHANNEL
        || b == RC_WORLD_ANY_CHANNEL)
        return TRUE;
    
    if (a == RC_WORLD_SYSTEM_PACKAGES)
        return b == NULL;
    if (b == RC_WORLD_SYSTEM_PACKAGES)
        return a == NULL;

    if (a == RC_WORLD_ANY_NON_SYSTEM)
        return b != NULL;
    if (b == RC_WORLD_ANY_NON_SYSTEM)
        return a != NULL;

    return a == b;
}

/**
 * rc_get_world:
 * 
 * This is a convenience function for applications that want
 * to use a single global #RCWorld for storing its state.
 * A new #RCWorld object is constructed and returned the first
 * time the function is called, and that same #RCWorld is
 * returned by all subsequent calls.
 * 
 * Return value: the global #RCWorld
 **/
RCWorld *
rc_get_world (void)
{
    static RCWorld *world = NULL;

    if (world == NULL) 
        world = rc_world_new ();

    return world;
}

/**
 * rc_world_new:
 * 
 * Creates a new #RCWorld.
 * 
 * Return value: a new #RCWorld
 **/
RCWorld *
rc_world_new (void)
{
	RCWorld *world = g_new0 (RCWorld, 1);

	world->packages_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	world->provides_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	world->requires_by_name = g_hash_table_new (g_str_hash, g_str_equal);
	
	return world;
}

/**
 * rc_world_free:
 * @world: an #RCWorld
 * 
 * Destroys the #RCWorld, freeing all associated memory.
 * This function is a no-op if @world is %NULL.
 **/
void
rc_world_free (RCWorld *world)
{
	if (world) {

        /* FIXME: Leaks almost everything */

        g_hash_table_destroy (world->packages_by_name);
        g_hash_table_destroy (world->provides_by_name);
        g_hash_table_destroy (world->requires_by_name);

		g_free (world);

	}
}

/**
 * rc_world_freeze:
 * @world: An #RCWorld.
 * 
 * Freezes the hash tables inside of an #RCWorld, which
 * can improve performance if you need to add a
 * large number of packages to @world via
 * multiple calls to rc_world_add_package() more
 * efficient.  Be sure to call rc_world_thaw() after you
 * are done modifying the #RCWorld and before making any
 * queries against it.
 * 
 **/
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

/**
 * rc_world_thaw:
 * @world: An #RCWorld.
 * 
 * Thaws the hash tables inside of #RCWorld.  This function
 * should always be called before making queries against
 * @world.
 * 
 **/
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

/**
 * rc_world_add_package:
 * @world: An #RCWorld.
 * @package: An #RCPackage to be added to @world.
 * 
 * Stores @package inside of @world and indexes all of its
 * dependency information for fast look-ups later on.
 * 
 **/
void
rc_world_add_package (RCWorld *world, RCPackage *package)
{
    const RCPackageDepSList *iter;
    RCPackageAndDep *pad;

    g_return_if_fail (world != NULL);
    g_return_if_fail (package != NULL);

    /* The world holds a reference to the package */
    rc_package_ref (package);

    /* Store all of our packages in a hash by name. */

    rc_hash_table_slist_insert (world->packages_by_name,
                                package->spec.name,
                                package);

    /* Store all of the package's provides in a hash by name.
       Packages implicitly provide themselves. */

    pad = g_new0 (RCPackageAndDep, 1);
    pad->package = package;
    pad->dep = rc_package_dep_new_from_spec (&package->spec,
                                             RC_RELATION_EQUAL);
    pad->dep->spec.type = RC_PACKAGE_SPEC_TYPE_PACKAGE;

    rc_hash_table_slist_insert (world->provides_by_name,
                                pad->dep->spec.name,
                                pad);

    for (iter = package->provides; iter != NULL; iter = iter->next) {

        RCPackageDep *dep = (RCPackageDep *) iter->data;

        pad = g_new0 (RCPackageAndDep, 1);
        pad->package = package;
        pad->dep = dep;

        rc_hash_table_slist_insert (world->provides_by_name,
                                    dep->spec.name,
                                    pad);
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


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_packages_generic (GSList *slist, RCPackage *package_to_remove, RCChannel *channel)
{
    GSList *orig, *iter;

    /* If channel == RC_WORLD_ANY_CHANNEL, remove everything. */
    if (package_to_remove == NULL
        && channel == RC_WORLD_ANY_CHANNEL) {
        for (iter = slist; iter != NULL; iter = iter->next) {
            if (iter->data)
                rc_package_unref (iter->data);
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
             || channel_match (package->channel, channel))) {

            rc_package_unref (package);

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
                 || channel_match (package->channel, channel))) {
                g_free (our_struct);
                if (iter == slist) {
                    iter->data = NULL;
                } else {
                    slist = g_slist_remove_link (slist, iter);
                    g_slist_free_1 (iter);
                }
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

/**
 * rc_world_remove_package:
 * @world: An #RCWorld.
 * @package: An #RCPackage that is stored in @world.
 * 
 * Removes the #RCPackage @package from @world.  
 *
 * As currently
 * implemented, this function is not very efficient; if you need
 * to remove a large number of packages, use rc_world_remove_packages()
 * if possible.
 * 
 **/
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

/**
 * rc_world_remove_packages:
 * @world:  An #RCWorld.
 * @channel: An #RCChannel or a channel wildcard.
 * 
 * Removes all of the packages from @world whose
 * channel matches @channel. 
 * 
 * This function should be favored over rc_world_remove_package() when
 * removing a large number of packages simulatenously.
 * 
 **/
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

/**
 * rc_world_find_installed_version:
 * @world: An #RCWorld.
 * @package: An #RCPackage.
 * 
 * Searches @world for an installed package with the same
 * name as @package.
 * 
 * 
 * Return value: The installed package.  If no such
 * package exists, returns %NULL.
 **/
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

/**
 * rc_world_get_package:
 * @world: An #RCWorld.
 * @channel: A non-wildcard #RCChannel.
 * @name: The name of a package.
 * 
 * Searches @world for a package in the specified channel
 * with the specified name.  @channel must be an actual
 * channel, not a wildcard.
 * 
 * Return value: The matching package, or %NULL if no such
 * package exists.
 **/
RCPackage *
rc_world_get_package (RCWorld *world,
                      RCChannel *channel,
                      const char *name)
{
    GSList *slist;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel != RC_WORLD_ANY_CHANNEL
                          && channel != RC_WORLD_ANY_NON_SYSTEM, NULL);
    g_return_val_if_fail (name && *name, NULL);

    slist = g_hash_table_lookup (world->packages_by_name, name);
    while (slist) {
        RCPackage *package = slist->data;
        if (package && package->channel == channel)
            return package;
        slist = slist->next;
    }

    return NULL;
}

/**
 * rc_world_get_package_with_constraint:
 * @world: An #RCWorld.
 * @channel: A non-wildcard #RCChannel.
 * @name: The name of a package.
 * @constraint: An #RCPackageDep with package version information.
 * @is_and: An unused argument that is ignored.
 * 
 * Searches @world for a package in the specified channel
 * with the specified name, and whose version information is
 * exactly equal to that in @constraint.
 *
 * @channel must be an actual channel, not a wildcard.
 * If @constraint is %NULL, using this function is equivalent
 * to calling rc_world_get_package().
 * 
 * Return value: The matching package, or %NULL if no such
 * package exists.
 **/
RCPackage *
rc_world_get_package_with_constraint (RCWorld *world,
                                      RCChannel *channel,
                                      const char *name,
                                      RCPackageDep *constraint,
                                      gboolean is_and)
{
    RCPackage *pkg;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel != RC_WORLD_ANY_CHANNEL
                          && channel != RC_WORLD_ANY_NON_SYSTEM, NULL);
    g_return_val_if_fail (name && *name, NULL);
    
    pkg = rc_world_get_package (world, channel, name);

    if (pkg != NULL && constraint != NULL) {
        RCPackageDep *dep;
        dep = rc_package_dep_new_from_spec (&(pkg->spec), RC_RELATION_EQUAL);
        if (! rc_package_dep_verify_relation (constraint, dep))
            pkg = NULL;
        rc_package_dep_free (dep);
    }

    return pkg;
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
        RCPackage *package = iter->data;
        if (package && channel_match (info->channel, package->channel)) {
            if (info->fn)
                info->fn (package, info->user_data);
            ++info->count;
        }
    }
}

/**
 * rc_world_foreach_package:
 * @world: An #RCWorld.
 * @channel: An #RCChannel or channel wildcard.
 * @fn: A callback function.
 * @user_data: Pointer passed to the callback function.
 * 
 * Iterates across all of the packages stored in @world whose
 * channel matches @channel, invoking the callback function
 * @fn on each one.
 * 
 * Return value: The number of matching packages that the callback
 * function was invoked on, or -1 in case of an error.
 **/
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

/**
 * rc_world_foreach_package_by_name:
 * @world: An #RCWorld.
 * @name: The name of a package.
 * @channel: An #RCChannel or channel wildcard.
 * @fn: A callback function.
 * @user_data: Pointer passed to the callback function.
 * 
 * Iterates across all of the packages stored in @world
 * whose channel matches @channel and whose name matches @name,
 * invoking the callback function @fn on each one.
 * 
 * Return value: The number of matching packages that the
 * callback function was invoked on, or -1 in case of an error.
 **/
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
        if (package && channel_match (channel, package->channel)) {
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

/**
 * rc_world_foreach_upgrade:
 * @world: An #RCWorld.
 * @package: An #RCPackage.
 * @channel: An #RCChannel or channel wildcard.
 * @fn: A callback function.
 * @user_data: Pointer passed to the callback function. 
 * 
 * Searchs @world for all packages whose channel matches
 * @channel and that are an upgrade for @package.
 * (To be precise, an upgrade is a package with the same
 * name as @package but with a greater version number.)
 * 
 * Return value: The number of matching packages
 * that the callback functions was invoked on, or
 * -1 in the case of an error.
 **/
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

static void
get_best_upgrade_cb (RCPackage *package, gpointer user_data)
{
    RCPackage **best_upgrade = user_data;
    if (rc_package_spec_compare (*best_upgrade, package) < 0) {
        *best_upgrade = package;
    }
}

/**
 * rc_world_get_best_upgrade:
 * @world: An #RCWorld.
 * @package: A #RCPackage.
 * 
 * Searches @world for the package in any non-system channel and
 * returns the package it finds that has the highest version number
 * of all of the possibilities with the same name as @package.
 *
 * If @world does not contain any like-named packages with
 * higher version numbers that @package, %NULL is returned.
 *
 * If @world contains two or more non-system packages with identical
 * version numbers that are greater than that of @package, one
 * of the higher-versioned packages will be chosen at random to
 * be returned.
 * 
 * Return value: the highest-versioned upgrade for @package.
 **/
RCPackage *
rc_world_get_best_upgrade (RCWorld *world, RCPackage *package)
{
    RCPackage *best_upgrade;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    best_upgrade = package;

    rc_world_foreach_upgrade (world, package, RC_WORLD_ANY_NON_SYSTEM, get_best_upgrade_cb, &best_upgrade);

    if (best_upgrade == package)
        best_upgrade = NULL;

    return best_upgrade;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct SystemUpgradeInfo {
    RCWorld *world;
    RCPackagePairFn fn;
    gpointer user_data;
    int count;
};

static void
system_upgrade_cb (RCPackage *package, gpointer user_data)
{
    struct SystemUpgradeInfo *info = user_data;
    RCPackage *upgrade = rc_world_get_best_upgrade (info->world, package);

    if (upgrade) {
        if (info->fn)
            info->fn (package, upgrade, info->user_data);
        ++info->count;
    }
}

/**
 * rc_world_foreach_system_package_with_upgrade:
 * @world: An #RCWorld.
 * @fn: A callback function.
 * @user_data: Pointer to be passed to the callback function.
 *
 * Iterates across all system packages in @world for which there
 * exists an upgrade, and passes both the original package and
 * the upgrade package to the callback function.
 *
 * The upgrade package is determined by calling
 * rc_world_get_best_upgrade().
 * 
 * Return value: The number of matching packages that the callback
 * function was invoked on, or -1 in case of an error.
 **/
int
rc_world_foreach_system_package_with_upgrade (RCWorld *world,
                                              RCPackagePairFn fn,
                                              gpointer user_data)
{
    struct SystemUpgradeInfo info;

    g_return_val_if_fail (world != NULL, -1);
    
    info.world = world;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    rc_world_foreach_package (world, RC_WORLD_SYSTEM_PACKAGES,
                              system_upgrade_cb, &info);
    
    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/**
 * rc_world_foreach_providing_package:
 * @world: An #RCWorld.
 * @dep: An #RCPackageDep.
 * @channel: An #RCChannel or channel wildcard.
 * @fn: A callback function.
 * @user_data: Pointer to be passed to the callback function.
 * 
 * Iterates across all packages in @world whose channel matches
 * @channel and which provides a token that satisfied the
 * dependency @dep.  The package and the provided token are
 * passed to the callback function.
 *
 * If one package provides multiple tokens that satisfy @dep,
 * the callback function will be invoked multiple times, one
 * for each token.
 *
 * If @dep is an or-dependency, we iterate across all package/provide
 * pairs that satisfy any part of the or-dependency.  In this case, it
 * is possible for the callback to be invoked on same package/provide
 * pair multiple times.
 * 
 * Return value: The number of times that the callback
 * function was invoked, or -1 in case of an error.
 **/
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
        RCPackageAndDep *pad = iter->data;
        if (pad && pad->package && rc_package_is_installed (pad->package))
            g_hash_table_insert (installed, & pad->package->spec, pad);
    }

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndDep *pad = iter->data;

        if (pad
            && channel_match (channel, pad->package->channel)
            && rc_package_dep_verify_relation (dep, pad->dep)) {

            /* If we have multiple identical packages in RCWorld, we want to only
               include the package that is installed and skip the rest. */
            if (rc_package_is_installed (pad->package)
                || g_hash_table_lookup (installed, & pad->package->spec) == NULL) {
                
                if (fn)
                    fn (pad->package, &pad->dep->spec, user_data);
                ++count;
            }
        }

        slist = slist->next;
    }

    g_hash_table_destroy (installed);

    return count;
}

/**
 * rc_world_check_providing_package:
 * @world: An #RCWorld.
 * @dep: An #RCPackageDep.
 * @channel: An #RCChannel or channel wildcard.
 * @filter_dups_of_installed: If %TRUE, we skip uninstalled versions of installed packages.
 * This is the default behavior for the other rc_world_* iterator functions.
 * @fn: A callback function.
 * @user_data: Pointer to be passed to the callback function.
 * 
 * This function behaves almost exactly the same as rc_world_foreach_providing_package,
 * except in two ways.  It allows iteration over all packages in all channels, without
 * skipping uninstalled versions of installed packages, by passing %FALSE in for the
 * @filter_dups_of_installed argument.  Also, the callback function returns a #gboolean
 * and the iteratior will terminate if it returns %FALSE.
 * 
 * Return value: Returns %TRUE if the iteration was prematurely terminated by the
 * callback function, and %FALSE otherwise.
 **/
gboolean
rc_world_check_providing_package (RCWorld *world, RCPackageDep *dep,
                                  RCChannel *channel, gboolean filter_dups_of_installed,
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
                                                           channel, filter_dups_of_installed,
                                                           fn, user_data);
        }
        rc_package_dep_slist_free (deps);
        return terminated;
    }

    slist = (GSList *) g_hash_table_lookup (world->provides_by_name,
                                            dep->spec.name);

    installed = g_hash_table_new (rc_package_spec_hash,
                                  rc_package_spec_equal);

    if (filter_dups_of_installed) {
        for (iter = slist; iter != NULL; iter = iter->next) {
            RCPackageAndDep *pad = iter->data;
            if (pad && pad->package && rc_package_is_installed (pad->package))
                g_hash_table_insert (installed, & pad->package->spec, pad);
        }
    }

    for (iter = slist; iter != NULL; iter = iter->next) {
        RCPackageAndDep *pad = iter->data;

        if (pad
            && channel_match (channel, pad->package->channel)
            && rc_package_dep_verify_relation (dep, pad->dep)) {

            /* Skip uninstalled dups */
            if ((! filter_dups_of_installed) 
                || rc_package_is_installed (pad->package)
                || (g_hash_table_lookup (installed, & pad->package->spec) == NULL) ) {

                if (! fn (pad->package, &pad->dep->spec, user_data)) {
                    ret = TRUE;
                    break;
                }
            }
        }
    }

    g_hash_table_destroy (installed);

    return ret;
}

/**
 * rc_world_foreach_requiring_package:
 * @world: An #RCWorld.
 * @dep: An #RCPackageDep.
 * @channel: An #RCChannel or channel wildcard.
 * @fn: A callback function.
 * @user_data: Pointer to be passed to the callback function.
 * 
 * Iterates across all packages in @world whose channel matches
 * @channel and which provides a token that satisfied the
 * requirement @dep.  The package and the provided token are
 * passed to the callback function.
 *
 * If one package provides multiple tokens that satisfy @dep,
 * the callback function will be invoked multiple times, one
 * for each token.
 *
 * If @dep is an or-dependency, we iterate across all package/provide
 * pairs that satisfy any part of the or-dependency.  In this case, it
 * is possible for the callback to be invoked on same package/provide
 * pair multiple times.
 * 
 * Return value: The number of times that the callback
 * function was invoked, or -1 in case of an error.
 **/
int
rc_world_foreach_requiring_package (RCWorld *world, RCPackageDep *dep,
                                    RCChannel *channel,
                                    RCPackageAndDepFn fn, gpointer user_data)
{
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    slist = (GSList *) g_hash_table_lookup (world->requires_by_name,
                                            dep->spec.name);

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
            && channel_match (channel, pad->package->channel)
            && rc_package_dep_verify_relation (pad->dep, dep)) {

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
            fprintf (out, rc_package_to_str_static (package));
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

    fprintf (out, "PRV %s: ", name);
    while (iter) {
        RCPackageAndDep *pad = iter->data;
        if (pad) {
            fprintf (out, rc_package_to_str_static (pad->package));
            fprintf (out, "::");
            fprintf (out, rc_package_dep_to_str_static (pad->dep));
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
            fprintf (out, rc_package_to_str_static (pad->package));
            fprintf (out, "::");
            fprintf (out, rc_package_spec_to_str_static (& pad->dep->spec));
            fprintf (out, " ");
        } else {
            fprintf (out, "(null) ");
        }
        iter = iter->next;
    }
    fprintf (out, "\n");
}

/**
 * rc_world_spew:
 * @world: An #RCWorld.
 * @out: A file handle
 * 
 * Dumps the entire state of the #RCWorld out to the file
 * handle in a quasi-human-readable format.  This can be
 * useful for debugging in certain contexts.
 *
 **/
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
