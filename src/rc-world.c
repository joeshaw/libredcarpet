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

#include "rc-debug.h"
#include "rc-dep-or.h"
#include "rc-marshal.h"
#include "rc-rollback.h"
#include "rc-packman-private.h"
#include "rc-util.h"
#include "rc-xml.h"

/* Gross hack */
static RCWorld *das_global_world = NULL;

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GObjectClass *parent_class;

enum {
    CHANGED_PACKAGES,
    CHANGED_CHANNELS,
    CHANGED_SUBSCRIPTIONS,
    CHANGED_LOCKS,
    REFRESHED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

static int
rc_world_foreach_lock_impl (RCWorld         *world,
                            RCPackageMatchFn callback,
                            gpointer         user_data)
{
    GList *iter;
    int count = 0;

    for (iter = world->lock_store; iter != NULL; iter = iter->next) {
        RCPackageMatch *lock = iter->data;
        if (callback != NULL && ! callback (lock, user_data))
            return -1;
        ++count;
    }

    return count;
}

static void
rc_world_add_lock_impl (RCWorld        *world,
                        RCPackageMatch *lock)
{
    world->lock_store = g_list_prepend (world->lock_store, lock);
}

static void
rc_world_remove_lock_impl (RCWorld        *world,
                           RCPackageMatch *lock)
{
    world->lock_store = g_list_remove (world->lock_store, lock);
}

static void
rc_world_clear_locks_impl (RCWorld *world)
{
    g_list_foreach (world->lock_store, 
                    (GFunc) rc_package_match_free,
                    NULL);
    g_list_free (world->lock_store);
    world->lock_store = NULL;
}

static void
rc_world_finalize (GObject *obj)
{
    RCWorld *world = RC_WORLD (obj);

    rc_world_clear_locks_impl (world);

    if (parent_class->finalize)
        parent_class->finalize (obj);
}

static void
rc_world_class_init (RCWorldClass *klass)
{
    GObjectClass *obj_class = G_OBJECT_CLASS (klass);

    parent_class = g_type_class_peek_parent (klass);

    klass->foreach_lock_fn = rc_world_foreach_lock_impl;
    klass->add_lock_fn     = rc_world_add_lock_impl;
    klass->remove_lock_fn  = rc_world_remove_lock_impl;
    klass->clear_lock_fn   = rc_world_clear_locks_impl;

    obj_class->finalize = rc_world_finalize;

    signals[CHANGED_PACKAGES] =
        g_signal_new ("changed_packages",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldClass, changed_packages),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[CHANGED_CHANNELS] =
        g_signal_new ("changed_channels",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldClass, changed_channels),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[CHANGED_SUBSCRIPTIONS] =
        g_signal_new ("changed_subscriptions",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldClass, changed_subscriptions),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[CHANGED_LOCKS] =
        g_signal_new ("changed_locks",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldClass, changed_locks),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[REFRESHED] =
        g_signal_new ("refreshed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCWorldClass, refreshed),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

static void
rc_world_init (RCWorld *world)
{
    world->seq_no_packages      = 1;
    world->seq_no_channels      = 1;
    world->seq_no_subscriptions = 1;
    world->seq_no_locks         = 1;

    world->no_changed_signals = FALSE;
}

GType
rc_world_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCWorldClass),
            NULL, NULL,
            (GClassInitFunc) rc_world_class_init,
            NULL, NULL,
            sizeof (RCWorld),
            0,
            (GInstanceInitFunc) rc_world_init
        };

        type = g_type_register_static (G_TYPE_OBJECT,
                                       "RCWorld",
                                       &type_info,
                                       0);
    }

    return type;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

guint
rc_world_get_package_sequence_number (RCWorld *world)
{
    g_return_val_if_fail (world != NULL, 0);

    rc_world_sync (world);
    return world->seq_no_packages;
}

guint
rc_world_get_channel_sequence_number (RCWorld *world)
{
    g_return_val_if_fail (world != NULL, 0);

    return world->seq_no_channels;
}

guint
rc_world_get_subscription_sequence_number (RCWorld *world)
{
    g_return_val_if_fail (world != NULL, 0);
    
    return world->seq_no_subscriptions;
}

guint
rc_world_get_lock_sequence_number (RCWorld *world)
{
    g_return_val_if_fail (world != NULL, 0);
    
    return world->seq_no_locks;
}

void
rc_world_touch_package_sequence_number (RCWorld *world)
{
    g_return_if_fail (world != NULL);

    ++world->seq_no_packages;
    if (! world->no_changed_signals)
        g_signal_emit (world, signals[CHANGED_PACKAGES], 0);
}

void
rc_world_touch_channel_sequence_number (RCWorld *world)
{
    g_return_if_fail (world != NULL);

    ++world->seq_no_channels;
    if (! world->no_changed_signals)
        g_signal_emit (world, signals[CHANGED_CHANNELS], 0);
}

