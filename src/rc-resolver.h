/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver.h
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

#ifndef __RC_RESOLVER_H__
#define __RC_RESOLVER_H__

#include "rc-resolver-context.h"

typedef struct _RCResolver RCResolver;

struct _RCResolver {
    RCChannel *current_channel;
    
    RCWorld *world;

    int timeout_seconds;
    gboolean verifying;

    GSList *initial_items;
    GSList *packages_to_install;
    GSList *packages_to_remove;
    GSList *packages_to_verify;
    GSList *extra_deps;
    GSList *extra_conflicts;

    GSList *pending_queues;
    GSList *pruned_queues;
    GSList *complete_queues;
    GSList *deferred_queues;
    GSList *invalid_queues;
    
    int valid_solution_count;

    RCResolverContext *best_context;
    gboolean timed_out;
};

RCResolver *rc_resolver_new  (void);
void        rc_resolver_free (RCResolver *);

void        rc_resolver_set_timeout (RCResolver *, int seconds);

void        rc_resolver_set_world (RCResolver *, RCWorld *);
RCWorld    *rc_resolver_get_world (RCResolver *);

void        rc_resolver_set_current_channel                (RCResolver *, RCChannel *);
void        rc_resolver_add_subscribed_channel             (RCResolver *, RCChannel *);

void        rc_resolver_add_package_to_install             (RCResolver *, RCPackage *);
void        rc_resolver_add_packages_to_install_from_slist (RCResolver *, GSList *);

void        rc_resolver_add_package_to_remove              (RCResolver *, RCPackage *);
void        rc_resolver_add_packages_to_remove_from_slist  (RCResolver *, GSList *);

void        rc_resolver_add_extra_dependency               (RCResolver *, RCPackageDep *);
void        rc_resolver_add_extra_conflict                 (RCResolver *, RCPackageDep *);

void        rc_resolver_verify_system          (RCResolver *);
void        rc_resolver_resolve_dependencies   (RCResolver *);

#endif /* __RC_RESOLVER_H__ */

