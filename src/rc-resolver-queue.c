/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-queue.c
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
#include "rc-resolver-queue.h"

#include <stdlib.h>
#include "rc-world.h"

RCResolverQueue *
rc_resolver_queue_new (void)
{
    RCResolverContext *context = rc_resolver_context_new ();
    RCResolverQueue *queue = rc_resolver_queue_new_with_context (context);
    rc_resolver_context_unref (context);
    return queue;
}

RCResolverQueue *
rc_resolver_queue_new_with_context (RCResolverContext *context)
{
    RCResolverQueue *queue = g_new0 (RCResolverQueue, 1);
    
    queue->context = rc_resolver_context_ref (context);

    return queue;
}

void
rc_resolver_queue_free (RCResolverQueue *queue)
{
    if (queue) {
        rc_resolver_context_unref (queue->context);
        g_slist_foreach (queue->items,
                         (GFunc) rc_queue_item_free,
                         NULL);
        g_free (queue);
    }
}

void
rc_resolver_queue_add_package_to_install (RCResolverQueue *queue,
                                          RCPackage *package)
{
    RCQueueItem *item;

    g_return_if_fail (queue != NULL);
    g_return_if_fail (package != NULL);

    if (rc_resolver_context_package_is_present (queue->context, package)) {
        g_message ("%s is already installed",
                   rc_package_spec_to_str_static (&package->spec));
        return;
    }

    item = rc_queue_item_new_install (rc_resolver_context_get_world (queue->context),
                                      package);

    queue->items = g_slist_prepend (queue->items, item);
}

void
rc_resolver_queue_add_package_to_remove (RCResolverQueue *queue,
                                         RCPackage *package,
                                         gboolean remove_only_mode)
{
    RCQueueItem *item;

    g_return_if_fail (queue != NULL);
    g_return_if_fail (package != NULL);

    if (rc_resolver_context_package_is_absent (queue->context, package))
        return;

    item = rc_queue_item_new_uninstall (rc_resolver_context_get_world (queue->context),
                                        package, "user request");
    if (remove_only_mode)
        rc_queue_item_uninstall_set_remove_only (item);
    queue->items = g_slist_prepend (queue->items, item);
}

void
rc_resolver_queue_add_package_to_verify (RCResolverQueue *queue,
                                         RCPackage *package)
{
    RCWorld *world;
    GSList *iter;
    
    g_return_if_fail (queue != NULL);
    g_return_if_fail (package != NULL);

    world = rc_resolver_context_get_world (queue->context);

    for (iter = package->requires; iter != NULL; iter = iter->next) {
        RCQueueItem *item;
        RCPackageDep *dep = iter->data;
        item = rc_queue_item_new_require (world, dep);
        rc_queue_item_require_add_package (item, package);
        queue->items = g_slist_prepend (queue->items, item);
    }

    for (iter = package->conflicts; iter != NULL; iter = iter->next) {
        RCQueueItem *item;
        RCPackageDep *dep = iter->data;
        item = rc_queue_item_new_conflict (world, dep, package);
        queue->items = g_slist_prepend (queue->items, item);
    }
}

gboolean
rc_resolver_queue_is_empty (RCResolverQueue *queue)
{
    g_return_val_if_fail (queue != NULL, TRUE);
    return queue->items == NULL;
}

gboolean
rc_resolver_queue_is_invalid (RCResolverQueue *queue)
{
    g_return_val_if_fail (queue != NULL, TRUE);

    return rc_resolver_context_is_invalid (queue->context);
}

gboolean
rc_resolver_queue_contains_only_branches (RCResolverQueue *queue)
{
    GSList *iter;
    g_return_val_if_fail (queue != NULL, TRUE);

    for (iter = queue->items; iter != NULL; iter = iter->next) {
        RCQueueItem *item = (RCQueueItem *) iter->data;

        if (rc_queue_item_type (item) != RC_QUEUE_ITEM_TYPE_BRANCH)
            return FALSE;
    }

    return TRUE;
}

static int
slist_max_priority (GSList *slist)
{
    int max_priority = -1;
    while (slist) {
        if (slist->data) {
            RCQueueItem *item = slist->data;
            if (item->priority > max_priority)
                max_priority = item->priority;
        }
        slist = slist->next;
    }

    return max_priority;
}