void
rc_world_touch_subscription_sequence_number (RCWorld *world)
{
    g_return_if_fail (world != NULL);

    ++world->seq_no_subscriptions;
    if (! world->no_changed_signals)
        g_signal_emit (world, signals[CHANGED_SUBSCRIPTIONS], 0);
}

void
rc_world_touch_lock_sequence_number (RCWorld *world)
{
    g_return_if_fail (world != NULL);

    ++world->seq_no_locks;
    if (! world->no_changed_signals)
        g_signal_emit (world, signals[CHANGED_LOCKS], 0);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */
 
void
rc_set_world (RCWorld *world)
{
    if (das_global_world)
        g_object_unref (das_global_world);

    das_global_world = NULL;

    if (world) {
        g_return_if_fail (RC_IS_WORLD (world));

        das_global_world = g_object_ref (world);
    }
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
    return das_global_world;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_world_sync (RCWorld *world)
{
    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);
    
    if (RC_WORLD_GET_CLASS (world)->sync_fn)
        return RC_WORLD_GET_CLASS (world)->sync_fn (world, NULL);

    return TRUE;
}

gboolean
rc_world_sync_conditional (RCWorld *world, RCChannel *channel)
{
    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);

    if (RC_WORLD_GET_CLASS (world)->sync_fn)
        return RC_WORLD_GET_CLASS (world)->sync_fn (world, channel);;

    return TRUE;
}

RCPending *
rc_world_refresh (RCWorld *world)
{
    RCWorldClass *klass;

    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), NULL);

    klass = RC_WORLD_GET_CLASS (world);

    if (klass->refresh_fn)
        return klass->refresh_fn (world);
    else {
        rc_world_refresh_begin (world);
        rc_world_refresh_complete (world);
        return NULL;
    }
}

gboolean
rc_world_has_refresh (RCWorld *world)
{
    g_return_val_if_fail (RC_IS_WORLD (world), FALSE);

    return RC_WORLD_GET_CLASS (world)->refresh_fn != NULL;
}

gboolean
rc_world_is_refreshing (RCWorld *world)
{
    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);
    return world->refresh_pending;
}

void
rc_world_refresh_begin (RCWorld *world)
{
    g_return_if_fail (RC_IS_WORLD (world));
    g_return_if_fail (!world->refresh_pending);

    world->refresh_pending = TRUE;
}

void
rc_world_refresh_complete (RCWorld *world)
{
    g_return_if_fail (world != NULL && RC_IS_WORLD (world));
    g_return_if_fail (world->refresh_pending);

    world->refresh_pending = FALSE;
    g_signal_emit (world, signals[REFRESHED], 0);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

int
rc_world_foreach_channel (RCWorld *world,
                          RCChannelFn fn,
                          gpointer user_data)
{
    g_return_val_if_fail (world != NULL, -1);

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_channel_fn != NULL);
    return RC_WORLD_GET_CLASS (world)->foreach_channel_fn (world,
                                                           fn,
                                                           user_data);
}

static gboolean
get_channel_foreach_fn (RCChannel *channel,
                        gpointer user_data)
{
    GSList **list = user_data;
    *list = g_slist_prepend (*list, channel);

    return TRUE;
}

GSList *
rc_world_get_channels (RCWorld *world)
{
    GSList *list = NULL;

    rc_world_foreach_channel (world, get_channel_foreach_fn, &list);
    return g_slist_reverse (list);
}


struct ContainsChannelInfo {
    RCChannel *match;
    gboolean found;
};

static gboolean
contains_channel_cb (RCChannel *ch, gpointer user_data)
{
    struct ContainsChannelInfo *info = user_data;
    if (rc_channel_equal (ch, info->match))
        info->found = TRUE;
    return ! info->found;
}

gboolean
rc_world_contains_channel (RCWorld   *world,
                           RCChannel *channel)
{
    struct ContainsChannelInfo info;

    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);

    info.match = channel;
    info.found = FALSE;

    rc_world_foreach_channel (world, contains_channel_cb, &info);
    return info.found;
}

void
rc_world_set_subscription (RCWorld *world,
                           RCChannel *channel,
                           gboolean is_subscribed)
{
    RCWorldClass *klass;
    gboolean curr_subs_status;

    g_return_if_fail (world != NULL && RC_IS_WORLD (world));
    g_return_if_fail (channel != NULL);

    if (rc_channel_is_system (channel)) {
        g_warning ("Can't subscribe to system channel '%s'",
                   rc_channel_get_name (channel));
        return;
    }

    curr_subs_status = rc_world_is_subscribed (world, channel);

    klass = RC_WORLD_GET_CLASS (world);
    if (klass->set_subscribed_fn)
        klass->set_subscribed_fn (world, channel, is_subscribed);

    if (curr_subs_status != rc_world_is_subscribed (world, channel))
        rc_world_touch_subscription_sequence_number (world);
}

