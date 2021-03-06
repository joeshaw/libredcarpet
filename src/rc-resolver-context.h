/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-context.h
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

#ifndef __RC_RESOLVER_CONTEXT_H__
#define __RC_RESOLVER_CONTEXT_H__

#include "rc-world.h"
#include "rc-package.h"
#include "rc-resolver-info.h"

typedef enum {
    RC_PACKAGE_STATUS_UNKNOWN = 0,
    RC_PACKAGE_STATUS_INSTALLED,
    RC_PACKAGE_STATUS_UNINSTALLED,
    RC_PACKAGE_STATUS_TO_BE_INSTALLED,
    RC_PACKAGE_STATUS_TO_BE_INSTALLED_SOFT,
    RC_PACKAGE_STATUS_TO_BE_UNINSTALLED,
    RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE,
    RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_UNLINK
} RCPackageStatus;

#define rc_package_status_is_to_be_installed(x) \
(((x) == RC_PACKAGE_STATUS_TO_BE_INSTALLED) || ((x) == RC_PACKAGE_STATUS_TO_BE_INSTALLED_SOFT))
#define rc_package_status_is_to_be_uninstalled(x) \
(((x) == RC_PACKAGE_STATUS_TO_BE_UNINSTALLED) || ((x) == RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE) || ((x) == RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_UNLINK))

typedef struct _RCResolverContext RCResolverContext;

typedef void (*RCResolverContextFn) (RCResolverContext *ctx, gpointer data);
typedef void (*RCMarkedPackageFn) (RCPackage *pkg,
                                   RCPackageStatus status,
                                   gpointer data);
typedef void (*RCMarkedPackagePairFn) (RCPackage *pkg1,
                                       RCPackageStatus status1, 
                                       RCPackage *pkg2,
                                       RCPackageStatus status2,
                                       gpointer data);

struct _RCResolverContext {
    int refs;
    
    RCWorld *world;

    GHashTable *status;  /* keys=ptr to package objects, vals=RCPackageStatus */
    RCPackage *last_checked_package;
    RCPackageStatus last_checked_status;

    GList *log;

    guint32 download_size;
    guint32 install_size;

    int total_priority;
    int min_priority;
    int max_priority;

    int other_penalties;

    RCChannel *current_channel;

    RCResolverContext *parent;

    guint verifying : 1;
    guint invalid   : 1;
};

const char        *rc_package_status_to_string (RCPackageStatus status);

GType              rc_resolver_context_get_type (void);
RCResolverContext *rc_resolver_context_new (void);
RCResolverContext *rc_resolver_context_new_child (RCResolverContext *parent);

RCResolverContext *rc_resolver_context_ref   (RCResolverContext *);
void               rc_resolver_context_unref (RCResolverContext *);

RCWorld           *rc_resolver_context_get_world (RCResolverContext *);
void               rc_resolver_context_set_world (RCResolverContext *context,
                                                  RCWorld *world);

void               rc_resolver_context_set_status (RCResolverContext *, RCPackage *,
                                                   RCPackageStatus status);
RCPackageStatus    rc_resolver_context_get_status (RCResolverContext *, RCPackage *);

gboolean           rc_resolver_context_install_package (RCResolverContext *context,
                                                        RCPackage         *package,
                                                        gboolean           is_soft,
                                                        int                other_penalty);

gboolean           rc_resolver_context_upgrade_package (RCResolverContext *context,
                                                        RCPackage         *new_package,
                                                        RCPackage         *old_package,
                                                        gboolean           is_soft,
                                                        int                other_penalty);

gboolean           rc_resolver_context_uninstall_package (RCResolverContext *context,
                                                          RCPackage         *package,
                                                          gboolean           part_of_upgrade,
                                                          gboolean           due_to_obsolete,
                                                          gboolean           due_to_unlink);

gboolean           rc_resolver_context_package_is_present (RCResolverContext *, RCPackage *);
gboolean           rc_resolver_context_package_is_absent  (RCResolverContext *, RCPackage *);

void               rc_resolver_context_foreach_marked_package (RCResolverContext *context,
                                                               RCMarkedPackageFn fn,
                                                               gpointer user_data);

GSList            *rc_resolver_context_get_marked_packages (RCResolverContext *context);

int                rc_resolver_context_foreach_install   (RCResolverContext *context,
                                                          RCMarkedPackageFn fn,
                                                          gpointer user_data);

GSList            *rc_resolver_context_get_installs (RCResolverContext *context);

int                rc_resolver_context_foreach_uninstall (RCResolverContext *context,
                                                          RCMarkedPackageFn fn,
                                                          gpointer user_data);

GSList            *rc_resolver_context_get_uninstalls (RCResolverContext *context);

int                rc_resolver_context_foreach_upgrade   (RCResolverContext *context,
                                                          RCMarkedPackagePairFn fn,
                                                          gpointer user_data);

GSList            *rc_resolver_context_get_upgrades (RCResolverContext *context);

int                rc_resolver_context_install_count   (RCResolverContext *context);
int                rc_resolver_context_uninstall_count (RCResolverContext *context);
int                rc_resolver_context_upgrade_count   (RCResolverContext *context);


gboolean           rc_resolver_context_is_valid (RCResolverContext *);
gboolean           rc_resolver_context_is_invalid (RCResolverContext *);

void               rc_resolver_context_add_info     (RCResolverContext *, 
                                                     RCResolverInfo *);
void               rc_resolver_context_add_info_str (RCResolverContext *,
                                                     RCPackage *, int priority,
                                                     char *str);
void               rc_resolver_context_add_error_str (RCResolverContext *,
                                                      RCPackage *,
                                                      char *str);
void               rc_resolver_context_foreach_info (RCResolverContext *,
                                                     RCPackage *, int priority,
                                                     RCResolverInfoFn fn,
                                                     gpointer user_data);
GSList            *rc_resolver_context_get_info     (RCResolverContext *ctx);


void               rc_resolver_context_spew         (RCResolverContext *);
void               rc_resolver_context_spew_info    (RCResolverContext *);

gboolean           rc_resolver_context_requirement_is_met (RCResolverContext *,
                                                           RCPackageDep *dep,
                                                           gboolean is_child);

gboolean           rc_resolver_context_requirement_is_possible (RCResolverContext *,
                                                                RCPackageDep *dep);

gboolean           rc_resolver_context_package_is_possible (RCResolverContext *,
                                                            RCPackage *);

gboolean           rc_resolver_context_is_parallel_install (RCResolverContext *,
                                                            RCPackage *);

gint               rc_resolver_context_get_channel_priority (RCResolverContext *,
                                                             RCChannel *);



#endif /* __RC_RESOLVER_CONTEXT_H__ */