gboolean
rc_resolver_queue_process_once (RCResolverQueue *queue)
{
    GSList *iter;
    GSList *new_items = NULL;
    int max_priority;
    gboolean did_something = FALSE;

    g_return_val_if_fail (queue != NULL, FALSE);

    while ( (max_priority = slist_max_priority (queue->items)) >= 0
            && rc_resolver_context_is_valid (queue->context) ) {
        
        gboolean did_something_recently = FALSE;
        
        for (iter = queue->items;
             iter != NULL && rc_resolver_context_is_valid (queue->context);
             iter = iter->next) {

            RCQueueItem *item = (RCQueueItem *) iter->data;
            
            if (item && item->priority == max_priority) {
                if (rc_queue_item_process (item, queue->context, &new_items)) {
                    did_something_recently = TRUE;
                }
                iter->data = NULL;
            }
        }

        if (did_something_recently)
            did_something = TRUE;
    }

    g_slist_free (queue->items);
    queue->items = new_items;

    return did_something;
}

void
rc_resolver_queue_process (RCResolverQueue *queue)
{
    gboolean very_noisy;

    g_return_if_fail (queue != NULL);

    very_noisy = getenv ("RC_SPEW") != NULL;
    
    if (very_noisy) {
        g_print ("----- Processing -----\n");
        rc_resolver_queue_spew (queue);
    }

    while (rc_resolver_context_is_valid (queue->context)
           && ! rc_resolver_queue_is_empty (queue)
           && rc_resolver_queue_process_once (queue)) {
        /* all of the work is in the conditional! */
        
        if (very_noisy)
            rc_resolver_queue_spew (queue);
    }
}

static RCResolverQueue *
copy_queue_except_for_branch (RCResolverQueue *queue,
                              RCQueueItem *branch_item,
                              RCQueueItem *subitem)
{
    RCResolverContext *new_context;
    RCResolverQueue *new_queue;
    GSList *iter;

    new_context = rc_resolver_context_new_child (queue->context);
    new_queue = rc_resolver_queue_new_with_context (new_context);
    rc_resolver_context_unref (new_context);

    for (iter = queue->items; iter != NULL; iter = iter->next) {
        RCQueueItem *item = iter->data;
        RCQueueItem *new_item;

        if (item == branch_item) {
            new_item = rc_queue_item_copy (subitem);

            if (rc_queue_item_type (new_item) == RC_QUEUE_ITEM_TYPE_INSTALL) {
                
                RCQueueItem_Install *install_item = (RCQueueItem_Install *) new_item;

                /* Penalties are negative priorities */
                int penalty;
                penalty = -rc_resolver_context_get_channel_priority (queue->context,
                                                                     install_item->package->channel);
                
                rc_queue_item_install_set_other_penalty (new_item, penalty);
            }

        } else {

            new_item = rc_queue_item_copy (item);

        }

        new_queue->items = g_slist_prepend (new_queue->items, new_item);
    }
    
    new_queue->items = g_slist_reverse (new_queue->items);

    return new_queue;
}

void
rc_resolver_queue_split_first_branch (RCResolverQueue *queue,
                                      GSList **new_queues)
{
    GSList *iter;
    RCQueueItem_Branch *first_branch = NULL;

    g_return_if_fail (queue != NULL);
    g_return_if_fail (new_queues != NULL);

    for (iter = queue->items; iter != NULL && first_branch == NULL; iter = iter->next) {
        RCQueueItem *item = iter->data;
        if (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_BRANCH)
            first_branch = (RCQueueItem_Branch *) item;
    }

    if (first_branch == NULL)
        return;

    for (iter = first_branch->possible_items; iter != NULL; iter = iter->next) {
        RCResolverQueue *new_queue;
        RCQueueItem *new_item = iter->data;

        new_queue = copy_queue_except_for_branch (queue,
                                                  (RCQueueItem *) first_branch,
                                                  new_item);


        *new_queues = g_slist_prepend (*new_queues, new_queue);
    }
}

void
rc_resolver_queue_spew (RCResolverQueue *queue)
{
    GSList *iter;

    g_return_if_fail (queue != NULL);

    g_print ("Resolver Queue: %s\n",
             queue->context->invalid ? "INVALID" : "");

    if (queue->items == NULL) {

        g_print ("  (empty)\n");

    } else {
    
        for (iter = queue->items; iter != NULL; iter = iter->next) {
            RCQueueItem *item = (RCQueueItem *) iter->data;
            if (item) {
                char *str = rc_queue_item_to_string (item);
                g_print ("  %s\n", str);
            }
        }

    }

    g_print ("\n");
}