gboolean
rc_world_is_subscribed (RCWorld *world,
                        RCChannel *channel)
{
    RCWorldClass *klass;

    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);
    g_return_val_if_fail (channel != NULL, FALSE);

    if (rc_channel_is_system (channel))
        return FALSE;

    /* Just a bit of paranoia, since gboolean is really an integer
       type and can contain values other than 0 and 1.  (Notice that
       in rc_world_set_subscription we use the != operator to compare
       two gbooleans that come out of rc_world_is_subscribed calls. */

    klass = RC_WORLD_GET_CLASS (world);
    if (klass->get_subscribed_fn)
        return klass->get_subscribed_fn (world, channel) ? TRUE : FALSE;

    return FALSE;
}

struct FindChannelInfo {
    const char *match_string;
    const char *(*channel_str_fn) (RCChannel *);
    RCChannel *match;
};

static gboolean
find_channel_cb (RCChannel *channel,
                 gpointer   user_data)
{
    struct FindChannelInfo *info = user_data;
    
    if (! g_strcasecmp (info->channel_str_fn (channel), info->match_string)) {
        info->match = channel;
        return FALSE;
    }
    
    return TRUE;
}

RCChannel *
rc_world_get_channel_by_name (RCWorld *world,
                              const char *channel_name)
{
    struct FindChannelInfo info;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel_name && *channel_name, NULL);

    info.match_string = channel_name;
    info.channel_str_fn = rc_channel_get_name;
    info.match = NULL;

    rc_world_foreach_channel (world, find_channel_cb, &info);

    return info.match;
}

RCChannel *
rc_world_get_channel_by_alias (RCWorld *world,
                               const char *alias)
{
    struct FindChannelInfo info;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (alias && *alias, NULL);

    info.match_string = alias;
    info.channel_str_fn = rc_channel_get_alias;
    info.match = NULL;

    rc_world_foreach_channel (world, find_channel_cb, &info);

    return info.match;
}

RCChannel *
rc_world_get_channel_by_id (RCWorld *world,
                            const char *channel_id)
{
    struct FindChannelInfo info;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel_id && *channel_id, NULL);

    info.match_string = channel_id;
    info.channel_str_fn = rc_channel_get_id;
    info.match = NULL;

    rc_world_foreach_channel (world, find_channel_cb, &info);

    return info.match;
}


int
rc_world_foreach_lock (RCWorld         *world,
                       RCPackageMatchFn fn,
                       gpointer         user_data)
{
    g_return_val_if_fail (world != NULL, -1);

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_lock_fn != NULL);
    return RC_WORLD_GET_CLASS (world)->foreach_lock_fn (world, fn, user_data);
}

static gboolean
get_locks_foreach_fn (RCPackageMatch *match, gpointer user_data)
{
    GSList **list = user_data;

    
    *list = g_slist_prepend (*list, match);
    return TRUE;
}

GSList *
rc_world_get_locks (RCWorld *world)
{
    GSList *list = NULL;

    rc_world_foreach_lock (world, get_locks_foreach_fn, &list);
    return g_slist_reverse (list);
}

struct IsLockedInfo {
    RCPackage *package;
    RCWorld   *world;
    gboolean   is_locked;
};

static gboolean
is_locked_cb (RCPackageMatch *match,
              gpointer        user_data)
{
    struct IsLockedInfo *info = user_data;

    if (rc_package_match_test (match, info->package, info->world)) {
        info->is_locked = TRUE;
        return FALSE;
    }

    return TRUE;
}
              

