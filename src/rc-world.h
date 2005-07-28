/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world.h
 *
 * Copyright (C) 2002 Ximian, Inc.
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

#ifndef __RC_WORLD_H__
#define __RC_WORLD_H__

#include <glib-object.h>

#include "rc-packman.h"
#include "rc-package.h"
#include "rc-channel.h"
#include "rc-package-match.h"
#include "rc-pending.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#define RC_TYPE_WORLD            (rc_world_get_type ())
#define RC_WORLD(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                  RC_TYPE_WORLD, RCWorld))
#define RC_WORLD_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                  RC_TYPE_WORLD, RCWorldClass))
#define RC_IS_WORLD(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                  RC_TYPE_WORLD))
#define RC_IS_WORLD_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                  RC_TYPE_WORLD))
#define RC_WORLD_GET_CLASS(obj)  (RC_WORLD_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCWorld      RCWorld;
typedef struct _RCWorldClass RCWorldClass;

typedef gboolean (*RCWorldFn) (RCWorld *world, gpointer user_data);

/* A 'sync' is a refresh that can happen automatically: an example
   would be the automatic re-scanning of system packages when the rpm
   db has changed.
   A 'refresh', on the other hand, only occurs when rc_world_refresh
   is called... the world will never do a refresh without an explicit
   request.
*/

typedef gboolean   (*RCWorldSyncFn)    (RCWorld *world, RCChannel *channel);
typedef RCPending *(*RCWorldRefreshFn) (RCWorld *world);

typedef void     (*RCWorldSpewFn) (RCWorld *world, FILE *out);

typedef RCWorld *(*RCWorldDupFn) (RCWorld *world);

typedef gboolean (*RCWorldCanTransactPackageFn) (RCWorld   *world,
                                                 RCPackage *package);

typedef gboolean (*RCWorldTransactFn) (RCWorld        *world,
                                       RCPackageSList *install_packages,
                                       RCPackageSList *remove_packages,
                                       int             flags);

typedef gboolean (*RCWorldGetSubscribedFn) (RCWorld *world,
                                            RCChannel *channel);

typedef void (*RCWorldSetSubscribedFn) (RCWorld   *world,
                                        RCChannel *channel,
                                        gboolean   subs_status);

typedef int (*RCWorldForeachChannelFn) (RCWorld    *world,
                                        RCChannelFn callback,
                                        gpointer    user_data);

typedef int (*RCWorldForeachLockFn)    (RCWorld         *world,
                                        RCPackageMatchFn callback,
                                        gpointer         user_data);

typedef void (*RCWorldAddLockFn) (RCWorld        *world,
                                  RCPackageMatch *lock);

typedef void (*RCWorldRemoveLockFn) (RCWorld        *world,
                                     RCPackageMatch *lock);

typedef void (*RCWorldClearLockFn) (RCWorld *world);

typedef int (*RCWorldForeachPackageFn) (RCWorld    *world,
                                        const char *name,
                                        RCChannel  *channel,
                                        RCPackageFn callback,
                                        gpointer    user_data);

typedef int (*RCWorldForeachPackageSpecFn) (RCWorld           *world,
                                            RCPackageDep      *dep,
                                            RCPackageAndSpecFn callback,
                                            gpointer           user_data);


typedef int (*RCWorldForeachPackageDepFn) (RCWorld          *world,
                                           RCPackageDep     *dep,
                                           RCPackageAndDepFn callback,
                                           gpointer          user_data);

typedef void (*RCWorldMembershipToXmlFn) (RCWorld *world, xmlNode *root);

struct _RCWorld {
    GObject parent;

    /* The sequence numbers gets incremented every
       time the RCWorld is changed. */

    guint seq_no_packages;
    guint seq_no_channels;
    guint seq_no_subscriptions;
    guint seq_no_locks;

    /* Every world needs to be able to store locks, so we provide a
       place for that.  Of course, derived classes are allowed to
       provide their own exotic lock semantics by providing their
       own *_lock_fn methods. */
    GList *lock_store;

    guint refresh_pending : 1;

