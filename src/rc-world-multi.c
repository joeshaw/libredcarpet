/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-multi.c
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
#include "rc-world-multi.h"

static GObjectClass *parent_class;

typedef struct _SubworldInfo SubworldInfo;
struct _SubworldInfo {
    RCWorld *subworld;
    RCWorld *refreshed_subworld;

    gboolean refreshed_ready;

    guint changed_packages_id;
    guint changed_channels_id;
    guint changed_subscriptions_id;
    guint changed_locks_id;
};

static SubworldInfo *
rc_world_multi_find_subworld_info_by_subworld (RCWorldMulti *multi,
                                               RCWorld *subworld)
{
    GSList *iter;
    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        if (info->subworld == subworld)
            return info;
    }
    return NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
changed_packages_cb (RCWorld *subworld, gpointer world)
{
    rc_world_touch_package_sequence_number ((RCWorld *) world);
}

static void
changed_channels_cb (RCWorld *subworld, gpointer world)
{
    rc_world_touch_channel_sequence_number ((RCWorld *) world);
}

static void
changed_subscriptions_cb (RCWorld *subworld, gpointer world)
{
    rc_world_touch_subscription_sequence_number ((RCWorld *) world);
}

static void
changed_locks_cb (RCWorld *subworld, gpointer world)
{
    rc_world_touch_lock_sequence_number ((RCWorld *) world);
}

static SubworldInfo *
subworld_info_new (RCWorld *subworld, RCWorldMulti *world)
{
    SubworldInfo *info = g_new0 (SubworldInfo, 1);

    info->subworld = subworld;
    g_object_ref (subworld);
    
    info->changed_packages_id =
        g_signal_connect (G_OBJECT (subworld),
                          "changed_packages",
                          (GCallback) changed_packages_cb,
                          world);

    info->changed_channels_id =
        g_signal_connect (G_OBJECT (subworld),
                          "changed_channels",
                          (GCallback) changed_channels_cb,
                          world);

    info->changed_subscriptions_id =
        g_signal_connect (G_OBJECT (subworld),
                          "changed_subscriptions",
                          (GCallback) changed_subscriptions_cb,
                          world);

    info->changed_locks_id = 
        g_signal_connect (G_OBJECT (subworld),
                          "changed_locks",
                          (GCallback) changed_locks_cb,
                          world);

    return info;
}

static void
subworld_info_free (SubworldInfo *info)
{
    g_signal_handler_disconnect (info->subworld, 
                                 info->changed_packages_id);
    g_signal_handler_disconnect (info->subworld, 
                                 info->changed_channels_id);
    g_signal_handler_disconnect (info->subworld, 
                                 info->changed_subscriptions_id);
    g_signal_handler_disconnect (info->subworld, 
                                 info->changed_locks_id);
    g_object_unref (info->subworld);
    g_free (info);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static SubworldInfo *
rc_world_multi_find_channel_subworld (RCWorldMulti *multi,
                                      RCChannel    *channel)
{
    GSList *iter;
    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        if (rc_world_contains_channel (info->subworld, channel))
            return info;
    }
    return NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_multi_finalize (GObject *obj)
{
    RCWorldMulti *multi = (RCWorldMulti *) obj;
    GSList *iter;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        subworld_info_free (info);
    }
    g_slist_free (multi->subworlds);

    if (parent_class->finalize)
        parent_class->finalize (obj);
}

static gboolean
rc_world_multi_sync_fn (RCWorld *world, RCChannel *channel)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    gboolean everything_worked = TRUE;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        if (channel == NULL) {
            if (! rc_world_sync (info->subworld))
                everything_worked = FALSE;
        } else if (rc_channel_is_wildcard (channel)
                   || rc_world_contains_channel (info->subworld, channel)) {
            if (! rc_world_sync_conditional (info->subworld, channel))
                everything_worked = FALSE;
        }
    }

    return everything_worked;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct {
    RCWorld *subworld;
    RCWorld *refreshed_subworld;
    RCWorldMulti *multi;

    RCPending *multi_pending;
    RCPending *subworld_pending;

    guint refreshed_id;
    guint update_id;
    guint complete_id;
} RefreshInfo;

