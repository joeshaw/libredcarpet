/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-queue.h
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

#ifndef __RC_RESOLVER_QUEUE_H__
#define __RC_RESOLVER_QUEUE_H__

#include "rc-resolver-context.h"
#include "rc-queue-item.h"

typedef struct _RCResolverQueue RCResolverQueue;

struct _RCResolverQueue {
    RCResolverContext *context;
    GSList *items;
};

RCResolverQueue *rc_resolver_queue_new (void);
RCResolverQueue *rc_resolver_queue_new_with_context (RCResolverContext *);

void             rc_resolver_queue_free (RCResolverQueue *);

void             rc_resolver_queue_add_package_to_install (RCResolverQueue *queue,
                                                           RCPackage *package);
void             rc_resolver_queue_add_package_to_remove  (RCResolverQueue *queue,
                                                           RCPackage *package,
                                                           gboolean remove_only_mode);
void             rc_resolver_queue_add_package_to_verify  (RCResolverQueue *queue,
                                                           RCPackage *package);
void             rc_resolver_queue_add_extra_dependency   (RCResolverQueue *queue,
                                                           RCPackageDep *dep);
void             rc_resolver_queue_add_extra_conflict     (RCResolverQueue *queue,
                                                           RCPackageDep *dep);

gboolean         rc_resolver_queue_is_empty (RCResolverQueue *);
gboolean         rc_resolver_queue_is_invalid (RCResolverQueue *);
gboolean         rc_resolver_queue_contains_only_branches (RCResolverQueue *);

gboolean         rc_resolver_queue_process_once (RCResolverQueue *);
void             rc_resolver_queue_process (RCResolverQueue *);

void             rc_resolver_queue_split_first_branch (RCResolverQueue *,
                                                       GSList **new_queues,
                                                       GSList **deferred_queues);

void             rc_resolver_queue_spew (RCResolverQueue *);

#endif /* __RC_RESOLVER_QUEUE_H__ */