    /* a bad hack to keep us from emitting signals while finalizing */
    guint no_changed_signals : 1;
};

struct _RCWorldClass {
    GObjectClass parent_class;

    RCWorldSyncFn               sync_fn;
    RCWorldRefreshFn            refresh_fn;
    RCWorldSpewFn               spew_fn;

    RCWorldDupFn                dup_fn;
    
    RCWorldCanTransactPackageFn can_transact_fn;
    RCWorldTransactFn           transact_fn;

    RCWorldForeachChannelFn     foreach_channel_fn;
    RCWorldGetSubscribedFn      get_subscribed_fn;
    RCWorldSetSubscribedFn      set_subscribed_fn;

    RCWorldForeachLockFn        foreach_lock_fn;
    RCWorldAddLockFn            add_lock_fn;
    RCWorldRemoveLockFn         remove_lock_fn;
    RCWorldClearLockFn          clear_lock_fn;

    RCWorldForeachPackageFn     foreach_package_fn;
    RCWorldForeachPackageSpecFn foreach_providing_fn;
    RCWorldForeachPackageDepFn  foreach_requiring_fn;
    RCWorldForeachPackageDepFn  foreach_parent_fn;
    RCWorldForeachPackageDepFn  foreach_conflicting_fn;

    RCWorldMembershipToXmlFn    membership_to_xml_fn;

    /* signals */
    void (*changed_packages)      (RCWorld *);
    void (*changed_channels)      (RCWorld *);
    void (*changed_subscriptions) (RCWorld *);
    void (*changed_locks)         (RCWorld *);

    void (*refreshed) (RCWorld *);
};

GType      rc_world_get_type (void);

void       rc_set_world (RCWorld *world);
RCWorld   *rc_get_world (void);

gboolean   rc_world_sync             (RCWorld *);
gboolean   rc_world_sync_conditional (RCWorld *, RCChannel *);
RCPending *rc_world_refresh          (RCWorld *);
gboolean   rc_world_has_refresh      (RCWorld *);
gboolean   rc_world_is_refreshing    (RCWorld *);

/* These functions are for RCWorld-implementers only!  Don't call them! */
void       rc_world_refresh_begin    (RCWorld *);
void       rc_world_refresh_complete (RCWorld *);

guint      rc_world_get_package_sequence_number      (RCWorld *world);
guint      rc_world_get_channel_sequence_number      (RCWorld *world);
guint      rc_world_get_subscription_sequence_number (RCWorld *world);
guint      rc_world_get_subscription_sequence_number (RCWorld *world);
guint      rc_world_get_lock_sequence_number         (RCWorld *world);

void       rc_world_touch_package_sequence_number      (RCWorld *world);
void       rc_world_touch_channel_sequence_number      (RCWorld *world);
void       rc_world_touch_subscription_sequence_number (RCWorld *world);
void       rc_world_touch_lock_sequence_number         (RCWorld *world);


int        rc_world_foreach_channel             (RCWorld      *world,
                                                 RCChannelFn   fn,
                                                 gpointer      user_data);

GSList    *rc_world_get_channels		(RCWorld      *world);

gboolean   rc_world_contains_channel            (RCWorld      *world,
                                                 RCChannel    *channel);

void       rc_world_set_subscription            (RCWorld      *world,
                                                 RCChannel    *channel,
                                                 gboolean      is_subscribed);

gboolean   rc_world_is_subscribed               (RCWorld     *world,
                                                 RCChannel   *channel);

RCChannel *rc_world_get_channel_by_name         (RCWorld      *world,
                                                 const char   *channel_name);

RCChannel *rc_world_get_channel_by_alias        (RCWorld     *world,
                                                 const char  *alias);

RCChannel *rc_world_get_channel_by_id           (RCWorld      *world,
                                                 const char   *channel_id);


/* Package Locks */

int        rc_world_foreach_lock      (RCWorld         *world,
                                       RCPackageMatchFn fn,
                                       gpointer         user_data);

GSList    *rc_world_get_locks         (RCWorld   *world);