static void
rc_world_multi_cut_over_to_new_subworlds (RCWorldMulti *multi)
{
    GSList *old_subworlds, *iter;

    if (!rc_world_is_refreshing (RC_WORLD (multi)))
        return;

    /* If any of the subworlds aren't finished refreshing, do nothing. */
    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        if (!info->refreshed_ready)
            return;
    }

    /* Copy the subworlds list, because rc_world_multi_{add|remove}_subworld()
       modifies it */
    old_subworlds = g_slist_copy (multi->subworlds);

    /* For each info item, switch over to the refreshed version. */
    for (iter = old_subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;

        /* Only do it for ones which actually refresh */
        if (info->refreshed_subworld) {
            RCWorld *refreshed_subworld = info->refreshed_subworld;

            /*
             * This call will free info, that's why we keep a local
             * pointer reference to info->refreshed_subworld here.
             */
            rc_world_multi_remove_subworld (multi, info->subworld);
            rc_world_multi_add_subworld (multi, refreshed_subworld);
        }
    }

    g_slist_free (old_subworlds);

    g_slist_foreach (multi->subworld_pendings, (GFunc) g_object_unref, NULL);
    g_slist_free (multi->subworld_pendings);
    multi->subworld_pendings = NULL;

    rc_world_refresh_complete (RC_WORLD (multi));
}

static void
refresh_info_free (RefreshInfo *refresh_info)
{
    g_object_unref (refresh_info->subworld);
    g_object_unref (refresh_info->refreshed_subworld);
    g_object_unref (refresh_info->multi);
    g_object_unref (refresh_info->multi_pending);

    if (refresh_info->subworld_pending)
        g_object_unref (refresh_info->subworld_pending);

    g_free (refresh_info);
}

static void
refreshed_cb (RCWorld *subworld, gpointer user_data)
{
    RefreshInfo *refresh_info = user_data;
    SubworldInfo *info;

    info =
        rc_world_multi_find_subworld_info_by_subworld (refresh_info->multi,
                                                       refresh_info->subworld);
    g_assert (info != NULL);

    /* This subworld is ready to be switched over */
    info->refreshed_ready = TRUE;

    rc_world_multi_cut_over_to_new_subworlds (refresh_info->multi);

    g_signal_handler_disconnect (refresh_info->refreshed_subworld,
                                 refresh_info->refreshed_id);
    refresh_info->refreshed_id = -1;

    if (refresh_info->update_id == -1 && refresh_info->complete_id == -1)
        refresh_info_free (refresh_info);
}

static void
pending_update_cb (RCPending *pending, gpointer user_data)
{
    RefreshInfo *refresh_info = user_data;
    int num_pendings;
    double percent_complete = 0.0;
    GSList *iter;

    num_pendings = g_slist_length (refresh_info->multi->subworld_pendings);

    if (!num_pendings) {
        percent_complete = 100.0;
    } else {
        for (iter = refresh_info->multi->subworld_pendings;
             iter; iter = iter->next) {
            RCPending *pending = RC_PENDING (iter->data);

            percent_complete += (1.0 / num_pendings) *
                rc_pending_get_percent_complete (pending);
        }
    }

    rc_pending_update (refresh_info->multi_pending, percent_complete);
}

static void
pending_complete_cb (RCPending *pending, gpointer user_data)
{
    RefreshInfo *refresh_info = user_data;
    gboolean all_complete = TRUE;
    GSList *iter;

    for (iter = refresh_info->multi->subworld_pendings;
         iter; iter = iter->next) {
        RCPending *pending = RC_PENDING (iter->data);

        if (rc_pending_is_active (pending)) {
            all_complete = FALSE;
            break;
        }
    }

    if (all_complete) {
        rc_pending_finished (refresh_info->multi_pending, 0);
    }

    g_signal_handler_disconnect (refresh_info->subworld_pending,
                                 refresh_info->update_id);
    g_signal_handler_disconnect (refresh_info->subworld_pending,
                                 refresh_info->complete_id);
    refresh_info->update_id = -1;
    refresh_info->complete_id = -1;

    if (refresh_info->refreshed_id == -1)
        refresh_info_free (refresh_info);
}        

