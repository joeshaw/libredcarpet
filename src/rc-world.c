/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world.c
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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
#include "rc-world-import.h"

#include "rc-debug.h"
#include "rc-channel-private.h"
#include "rc-util.h"
#include "rc-dep-or.h"
#include "rc-xml.h"

struct _RCWorld {

    GHashTable *packages_by_name;
    GHashTable *provides_by_name;
    GHashTable *requires_by_name;
    GHashTable *conflicts_by_name;

    int freeze_count;

    GSList *channels;

    RCPackman *packman;

    /* The sequence number gets incremented every
       time the RCWorld is changed. */
    gboolean changed;
    guint seq_no;

    /* Set if we know package db has changed. */
    gboolean dirty;
};

typedef struct _RCPackageAndDep RCPackageAndDep;
struct _RCPackageAndDep {
    RCPackage *package;
    RCPackageDep *dep;
};

static RCPackageAndDep *
rc_package_and_dep_new_pair (RCPackage *package, RCPackageDep *dep)
{
    RCPackageAndDep *pad;

    pad = g_new0 (RCPackageAndDep, 1);
    pad->package = rc_package_ref (package);
    pad->dep = rc_package_dep_ref (dep);

    return pad;
}

void
rc_package_and_dep_free (RCPackageAndDep *pad)
{
    if (pad) {
        rc_package_dep_unref (pad->dep);
        rc_package_unref (pad->package);
        g_free (pad);
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct _SListAnchor SListAnchor;
struct _SListAnchor {
    GQuark key;
    GSList *slist;
};

static GHashTable *
hashed_slist_new (void)
{
    return g_hash_table_new (NULL, NULL);
}

static void
hashed_slist_destroy (GHashTable *hash)
{
    g_hash_table_destroy (hash);
}

static void
hashed_slist_add (GHashTable *hash,
                  GQuark key,
                  gpointer val)
{
    SListAnchor *anchor;

    anchor = g_hash_table_lookup (hash, GINT_TO_POINTER (key));

    if (anchor == NULL) {
        anchor = g_new0 (SListAnchor, 1);
        anchor->key = key;
        g_hash_table_insert (hash, GINT_TO_POINTER (anchor->key), anchor);
    }

    anchor->slist = g_slist_prepend (anchor->slist, val);
}

static GSList *
hashed_slist_get (GHashTable *hash,
                  GQuark key)
{
    SListAnchor *anchor;

    anchor = g_hash_table_lookup (hash, GINT_TO_POINTER (key));
    return anchor ? anchor->slist : NULL;
}

struct ForeachInfo {
    void (*fn) (gpointer, gpointer, gpointer);
    gpointer user_data;
};

static void
hashed_slist_foreach_cb (gpointer key, gpointer val, gpointer user_data)
{
    SListAnchor *anchor = val;
    GSList *iter = anchor->slist;
    struct ForeachInfo *info = user_data;

    while (iter) {
        info->fn (GINT_TO_POINTER (anchor->key), iter->data, info->user_data);
        iter = iter->next;
    }
}

static void
hashed_slist_foreach (GHashTable *hash,
                      void (*fn) (gpointer key, gpointer val, gpointer user_data),
                      gpointer user_data)
{
    struct ForeachInfo info;

    info.fn = fn;
    info.user_data = user_data;

    g_hash_table_foreach (hash,
                          hashed_slist_foreach_cb,
                          &info);
}

struct ForeachRemoveInfo {
    gboolean (*func) (gpointer item, gpointer user_data);
    gpointer user_data;
};

static gboolean
foreach_remove_func (gpointer key, gpointer val, gpointer user_data)
{
    SListAnchor *anchor = val;
    GSList *iter = anchor->slist;
    struct ForeachRemoveInfo *info = user_data;

    while (iter != NULL) {
        GSList *next = iter->next;
        if (info->func (iter->data, info->user_data)) {
            anchor->slist = g_slist_delete_link (anchor->slist, iter);
        }
        iter = next;
    }

    if (anchor->slist == NULL) {
        g_free (anchor);
        return TRUE;
    }
    
    return FALSE;
}

static void
hashed_slist_foreach_remove (GHashTable *hash,
                             gboolean (*func) (gpointer val, gpointer user_data),
                             gpointer user_data)
{
    struct ForeachRemoveInfo info;
    
    info.func = func;
    info.user_data = user_data;

    g_hash_table_foreach_remove (hash,
                                 foreach_remove_func,
                                 &info);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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

    world->changed = FALSE;
    world->seq_no  = 1;

	world->packages_by_name = hashed_slist_new ();
    world->provides_by_name = hashed_slist_new ();
    world->requires_by_name = hashed_slist_new ();
    world->conflicts_by_name = hashed_slist_new ();
	
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
        GSList *iter;

        rc_world_remove_packages (world, RC_WORLD_ANY_CHANNEL);

        hashed_slist_destroy (world->packages_by_name);
        hashed_slist_destroy (world->provides_by_name);
        hashed_slist_destroy (world->requires_by_name);
        hashed_slist_destroy (world->conflicts_by_name);

        /* Free our channels */
        for (iter = world->channels; iter != NULL; iter = iter->next) {
            RCChannel *channel = iter->data;

            /* Set the channel's world to NULL, so that there won't
               be a dangling pointer if someone else is holding a reference
               to this channel. */
            channel->world = NULL;

            rc_channel_unref (channel);
        }
        g_slist_free (world->channels);

		g_free (world);

	}
}

guint
rc_world_sequence_number (RCWorld *world)
{
    g_return_val_if_fail (world != NULL, 0);

    rc_world_sync (world);

    if (world->changed) {
        ++world->seq_no;
        world->changed = FALSE;
    }

    return world->seq_no;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/* Packmanish operations */

static void
rc_world_sync (RCWorld *world)
{
    if (world->packman == NULL)
        return;

    if (rc_packman_is_locked (world->packman)) {
        rc_debug (RC_DEBUG_LEVEL_MESSAGE,
                  "Skipping sync: database is locked");
        return;
    }

    if (!world->dirty && rc_packman_is_database_changed (world->packman))
        world->dirty = TRUE;

    if (world->dirty) {
        rc_debug (RC_DEBUG_LEVEL_MESSAGE,
                  "Database changed; rescanning system packages");

        if (rc_world_get_system_packages (world))
            world->dirty = FALSE;
    }
}

static void
rc_world_conditional_sync (RCWorld *world, RCChannel *channel)
{
    if (channel_match (channel, RC_WORLD_SYSTEM_PACKAGES))
        rc_world_sync (world);
}

static void
database_changed_cb (RCPackman *packman, gpointer user_data)
{
    RCWorld *world = user_data;
    
    world->dirty = TRUE;
    
    rc_world_sync (world);
}

void
rc_world_register_packman (RCWorld *world,
                           RCPackman *packman)
{
    g_return_if_fail (world != NULL);
    g_return_if_fail (RC_IS_PACKMAN (packman));
    
    if (world->packman) {
        g_warning ("Multiple packman registry is not allowed!");
        return;
    }

    world->packman = packman;

    g_signal_connect (packman,
                      "database_changed",
                      G_CALLBACK (database_changed_cb),
                      world);
}

RCPackman *
rc_world_get_packman (RCWorld *world)
{
    g_return_val_if_fail (world != NULL, NULL);

    return world->packman;
} /* rc_world_get_packman */

gboolean
rc_world_get_system_packages (RCWorld *world)
{
    GSList *system_packages = NULL;

    g_return_val_if_fail (world != NULL, FALSE);
    g_return_val_if_fail (world->packman != NULL, FALSE);

    system_packages = rc_packman_query_all (world->packman);
    if (rc_packman_get_error (world->packman)) {
        rc_debug (RC_DEBUG_LEVEL_MESSAGE,
                  "System query failed: %s",
                  rc_packman_get_reason (world->packman));
        if (system_packages)
            rc_package_slist_unref (system_packages);
        return FALSE;
    }

    rc_world_remove_packages (world, RC_WORLD_SYSTEM_PACKAGES);
    rc_world_add_packages_from_slist (world, system_packages);

    g_slist_free (system_packages);

    return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCChannel *
rc_world_add_channel (RCWorld *world,
                      const char *channel_name,
                      const char *alias,
                      guint32 channel_id,
                      guint32 base_id,
                      RCChannelType type)
{
    RCChannel *channel;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel_name && *channel_name, NULL);
    g_return_val_if_fail (alias, NULL);

    world->changed = TRUE;

    channel = rc_channel_new ();

    channel->world   = world;
    channel->id      = channel_id;
    channel->base_id = base_id;
    channel->name    = g_strdup (channel_name);
    channel->alias   = g_strdup (alias);
    channel->type    = type;
    
    world->channels = g_slist_prepend (world->channels,
                                       channel);

    rc_debug (RC_DEBUG_LEVEL_DEBUG,
              "Adding channel '%s' (cid=%d, bid=%d)",
              channel_name, channel_id, base_id);

    return channel;
}

void
rc_world_remove_channel (RCWorld *world,
                         RCChannel *channel)
{
    GSList *iter;

    g_return_if_fail (world != NULL);
    g_return_if_fail (channel != NULL);

    world->changed = TRUE;

    for (iter = world->channels; iter != NULL; iter = iter->next) {
        if (iter->data == channel) {

            /* We we remove a channel, we also remove all of its
               packages. */
            rc_world_remove_packages (world, channel);

            /* Set the channel's world to NULL, so that there won't
               be a dangling pointer if someone else is holding a reference
               to this channel. */
            channel->world = NULL;
            
            rc_channel_unref (channel);
            world->channels = g_slist_delete_link (world->channels,
                                                   iter);
            return;
        }
    }

    g_warning ("Couldn't remove channel '%s'", channel->name);
}

void
rc_world_foreach_channel (RCWorld *world,
                          RCChannelFn fn,
                          gpointer user_data)
{
    GSList *iter;
    GSList *next;

    g_return_if_fail (world != NULL);
    g_return_if_fail (fn != NULL);

    iter = world->channels;
    while (iter) {
        next = iter->next;

        fn (iter->data, user_data);

        iter = next;
    }
}

RCChannel *
rc_world_get_channel_by_name (RCWorld *world,
                              const char *channel_name)
{
    GSList *iter;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel_name && *channel_name, NULL);

    for (iter = world->channels; iter != NULL; iter = iter->next) {
        RCChannel *channel = iter->data;
        if (!g_strcasecmp (channel->name, channel_name))
            return channel;
    }

    return NULL;
}

RCChannel *
rc_world_get_channel_by_alias (RCWorld *world,
                               const char *alias)
{
    GSList *iter;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (alias && *alias, NULL);

    for (iter = world->channels; iter != NULL; iter = iter->next) {
        RCChannel *channel = iter->data;
        if (channel->alias
            && !g_strcasecmp (channel->alias, alias))
            return channel;
    }
    
    return NULL;
}

RCChannel *
rc_world_get_channel_by_id (RCWorld *world,
                            guint32 channel_id)
{
    GSList *iter;

    g_return_val_if_fail (world != NULL, NULL);

    for (iter = world->channels; iter != NULL; iter = iter->next) {
        RCChannel *channel = iter->data;
        if (channel->id == channel_id)
            return channel;
    }

    return NULL;
}

RCChannel *
rc_world_get_channel_by_base_id (RCWorld *world,
                                 guint32 base_id)
{
    GSList *iter;

    g_return_val_if_fail (world != NULL, NULL);

    for (iter = world->channels; iter != NULL; iter = iter->next) {
        RCChannel *channel = iter->data;
        if (channel->base_id == base_id)
            return channel;
    }

    return NULL;
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
        g_hash_table_freeze (world->conflicts_by_name);

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
        g_hash_table_thaw (world->conflicts_by_name);

    }
}