gboolean
rc_world_package_is_locked (RCWorld   *world,
                            RCPackage *package)
{
    struct IsLockedInfo info;

    g_return_val_if_fail (world != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    info.package = package;
    info.world   = world;
    info.is_locked = FALSE;

    rc_world_foreach_lock (world, is_locked_cb, &info);
    
    return info.is_locked;
}

void
rc_world_add_lock (RCWorld        *world,
                   RCPackageMatch *lock)
{
    RCWorldClass *klass;

    g_return_if_fail (RC_IS_WORLD (world));

    if (lock == NULL)
        return;

    klass = RC_WORLD_GET_CLASS (world);

    g_assert (klass->add_lock_fn);
    klass->add_lock_fn (world, lock);
}

void
rc_world_remove_lock (RCWorld        *world,
                      RCPackageMatch *lock)
{
    RCWorldClass *klass;

    g_return_if_fail (RC_IS_WORLD (world));

    if (lock == NULL)
        return;

    klass = RC_WORLD_GET_CLASS (world);

    g_assert (klass->remove_lock_fn != NULL);
    klass->remove_lock_fn (world, lock);
}

void
rc_world_clear_locks (RCWorld *world)
{
    RCWorldClass *klass;

    g_return_if_fail (RC_IS_WORLD (world));

    klass = RC_WORLD_GET_CLASS (world);
    
    g_assert (klass->clear_lock_fn != NULL);
    klass->clear_lock_fn (world);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
installed_version_cb (RCPackage *pkg, gpointer user_data)
{
    RCPackage **installed = user_data;
    
    if (rc_package_is_installed (pkg)) {
        *installed = pkg;
        return FALSE;
    }
    return TRUE;
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
    RCPackage *installed = NULL;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    rc_world_sync (world);

    rc_world_foreach_package_by_name (world,
                                      rc_package_get_name (package),
                                      RC_CHANNEL_ANY, /* is this right? */
                                      installed_version_cb,
                                      &installed);

    return installed;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct GetPackageInfo {
    RCChannel     *channel;
    RCPackageSpec *spec;
    RCPackage     *package;
};

static gboolean
get_package_cb (RCPackage *pkg, gpointer user_data)
{
    struct GetPackageInfo *info = user_data;
    
    if (rc_channel_equal (pkg->channel, info->channel) &&
        rc_package_spec_equal ((RCPackageSpec *) pkg, info->spec)) {
        info->package = pkg;
        return FALSE;
    }
    return TRUE;
}

/**
 * rc_world_get_package:
 * @world: An #RCWorld.
 * @channel: A non-wildcard #RCChannel.
 * @spec: The spec of a package.
 * 
 * Searches @world for a package in the specified channel
 * with the specified spec.  @channel must be an actual
 * channel, not a wildcard.
 * 
 * Return value: The matching package, or %NULL if no such
 * package exists.
 **/
RCPackage *
rc_world_get_package (RCWorld       *world,
                      RCChannel     *channel,
                      RCPackageSpec *spec)
{
    struct GetPackageInfo info;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel != RC_CHANNEL_ANY
                          && channel != RC_CHANNEL_NON_SYSTEM, NULL);
    g_return_val_if_fail (spec != NULL, NULL);

    rc_world_sync_conditional (world, channel);

    info.channel = channel;
    info.spec    = spec;
    info.package = NULL;

    rc_world_foreach_package_by_name (world,
                                      rc_package_spec_get_name (spec),
                                      channel,
                                      get_package_cb, &info);

    return info.package;
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
                                      RCPackageDep *constraint,
                                      gboolean is_and)
{
    RCPackage *pkg;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel != RC_CHANNEL_ANY
                          && channel != RC_CHANNEL_NON_SYSTEM, NULL);
    g_return_val_if_fail (constraint != NULL, NULL);

    /* rc_world_get_package will call rc_world_sync */
    
    pkg = rc_world_get_package (world, channel, (RCPackageSpec *) constraint);

    if (pkg) {
        RCPackman *packman;
        RCPackageDep *dep;

        packman = rc_packman_get_global ();
        g_assert (packman != NULL);

        dep = rc_package_dep_new_from_spec (&(pkg->spec),
                                            RC_RELATION_EQUAL,
                                            pkg->channel,
                                            FALSE, FALSE);

        if (! rc_package_dep_verify_relation (packman, constraint, dep))
            pkg = NULL;

        rc_package_dep_unref (dep);
    }

    return pkg;
}

typedef struct {
    RCPackage *package;

    RCChannel *guess;
} GuessInfo;

static gboolean
guess_cb (RCPackage *package, gpointer user_data)
{
    GuessInfo *info = user_data;
    GSList *iter;

    if (!package->channel)
        return TRUE;

    for (iter = package->history; iter; iter = iter->next) {
        RCPackageUpdate *update = iter->data;

        if (rc_package_spec_equal (RC_PACKAGE_SPEC (update),
                                   RC_PACKAGE_SPEC (info->package)))
        {
            info->guess = package->channel;
            return FALSE;
        }
    }

    return TRUE;
}

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
    GuessInfo info;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    if (package->channel != NULL &&
        !rc_channel_is_system (package->channel) &&
        !rc_channel_is_hidden (package->channel))
        return package->channel;

    /* We don't need to call rc_world_sync, because we are searching
       over the channel data --- not the installed packages. */

    info.package = package;
    info.guess = NULL;

    rc_world_foreach_package_by_name (world,
                                      g_quark_to_string (package->spec.nameq),
                                      RC_CHANNEL_NON_SYSTEM,
                                      guess_cb, &info);

    return info.guess;
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
    g_return_val_if_fail (world != NULL, -1);

    rc_world_sync_conditional (world, channel);

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_package_fn != NULL);
    return RC_WORLD_GET_CLASS (world)->foreach_package_fn (world,
                                                           NULL,
                                                           channel,
                                                           fn,
                                                           user_data);
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
    g_return_val_if_fail (world != NULL, -1);

    rc_world_sync_conditional (world, channel);

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_package_fn != NULL);
    return RC_WORLD_GET_CLASS (world)->foreach_package_fn (world,
                                                           name,
                                                           channel,
                                                           fn,
                                                           user_data);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct ForeachMatchInfo {
    RCWorld *world;
    RCPackageMatch *match;
    RCPackageFn fn;
    gpointer user_data;
    int count;
};

