/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-store.c
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
#include "rc-world-store.h"

#include "rc-arch.h"
#include "rc-channel.h"
#include "rc-debug.h"
#include "rc-xml.h"

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

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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

/* This function also checks channels in addition to just dep relations */
gboolean
rc_package_and_dep_verify_relation (RCPackageAndDep *pad, RCPackageDep *dep)
{
    RCPackman *packman;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    if (!rc_package_dep_verify_relation (packman, dep, pad->dep))
        return FALSE;

    return rc_channel_equal (rc_package_get_channel (pad->package),
                             rc_package_dep_get_channel (dep));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct _ChannelInfo ChannelInfo;
struct _ChannelInfo {
    RCChannel *channel;
};

static ChannelInfo *
channel_info_new (RCChannel *channel)
{
    ChannelInfo *info = g_new0 (ChannelInfo, 1);
    info->channel = rc_channel_ref (channel);
    return info;
}

static void
channel_info_free (ChannelInfo *info)
{
    if (info != NULL) {
        rc_channel_unref (info->channel);
        g_free (info);
    }
}

#if 0
static ChannelInfo *
rc_world_store_find_channel_info (RCWorldStore *store, RCChannel *channel)
{
    GSList *iter;
    for (iter = store->channels; iter != NULL; iter = iter->next) {
        ChannelInfo *info = iter->data;
        if (info->channel == channel)
            return info;
    }
    return NULL;
}
#endif

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct {
    RCWorld *old_world;
    RCWorld *new_world;
} DupInfo;

static gboolean
package_dup_fn (RCPackage *package,
                gpointer user_data)
{
    DupInfo *info = user_data;
    rc_world_store_add_package (RC_WORLD_STORE (info->new_world), package);

    return TRUE;
}

static gboolean
channel_dup_fn (RCChannel *channel,
                gpointer user_data)
{
    DupInfo *info = user_data;

    rc_world_store_add_channel (RC_WORLD_STORE (info->new_world), channel);
    rc_world_foreach_package (info->old_world, channel,
                              package_dup_fn,
                              info);

    return TRUE;
}

static RCWorld *
rc_world_store_dup_fn (RCWorld *old_world)
{
    RCWorld *new_world;
    DupInfo info;
    GSList *l;

    new_world = g_object_new (G_TYPE_FROM_INSTANCE (old_world), NULL);

    info.old_world = old_world;
    info.new_world = new_world;
    
    rc_world_foreach_channel (old_world,
                              channel_dup_fn,
                              &info);
    for (l = RC_WORLD_STORE (old_world)->locks; l; l = l->next) {
        rc_world_store_add_lock (RC_WORLD_STORE (new_world),
                                 (RCPackageMatch *)l->data);
    }    

    return new_world;
}

static int
rc_world_store_foreach_channel_fn (RCWorld    *world,
                                   RCChannelFn callback,
                                   gpointer    user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;
    GSList *iter, *next;
    int count = 0;

    iter = store->channels;
    while (iter) {
        ChannelInfo *info = iter->data;
        next = iter->next;
        if (! callback (info->channel, user_data))
            return -1;
        iter = next;
        ++count;
    }

    return count;
}

static int
rc_world_store_foreach_lock_fn (RCWorld         *world,
                                RCPackageMatchFn callback,
                                gpointer         user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;
    GSList *iter, *next;
    int count = 0;

    iter = store->locks;
    while (iter) {
        next = iter->next;
        if (! callback (iter->data, user_data))
            return -1;
        iter = next;
        ++count;
    }

    return count;
}

struct ForeachPackageInfo {
    RCChannel *channel;
    RCPackageFn callback;
    gpointer user_data;
    int count;
    gboolean short_circuit;
};

static void
foreach_package_cb (gpointer key, gpointer val, gpointer user_data)
{
    struct ForeachPackageInfo *info = user_data;
    RCPackage *package = val;

    if (info->short_circuit)
        return;

    /* FIXME: we should filter out dup uninstalled packages. */

    if (package && rc_channel_equal (info->channel, package->channel)) {
        if (info->callback) {
            if (! info->callback (package, info->user_data))
                info->short_circuit = TRUE;
        }
        ++info->count;
    }
}

static int
rc_world_store_foreach_package_fn (RCWorld    *world,
                                   const char *name,
                                   RCChannel  *channel,
                                   RCPackageFn callback,
                                   gpointer    user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;

    if (name != NULL) {
        
        GSList *slist, *iter;
        GHashTable *installed;
        int count = 0;

        slist = hashed_slist_get (store->packages_by_name,
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
            if (package && rc_channel_equal (channel, package->channel)) {
                if (rc_package_is_installed (package)
                    || g_hash_table_lookup (installed, &package->spec) == NULL) {
                    if (callback) {
                        if (! callback (package, user_data)) {
                            count = -1;
                            goto finished;
                        }
                    }
                    ++count;
                }
            }
        }
        
    finished:
        g_hash_table_destroy (installed);
        
        return count;

    } else {

        struct ForeachPackageInfo info;

        info.channel = channel;
        info.callback = callback;
        info.user_data = user_data;
        info.count = 0;
        info.short_circuit = FALSE;
        
        hashed_slist_foreach (store->packages_by_name,
                              foreach_package_cb,
                              &info);

        return info.short_circuit ? -1 : info.count;
    }
}

static int
rc_world_store_foreach_providing_fn (RCWorld           *world,
                                     RCPackageDep      *dep,
                                     RCPackageAndSpecFn callback,
                                     gpointer           user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;
    GSList *slist, *iter;
    int count = 0;
    GHashTable *installed;
    
    slist = hashed_slist_get (store->provides_by_name,
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
            && rc_package_and_dep_verify_relation (pad, dep)) {
            /* If we have multiple identical packages in RCWorld,
               we want to only include the package that is installed and
               skip the rest. */
            if (rc_package_is_installed (pad->package)
                || g_hash_table_lookup (installed, &pad->package->spec) == NULL) {
                
                if (callback) {
                    if (! callback (pad->package,
                                    RC_PACKAGE_SPEC (pad->dep),
                                    user_data)) {
                        count = -1;
                        goto finished;
                    }
                }
                ++count;
            }
        }

        slist = slist->next;
    }

 finished:
    g_hash_table_destroy (installed);

    return count;
}