static RCPending *
rc_world_multi_refresh_fn (RCWorld *world)
{
    RCWorldMulti *multi = RC_WORLD_MULTI (world);
    GSList *iter;
    RCPending *pending = NULL;
    
    for (iter = multi->subworlds; iter; iter = iter->next) {
        SubworldInfo *info = iter->data;
        RefreshInfo *refresh_info;
        RCPending *subworld_pending;

        if (rc_world_has_refresh (info->subworld)) {
            info->refreshed_subworld = rc_world_dup (info->subworld);

            refresh_info = g_new0 (RefreshInfo, 1);
            refresh_info->subworld = g_object_ref (info->subworld);
            refresh_info->refreshed_subworld =
                g_object_ref (info->refreshed_subworld);
            refresh_info->multi = g_object_ref (multi);
            refresh_info->refreshed_id = -1;
            refresh_info->update_id = -1;
            refresh_info->complete_id = -1;

            if (!pending) {
                pending = rc_pending_new ("Refreshing multi world");
                refresh_info->multi_pending = pending;
                rc_pending_begin (pending);
            } else
                refresh_info->multi_pending = g_object_ref (pending);

            refresh_info->refreshed_id =
                g_signal_connect (refresh_info->refreshed_subworld,
                                  "refreshed",
                                  (GCallback) refreshed_cb,
                                  refresh_info);

            subworld_pending = rc_world_refresh (info->refreshed_subworld);

            if (subworld_pending) {
                refresh_info->subworld_pending =
                    g_object_ref (subworld_pending);

                multi->subworld_pendings =
                    g_slist_prepend (multi->subworld_pendings,
                                     g_object_ref (refresh_info->subworld_pending));

                refresh_info->update_id =
                    g_signal_connect (refresh_info->subworld_pending,
                                      "update",
                                      (GCallback) pending_update_cb,
                                      refresh_info);

                refresh_info->complete_id =
                    g_signal_connect (refresh_info->subworld_pending,
                                      "complete",
                                      (GCallback) pending_complete_cb,
                                      refresh_info);
            }
        }
        else {
            info->refreshed_ready = TRUE;
        }
    }

    rc_world_multi_cut_over_to_new_subworlds (multi);

    return pending;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_multi_spew_fn (RCWorld *world, FILE *out)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        rc_world_spew (info->subworld, out);
    }
}

static gboolean
rc_world_multi_can_transact_fn (RCWorld   *world,
                                RCPackage *package)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int transactable = 0;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        if (rc_world_can_transact_package (info->subworld, package))
            ++transactable;
    }

    if (package == NULL)
        return transactable > 0;

    if (transactable == 0)
        return FALSE;
    else if (transactable == 1)
        return TRUE;
    
    /* If we've reached this point, transactable > 1 */

    /* This is probably the worst warning message in the entire
       history of computing. */
    g_warning ("%d transactable subworlds for '%s'.",
               transactable, rc_package_to_str_static (package));
    return FALSE;
}

static gboolean
rc_world_multi_transact_fn (RCWorld        *world,
                            RCPackageSList *install_packages,
                            RCPackageSList *remove_packages,
                            int             flags)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter, *pkg_iter;
    gboolean success = TRUE;

    /* For every subworld that has any transactional capability,
       build up sublists of install_packages and remove_packages
       that contain the packages that need to be transacted by
       that world, and then transact them.

       Yeah, this isn't maximally efficient, but it is the simplest
       approach.
    */

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;

        if (rc_world_can_transact_package (info->subworld, NULL)) {

            RCPackageSList *install_subset = NULL;
            RCPackageSList *remove_subset = NULL;

            for (pkg_iter = install_packages;
                 pkg_iter != NULL; pkg_iter = pkg_iter->next) {
                RCPackage *pkg = pkg_iter->data;
                if (rc_world_can_transact_package (info->subworld, pkg))
                    install_subset = g_slist_prepend (install_subset, pkg);
            }

            for (pkg_iter = remove_packages;
                 pkg_iter != NULL; pkg_iter = pkg_iter->next) {
                RCPackage *pkg = pkg_iter->data;
                if (rc_world_can_transact_package (info->subworld, pkg))
                    remove_subset = g_slist_prepend (remove_subset, pkg);
            }

            /* We don't short-circuit if one of our transactions goes
               bad.  This probably isn't smart.  And talk about
               lame-ass error reporting! */
            if (install_subset != NULL || remove_subset != NULL) {
                if (! rc_world_transact (info->subworld,
                                         install_subset,
                                         remove_subset,
                                         flags)) {
                    g_warning ("Problematic transaction!");
                    success = FALSE;
                }

                g_slist_free (install_subset);
                g_slist_free (remove_subset);
            }
        }
    }

    return success;
}

static int
rc_world_multi_foreach_channel_fn (RCWorld    *world,
                                   RCChannelFn callback,
                                   gpointer    user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_channel (info->subworld,
                                               callback, user_data);
        if (this_count < 0)
            return -1;
        
        count += this_count;
    }

    return count;
}

static gboolean
rc_world_multi_get_subscribed_fn (RCWorld   *world,
                                  RCChannel *channel)
{
    SubworldInfo *info;
    info = rc_world_multi_find_channel_subworld ((RCWorldMulti *) world,
                                                 channel);
    g_assert (info != NULL);

    return rc_world_is_subscribed (info->subworld, channel);
}