gboolean
foreach_match_fn (RCPackage *pkg,
                  gpointer   user_data)
{
    struct ForeachMatchInfo *info = user_data;

    if (rc_package_match_test (info->match, pkg, info->world)) {
        if (info->fn)
            info->fn (pkg, info->user_data);
        ++info->count;
    }

    return TRUE;
}

int
rc_world_foreach_package_by_match (RCWorld        *world,
                                   RCPackageMatch *match,
                                   RCPackageFn     fn,
                                   gpointer        user_data)
{
    struct ForeachMatchInfo info;

    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (match != NULL, -1);

    info.world = world;
    info.match = match;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    rc_world_foreach_package (world,
                              RC_CHANNEL_ANY,
                              foreach_match_fn, 
                              &info);

    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct ForeachUpgradeInfo {
    RCPackage *original_package;
    RCPackageFn fn;
    gpointer user_data;
    int count;
    RCWorld *world;
};

static gboolean
foreach_upgrade_cb (RCPackage *package, gpointer user_data)
{
    struct ForeachUpgradeInfo *info = user_data;
    RCWorld *world = info->world;
    RCPackman *packman;
    int cmp;
    
    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    cmp = rc_packman_version_compare (packman,
                                      RC_PACKAGE_SPEC (info->original_package),
                                      RC_PACKAGE_SPEC (package));
                                       
    if (cmp >= 0)
        return TRUE;

    if (rc_world_package_is_locked (world, package))
        return TRUE;

    if (info->fn)
        info->fn (package, info->user_data);
    ++info->count;

    return TRUE;
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

    rc_world_sync_conditional (world, channel);

    info.original_package = package;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;
    info.world = world;

    rc_world_foreach_package_by_name (world,
                                      g_quark_to_string (package->spec.nameq),
                                      channel,
                                      foreach_upgrade_cb,
                                      &info);

    return info.count;
}

static gboolean
get_upgrades_foreach_fn (RCPackage *package, gpointer user_data)
{
    GSList **list = user_data;

    *list = g_slist_prepend (*list, package);

    return TRUE;
}

GSList *
rc_world_get_upgrades (RCWorld *world,
                       RCPackage *package,
                       RCChannel *channel)
{
    GSList *list = NULL;

    rc_world_foreach_upgrade (world, package, channel,
                              get_upgrades_foreach_fn, &list);

    return g_slist_reverse (list);
}

struct BestUpgradeInfo {
    RCPackage *best_upgrade;
    gboolean subscribed_only;
    RCWorld *world;
};

static gboolean
get_best_upgrade_cb (RCPackage *package, gpointer user_data)
{
    struct BestUpgradeInfo *info = user_data;
    RCPackman *packman;
    int cmp;

    if (info->subscribed_only)
        if (!(package->channel && rc_channel_is_subscribed (package->channel)))
            return TRUE;

    if (rc_world_package_is_locked (info->world, package))
        return TRUE;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);
    cmp = rc_packman_version_compare (packman,
                                      RC_PACKAGE_SPEC (info->best_upgrade),
                                      RC_PACKAGE_SPEC (package));

    if (cmp < 0)
        info->best_upgrade = package;

    return TRUE;
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
    info.world = world;

    rc_world_foreach_upgrade (world, package, RC_CHANNEL_NON_SYSTEM,
                              get_best_upgrade_cb, &info);

    if (info.best_upgrade == package)
        info.best_upgrade = NULL;

    return info.best_upgrade;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct SystemUpgradeInfo {
    RCWorld *world;
    RCPackage *system_package;
    RCPackageSList *best_upgrades;
    gboolean subscribed_only;
    RCPackagePairFn fn;
    gpointer user_data;
    int count;
};

static gboolean
foreach_system_upgrade_cb (RCPackage *upgrade, gpointer user_data)
{
    struct SystemUpgradeInfo *info = user_data;
    RCPackman *packman;
    int cmp;

    if (info->subscribed_only) {
        if (!(upgrade->channel && rc_channel_is_subscribed (upgrade->channel)))
            return TRUE;
    }

    if (rc_world_package_is_locked (info->world, upgrade))
        return TRUE;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    if (!info->best_upgrades)
        info->best_upgrades = g_slist_prepend (info->best_upgrades, upgrade);
    else {
        /* All the versions are equal, so picking the first is fine */
        RCPackage *best_up = info->best_upgrades->data;
        
        cmp = rc_packman_version_compare (packman,
                                          RC_PACKAGE_SPEC (best_up),
                                          RC_PACKAGE_SPEC (upgrade));

        if (cmp <= 0) {
            /* We have a new best package... */
            g_slist_free (info->best_upgrades);
            info->best_upgrades = g_slist_prepend (NULL, upgrade);
        } 
    }

    return TRUE;
}

static void
foreach_system_package_cb (gpointer key, gpointer value, gpointer user_data)
{
    struct SystemUpgradeInfo *info = user_data;
    RCPackageSList *iter;

    info->system_package = (RCPackage *) value;
    info->best_upgrades = NULL;

    /* If the package is excluded, skip it. */
    if (rc_world_package_is_locked (info->world, info->system_package))
        return;

    rc_world_foreach_upgrade (info->world, info->system_package,
                              RC_CHANNEL_NON_SYSTEM,
                              foreach_system_upgrade_cb, info);

    for (iter = info->best_upgrades; iter; iter = iter->next) {
        RCPackage *upgrade = iter->data;

        if (info->fn)
            info->fn (info->system_package, upgrade, info->user_data);

        ++info->count;
    }

    g_slist_free (info->best_upgrades);
    info->best_upgrades = NULL;
}

static gboolean
build_unique_hash_cb (RCPackage *package, gpointer user_data)
{
    GHashTable *unique_hash = user_data;
    RCPackman *packman = rc_packman_get_global ();
    RCPackage *old_package;

    old_package = g_hash_table_lookup (unique_hash,
                                       GINT_TO_POINTER (package->spec.nameq));

    if (old_package) {
        if (rc_packman_version_compare (packman,
                                        RC_PACKAGE_SPEC (package),
                                        RC_PACKAGE_SPEC (old_package)) <= 0)
            return TRUE;
    }

    g_hash_table_replace (unique_hash, GINT_TO_POINTER (package->spec.nameq),
                          package);

    return TRUE;
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
 * Return value: The number of matching packages that the callback
 * function was invoked on, or -1 in case of an error.
 **/
int
rc_world_foreach_system_upgrade (RCWorld *world,
                                 gboolean subscribed_only,
                                 RCPackagePairFn fn,
                                 gpointer user_data)
{
    struct SystemUpgradeInfo info;
    GHashTable *unique_hash;

    g_return_val_if_fail (world != NULL, -1);

    unique_hash = g_hash_table_new (NULL, NULL);

    /* rc_world_foreach_package calls rc_world_sync */

    rc_world_foreach_package (world, RC_CHANNEL_SYSTEM,
                              build_unique_hash_cb, unique_hash);

    info.world = world;
    info.subscribed_only = subscribed_only;
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    g_hash_table_foreach (unique_hash, foreach_system_package_cb, &info);

    g_hash_table_destroy (unique_hash);

    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/**
 * rc_world_foreach_providing_package:
 * @world: An #RCWorld.
 * @dep: An #RCPackageDep.
 * @fn: A callback function.
 * @user_data: Pointer to be passed to the callback function.
 * 
 * Iterates across all packages in @world
 * that provides a token that satisfied the
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
rc_world_foreach_providing_package (RCWorld           *world,
                                    RCPackageDep      *dep,
                                    RCPackageAndSpecFn fn,
                                    gpointer           user_data)
{
    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    if (rc_package_dep_is_or (dep)) {
        RCPackageDepSList *deps, *iter;
        int count = 0;
        deps = rc_dep_string_to_or_dep_slist (
            g_quark_to_string (RC_PACKAGE_SPEC (dep)->nameq));
        for (iter = deps; iter != NULL; iter = iter->next) {
            count += rc_world_foreach_providing_package (world, iter->data,
                                                         fn, user_data);
        }
        g_slist_free (deps);
        return count;
    }

    rc_world_sync_conditional (world, rc_package_dep_get_channel (dep));

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_providing_fn);
    return RC_WORLD_GET_CLASS (world)->foreach_providing_fn (world,
                                                             dep,
                                                             fn,
                                                             user_data);
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
rc_world_foreach_requiring_package (RCWorld          *world,
                                    RCPackageDep     *dep,
                                    RCPackageAndDepFn fn,
                                    gpointer          user_data)
{
    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    rc_world_sync_conditional (world, rc_package_dep_get_channel (dep));

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_requiring_fn != NULL);
    return RC_WORLD_GET_CLASS (world)->foreach_requiring_fn (world,
                                                             dep,
                                                             fn,
                                                             user_data);
}

int
rc_world_foreach_parent_package (RCWorld          *world,
                                 RCPackageDep     *dep,
                                 RCPackageAndDepFn fn,
                                 gpointer          user_data)
{
    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    rc_world_sync_conditional (world, rc_package_dep_get_channel (dep));

    g_assert (RC_WORLD_GET_CLASS (world)->foreach_package_fn);
    return RC_WORLD_GET_CLASS (world)->foreach_parent_fn (world,
                                                          dep,
                                                          fn,
                                                          user_data);
}

int
rc_world_foreach_conflicting_package (RCWorld          *world,
                                      RCPackageDep     *dep,
                                      RCPackageAndDepFn fn,
                                      gpointer          user_data)
{
    g_return_val_if_fail (world != NULL, -1);
    g_return_val_if_fail (dep != NULL, -1);

    rc_world_sync_conditional (world, rc_package_dep_get_channel (dep));
    
    g_assert (RC_WORLD_GET_CLASS (world)->foreach_conflicting_fn);
    return RC_WORLD_GET_CLASS (world)->foreach_conflicting_fn (world,
                                                               dep,
                                                               fn,
                                                               user_data);
}

struct SingleProviderInfo {
    RCPackage *package;
    RCChannel *channel;
    int count;
};

static gboolean 
single_provider_cb (RCPackage     *package, 
                    RCPackageSpec *spec, 
                    gpointer       user_data)
{
    struct SingleProviderInfo *info = user_data;

    if (! rc_channel_equal (package->channel, info->channel))
        return TRUE;

    if (info->package == NULL) {
        
        info->package = package;
        info->count = 1;

    } else if (rc_package_spec_not_equal (RC_PACKAGE_SPEC (package),
                                          RC_PACKAGE_SPEC (info->package))) {
        ++info->count;
    }

    return TRUE;
}

gboolean
rc_world_get_single_provider (RCWorld *world,
                              RCPackageDep *dep,
                              RCChannel *channel,
                              RCPackage **package)
{
    struct SingleProviderInfo info;

    g_return_val_if_fail (world != NULL, FALSE);
    g_return_val_if_fail (dep != NULL, FALSE);

    info.package = NULL;
    info.channel = channel;
    info.count = 0;

    rc_world_foreach_providing_package (world, dep,
                                        single_provider_cb,
                                        &info);

    if (info.count != 1)
        return FALSE;

    if (package)
        *package = info.package;

    return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_world_can_transact_package (RCWorld   *world,
                               RCPackage *package)
{
    RCWorldClass *klass;

    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);

    klass = RC_WORLD_GET_CLASS (world);

    /* If we haven't set transact_fn in the vtable,
       then obviously we can't transact anything! */
    if (klass->transact_fn == NULL)
        return FALSE;

    /* If package is NULL, we just check if there is a can_transact_fn
       entry in the vtable.  If not, this world isn't capable of
       transacting anything. */
    if (package == NULL) 
        return klass->can_transact_fn != NULL;

    if (klass->can_transact_fn == NULL)
        return FALSE;

    return klass->can_transact_fn (world, package);
}

gboolean
rc_world_transact (RCWorld *world,
                   RCPackageSList *install_packages,
                   RCPackageSList *remove_packages,
                   int flags)
{
    RCWorldClass *klass;
    GSList *iter;
    gboolean had_problem = FALSE;
    RCPackman *packman = rc_packman_get_global ();
    gboolean rollback_enabled;
    RCRollbackInfo *rollback_info = NULL;
    gboolean success;

    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), FALSE);

    /* If both lists are empty, our transaction succeeds trivially. */
    if (install_packages == NULL && remove_packages == NULL)
        return TRUE;

    /* Check to ensure that all of the packages in both lists are
       transactable by this world.  If not, spew a warning for
       every non-transactable package before returning FALSE. */

    for (iter = install_packages; iter != NULL; iter = iter->next) {
        RCPackage *pkg = iter->data;
        if (! rc_world_can_transact_package (world, pkg)) {
            rc_debug (RC_DEBUG_LEVEL_ERROR,
                      "World can't install package '%s'",
                      rc_package_to_str_static (pkg));
            had_problem = TRUE;
        }
    }

    for (iter = remove_packages; iter != NULL; iter = iter->next) {
        RCPackage *pkg = iter->data;
        if (! rc_world_can_transact_package (world, pkg)) {
            rc_debug (RC_DEBUG_LEVEL_ERROR,
                      "World can't remove package '%s'",
                      rc_package_to_str_static (pkg));
            had_problem = TRUE;
        }
    }

    if (had_problem)
        return FALSE;

    klass = RC_WORLD_GET_CLASS (world);

    g_assert (klass->transact_fn);

    rollback_enabled =
        rc_packman_get_capabilities (packman) & RC_PACKMAN_CAP_ROLLBACK &&
        rc_packman_get_rollback_enabled (packman) &&
        !(flags & RC_TRANSACT_FLAG_NO_ACT);

    if (rollback_enabled) {
        GError *err = NULL;

        rollback_info = rc_rollback_info_new (world, install_packages,
                                              remove_packages, &err);

        if (!rollback_info) {
            /* Lame error reporting */
            rc_debug (RC_DEBUG_LEVEL_ERROR, "Rollback preparation failed: %s",
                      err->message);
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "Rollback preparation failed: %s",
                                  err->message);
            g_error_free (err);
            return FALSE;
        }
    }

    success = klass->transact_fn (world, install_packages, 
                                  remove_packages, flags);

    if (rollback_enabled) {
        if (success)
            rc_rollback_info_save (rollback_info);
        else
            rc_rollback_info_discard (rollback_info);

        rc_rollback_info_free (rollback_info);
    }

    return success;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