static int
rc_world_store_foreach_requiring_fn (RCWorld          *world,
                                     RCPackageDep     *dep,
                                     RCPackageAndDepFn callback,
                                     gpointer          user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;
    RCPackman *packman;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    slist = hashed_slist_get (store->requires_by_name,
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

        if (pad && rc_package_dep_verify_relation (packman, pad->dep, dep)) {

            /* Skip dups if one of them in installed. */
            if (rc_package_is_installed (pad->package)
                || g_hash_table_lookup (installed, & pad->package->spec) == NULL) {

                if (callback) {
                    if (! callback (pad->package, pad->dep, user_data)) {
                        count = -1;
                        goto finished;
                    }
                }
                ++count;
            }
        }
    }

 finished:
    g_hash_table_destroy (installed);

    return count;
}

static int
rc_world_store_foreach_parent_fn (RCWorld          *world,
                                  RCPackageDep     *dep,
                                  RCPackageAndDepFn callback,
                                  gpointer          user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;
    RCPackman *packman;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    slist = hashed_slist_get (store->children_by_name,
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

        if (pad && rc_package_dep_verify_relation (packman, pad->dep, dep)) {

            /* Skip dups if one of them in installed. */
            if (rc_package_is_installed (pad->package)
                || g_hash_table_lookup (installed, &pad->package->spec) == NULL) {

                if (callback) {
                    if (! callback (pad->package, pad->dep, user_data)) {
                        count = -1;
                        goto finished;
                    }
                }
                ++count;
            }
        }
    }

 finished:
    g_hash_table_destroy (installed);

    return count;
}