#ifdef SIZE_TRACKING_FUNCTIONS
static int
spec_size (RCPackageSpec *spec)
{
    int size = sizeof (RCPackageSpec);
    if (spec->name)
        size += strlen (spec->name);
    if (spec->version)
        size += strlen (spec->version);
    if (spec->release)
        size += strlen (spec->release);
    return size;
}

static int
dep_size (RCPackageDep *dep)
{
    int size = spec_size (&dep->spec);
    size += sizeof (RCPackageDep) - sizeof (RCPackageSpec);

    return size;
}

static int
dep_slist_size (RCPackageDepSList *slist)
{
    int size = 0;

    while (slist) {
        size += sizeof (*slist);
        size += dep_size (slist->data);
        slist = slist->next;
    }

    return size;
}

static int
package_size (RCPackage *package)
{
    int size = spec_size (&package->spec);
    size += sizeof (RCPackage) - sizeof (RCPackageSpec);
    
    if (package->summary)
        size += strlen (package->summary);
    if (package->description)
        size += strlen (package->description);
    if (package->package_filename)
        size += strlen (package->package_filename);
    if (package->signature_filename)
        size += strlen (package->signature_filename);

    size += dep_slist_size (package->requires);
    size += dep_slist_size (package->provides);
    size += dep_slist_size (package->conflicts);
    size += dep_slist_size (package->obsoletes);
    size += dep_slist_size (package->suggests);
    size += dep_slist_size (package->recommends);

    return size;
}
#endif

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
//    const RCPackageDepSList *iter;
    RCPackageAndDep *pad;
    int i;

    g_return_if_fail (world != NULL);
    g_return_if_fail (package != NULL);

    world->changed = TRUE;

    /* The world holds a reference to the package */
    rc_package_ref (package);

    /* Store all of our packages in a hash by name. */
    hashed_slist_add (world->packages_by_name,
                      package->spec.nameq,
                      package);

    /* Store all of the package's provides in a hash by name. */

    if (package->provides_a)
        for (i = 0; i < package->provides_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->provides_a->data[i]);

            hashed_slist_add (world->provides_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    /* Store all of the package's requires in a hash by name. */

    if (package->requires_a)
        for (i = 0; i < package->requires_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->requires_a->data[i]);

            hashed_slist_add (world->requires_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    /* Store all of the package's conflicts in a hash by name. */

    if (package->conflicts_a)
        for (i = 0; i < package->conflicts_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->conflicts_a->data[i]);

            hashed_slist_add (world->conflicts_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_package_cb (gpointer val, gpointer user_data)
{
    RCPackage *package = val;
    RCPackage *package_to_remove = user_data;

    if (package_to_remove && package == package_to_remove) {
        rc_package_unref (package);
        return TRUE;
    }

    return FALSE;
}

static gboolean
remove_package_struct_cb (gpointer val, gpointer user_data)
{
    RCPackageAndDep *pad = val;
    RCPackage *package_to_remove = user_data;

    if (pad && package_to_remove && pad->package == package_to_remove) {
        rc_package_and_dep_free (pad);
        return TRUE;
    }

    return FALSE;
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

    world->changed = TRUE;

    /* FIXME: This is fairly inefficient */

    hashed_slist_foreach_remove (world->provides_by_name,
                                 remove_package_struct_cb,
                                 package);

    hashed_slist_foreach_remove (world->requires_by_name,
                                 remove_package_struct_cb,
                                 package);

    hashed_slist_foreach_remove (world->conflicts_by_name,
                                 remove_package_struct_cb,
                                 package);
    
    hashed_slist_foreach_remove (world->packages_by_name,
                                 remove_package_cb,
                                 package);

}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_package_by_channel_cb (gpointer val, gpointer user_data)
{
    RCPackage *package = val;
    RCChannel *channel = user_data;

    if (package && channel_match (package->channel, channel)) {
        rc_package_unref (package);
        return  TRUE;
    }

    return FALSE;
}

static gboolean
remove_package_struct_by_channel_cb (gpointer val, gpointer user_data)
{
    RCPackageAndDep *pad = val;
    RCChannel *channel = user_data;
    
    if (pad && channel_match (pad->package->channel, channel)) {
        rc_package_and_dep_free (pad);
        return TRUE;
    }

    return FALSE;
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

    world->changed = TRUE;

    hashed_slist_foreach_remove (world->provides_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);
    
    hashed_slist_foreach_remove (world->requires_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);

    hashed_slist_foreach_remove (world->conflicts_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);

    hashed_slist_foreach_remove (world->packages_by_name,
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

    rc_world_sync (world);

    iter = hashed_slist_get (world->packages_by_name,
                             package->spec.nameq);
    
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

    rc_world_conditional_sync (world, channel);

    slist = hashed_slist_get (world->packages_by_name,
                              g_quark_try_string (name));
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

    /* rc_world_get_package will call rc_world_sync */
    
    pkg = rc_world_get_package (world, channel, name);

    if (pkg != NULL && constraint != NULL) {
        RCPackageDep *dep;
        dep = rc_package_dep_new_from_spec (&(pkg->spec), RC_RELATION_EQUAL,
                                            FALSE, FALSE);
        if (! rc_package_dep_verify_relation (constraint, dep))
            pkg = NULL;
        rc_package_dep_unref (dep);
    }

    return pkg;
}

struct GuessChannelInfo {
    RCPackage *installed_pkg;
    RCChannel *guess;
};

/**
 * rc_world_guess_package_channel:
 * @world: An #RCWorld
 * @package: An #RCPackage
 *
 * If @package is an installed package, we try to guess which channel
 * it originated from.  This cannot be done in a fool-proof manner ---
 * if we just can't figure out where it came from, %NULL is returned.
 * This can happen because the package is from a Red Carpet channel
 * but is too old, or if the package is not available from Red Carpet
 * and was installed locally from a downloaded package.
 *
 * If @package is from a channel, just return the channel.
 **/
RCChannel *
rc_world_guess_package_channel (RCWorld *world,
                                RCPackage *package)
{
    GSList *iter;
    RCPackageUpdateSList *updates;
    
    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    if (package->channel != NULL)
        return package->channel;

    /* We don't need to call rc_world_sync, because we are searching
       over the channel data --- not the installed packages. */

    iter = hashed_slist_get (world->packages_by_name,
                             RC_PACKAGE_SPEC (package)->nameq);

    while (iter != NULL) {
        RCPackage *iter_pkg = iter->data;

        if (iter_pkg && iter_pkg->channel) {
            updates = iter_pkg->history;

            while (updates != NULL) {

                if (rc_package_spec_equal (RC_PACKAGE_SPEC (updates->data),
                                           RC_PACKAGE_SPEC (package)))
                    return iter_pkg->channel;
                
                updates = updates->next;
            }
        }

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
    RCPackage *package = val;

    /* FIXME: we should filter out dup uninstalled packages. */

    if (package && channel_match (info->channel, package->channel)) {
        if (info->fn)
            info->fn (package, info->user_data);
        ++info->count;
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

    rc_world_conditional_sync (world, channel);
    
    info.channel = channel;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    hashed_slist_foreach (world->packages_by_name,
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

    rc_world_conditional_sync (world, channel);

    slist = hashed_slist_get (world->packages_by_name,
                              g_quark_try_string (name));

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

    rc_world_conditional_sync (world, channel);

    info.original_package = package;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    rc_world_foreach_package_by_name (world,
                                      g_quark_to_string (package->spec.nameq),
                                      channel,
                                      foreach_upgrade_cb,
                                      &info);

    return info.count;
}

struct BestUpgradeInfo {
    RCPackage *best_upgrade;
    gboolean subscribed_only;
};

static void
get_best_upgrade_cb (RCPackage *package, gpointer user_data)
{
    struct BestUpgradeInfo *info = user_data;

    if (info->subscribed_only)
        if (!(package->channel && rc_channel_subscribed (package->channel)))
            return;

    if (rc_package_spec_compare (info->best_upgrade, package) < 0) {
        info->best_upgrade = package;
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
rc_world_get_best_upgrade (RCWorld *world, RCPackage *package,
                           gboolean subscribed_only)
{
    struct BestUpgradeInfo info;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    /* rc_world_foreach_upgrade calls rc_world_sync */

    info.best_upgrade = package;
    info.subscribed_only = subscribed_only;

    rc_world_foreach_upgrade (world, package, RC_WORLD_ANY_NON_SYSTEM, get_best_upgrade_cb, &info);

    if (info.best_upgrade == package)
        info.best_upgrade = NULL;

    return info.best_upgrade;
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
    RCPackage *upgrade = rc_world_get_best_upgrade (info->world, package,
                                                    TRUE);

    if (upgrade) {
        if (info->fn)
            info->fn (package, upgrade, info->user_data);
        ++info->count;
    }
}

/**
 * rc_world_foreach_system_upgrade:
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
rc_world_foreach_system_upgrade (RCWorld *world,
                                 RCPackagePairFn fn,
                                 gpointer user_data)
{
    struct SystemUpgradeInfo info;

    g_return_val_if_fail (world != NULL, -1);

    /* rc_world_foreach_package calls rc_world_sync */
    
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

    if (rc_package_dep_is_or (dep)) {
        RCPackageDepSList *deps, *iter;
        deps = rc_dep_string_to_or_dep_slist (
            g_quark_to_string (RC_PACKAGE_SPEC (dep)->nameq));
        for (iter = deps; iter != NULL; iter = iter->next) {
            count += rc_world_foreach_providing_package (world, iter->data,
                                                         channel, fn, user_data);
        }
        g_slist_free (deps);
        return count;
    }

    rc_world_conditional_sync (world, channel);

    slist = hashed_slist_get (world->provides_by_name,
                              RC_PACKAGE_SPEC (dep)->nameq);

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
                    fn (pad->package, RC_PACKAGE_SPEC (pad->dep), user_data);
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

    if (rc_package_dep_is_or (dep)) {
        RCPackageDepSList *deps, *iter;
        gboolean terminated = FALSE;
        deps = rc_dep_string_to_or_dep_slist (
            g_quark_to_string (RC_PACKAGE_SPEC (dep)->nameq));
        for (iter = deps; iter != NULL && !terminated; iter = iter->next) {
            terminated = rc_world_check_providing_package (world, iter->data,
                                                           channel, filter_dups_of_installed,
                                                           fn, user_data);
        }
        rc_package_dep_slist_free (deps);
        return terminated;
    }

    rc_world_conditional_sync (world, channel);

    slist = hashed_slist_get (world->provides_by_name,
                              RC_PACKAGE_SPEC (dep)->nameq);

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

                if (! fn (pad->package, RC_PACKAGE_SPEC (pad->dep), user_data)) {
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

    rc_world_conditional_sync (world, channel);

    slist = hashed_slist_get (world->requires_by_name,
                              RC_PACKAGE_SPEC (dep)->nameq);

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

int
rc_world_foreach_conflicting_package (RCWorld *world, RCPackageDep *dep,
                                      RCChannel *channel,
                                      RCPackageAndDepFn fn, gpointer user_data)
{
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    rc_world_conditional_sync (world, channel);

    slist = hashed_slist_get (world->conflicts_by_name,
                              RC_PACKAGE_SPEC (dep)->nameq);

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

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */


static void
foreach_package_by_name_cb (gpointer key, gpointer val, gpointer user_data)
{
    FILE *out = user_data;
    char *name = key;
    SListAnchor *anchor = val;
    GSList *iter = anchor->slist;

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
    SListAnchor *anchor = val;
    GSList *iter = anchor->slist;

    fprintf (out, "PRV %s: ", name);
    while (iter) {
        RCPackageAndDep *pad = iter->data;
        if (pad) {
            fprintf (out, rc_package_to_str_static (pad->package));
            fprintf (out, "::");
            fprintf (out, rc_package_dep_to_string_static (pad->dep));
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
    SListAnchor *anchor = val;
    GSList *iter = anchor->slist;

    fprintf (out, "REQ %s: ", name);
    while (iter) {
        RCPackageAndDep *pad = iter->data;
        if (pad) {
            fprintf (out, rc_package_to_str_static (pad->package));
            fprintf (out, "::");
            fprintf (out, rc_package_spec_to_str_static (
                         RC_PACKAGE_SPEC (pad->dep)));
            fprintf (out, " ");
        } else {
            fprintf (out, "(null) ");
        }
        iter = iter->next;
    }
    fprintf (out, "\n");
}

static void
foreach_conflicts_by_name_cb (gpointer key, gpointer val, gpointer user_data)
{
    FILE *out = user_data;
    char *name = key;
    SListAnchor *anchor = val;
    GSList *iter = anchor->slist;

    fprintf (out, "CON %s: ", name);
    while (iter) {
        RCPackageAndDep *pad = iter->data;
        if (pad) {
            fprintf (out, rc_package_to_str_static (pad->package));
            fprintf (out, "::");
            fprintf (out, rc_package_spec_to_str_static (
                         RC_PACKAGE_SPEC (pad->dep)));
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
    rc_world_sync (world);

    g_hash_table_foreach (world->packages_by_name,
                          foreach_package_by_name_cb,
                          out);

    g_hash_table_foreach (world->provides_by_name,
                          foreach_provides_by_name_cb,
                          out);

    g_hash_table_foreach (world->requires_by_name,
                          foreach_requires_by_name_cb,
                          out);

    g_hash_table_foreach (world->conflicts_by_name,
                          foreach_conflicts_by_name_cb,
                          out);

}
