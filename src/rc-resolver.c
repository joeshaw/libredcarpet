/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver.c
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
#include "rc-resolver.h"

#include <stdlib.h>
#include "rc-world.h"
#include "rc-resolver-queue.h"
#include "rc-resolver-compare.h"

RCResolver *
rc_resolver_new (void)
{
    RCResolver *resolver;

    resolver = g_new0 (RCResolver, 1);
    
    return resolver;
}

void
rc_resolver_free (RCResolver *resolver)
{
    if (resolver) {

        g_slist_foreach (resolver->initial_items,
                         (GFunc) rc_queue_item_free,
                         NULL);

        g_slist_foreach (resolver->pending_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->pruned_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->complete_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->deferred_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->invalid_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        if (resolver->extra_deps)
            rc_package_dep_slist_free (resolver->extra_deps);

        if (resolver->extra_conflicts)
            rc_package_dep_slist_free (resolver->extra_conflicts);

        g_slist_free (resolver->initial_items);
        g_slist_free (resolver->packages_to_install);
        g_slist_free (resolver->packages_to_remove);
        g_slist_free (resolver->packages_to_verify);
        g_slist_free (resolver->pending_queues);
        g_slist_free (resolver->pruned_queues);
        g_slist_free (resolver->complete_queues);
        g_slist_free (resolver->deferred_queues);
        g_slist_free (resolver->invalid_queues);

        g_free (resolver);
    }
}

void
rc_resolver_set_timeout (RCResolver *resolver, int timeout)
{
    g_return_if_fail (resolver != NULL);

    resolver->timeout_seconds = timeout;
}

void
rc_resolver_set_world (RCResolver *resolver, RCWorld *world)
{
    g_return_if_fail (resolver != NULL);

    resolver->world = world;
}

RCWorld *
rc_resolver_get_world (RCResolver *resolver)
{
    g_return_val_if_fail (resolver != NULL, NULL);

    if (resolver->world)
        return resolver->world;

    return rc_get_world ();
}

void
rc_resolver_set_current_channel (RCResolver *resolver,
                                 RCChannel *channel)
{
    g_return_if_fail (resolver != NULL);
    g_return_if_fail (channel != NULL);

    resolver->current_channel = channel;
}

void
rc_resolver_add_package_to_install (RCResolver *resolver,
                                    RCPackage *package)
{
    g_return_if_fail (resolver != NULL);
    g_return_if_fail (package != NULL);

    resolver->packages_to_install =
        g_slist_prepend (resolver->packages_to_install, package);
}

void
rc_resolver_add_packages_to_install_from_slist (RCResolver *resolver,
                                                GSList *slist)
{
    g_return_if_fail (resolver != NULL);
    
    while (slist) {
        rc_resolver_add_package_to_install (resolver,
                                            slist->data);
        slist = slist->next;
    }
}

void
rc_resolver_add_package_to_remove (RCResolver *resolver,
                                   RCPackage *package)
{
    g_return_if_fail (resolver != NULL);
    g_return_if_fail (package != NULL);

    resolver->packages_to_remove =
        g_slist_prepend (resolver->packages_to_remove, package);
}

void
rc_resolver_add_packages_to_remove_from_slist (RCResolver *resolver,
                                               GSList *slist)
{
    g_return_if_fail (resolver != NULL);

    while (slist) {
        rc_resolver_add_package_to_remove (resolver,
                                           slist->data);
        slist = slist->next;
    }
}

void
rc_resolver_add_extra_dependency (RCResolver   *resolver,
                                  RCPackageDep *dep)
{
    g_return_if_fail (resolver != NULL);
    g_return_if_fail (dep != NULL);

    resolver->extra_deps = 
        g_slist_prepend (resolver->extra_deps, rc_package_dep_ref (dep));
}

void
rc_resolver_add_extra_conflict (RCResolver   *resolver,
                                RCPackageDep *dep)
{
    g_return_if_fail (resolver != NULL);
    g_return_if_fail (dep != NULL);

    resolver->extra_conflicts =
        g_slist_prepend (resolver->extra_conflicts, rc_package_dep_ref (dep));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
verify_system_cb (RCPackage *package, gpointer user_data)
{
    RCResolver *resolver  = user_data;

    resolver->packages_to_verify =
        g_slist_insert_sorted (resolver->packages_to_verify,
                               package,
                               (GCompareFunc) rc_package_spec_compare_name);
}


void
rc_resolver_verify_system (RCResolver *resolver)
{
#if 0
    GSList *i0, *i1, *i, *j;
#endif

    g_return_if_fail (resolver != NULL);

    rc_world_foreach_package (rc_resolver_get_world (resolver),
                              RC_CHANNEL_SYSTEM,
                              verify_system_cb,
                              resolver);

    resolver->verifying = TRUE;

#if 0
    /*
      Walk across the (sorted-by-name) list of installed packages and look for
      packages with the same name.  If they exist, construct a branch item
      containing multiple group items.  Each group item corresponds to removing
      all but one of the duplicates.
    */

    for (i0 = resolver->packages_to_verify; i0 != NULL;) {

        for (i1 = i0->next;
             i1 != NULL && ! rc_package_spec_compare_name (i0->data, i1->data);
             i1 = i1->next);

        if (i1 != i0->next) {
            RCQueueItem *branch_item;

            branch_item = rc_queue_item_new_branch (resolver->world);

            for (i = i0; i != i1; i = i->next) {
                
                RCQueueItem *grp_item = rc_queue_item_new_group (resolver->world);

                for (j = i0; j != i1; j = j->next) {
                    RCPackage *dup_pkg = j->data;
                    RCQueueItem *uninstall_item;

                    if (i != j) {
                        uninstall_item = rc_queue_item_new_uninstall(resolver->world,
                                                                     dup_pkg,
                                                                     "duplicate install");
                        rc_queue_item_group_add_item (grp_item, uninstall_item);
                    }

                }

                rc_queue_item_branch_add_item (branch_item, grp_item);
            }
            
            resolver->initial_items = g_slist_prepend (resolver->initial_items,
                                                       branch_item);
        }

        i0 = i1;
    }
#endif

    /* OK, that was fun.  Now just resolve the dependencies. */
    rc_resolver_resolve_dependencies (resolver);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GSList *
remove_head (GSList *slist)
{
    GSList *head = slist;
    slist = g_slist_remove_link (slist, slist);
    g_slist_free_1 (head);
    return slist;
}

void
rc_resolver_resolve_dependencies (RCResolver *resolver)
{
    RCWorld *world;
    RCResolverQueue *initial_queue;
    RCChannel *local_pkg_channel;
    GSList *iter;
    time_t t_start, t_now;
    gboolean extremely_noisy = getenv ("RC_SPEW") != NULL;

    g_return_if_fail (resolver != NULL);

    world = resolver->world;
    if (world == NULL)
        world = rc_get_world ();

    /* Create a dummy channel for our local packages, so that the
       RCWorld can see than and take their dependencies into
       account.*/
    local_pkg_channel = 
        rc_world_add_channel_with_priorities (world,
                                              "Local Packages",
                                              "local-pkg-alias-blah-blah-blah",
                                              NULL,
                                              TRUE, /* a silent channel */
                                              RC_CHANNEL_TYPE_UNKNOWN,
                                              -1, -1 /* default
                                                        priorities */
                                              );

    initial_queue = rc_resolver_queue_new ();
    
    /* Stick the current/subscribed channel and world info into the context */

    initial_queue->context->world = world;
    
    initial_queue->context->current_channel = resolver->current_channel;

    /* If this is a verify, we do a "soft resolution" */
    initial_queue->context->verifying = resolver->verifying;

    /* Add extra items. */
    for (iter = resolver->initial_items; iter != NULL; iter = iter->next) {
        RCQueueItem *item = iter->data;
        rc_resolver_queue_add_item (initial_queue, item);
        iter->data = NULL;
    }
    
    for (iter = resolver->packages_to_install; iter != NULL; iter = iter->next) {
        RCPackage *pkg = iter->data;
        
        /* Add local packages to our dummy channel. */
        if (pkg->local_package) {
            pkg->channel = rc_channel_ref (local_pkg_channel);
            rc_world_add_package (world, pkg);
        }
        
        rc_resolver_queue_add_package_to_install (initial_queue, pkg);
    }

    for (iter = resolver->packages_to_remove; iter != NULL; iter = iter->next) {
        rc_resolver_queue_add_package_to_remove (initial_queue,
                                                 (RCPackage *) iter->data,
                                                 TRUE /* remove-only mode */);
    }

    for (iter = resolver->packages_to_verify; iter != NULL; iter = iter->next) {
        rc_resolver_queue_add_package_to_verify (initial_queue,
                                                 (RCPackage *) iter->data);
    }

    for (iter = resolver->extra_deps; iter != NULL; iter = iter->next) {
        rc_resolver_queue_add_extra_dependency (initial_queue,
                                                (RCPackageDep *) iter->data);
    }

    for (iter = resolver->extra_conflicts; iter != NULL; iter = iter->next) {
        rc_resolver_queue_add_extra_conflict (initial_queue,
                                              (RCPackageDep *) iter->data);
    }

    resolver->pending_queues = g_slist_prepend (resolver->pending_queues, initial_queue);

    time (&t_start);

    while (resolver->pending_queues) {

        RCResolverQueue *queue = resolver->pending_queues->data;
        RCResolverContext *context = queue->context;

        if (extremely_noisy) {
            g_print ("%d / %d / %d / %d / %d\n",
                     g_slist_length (resolver->pending_queues),
                     g_slist_length (resolver->complete_queues),
                     g_slist_length (resolver->pruned_queues),
                     g_slist_length (resolver->deferred_queues),
                     g_slist_length (resolver->invalid_queues));
        }

        if (resolver->timeout_seconds > 0) {
            time (&t_now);
            if (difftime (t_now, t_start) > resolver->timeout_seconds) {
                resolver->timed_out = TRUE;
                break;
            }
        }

        resolver->pending_queues = remove_head (resolver->pending_queues);

        rc_resolver_queue_process (queue);

        if (rc_resolver_queue_is_invalid (queue)) {
            
            resolver->invalid_queues = g_slist_prepend (resolver->invalid_queues,
                                                        queue);
        } else if (rc_resolver_queue_is_empty (queue)) {
            
            resolver->complete_queues = g_slist_prepend (resolver->complete_queues,
                                                         queue);
            
            ++resolver->valid_solution_count;

            /* Compare this solution to our previous favorite.  In the case of a tie,
               the first solution wins --- yeah, I know this is lame, but it shouldn't
               be an issue too much of the time. */
            if (resolver->best_context == NULL
                || rc_resolver_context_cmp (resolver->best_context, context) < 0) {
                
                resolver->best_context = context;
            }
                
        } else if (resolver->best_context
                   && rc_resolver_context_partial_cmp (resolver->best_context, context) > 0) {
            
            /* If we aren't currently as good as our previous best complete solution,
               this solution gets pruned. */

            if (extremely_noisy)
                g_print ("PRUNED!\n");

            resolver->pruned_queues = g_slist_prepend (resolver->pruned_queues,
                                                       queue);
            
        } else {
            
            /* If our queue is isn't empty and isn't invalid, that can only mean
               one thing: we are down to nothing but branches. */

            rc_resolver_queue_split_first_branch (queue,
                                                  &resolver->pending_queues,
                                                  &resolver->deferred_queues);
            rc_resolver_queue_free (queue);
            
        }

        /* If we have run out of pending queues w/o finding any solutions,
           and if we have deferred queues, make the first deferred queue
           pending. */
        if (resolver->pending_queues == NULL
            && resolver->complete_queues == NULL
            && resolver->deferred_queues != NULL) {
            resolver->pending_queues = g_slist_prepend (resolver->pending_queues,
                                                        resolver->deferred_queues->data);
            resolver->deferred_queues = g_slist_delete_link (resolver->deferred_queues,
                                                             resolver->deferred_queues);
        }
    }

    if (extremely_noisy) {
        g_print ("Final: %d / %d / %d / %d / %d\n",
                 g_slist_length (resolver->pending_queues),
                 g_slist_length (resolver->complete_queues),
                 g_slist_length (resolver->pruned_queues),
                 g_slist_length (resolver->deferred_queues),
                 g_slist_length (resolver->invalid_queues));
    }

    /* Clean up our local package channel. */
    rc_world_remove_channel (world, local_pkg_channel);
}

