/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver.c
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
    
    resolver->allow_conflicts_with_virtual_provides = FALSE;

    return resolver;
}

void
rc_resolver_free (RCResolver *resolver)
{
    if (resolver) {

        g_slist_foreach (resolver->pending_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->pruned_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->complete_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_foreach (resolver->invalid_queues,
                         (GFunc) rc_resolver_queue_free,
                         NULL);

        g_slist_free (resolver->subscribed_channels);
        g_slist_free (resolver->packages_to_install);
        g_slist_free (resolver->packages_to_remove);
        g_slist_free (resolver->packages_to_verify);
        g_slist_free (resolver->pending_queues);
        g_slist_free (resolver->pruned_queues);
        g_slist_free (resolver->complete_queues);
        g_slist_free (resolver->invalid_queues);

        g_free (resolver);
    }
}

void
rc_resolver_allow_virtual_conflicts (RCResolver *resolver,
                                     gboolean x)
{
    g_return_if_fail (resolver != NULL);

    resolver->allow_conflicts_with_virtual_provides = x;
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
rc_resolver_add_subscribed_channel (RCResolver *resolver,
                                    RCChannel *channel)
{
    g_return_if_fail (resolver != NULL);
    g_return_if_fail (channel != NULL);

    resolver->subscribed_channels = g_slist_prepend (resolver->subscribed_channels,
                                                     channel);
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

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
verify_system_cb (RCPackage *package, gpointer user_data)
{
    RCResolver *resolver  = user_data;

    resolver->packages_to_verify = g_slist_prepend (resolver->packages_to_verify,
                                                    package);
}


void
rc_resolver_verify_system (RCResolver *resolver)
{
    g_return_if_fail (resolver != NULL);

    rc_world_foreach_package (rc_get_world (),
                              RC_WORLD_SYSTEM_PACKAGES,
                              verify_system_cb,
                              resolver);

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
    RCResolverQueue *initial_queue;
    GSList *iter;
    gboolean extremely_noisy = getenv ("RC_SPEW") != NULL;

    g_return_if_fail (resolver != NULL);

    initial_queue = rc_resolver_queue_new ();
    
    /* Stick the current/subscribed channel info into the context */
    
    initial_queue->context->current_channel = resolver->current_channel;
    
    initial_queue->context->subscribed_channels = g_hash_table_new (NULL, NULL);
    for (iter = resolver->subscribed_channels; iter != NULL; iter = iter->next) {
        g_hash_table_insert (initial_queue->context->subscribed_channels,
                             iter->data, (gpointer) 0x1);
    }

    initial_queue->context->allow_conflicts_with_virtual_provides =
        resolver->allow_conflicts_with_virtual_provides;

    for (iter = resolver->packages_to_install; iter != NULL; iter = iter->next) {
        rc_resolver_queue_add_package_to_install (initial_queue,
                                                  (RCPackage *) iter->data);
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

    resolver->pending_queues = g_slist_prepend (resolver->pending_queues, initial_queue);

    while (resolver->pending_queues) {

        RCResolverQueue *queue = resolver->pending_queues->data;
        RCResolverContext *context = queue->context;

        if (extremely_noisy) {
            g_print ("%d / %d / %d / %d\n",
                     g_slist_length (resolver->pending_queues),
                     g_slist_length (resolver->complete_queues),
                     g_slist_length (resolver->pruned_queues),
                     g_slist_length (resolver->invalid_queues));
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
                                                  &resolver->pending_queues);
            
        }
        
    }
}

