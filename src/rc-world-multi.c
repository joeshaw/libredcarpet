/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-multi.c
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

#include "rc-world-multi.h"

#include "rc-marshal.h"

static GObjectClass *parent_class;

enum {
    SUBWORLD_ADDED,
    SUBWORLD_REMOVED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

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

    RCPending *subworld_pending;

    guint refreshed_id;
    guint update_id;
} RefreshInfo;

static gboolean
rc_world_multi_cut_over_to_new_subworlds (RCWorldMulti *multi)
{
    GSList *old_subworlds, *iter;

    if (!rc_world_is_refreshing (RC_WORLD (multi)))
        return FALSE;

    /* If any of the subworlds aren't finished refreshing, do nothing. */
    for (iter = multi->subworlds; iter != NULL; iter = iter->next) {
        SubworldInfo *info = iter->data;
        if (!info->refreshed_ready)
            return FALSE;
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

            g_object_unref (refreshed_subworld);
        }
    }

    g_slist_free (old_subworlds);

    g_slist_foreach (multi->subworld_pendings, (GFunc) g_object_unref, NULL);
    g_slist_free (multi->subworld_pendings);
    multi->subworld_pendings = NULL;

    g_object_unref (multi->multi_pending);
    multi->multi_pending = NULL;

    rc_world_refresh_complete (RC_WORLD (multi));

    return TRUE;
}

static void
refresh_info_free (RefreshInfo *refresh_info)
{
    if (refresh_info->refreshed_id) {
        g_signal_handler_disconnect (refresh_info->refreshed_subworld,
                                     refresh_info->refreshed_id);
    }

    if (refresh_info->update_id) {
        g_signal_handler_disconnect (refresh_info->subworld_pending,
                                     refresh_info->update_id);
    }

    g_object_unref (refresh_info->subworld);
    g_object_unref (refresh_info->refreshed_subworld);
    g_object_unref (refresh_info->multi);

    if (refresh_info->subworld_pending)
        g_object_unref (refresh_info->subworld_pending);

    g_free (refresh_info);
}

static void
refreshed_cb (RCWorld *refreshed_subworld, gpointer user_data)
{
    RefreshInfo *refresh_info = user_data;
    SubworldInfo *info;
    RCPending *multi_pending = NULL;

    info =
        rc_world_multi_find_subworld_info_by_subworld (refresh_info->multi,
                                                       refresh_info->subworld);

    g_assert (info != NULL);

    /* This subworld is ready to be switched over */
    info->refreshed_ready = TRUE;

    /*
     * Assign to a local variable and ref it because the following function
     * will unref it and remove it.
     */
    if (refresh_info->multi->multi_pending != NULL)
        multi_pending = g_object_ref (refresh_info->multi->multi_pending);

    if (rc_world_multi_cut_over_to_new_subworlds (refresh_info->multi) &&
        multi_pending != NULL)
    {
        rc_pending_finished (multi_pending, 0);
        g_object_unref (multi_pending);
    }

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

    /* No pendings, the refresh must be done. */
    if (!num_pendings)
        percent_complete = 100.0;
    else {
        for (iter = refresh_info->multi->subworld_pendings;
             iter; iter = iter->next)
        {
            RCPending *pending = RC_PENDING (iter->data);
            
            percent_complete += (1.0 / num_pendings) *
                rc_pending_get_percent_complete (pending);
        }
    }

    if (rc_pending_is_active (refresh_info->multi->multi_pending) &&
        rc_pending_get_status (refresh_info->multi->multi_pending) !=
        RC_PENDING_STATUS_PRE_BEGIN)
    {
        rc_pending_update (refresh_info->multi->multi_pending,
                           percent_complete);
    }
}