xmlNode *
rc_world_membership_to_xml (RCWorld *world)
{
    RCWorldClass *klass;
    xmlNode *world_root;
    
    g_return_val_if_fail (world != NULL && RC_IS_WORLD (world), NULL);
    
    world_root = xmlNewNode (NULL, "world");
    xmlNewProp (world_root, "type", G_OBJECT_TYPE_NAME (world));

    klass = RC_WORLD_GET_CLASS (world);
    if (klass->membership_to_xml_fn != NULL)
        klass->membership_to_xml_fn (world, world_root);

    return world_root;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
add_lock_xml_cb (RCPackageMatch *lock,
                 gpointer        user_data)
{
    xmlNode *parent = user_data;
    xmlNode *node = rc_package_match_to_xml_node (lock);
    if (node)
        xmlAddChild (parent, node);
    return TRUE;
}

static gboolean
add_package_xml_cb (RCPackage *package,
                    gpointer   user_data)
{
    xmlNode *parent = user_data;
    xmlNode *node;

    node = rc_package_to_xml_node (package);
    xmlAddChild (parent, node);
    return TRUE;
}

typedef struct {
    RCWorld *world;
    xmlNode *parent;
} AddChannelClosure;

static gboolean
add_channel_packages_cb (RCChannel *channel,
                         gpointer   user_data)
{
    AddChannelClosure *closure = user_data;
    xmlNode *node;

    /* These are handled by the "system_packages" section */
    if (rc_channel_is_system (channel))
        return TRUE;

    node = rc_channel_to_xml_node (channel);
    rc_world_foreach_package (closure->world,
                              channel,
                              add_package_xml_cb,
                              node);

    xmlAddChild (closure->parent, node);
    return TRUE;
}

xmlNode *
rc_world_dump_to_xml (RCWorld *world,
                      xmlNode *extra_xml)
{
    xmlNode *parent;
    xmlNode *system_packages;
    xmlNode *locks;
    AddChannelClosure channel_closure;
   
    g_return_val_if_fail (world != NULL, NULL);

    parent = xmlNewNode (NULL, "world");
    
    if (extra_xml != NULL)
        xmlAddChild (parent, extra_xml);

    locks = xmlNewNode (NULL, "locks");
    rc_world_foreach_lock (world, add_lock_xml_cb, locks);
    xmlAddChild (parent, locks);

    system_packages = xmlNewNode (NULL, "system_packages");
    xmlAddChild (parent, system_packages);

    rc_world_foreach_package (world,
                              RC_CHANNEL_SYSTEM,
                              add_package_xml_cb,
                              system_packages);

    channel_closure.world = world;
    channel_closure.parent = parent;

    rc_world_foreach_channel (world,
                              add_channel_packages_cb,
                              &channel_closure);

    return parent;
}

char *
rc_world_dump (RCWorld *world, xmlNode *extra_xml)
{
    xmlNode *dump;
    xmlDoc *doc;
    xmlChar *data;
    int data_size;

    g_return_val_if_fail (world != NULL, NULL);

    dump = rc_world_dump_to_xml (world, extra_xml);
    g_return_val_if_fail (dump != NULL, NULL);

    doc = xmlNewDoc ("1.0");
    xmlDocSetRootElement (doc, dump);
    xmlDocDumpMemory (doc, &data, &data_size);
    xmlFreeDoc (doc);

    return data;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCWorld *
rc_world_dup (RCWorld *old_world)
{
    RCWorld *new_world;

    g_assert (RC_WORLD_GET_CLASS (old_world)->dup_fn);

    new_world = RC_WORLD_GET_CLASS (old_world)->dup_fn (old_world);

    return new_world;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
spew_cb (RCPackage *pkg, gpointer user_data)
{
    FILE *out = user_data;

    fprintf (out, "%s\n", rc_package_to_str_static (pkg));
    return TRUE;
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
    if (RC_WORLD_GET_CLASS (world)->spew_fn) {
        RC_WORLD_GET_CLASS (world)->spew_fn (world, out);
    } else {
        rc_world_foreach_package (world, RC_CHANNEL_ANY, spew_cb, out);
    }
}

void
rc_world_set_refresh_function (RCWorld *world,
                               RCWorldRefreshFn refresh_fn)
{
	RCWorldClass *class = RC_WORLD_GET_CLASS (world);
	class->refresh_fn = refresh_fn;
}