static void
rc_world_multi_set_subscribed_fn (RCWorld   *world,
                                  RCChannel *channel,
                                  gboolean   subs_status)
{
    SubworldInfo *info;
    info = rc_world_multi_find_channel_subworld ((RCWorldMulti *) world,
                                                 channel);
    g_assert (info != NULL);

    return rc_world_set_subscription (info->subworld, channel, subs_status);
}

static int
rc_world_multi_foreach_lock_fn (RCWorld         *world,
                                RCPackageMatchFn callback,
                                gpointer         user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    RCWorldClass *pwc = RC_WORLD_CLASS (parent_class);
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_lock (info->subworld,
                                            callback, user_data);
        if (this_count < 0)
            return -1;
        count += this_count;
    }

    /* Handle our own locks. */
    if (pwc->foreach_lock_fn) {
        this_count = pwc->foreach_lock_fn (world, callback, user_data);
        if (this_count < 0)
            return -1;
        count += this_count;
    }

    return count;
}

static int
rc_world_multi_foreach_package_fn (RCWorld    *world,
                                   const char *name,
                                   RCChannel  *channel,
                                   RCPackageFn callback,
                                   gpointer    user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_package_by_name (info->subworld,
                                                       name, channel,
                                                       callback, user_data);
        if (this_count < 0)
            return -1;
        count += this_count;
    }

    return count;
}

static int
rc_world_multi_foreach_providing_fn (RCWorld           *world,
                                     RCPackageDep      *dep,
                                     RCPackageAndSpecFn callback,
                                     gpointer           user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_providing_package (info->subworld, dep,
                                                         callback, user_data);
        if (this_count < 0)
            return -1;

        count += this_count;
    }

    return count;
}

static int
rc_world_multi_foreach_requiring_fn (RCWorld          *world,
                                     RCPackageDep     *dep,
                                     RCPackageAndDepFn callback,
                                     gpointer          user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_requiring_package (info->subworld, dep,
                                                         callback, user_data);
        if (this_count < 0)
            return -1;

        count += this_count;
    }

    return count;
}

static int
rc_world_multi_foreach_parent_fn (RCWorld          *world,
                                  RCPackageDep     *dep,
                                  RCPackageAndDepFn callback,
                                  gpointer          user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_parent_package (info->subworld, dep,
                                                      callback, user_data);
        if (this_count < 0)
            return -1;

        count += this_count;
    }

    return count;
}

static int
rc_world_multi_foreach_conflicting_fn (RCWorld          *world,
                                       RCPackageDep     *dep,
                                       RCPackageAndDepFn callback,
                                       gpointer          user_data)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    GSList *iter;
    int count = 0, this_count;

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        this_count = rc_world_foreach_conflicting_package (info->subworld, dep,
                                                           callback, user_data);
        if (this_count < 0)
            return -1;

        count += this_count;
    }

    return count;
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_world_multi_class_init (RCWorldMultiClass *klass)
{
    RCWorldClass *world_class = (RCWorldClass *) klass;
    GObjectClass *object_class = (GObjectClass *) klass;

    parent_class = g_type_class_peek_parent (klass);
    object_class->finalize = rc_world_multi_finalize;

    world_class->sync_fn                = rc_world_multi_sync_fn;
    world_class->refresh_fn             = rc_world_multi_refresh_fn;
    world_class->spew_fn                = rc_world_multi_spew_fn;
    world_class->can_transact_fn        = rc_world_multi_can_transact_fn;
    world_class->transact_fn            = rc_world_multi_transact_fn;
    world_class->foreach_channel_fn     = rc_world_multi_foreach_channel_fn;
    world_class->get_subscribed_fn      = rc_world_multi_get_subscribed_fn;
    world_class->set_subscribed_fn      = rc_world_multi_set_subscribed_fn;
    world_class->foreach_lock_fn        = rc_world_multi_foreach_lock_fn;
    world_class->foreach_package_fn     = rc_world_multi_foreach_package_fn;
    world_class->foreach_providing_fn   = rc_world_multi_foreach_providing_fn;
    world_class->foreach_requiring_fn   = rc_world_multi_foreach_requiring_fn;
    world_class->foreach_parent_fn      = rc_world_multi_foreach_parent_fn;
    world_class->foreach_conflicting_fn = rc_world_multi_foreach_conflicting_fn;
}

static void
rc_world_multi_init (RCWorldMulti *multi)
{
    multi->subworlds = NULL;
}