static RCPending *
rc_world_multi_refresh_fn (RCWorld *world)
{
    RCWorldMulti *multi = RC_WORLD_MULTI (world);
    GSList *iter;

    /* Another refresh is already running */
    if (rc_world_is_refreshing (world))
        return multi->multi_pending;

    /* If we don't have any subworlds to refresh, just return NULL */
    if (!multi->subworlds) {
        rc_world_refresh_begin (world);
        rc_world_refresh_complete (world);
        return NULL;
    }

    multi->multi_pending = rc_pending_new ("Refreshing multi world");
    rc_pending_begin (multi->multi_pending);

    rc_world_refresh_begin (world);

    for (iter = multi->subworlds; iter; iter = iter->next) {
        SubworldInfo *info = iter->data;
        RefreshInfo *refresh_info;
        RCPending *subworld_pending;

        if (!rc_world_has_refresh (info->subworld)) {
            info->refreshed_ready = TRUE;
            continue;
        }

        info->refreshed_subworld = rc_world_dup (info->subworld);

        refresh_info = g_new0 (RefreshInfo, 1);
        refresh_info->subworld = g_object_ref (info->subworld);
        refresh_info->refreshed_subworld =
            g_object_ref (info->refreshed_subworld);
        refresh_info->multi = g_object_ref (multi);
        refresh_info->refreshed_id =
            g_signal_connect (refresh_info->refreshed_subworld,
                              "refreshed",
                              (GCallback) refreshed_cb,
                              refresh_info);

        subworld_pending = rc_world_refresh (info->refreshed_subworld);

        /*
         * It's possible that the pending could have completed during
         * rc_world_refresh(), that's why we check is_active().
         */
        if (subworld_pending && rc_pending_is_active (subworld_pending)) {
            refresh_info->subworld_pending = g_object_ref (subworld_pending);

            multi->subworld_pendings =
                g_slist_prepend (multi->subworld_pendings,
                                 g_object_ref (refresh_info->subworld_pending));

            refresh_info->update_id =
                g_signal_connect (refresh_info->subworld_pending,
                                  "update",
                                  (GCallback) pending_update_cb,
                                  refresh_info);
        }
    }

    rc_world_multi_cut_over_to_new_subworlds (multi);

    return multi->multi_pending;
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

typedef struct {
    RCPackageSList *install_packages;
    RCPackageSList *remove_packages;
    int flags;
    gboolean ignore_system_worlds;
    gboolean failures;
} MultiTransactInfo;

static gboolean
rc_world_multi_transact_real (RCWorld *world, gpointer user_data)
{
    MultiTransactInfo *info = user_data;
    RCPackageSList *install_subset = NULL;
    RCPackageSList *remove_subset = NULL;
    RCPackageSList *pkg_iter;

    if ((G_TYPE_FROM_INSTANCE (world) == rc_world_system_get_type () &&
         info->ignore_system_worlds) ||
        !rc_world_can_transact_package (world, NULL))
        return TRUE;

    for (pkg_iter = info->install_packages;
         pkg_iter != NULL; pkg_iter = pkg_iter->next) {
        RCPackage *pkg = pkg_iter->data;
        if (rc_world_can_transact_package (world, pkg))
            install_subset = g_slist_prepend (install_subset, pkg);
    }

    for (pkg_iter = info->remove_packages;
         pkg_iter != NULL; pkg_iter = pkg_iter->next) {
        RCPackage *pkg = pkg_iter->data;
        if (rc_world_can_transact_package (world, pkg))
            remove_subset = g_slist_prepend (remove_subset, pkg);
    }

    if (install_subset != NULL || remove_subset != NULL) {
        if (! rc_world_transact (world,
                                 install_subset,
                                 remove_subset,
                                 info->flags)) {
            g_warning ("Problematic transaction!");
            info->failures = TRUE;
        }

        g_slist_free (install_subset);
        g_slist_free (remove_subset);
    }

    return TRUE;
}

static gboolean
rc_world_multi_transact_fn (RCWorld        *world,
                            RCPackageSList *install_packages,
                            RCPackageSList *remove_packages,
                            int             flags)
{
    RCWorldMulti *multi = (RCWorldMulti *) world;
    RCPackman *packman = rc_packman_get_global ();
    gboolean rollback_enabled = FALSE;
    MultiTransactInfo info;

    /*
     * Ugh.  This is such a hack... it prevents recursion while getting
     * rollback info.
     */
    if (packman) {
        rollback_enabled = rc_packman_get_rollback_enabled (packman);
        rc_packman_set_rollback_enabled (packman, FALSE);
    }

    info.install_packages = install_packages;
    info.remove_packages = remove_packages;
    info.flags = flags;
    info.failures = FALSE;

    /* First, we transact on system world(s) only */
    info.ignore_system_worlds = FALSE;
    rc_world_multi_foreach_subworld_by_type (multi,
                                             rc_world_system_get_type (),
                                             rc_world_multi_transact_real,
                                             &info);

    if (!info.failures) {
        /* Then, if there's no failures, go on all others */
        info.ignore_system_worlds = TRUE;
        rc_world_multi_foreach_subworld (multi,
                                         rc_world_multi_transact_real,
                                         &info);
    }

    if (packman)
        rc_packman_set_rollback_enabled (packman, rollback_enabled);

    return !info.failures;
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
touch_all_sequence_numbers (RCWorldMulti *multi)
{
    rc_world_touch_package_sequence_number (RC_WORLD (multi));
    rc_world_touch_channel_sequence_number (RC_WORLD (multi));
    rc_world_touch_subscription_sequence_number (RC_WORLD (multi));
    rc_world_touch_lock_sequence_number (RC_WORLD (multi));
}

static void
rc_world_multi_subworld_added (RCWorldMulti *multi, RCWorld *subworld)
{
    touch_all_sequence_numbers (multi);
}

static void
rc_world_multi_subworld_removed (RCWorldMulti *multi, RCWorld *subworld)
{
    touch_all_sequence_numbers (multi);
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

    klass->subworld_added   = rc_world_multi_subworld_added;
    klass->subworld_removed = rc_world_multi_subworld_removed;

    signals[SUBWORLD_ADDED] =
        g_signal_new ("subworld_added",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldMultiClass, subworld_added),
                      NULL, NULL,
                      rc_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1,
                      G_TYPE_OBJECT);

    signals[SUBWORLD_REMOVED] =
        g_signal_new ("subworld_removed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldMultiClass, subworld_removed),
                      NULL, NULL,
                      rc_marshal_VOID__OBJECT,
                      G_TYPE_NONE, 1,
                      G_TYPE_OBJECT);
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

typedef struct {
    int depth;
    RCWorldMulti *multi;
    RCWorld *subworld;
    char *name;
} NameConflictInfo;

static gboolean
service_name_conflict_cb (RCWorld *subworld, gpointer user_data)
{
    NameConflictInfo *info = user_data;

    if (!g_strcasecmp (RC_WORLD_SERVICE (subworld)->name, info->name)) {
        info->depth++;
        g_free (info->name);
        info->name = g_strdup_printf ("%s (%d)",
                                      RC_WORLD_SERVICE (info->subworld)->name,
                                      info->depth);
        rc_world_multi_foreach_subworld_by_type (info->multi,
                                                 RC_TYPE_WORLD_SERVICE,
                                                 service_name_conflict_cb,
                                                 info);
        return FALSE;
    }

    return TRUE;
}

void
rc_world_multi_add_subworld (RCWorldMulti *multi,
                             RCWorld      *subworld)
{
    SubworldInfo *info;
    NameConflictInfo conflict_info;

    g_return_if_fail (multi != NULL && RC_IS_WORLD_MULTI (multi));
    g_return_if_fail (subworld != NULL && RC_IS_WORLD (subworld));

    /* 
     * If we're adding a service, make sure that the name of the service
     * doesn't conflict with any other.
     */
    if (g_type_is_a (G_TYPE_FROM_INSTANCE (subworld), RC_TYPE_WORLD_SERVICE)) {
        RCWorldService *service = RC_WORLD_SERVICE (subworld);

        conflict_info.depth = 0;
        conflict_info.multi = multi;
        conflict_info.subworld = subworld;
        conflict_info.name = g_strdup (service->name);

        rc_world_multi_foreach_subworld_by_type (multi, RC_TYPE_WORLD_SERVICE,
                                                 service_name_conflict_cb,
                                                 &conflict_info);

        g_free (service->name);
        service->name = conflict_info.name;
    }

    info = subworld_info_new (subworld, multi);
    multi->subworlds = g_slist_append (multi->subworlds, info);

    g_signal_emit (multi, signals[SUBWORLD_ADDED], 0, subworld);
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
            g_object_ref (subworld);
            subworld_info_free (info);
            multi->subworlds = g_slist_remove_link (multi->subworlds, iter);
            g_signal_emit (multi, signals[SUBWORLD_REMOVED], 0, subworld);
            g_object_unref (subworld);
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

static gboolean
subworld_foreach_fn (RCWorld *world, gpointer user_data)
{
    GSList **list = user_data;

    *list = g_slist_prepend (*list, world);

    return TRUE;
}

GSList *
rc_world_multi_get_subworlds (RCWorldMulti *multi)
{
    GSList *list = NULL;

    rc_world_multi_foreach_subworld (multi, subworld_foreach_fn, &list);

    return list;
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
    const char *id;

    RCWorldService *matching_service;
} ForeachServiceLookupInfo;

static gboolean
foreach_service_lookup_cb (RCWorld *world, gpointer user_data)
{
    RCWorldService *service = RC_WORLD_SERVICE (world);
    ForeachServiceLookupInfo *info = user_data;

    if (info->url && g_strcasecmp (service->url, info->url) == 0) {
        info->matching_service = service;
        return FALSE; /* short-circuit foreach */
    }

    if (info->id && strcmp (service->unique_id, info->id) == 0) {
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
    info.id = NULL;
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

RCWorldService *
rc_world_multi_lookup_service_by_id (RCWorldMulti *multi, const char *id)
{
    ForeachServiceLookupInfo info;

    g_return_val_if_fail (RC_IS_WORLD_MULTI (multi), NULL);

    info.id = id;
    info.url = NULL;
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
rc_world_multi_mount_service (RCWorldMulti  *multi,
                              const char    *url,
                              GError       **error)
{
    RCWorldService *existing_service;
    RCWorld *world;

    g_return_val_if_fail (RC_IS_WORLD_MULTI (multi), FALSE);
    g_return_val_if_fail (url && *url, FALSE);

    /* Check to see if this service is already mounted */
    existing_service = rc_world_multi_lookup_service (multi, url);
    if (existing_service) {
        g_set_error (error, RC_ERROR, RC_ERROR,
                     "A service with id '%s' is already mounted",
                     existing_service->unique_id);
        return FALSE;
    }

    world = rc_world_service_mount (url, error);

    if (world) {
        gboolean success;

        /* We can't check this until after we've already mounted, bah. */
        if (rc_world_multi_lookup_service_by_id (multi,
                                                 RC_WORLD_SERVICE (world)->unique_id)) {
            g_set_error (error, RC_ERROR, RC_ERROR,
                         "A service with id '%s' is already mounted",
                         RC_WORLD_SERVICE (world)->unique_id);
            success = FALSE;
        } else {
            rc_world_multi_add_subworld (multi, world);
            success = TRUE;
        }

        g_object_unref (world);

        return success;
    } else
        return FALSE;
}