static int
rc_world_store_foreach_conflicting_fn (RCWorld          *world,
                                       RCPackageDep     *dep,
                                       RCPackageAndDepFn callback,
                                       gpointer          user_data)
{
    RCWorldStore *store = (RCWorldStore *) world;
    GSList *slist, *iter;
    GHashTable *installed;
    int count = 0;
    RCPackman *packman;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    slist = hashed_slist_get (store->conflicts_by_name,
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

        if (pad && rc_package_dep_verify_relation (packman, pad->dep, dep)) {

            /* Skip dups if one of them in installed. */
            if (rc_package_is_installed (pad->package)
                || g_hash_table_lookup (installed, & pad->package->spec) == NULL) {

                if (callback) {
                    if (! callback (pad->package, pad->dep, user_data)) {
                        count = -1;
                        goto finished;
                    }
                }
                ++count;
            }
        }
    }

 finished:
    g_hash_table_destroy (installed);

    return count;
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static RCWorldClass *parent_class;

static void
rc_world_store_finalize (GObject *obj)
{
    RCWorldStore *store = RC_WORLD_STORE (obj);

    /* Don't emit any changed signals while we clean up. */
    ((RCWorld *) store)->no_changed_signals = TRUE;
    rc_world_store_remove_packages (store, RC_CHANNEL_ANY);

    hashed_slist_destroy (store->packages_by_name);
    hashed_slist_destroy (store->provides_by_name);
    hashed_slist_destroy (store->requires_by_name);
    hashed_slist_destroy (store->children_by_name);
    hashed_slist_destroy (store->conflicts_by_name);

    g_slist_foreach (store->channels, (GFunc) channel_info_free, NULL);
    g_slist_free (store->channels);

    rc_world_store_clear_locks (store);

    if (G_OBJECT_CLASS (parent_class)->finalize)
        G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
rc_world_store_class_init (RCWorldStoreClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    RCWorldClass *world_class = (RCWorldClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    object_class->finalize = rc_world_store_finalize;

    world_class->dup_fn                 = rc_world_store_dup_fn;
    world_class->foreach_channel_fn     = rc_world_store_foreach_channel_fn;
    world_class->foreach_lock_fn        = rc_world_store_foreach_lock_fn;
    world_class->foreach_package_fn     = rc_world_store_foreach_package_fn;
    world_class->foreach_providing_fn   = rc_world_store_foreach_providing_fn;
    world_class->foreach_requiring_fn   = rc_world_store_foreach_requiring_fn;
    world_class->foreach_parent_fn      = rc_world_store_foreach_parent_fn;
    world_class->foreach_conflicting_fn = rc_world_store_foreach_conflicting_fn;
}

static void
rc_world_store_init (RCWorldStore *store)
{
    store->packages_by_name = hashed_slist_new ();
    store->provides_by_name = hashed_slist_new ();
    store->requires_by_name = hashed_slist_new ();
    store->children_by_name = hashed_slist_new ();
    store->conflicts_by_name = hashed_slist_new ();
}

GType
rc_world_store_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldStoreClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_store_class_init,
            NULL, NULL,
            sizeof (RCWorldStore),
            0,
            (GInstanceInitFunc) rc_world_store_init
        };

        type = g_type_register_static (RC_TYPE_WORLD,
                                       "RCWorldStore",
                                       &type_info,
                                       0);
    }

    return type;
}