gboolean   rc_world_package_is_locked (RCWorld   *world,
                                       RCPackage *package);

/* The world assumes ownership of the RCPackageMatch */
void       rc_world_add_lock          (RCWorld        *world,
                                       RCPackageMatch *lock);

void       rc_world_remove_lock       (RCWorld        *world,
                                       RCPackageMatch *lock);

void       rc_world_clear_locks       (RCWorld        *world);


/* Single package queries */

RCPackage *rc_world_find_installed_version      (RCWorld *world, 
                                                 RCPackage *package);

RCPackage *rc_world_get_package                 (RCWorld *world,
                                                 RCChannel *channel,
                                                 RCPackageSpec *spec);

RCPackage *rc_world_get_package_with_constraint (RCWorld *world,
                                                 RCChannel *channel,
                                                 RCPackageDep *constraint, 
                                                 gboolean is_and);

RCChannel *rc_world_guess_package_channel (RCWorld *world,
                                           RCPackage *package);


/* Iterate across packages */

int        rc_world_foreach_package          (RCWorld *world,
                                              RCChannel *channel,
                                              RCPackageFn fn,
                                              gpointer user_data);

int        rc_world_foreach_package_by_name  (RCWorld *world,
                                              const char *name,
                                              RCChannel *channel,
                                              RCPackageFn fn,
                                              gpointer user_data);

int        rc_world_foreach_package_by_match (RCWorld        *world,
                                              RCPackageMatch *match,
                                              RCPackageFn     fn,
                                              gpointer        user_data);

int        rc_world_foreach_upgrade          (RCWorld *world,
                                              RCPackage *package,
                                              RCChannel *channel,
                                              RCPackageFn fn,
                                              gpointer user_data);

GSList    *rc_world_get_upgrades             (RCWorld   *world,
                                              RCPackage *package,
                                              RCChannel *channel);

RCPackage *rc_world_get_best_upgrade        (RCWorld *world,
                                             RCPackage *package,
                                             gboolean subscribed_only);

int        rc_world_foreach_system_upgrade  (RCWorld *world, 
                                             gboolean subscribed_only,
                                             RCPackagePairFn fn,
                                             gpointer user_data);


/* Iterate across provides or requirements */

int        rc_world_foreach_providing_package (RCWorld *world,
                                               RCPackageDep *dep, 
                                               RCPackageAndSpecFn fn,
                                               gpointer user_data);

int        rc_world_foreach_requiring_package (RCWorld *world, 
                                               RCPackageDep *dep,
                                               RCPackageAndDepFn fn, 
                                               gpointer user_data);

int        rc_world_foreach_parent_package    (RCWorld *world, 
                                               RCPackageDep *dep,
                                               RCPackageAndDepFn fn, 
                                               gpointer user_data);

int        rc_world_foreach_conflicting_package (RCWorld *world, 
                                                 RCPackageDep *dep,
                                                 RCPackageAndDepFn fn, 
                                                 gpointer user_data);

gboolean   rc_world_get_single_provider (RCWorld *world,
                                         RCPackageDep *dep,
                                         RCChannel *channel,
                                         RCPackage **package);

/* Transacting */

gboolean  rc_world_can_transact_package (RCWorld   *world,
                                         RCPackage *package);

gboolean  rc_world_transact (RCWorld *world,
                             RCPackageSList *install_packages,
                             RCPackageSList *remove_packages,
                             int flags);

/* Membership XML */

xmlNode *rc_world_membership_to_xml (RCWorld *world);

/* Dumping */

xmlNode *rc_world_dump_to_xml     (RCWorld *world, xmlNode *extra_xml);

char    *rc_world_dump            (RCWorld *world, xmlNode *extra_xml);


/* Duplicating (primarily for atomic refreshes) */
RCWorld *rc_world_dup (RCWorld *world);

/* Debugging output */

void       rc_world_spew (RCWorld *world, FILE *out);

/* only used for bindings */
void       rc_world_set_refresh_function (RCWorld *world,
                                          RCWorldRefreshFn refresh_fn);
    
#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RC_WORLD_H__ */