GType
rc_world_multi_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldMultiClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_multi_class_init,
            NULL, NULL,
            sizeof (RCWorldMulti),
            0,
            (GInstanceInitFunc) rc_world_multi_init
        };

        type = g_type_register_static (RC_TYPE_WORLD,
                                       "RCWorldMulti",
                                       &type_info,
                                       0);
    }

    return type;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCWorld *
rc_world_multi_new (void)
{
    return g_object_new (RC_TYPE_WORLD_MULTI, NULL);
}

void
rc_world_multi_add_subworld (RCWorldMulti *multi,
                             RCWorld      *subworld)
{
    SubworldInfo *info;

    g_return_if_fail (multi != NULL && RC_IS_WORLD_MULTI (multi));
    g_return_if_fail (subworld != NULL && RC_IS_WORLD (subworld));

    info = subworld_info_new (subworld, multi);
    multi->subworlds = g_slist_prepend (multi->subworlds, info);
}

void
rc_world_multi_remove_subworld (RCWorldMulti *multi,
                                RCWorld      *subworld)
{
    SubworldInfo *info;
    GSList *iter;

    g_return_if_fail (multi != NULL && RC_IS_WORLD_MULTI (multi));
    g_return_if_fail (subworld != NULL && RC_IS_WORLD (subworld));

    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        info = iter->data;
        if (info->subworld == subworld) {
            subworld_info_free (info);
            multi->subworlds = g_slist_remove_link (multi->subworlds, iter);
            return;
        }
    }
}

gint
rc_world_multi_foreach_subworld (RCWorldMulti *multi,
                                 RCWorldFn callback,
                                 gpointer user_data)
{
    GSList *iter;
    int count = 0;

    g_return_val_if_fail (multi != NULL && RC_IS_WORLD_MULTI (multi), -1);

    iter = multi->subworlds;
    while (iter != NULL) {
        SubworldInfo *info = iter->data;
        iter = iter->next;
        if (callback) {
            if (! callback (info->subworld, user_data)) {
                count = -1;
                break;
            } else
                ++count;
        }
    }
    
    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct {
    GType type;
    RCWorldFn callback;
    gpointer user_data;

    int count;
} ForeachByTypeInfo;

static gboolean
foreach_by_type_cb (RCWorld *subworld, gpointer user_data)
{
    ForeachByTypeInfo *info = user_data;

    if (g_type_is_a (G_TYPE_FROM_INSTANCE (subworld), info->type)
        && info->callback)
    {
        if (! info->callback (subworld, info->user_data)) {
            info->count = -1;
            return FALSE;
        } else {
            ++info->count;
            return TRUE;
        }
    } else
        return TRUE;
}

gint
rc_world_multi_foreach_subworld_by_type (RCWorldMulti *multi,
                                         GType type,
                                         RCWorldFn callback,
                                         gpointer user_data)
{
    ForeachByTypeInfo info;

    info.type = type;
    info.callback = callback;
    info.user_data = user_data;
    info.count = 0;

    rc_world_multi_foreach_subworld (multi, foreach_by_type_cb, &info);

    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct {
    const char *url;

    RCWorldService *matching_service;
} ForeachServiceLookupInfo;

static gboolean
foreach_service_lookup_cb (RCWorld *world, gpointer user_data)
{
    RCWorldService *service = RC_WORLD_SERVICE (world);
    ForeachServiceLookupInfo *info = user_data;

    if (g_strcasecmp (service->url, info->url) == 0) {
        info->matching_service = service;
        return FALSE; /* short-circuit foreach */
    }

    return TRUE;
}

RCWorldService *
rc_world_multi_lookup_service (RCWorldMulti *multi, const char *url)
{
    ForeachServiceLookupInfo info;

    g_return_val_if_fail (RC_IS_WORLD_MULTI (multi), NULL);

    info.url = url;
    info.matching_service = NULL;

    rc_world_multi_foreach_subworld_by_type (multi,
                                             RC_TYPE_WORLD_SERVICE,
                                             foreach_service_lookup_cb,
                                             &info);

    if (info.matching_service)
        return info.matching_service;
    else
        return NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */
    
gboolean
rc_world_multi_mount_service (RCWorldMulti *multi, const char *url)
{
    RCWorld *world;

    g_return_val_if_fail (RC_IS_WORLD_MULTI (multi), FALSE);
    g_return_val_if_fail (url && *url, FALSE);

    /* Check to see if this service is already mounted */
    if (rc_world_multi_lookup_service (multi, url))
        return FALSE;

    world = rc_world_service_mount (url);

    if (world) {
        rc_world_multi_add_subworld (multi, world);
        g_object_unref (world);
        return TRUE;
    } else
        return FALSE;
}