RCWorld *
rc_world_store_new (void)
{
    return g_object_new (RC_TYPE_WORLD_STORE, NULL);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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

static void
rc_world_store_freeze (RCWorldStore *store)
{
    g_assert (store->freeze_count >= 0);
    ++store->freeze_count;
    if (store->freeze_count == 1) {
        g_hash_table_freeze (store->packages_by_name);
        g_hash_table_freeze (store->provides_by_name);
        g_hash_table_freeze (store->requires_by_name);
        g_hash_table_freeze (store->children_by_name);
        g_hash_table_freeze (store->conflicts_by_name);
    }
}

static void
rc_world_store_thaw (RCWorldStore *store)
{
    g_assert (store->freeze_count > 0);
    --store->freeze_count;
    if (store->freeze_count == 0) {
        g_hash_table_thaw (store->packages_by_name);
        g_hash_table_thaw (store->provides_by_name);
        g_hash_table_thaw (store->requires_by_name);
        g_hash_table_thaw (store->children_by_name);
        g_hash_table_thaw (store->conflicts_by_name);
    }
}

gboolean
rc_world_store_add_package (RCWorldStore *store,
                            RCPackage    *package)
{
    RCWorld *world = (RCWorld *) store;
    GSList *compat_arch_list;
    RCPackageAndDep *pad;
    const char *package_name;
    int i, arch_score;
    gboolean actually_added_package = FALSE;

    g_return_val_if_fail (store != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    compat_arch_list = \
        rc_arch_get_compat_list (rc_arch_get_system_arch ());

    arch_score = rc_arch_get_compat_score (compat_arch_list,
                                           package->spec.arch);

    /* Before we do anything, check to make sure that a package of the
       same name isn't already in that channel.  If there is a
       duplicate, we keep the one with the most recent version number
       and drop the other.

       This check only applies to packages in a channel.  We have
       to allow for multiple installs.  Grrrr...
    */

    if (! rc_package_is_installed (package)) {

        RCPackage *dup_package;
        int dup_arch_score;

        /* Filter out packages with totally incompatible arches */
        if (arch_score < 0) {
            rc_debug (RC_DEBUG_LEVEL_DEBUG,
                      "Ignoring package with incompatible arch: %s",
                      rc_package_to_str_static (package));
            goto finished;
        }

        
        package_name = g_quark_to_string (RC_PACKAGE_SPEC (package)->nameq);
        dup_package = rc_world_get_package ((RCWorld *) store,
                                            package->channel,
                                            package_name);


        /* This shouldn't happen (and would be caught by the check
           below, because cmp will equal 0), but it never hurts to
           check and produce a more explicit warning message. */

        if (package == dup_package) {
            rc_debug (RC_DEBUG_LEVEL_WARNING,
                      "Ignoring re-add of package '%s'",
                      package_name);
            goto finished;
        }

        if (dup_package != NULL) {
            RCPackman *packman;
            int cmp;

            packman = rc_packman_get_global ();
            g_assert (packman != NULL);

            cmp = rc_packman_version_compare (packman,
                                              RC_PACKAGE_SPEC (package),
                                              RC_PACKAGE_SPEC (dup_package));

            dup_arch_score = rc_arch_get_compat_score (compat_arch_list,
                                                       dup_package->spec.arch);
        

            /* If the package we are trying to add has a lower 
               version number, just ignore it. */

            if (cmp < 0) {
                rc_debug (RC_DEBUG_LEVEL_INFO,
                          "Not adding package '%s'.  A newer version is "
                          "already in the channel.",
                          rc_package_to_str_static (package));
                goto finished;
            }


            /* If the version numbers are equal, we ignore the package to
               add if it has a less-preferable arch.  If both
               packages have the same version # and arch, we favor the
               first package and just return. */

            if (cmp == 0 && arch_score > dup_arch_score) {
                rc_debug (RC_DEBUG_LEVEL_INFO,
                          "Not adding package '%s'.  Another package "
                          "with the same version but with a preferred arch "
                          "is already in the channel.",
                          rc_package_to_str_static (package));
                goto finished;
            }


            /* Otherwise we throw out the old package and proceed with
               adding the newer one. */

            rc_debug (RC_DEBUG_LEVEL_INFO,
                      "Replacing package '%s'.  Another package in "
                      "the channel has the same name and a superior %s.",
                      rc_package_to_str_static (dup_package),
                      cmp ? "version" : "arch");

            rc_world_store_remove_package (store, dup_package);
        }
    }

    actually_added_package = TRUE;

    if (! (package->channel && rc_channel_is_hidden (package->channel)))
        rc_world_touch_package_sequence_number (world);

    /* The world holds a reference to the package */
    rc_package_ref (package);

    /* Store all of our packages in a hash by name. */
    hashed_slist_add (store->packages_by_name,
                      package->spec.nameq,
                      package);

    /* Store all of the package's provides in a hash by name. */

    if (package->provides_a)
        for (i = 0; i < package->provides_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->provides_a->data[i]);

            hashed_slist_add (store->provides_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    /* Store all of the package's requires in a hash by name. */

    if (package->requires_a)
        for (i = 0; i < package->requires_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->requires_a->data[i]);

            hashed_slist_add (store->requires_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    /* Store all of the package's children in a hash by name. */

    if (package->children_a)
        for (i = 0; i < package->children_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->children_a->data[i]);

            hashed_slist_add (store->children_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    /* "Recommends" are treated as requirements. */

    if (package->recommends_a)
        for (i = 0; i < package->recommends_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->recommends_a->data[i]);

            hashed_slist_add (store->requires_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    /* Store all of the package's conflicts in a hash by name. */

    if (package->conflicts_a)
        for (i = 0; i < package->conflicts_a->len; i++) {
            pad = rc_package_and_dep_new_pair (
                package, package->conflicts_a->data[i]);

            hashed_slist_add (store->conflicts_by_name,
                              RC_PACKAGE_SPEC (pad->dep)->nameq,
                              pad);
        }

    
 finished:
    /* Clean-up */
    g_slist_free (compat_arch_list);

    return actually_added_package;
}

void
rc_world_store_add_packages_from_slist (RCWorldStore   *store,
                                        RCPackageSList *slist)
{
    g_return_if_fail (store != NULL && RC_IS_WORLD_STORE (store));

    rc_world_store_freeze (store);

    while (slist) {
        rc_world_store_add_package (store, (RCPackage *) slist->data);
        slist = slist->next;
    }

    rc_world_store_thaw (store);
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

void
rc_world_store_remove_package (RCWorldStore *store,
                               RCPackage    *package)
{
    RCWorld *world = (RCWorld *) store;

    g_return_if_fail (store != NULL);
    g_return_if_fail (package != NULL);

    if (! (package->channel && rc_channel_is_hidden (package->channel)))
        rc_world_touch_package_sequence_number (world);

    /* FIXME: This is fairly inefficient */

    hashed_slist_foreach_remove (store->provides_by_name,
                                 remove_package_struct_cb,
                                 package);

    hashed_slist_foreach_remove (store->requires_by_name,
                                 remove_package_struct_cb,
                                 package);

    hashed_slist_foreach_remove (store->children_by_name,
                                 remove_package_struct_cb,
                                 package);

    hashed_slist_foreach_remove (store->conflicts_by_name,
                                 remove_package_struct_cb,
                                 package);
    
    hashed_slist_foreach_remove (store->packages_by_name,
                                 remove_package_cb,
                                 package);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
remove_package_by_channel_cb (gpointer val, gpointer user_data)
{
    RCPackage *package = val;
    RCChannel *channel = user_data;

    if (package && rc_channel_equal (package->channel, channel)) {
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
    
    if (pad && rc_channel_equal (pad->package->channel, channel)) {
        rc_package_and_dep_free (pad);
        return TRUE;
    }

    return FALSE;
}

void
rc_world_store_remove_packages (RCWorldStore *store,
                                RCChannel    *channel)
{
    g_return_if_fail (store != NULL);

    if (channel == RC_CHANNEL_SYSTEM
        || channel == RC_CHANNEL_ANY
        || channel == RC_CHANNEL_NON_SYSTEM
        || ! rc_channel_is_hidden (channel))
        rc_world_touch_package_sequence_number ((RCWorld *) store);

    rc_world_store_freeze (store);

    hashed_slist_foreach_remove (store->provides_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);
    
    hashed_slist_foreach_remove (store->requires_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);

    hashed_slist_foreach_remove (store->children_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);

    hashed_slist_foreach_remove (store->conflicts_by_name,
                                 remove_package_struct_by_channel_cb,
                                 channel);

    hashed_slist_foreach_remove (store->packages_by_name,
                                 remove_package_by_channel_cb,
                                 channel);

    rc_world_store_thaw (store);
}

void
rc_world_store_clear (RCWorldStore *store)
{
    g_return_if_fail (store != NULL && RC_IS_WORLD_STORE (store));

    rc_world_store_remove_packages (store, RC_CHANNEL_ANY);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_world_store_add_channel (RCWorldStore *store,
                            RCChannel    *channel)
{
    ChannelInfo *info;

    g_return_if_fail (store != NULL && RC_IS_WORLD_STORE (store));
    g_return_if_fail (channel != NULL);

    rc_channel_set_world (channel, (RCWorld *) store);
    rc_channel_make_immutable (channel);

    info = channel_info_new (channel);

    store->channels = g_slist_prepend (store->channels, 
                                       info);

    rc_world_touch_channel_sequence_number (RC_WORLD (store));
}

void
rc_world_store_remove_channel (RCWorldStore *store,
                               RCChannel    *channel)
{
    GSList *iter;
    ChannelInfo *info = NULL;

    g_return_if_fail (store != NULL && RC_IS_WORLD_STORE (store));

    if (channel == NULL
        || ! rc_world_contains_channel (RC_WORLD (store), channel))
        return;

    rc_world_store_remove_packages (store, channel);

    for (iter = store->channels; iter != NULL; iter = iter->next) {
        info = iter->data;
        if (rc_channel_equal (info->channel, channel))
            break;
    }

    if (info != NULL) {
        channel_info_free (info);
        store->channels = g_slist_remove_link (store->channels, iter);
        rc_world_touch_channel_sequence_number (RC_WORLD (store));
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_world_store_add_lock (RCWorldStore   *store,
                         RCPackageMatch *lock)
{
    g_return_if_fail (store != NULL);
    g_return_if_fail (lock != NULL);

    store->locks = g_slist_prepend (store->locks, lock);
}

void
rc_world_store_remove_lock (RCWorldStore   *store,
                            RCPackageMatch *lock)
{
    g_return_if_fail (store != NULL);
    g_return_if_fail (lock != NULL);

    store->locks = g_slist_remove (store->locks, lock);
}

void
rc_world_store_clear_locks (RCWorldStore *store)
{
    GSList *iter;

    g_return_if_fail (store != NULL);

    for (iter = store->locks; iter != NULL; iter = iter->next) {
        RCPackageMatch *lock = iter->data;
        rc_package_match_free (lock);
    }

    g_slist_free (store->locks);
    store->locks = NULL;
}
