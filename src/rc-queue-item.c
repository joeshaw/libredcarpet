/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-queue-item.c
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
#include "rc-queue-item.h"

#include <string.h>
#include "rc-world.h"

RCQueueItemType
rc_queue_item_type (RCQueueItem *item)
{
    g_return_val_if_fail (item != NULL, RC_QUEUE_ITEM_TYPE_UNKNOWN);
    
    return item->type;
}

gboolean
rc_queue_item_is_redundant (RCQueueItem *item,
                            RCResolverContext *context)
{
    g_return_val_if_fail (item != NULL, TRUE);
    g_return_val_if_fail (context != NULL, TRUE);

    if (item->is_redundant)
        return item->is_redundant (item, context);

    return FALSE;
}

gboolean
rc_queue_item_is_satisfied (RCQueueItem *item,
                            RCResolverContext *context)
{
    g_return_val_if_fail (item != NULL, FALSE);
    g_return_val_if_fail (context != NULL, FALSE);

    if (item->is_satisfied)
        return item->is_satisfied (item, context);

    return FALSE;
}

gboolean
rc_queue_item_process (RCQueueItem *item,
                       RCResolverContext *context,
                       GSList **new_items)
{
    g_return_val_if_fail (item != NULL, FALSE);
    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (new_items != NULL, FALSE);
    g_return_val_if_fail (item->process, FALSE);

    return item->process (item, context, new_items);
}

void
rc_queue_item_free (RCQueueItem *item)
{
    if (item != NULL) {
        if (item->destroy)
            item->destroy (item);

        g_slist_foreach (item->pending_info,
                         (GFunc) rc_resolver_info_free,
                         NULL);
        g_slist_free (item->pending_info);

        g_free (item);
    }
}

RCQueueItem *
rc_queue_item_copy (RCQueueItem *item)
{
    RCQueueItem *new_item;
    GSList *iter;

    g_return_val_if_fail (item != NULL, NULL);

    g_assert (item->size >= sizeof (RCQueueItem));

    new_item = g_malloc0 (item->size);
    memcpy (new_item, item, sizeof (RCQueueItem));

    /* We need to clear this pointer out, since it gets memcpy-ed from the
       existing struct. */
    new_item->pending_info = NULL;
    for (iter = item->pending_info; iter != NULL; iter = iter->next) {
        RCResolverInfo *info = iter->data;
        new_item->pending_info = g_slist_prepend (new_item->pending_info,
                                                  rc_resolver_info_copy (info));
    }
    new_item->pending_info = g_slist_reverse (new_item->pending_info);

    if (item->copy)
        item->copy (item, new_item);

    return new_item;
}

char *
rc_queue_item_to_string (RCQueueItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);

    if (item->to_string)
        return item->to_string (item);

    return g_strdup ("???");
}

void
rc_queue_item_add_info (RCQueueItem *item, RCResolverInfo *info)
{
    g_return_if_fail (item != NULL);
    g_return_if_fail (info != NULL);

    item->pending_info = g_slist_prepend (item->pending_info, info);
}

void
rc_queue_item_log_info (RCQueueItem *item, RCResolverContext *context)
{
    GSList *iter;

    g_return_if_fail (item != NULL);
    g_return_if_fail (context != NULL);

    item->pending_info = g_slist_reverse (item->pending_info);
    for (iter = item->pending_info; iter != NULL; iter = iter->next) {
        RCResolverInfo *info = iter->data;
        rc_resolver_context_add_info (context, info);
    }
    g_slist_free (item->pending_info);
    item->pending_info = NULL;
}

RCWorld *
rc_queue_item_get_world (RCQueueItem *item)
{
    g_return_val_if_fail (item != NULL, NULL);

    if (item->world)
        return item->world;

    return rc_get_world ();
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GSList *
copy_slist (GSList *slist)
{
    GSList *new_slist = NULL;

    while (slist) {
        new_slist = g_slist_prepend (new_slist, slist->data);
        slist = slist->next;
    }

    new_slist = g_slist_reverse (new_slist);
    return new_slist;
}

static char *
item_slist_to_string (GSList *slist)
{
    char **strv, *str;
    int i;

    if (slist == NULL)
        return g_strdup ("(none)");

    strv = g_new0 (char *, g_slist_length (slist)+1);
    
    i = 0;
    while (slist) {
        RCQueueItem *item = slist->data;
        strv[i] = rc_queue_item_to_string (item);
        slist = slist->next;
        ++i;
    }

    str = g_strjoinv ("\n     ", strv);

    g_strfreev (strv);

    return str;
}

static char *
dep_slist_to_string (GSList *slist)
{
    char **strv, *str;
    int i;

    if (slist == NULL)
        return g_strdup ("(none)");

    strv = g_new0 (char *, g_slist_length (slist)+1);
    
    i = 0;
    while (slist) {
        RCPackageDep *dep = slist->data;
        strv[i] = g_strconcat (rc_package_relation_to_string (dep->relation, 0),
                               " ",
                               rc_package_spec_to_str_static (& dep->spec),
                               NULL);
        slist = slist->next;
        ++i;
    }

    str = g_strjoinv (", ", strv);

    g_strfreev (strv);

    return str;
}

static char *
package_slist_to_string (GSList *slist)
{
    char **strv, *str;
    int i;

    if (slist == NULL)
        return g_strdup ("(none)");

    strv = g_new0 (char *, g_slist_length (slist)+1);
    
    i = 0;
    while (slist) {
        RCPackage *pkg = slist->data;
        strv[i] = rc_package_to_str (pkg);
        slist = slist->next;
        ++i;
    }

    str = g_strjoinv (", ", strv);

    g_strfreev (strv);

    return str;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
install_item_is_satisfied (RCQueueItem *item, RCResolverContext *context)
{
    RCQueueItem_Install *install = (RCQueueItem_Install *) item;
    return rc_resolver_context_package_is_present (context, install->package);
}

static void
build_conflict_list (RCPackage *package, RCPackageDep *dep, gpointer user_data)
{
    GSList **slist = user_data;
    *slist = g_slist_prepend (*slist, package);
}

static gboolean
install_item_process (RCQueueItem *item, RCResolverContext *context, GSList **new_items)
{
    RCQueueItem_Install *install = (RCQueueItem_Install *) item;

    RCPackage *package = install->package;
    char *pkg_name = rc_package_to_str (package);
    char *msg;
    RCPackageStatus status = rc_resolver_context_get_status (context, package);

    GSList *iter, *conflicts;
    RCPackageDep *pkg_dep;

    /* If we are trying to upgrade package A with package B and they both have the
       same version number, do nothing.  This shouldn't happen in general with
       red-carpet, but can come up with the installer & autopull. */
    if (install->upgrades
        && rc_package_spec_equal (&install->package->spec, &install->upgrades->spec)) {
        RCResolverInfo *info;
        msg = g_strdup_printf ("Skipping %s: already installed", pkg_name);
        info = rc_resolver_info_misc_new (install->package, 
                                          RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                          msg);
        rc_resolver_context_add_info (context, info);
        goto finished;
    }

    if (install->needed_by) {
        gboolean still_needed = FALSE;
        GSList *iter;
        for (iter = install->needed_by; iter != NULL && !still_needed; iter = iter->next) {
            RCPackage *needer = iter->data;
            RCPackageStatus status = rc_resolver_context_get_status (context, needer);
            if (status != RC_PACKAGE_STATUS_TO_BE_UNINSTALLED
                && status != RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE)
                still_needed = TRUE;
        }

        if (! still_needed)
            goto finished;
    }

    if (install->upgrades == NULL) {

        rc_resolver_context_install_package (context, package, 
                                             rc_queue_item_install_get_other_penalty (item));

    } else {

        RCQueueItem *uninstall_item;

        rc_resolver_context_upgrade_package (context, package,
                                             rc_queue_item_install_get_other_penalty (item));

        uninstall_item = rc_queue_item_new_uninstall (rc_queue_item_get_world (item),
                                                      install->upgrades, "upgrade");
        rc_queue_item_uninstall_set_upgraded_to (uninstall_item, package);

        *new_items = g_slist_prepend (*new_items, uninstall_item);

    }

    /* Log which packages need this install */
    if (install->needed_by != NULL
        && install->upgrades == NULL) {
        RCResolverInfo *info;

        info = rc_resolver_info_needed_by_new (package);
        rc_resolver_info_needed_add_slist (info, install->needed_by);
        rc_resolver_context_add_info (context, info);
    }

    if (status != RC_PACKAGE_STATUS_UNINSTALLED)
        goto finished;
    
    if (install->upgrades) {
        msg = g_strconcat ("Upgrading ", 
                           rc_package_to_str_static (install->upgrades),
                           " => ",
                           pkg_name,
                           NULL);
    } else {
        msg = g_strconcat ("Installing ", pkg_name, NULL);
    }
    
#if 0
    if (install->deps_satisfied_by_this_install) {
        char *deps_str = dep_slist_to_string (install->deps_satisfied_by_this_install);
        char *new_msg = g_strconcat (msg, " to satisfy ", deps_str, NULL);
        g_free (deps_str);
        g_free (msg);
        msg = new_msg;
    }
#endif

    rc_resolver_context_add_info_str (context,
                                      package,
                                      RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                      msg);

    rc_queue_item_log_info (item, context);

    /* Construct require items for each of the package's requires that is still 
       unsatisfied. */
    for (iter = package->requires; iter != NULL; iter = iter->next) {
        RCPackageDep *dep = iter->data;

        if (!rc_resolver_context_requirement_is_met (context, dep)) {
            RCQueueItem *req_item = rc_queue_item_new_require (rc_queue_item_get_world (item), dep);
            rc_queue_item_require_add_package (req_item, package);
            
            *new_items = g_slist_prepend (*new_items, req_item);
        }
    }

    /* Construct conflict items for each of the package's conflicts. */
    for (iter = package->conflicts; iter != NULL; iter = iter->next) {
        RCPackageDep *dep = iter->data;
        RCQueueItem *conflict_item = rc_queue_item_new_conflict (rc_queue_item_get_world (item),
                                                                 dep, package);
        
        *new_items = g_slist_prepend (*new_items, conflict_item);
    }

    /* Construct conflict items for each of the package's obsoletes. */
    for (iter = package->obsoletes; iter != NULL; iter = iter->next) {
        RCPackageDep *dep = iter->data;
        RCQueueItem *conflict_item = rc_queue_item_new_conflict (rc_queue_item_get_world (item),
                                                                 dep, package);
        ((RCQueueItem_Conflict *)conflict_item)->actually_an_obsolete = TRUE;
        
        *new_items = g_slist_prepend (*new_items, conflict_item);
    }

    /* Constuct uninstall items for things that conflict with us. */
    conflicts = NULL;
    pkg_dep = rc_package_dep_new_from_spec (&package->spec,
                                            RC_RELATION_ANY);
    rc_world_foreach_conflicting_package (rc_queue_item_get_world (item),
                                          pkg_dep,
                                          NULL,
                                          build_conflict_list, &conflicts);

    for (iter = conflicts; iter != NULL; iter = iter->next) {
        RCPackage *conflicting_package = iter->data;
        RCQueueItem *uninstall_item = rc_queue_item_new_uninstall (rc_queue_item_get_world (item),
                                                                   conflicting_package,
                                                                   "conflict");
        *new_items = g_slist_prepend (*new_items, uninstall_item);
    }
    
    rc_package_dep_free (pkg_dep);
    g_slist_free (conflicts);
    
 finished:
    g_free (pkg_name);
    rc_queue_item_free (item);
    
    return TRUE;
}

static void
install_item_destroy (RCQueueItem *item)
{
    RCQueueItem_Install *install = (RCQueueItem_Install *) item;

    g_slist_free (install->deps_satisfied_by_this_install);
    g_slist_free (install->needed_by);
}

static void
install_item_copy (const RCQueueItem *src, RCQueueItem *dest)
{
    const RCQueueItem_Install *src_install = (const RCQueueItem_Install *) src;
    RCQueueItem_Install *dest_install = (RCQueueItem_Install *) dest;

    dest_install->package                        = src_install->package;
    dest_install->upgrades                       = src_install->upgrades;
    dest_install->deps_satisfied_by_this_install = copy_slist (src_install->deps_satisfied_by_this_install);
    dest_install->needed_by                      = copy_slist (src_install->needed_by);
    dest_install->channel_priority               = src_install->channel_priority;
    dest_install->other_penalty                  = src_install->other_penalty;
}

static char *
install_item_to_string (RCQueueItem *item)
{
    RCQueueItem_Install *install = (RCQueueItem_Install *) item;
    char *pkg_str = NULL, *dep_str = NULL, *str;
    
    if (install->deps_satisfied_by_this_install)
        dep_str = dep_slist_to_string (install->deps_satisfied_by_this_install);

    if (install->needed_by)
        pkg_str = package_slist_to_string (install->needed_by);

    str = g_strconcat ("install ",
                       rc_package_to_str_static (install->package),
                       pkg_str ? " needed by ": NULL,
                       pkg_str,
                       dep_str ? " (" : NULL,
                       dep_str,
                       ")",
                       NULL);

    return str;
}

RCQueueItem *
rc_queue_item_new_install (RCWorld *world, RCPackage *package)
{
    RCQueueItem *item;
    RCQueueItem_Install *install;
    RCPackage *upgrades;

    g_return_val_if_fail (package != NULL, NULL);

    install = g_new0 (RCQueueItem_Install, 1);
    item = (RCQueueItem *) install;

    item->type         = RC_QUEUE_ITEM_TYPE_INSTALL;
    item->size         = sizeof (RCQueueItem_Install);
    item->world        = world;
    item->process      = install_item_process;
    item->destroy      = install_item_destroy;
    item->copy         = install_item_copy;
    item->to_string    = install_item_to_string;
    item->is_satisfied = install_item_is_satisfied;

    install->package = package;

    upgrades = rc_world_find_installed_version (rc_queue_item_get_world (item), package);
    if (upgrades && upgrades != package)
        rc_queue_item_install_set_upgrade_package (item, upgrades);

    return item;
}

void
rc_queue_item_install_set_upgrade_package (RCQueueItem *item,
                                           RCPackage *upgrade_package)
{
    RCQueueItem_Install *install;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL);
    g_return_if_fail (upgrade_package != NULL);

    install = (RCQueueItem_Install *) item;

    g_assert (install->package != upgrade_package);

    install->upgrades = upgrade_package;
}

void
rc_queue_item_install_add_dep (RCQueueItem *item, RCPackageDep *dep)
{
    RCQueueItem_Install *install;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL);
    g_return_if_fail (dep != NULL);

    install = (RCQueueItem_Install *) item;

    install->deps_satisfied_by_this_install = 
        g_slist_prepend (install->deps_satisfied_by_this_install, dep);
}

void
rc_queue_item_install_add_needed_by (RCQueueItem *item, RCPackage *needed_by)
{
    RCQueueItem_Install *install;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL);
    g_return_if_fail (needed_by != NULL);

    install = (RCQueueItem_Install *) item;

    install->needed_by = g_slist_prepend (install->needed_by, needed_by);
}

void
rc_queue_item_install_set_channel_priority (RCQueueItem *item, int priority)
{
    RCQueueItem_Install *install;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL);
    g_return_if_fail (priority >= 0);

    install = (RCQueueItem_Install *) item;

    install->channel_priority = priority;
}

int
rc_queue_item_install_get_channel_penalty (RCQueueItem *item)
{
    RCQueueItem_Install *install;

    g_return_val_if_fail (item != NULL, 0);
    g_return_val_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL, 0);

    install = (RCQueueItem_Install *) item;

    return install->channel_priority;
}

void
rc_queue_item_install_set_other_penalty (RCQueueItem *item, int penalty)
{
    RCQueueItem_Install *install;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL);

    install = (RCQueueItem_Install *) item;

    install->other_penalty = penalty;
}

int
rc_queue_item_install_get_other_penalty (RCQueueItem *item)
{
    RCQueueItem_Install *install;

    g_return_val_if_fail (item != NULL, 0);
    g_return_val_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_INSTALL, 0);

    install = (RCQueueItem_Install *) item;

    return install->other_penalty;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct RequireProcessInfo {
    RCResolverContext *context;
    GSList *providers;
    GHashTable *uniq;
};

static void
require_process_cb (RCPackage *package, RCPackageSpec *spec, gpointer user_data)
{
    struct RequireProcessInfo *info = user_data;
    RCPackageStatus status;

    status = rc_resolver_context_get_status (info->context, package);

    if (status != RC_PACKAGE_STATUS_TO_BE_UNINSTALLED
        && status != RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE
        && ! rc_resolver_context_is_parallel_install (info->context, package)
        && g_hash_table_lookup (info->uniq, package) == NULL
        && rc_resolver_context_package_is_possible (info->context, package)) {

        info->providers = g_slist_prepend (info->providers, package);
        g_hash_table_insert (info->uniq, package, GINT_TO_POINTER (1));
    }
}

static void
look_for_upgrades_cb (RCPackage *package, gpointer user_data)
{
    GSList **slist = user_data;
    *slist = g_slist_prepend (*slist, package);
}

static gboolean
codependent_packages (RCPackage *pkg1, RCPackage *pkg2)
{
    const char *name1 = pkg1->spec.name;
    const char *name2 = pkg2->spec.name;
    int len1 = strlen (name1), len2 = strlen (name2);

    if (len2 < len1) {
        const char *swap = name1;
        int swap_len = len1;
        name1 = name2;
        name2 = swap;
        len1 = len2;
        len2 = swap_len;
    }

    /* foo and foo-bar are automatically co-dependent */
    if (len1 < len2
        && ! strncmp (name1, name2, len1)
        && name2[len1] == '-') {
        return TRUE;
    }
    
    return FALSE;
}

static gboolean
require_item_process (RCQueueItem *item,
                      RCResolverContext *context,
                      GSList **new_items)
{
    RCQueueItem_Require *require = (RCQueueItem_Require *) item;
    GSList *iter;
    int num_providers = 0;
    struct RequireProcessInfo info;

    RCWorld *world = rc_queue_item_get_world (item);
    char *msg;

    if (rc_resolver_context_requirement_is_met (context, require->dep))
        goto finished;

    info.context = context;
    info.providers = NULL;
    info.uniq = g_hash_table_new (rc_package_spec_hash,
                                  rc_package_spec_equal);

    if (! require->remove_only) {
        
        rc_world_foreach_providing_package (world, require->dep,
                                            RC_WORLD_ANY_CHANNEL,
                                            require_process_cb, &info);
        
        num_providers = g_slist_length (info.providers);
    } 

    if (num_providers == 0) {
        RCQueueItem *uninstall_item = NULL;
        RCQueueItem *branch_item = NULL;
        gboolean explore_uninstall_branch = TRUE;

        if (require->upgraded_package == NULL) {
            msg = g_strconcat ("There are no ",
                               require->remove_only ? "alternative installed" : "installable",
                               " providers of ",
                               rc_package_dep_to_str_static (require->dep),
                               " for ",
                               rc_package_to_str_static (require->requiring_package),
                               NULL);
            
            rc_resolver_context_add_info_str (context,
                                              require->requiring_package,
                                              RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                              msg);
        }

        /* If this is an upgrade, we might be able to avoid removing stuff by upgrading
           it instead. */
        if (require->upgraded_package) {
            GSList *upgrade_list = NULL;

            rc_world_foreach_upgrade (rc_queue_item_get_world (item),
                                      require->requiring_package,
                                      RC_WORLD_ANY_CHANNEL,
                                      look_for_upgrades_cb,
                                      &upgrade_list);

            if (upgrade_list) {
                GSList *iter;
                branch_item = rc_queue_item_new_branch (world);

                for (iter = upgrade_list; iter != NULL; iter = iter->next) {
                    RCPackage *upgrade_package = iter->data;
                    RCQueueItem *install_item;
                    RCResolverInfo *upgrade_info;

                    if (rc_resolver_context_package_is_possible (context, upgrade_package)) {
                    
                        install_item = rc_queue_item_new_install (world, upgrade_package);
                    
                        rc_queue_item_install_set_upgrade_package (install_item,
                                                                   require->requiring_package);
                        rc_queue_item_branch_add_item (branch_item, install_item);
                        
                        upgrade_info = rc_resolver_info_needed_by_new (upgrade_package);
                        rc_resolver_info_needed_add (upgrade_info, require->upgraded_package);
                        rc_queue_item_add_info (install_item, upgrade_info);

                        /* If an upgrade package has its requirements met,
                           don't do the uninstall branch.
                           FIXME: should we also look at conflicts here?
                        */
                        if (explore_uninstall_branch) {
                            GSList *req_iter = upgrade_package->requires;
                            while (req_iter) {
                                RCPackageDep *req = req_iter->data;
                                if (! rc_resolver_context_requirement_is_met (context, req))
                                    break;
                                
                                req_iter = req_iter->next;
                            }
                            if (req_iter == NULL) {
                                explore_uninstall_branch = FALSE;
                            }
                        }
                        
                    } /* if (rc_resolver_context_package_is_possible ( ... */
                } /* for (iter = upgrade_list; ... */
            } /* if (upgrade_list) ... */

            if (upgrade_list != NULL
                && rc_queue_item_branch_is_empty (branch_item)) {

                GSList *iter;

                for (iter = upgrade_list; iter != NULL; iter = iter->next) {
                    char *str;
                    char *p1, *p2;
                    RCResolverInfo *log_info;

                    p1 = rc_package_to_str (require->requiring_package);
                    p2 = rc_package_to_str ((RCPackage *) iter->data);
                    str = g_strconcat ("Upgrade to ", p2, " to avoid removing ", p1, 
                                       " is not possible.",
                                       NULL);
                    g_free (p1);
                    g_free (p2);

                    log_info = rc_resolver_info_misc_new (NULL,
                                                          RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                                          str);
                    rc_resolver_info_add_related_package (log_info, require->requiring_package);
                    rc_resolver_info_add_related_package (log_info, (RCPackage *) iter->data);
                    rc_resolver_context_add_info (context, log_info);

                    explore_uninstall_branch = TRUE;
                }
                
            } else if (upgrade_list != NULL
                && explore_uninstall_branch
                && codependent_packages (require->requiring_package,
                                         require->upgraded_package)) {
                explore_uninstall_branch = FALSE;
            }
            
            g_slist_free (upgrade_list);
        }
            
        if (explore_uninstall_branch) {
            RCResolverInfo *log_info;
            uninstall_item = rc_queue_item_new_uninstall (world,
                                                          require->requiring_package,
                                                          "unsatisfied requirements");
            rc_queue_item_uninstall_set_dep (uninstall_item, require->dep);
            
            if (require->lost_package) {
                log_info = rc_resolver_info_depends_on_new (require->requiring_package,
                                                            require->lost_package);
                rc_queue_item_add_info (uninstall_item, log_info);
            }
            
            if (require->remove_only)
                rc_queue_item_uninstall_set_remove_only (uninstall_item);
        }
        
        if (uninstall_item && branch_item) {
            rc_queue_item_branch_add_item (branch_item, uninstall_item);            
            *new_items = g_slist_prepend (*new_items, branch_item);
        } else if (uninstall_item) {
            *new_items = g_slist_prepend (*new_items, uninstall_item);
        } else if (branch_item) {
            *new_items = g_slist_prepend (*new_items, branch_item);
        }
        
    } else if (num_providers == 1) {
        
        RCQueueItem *install_item = rc_queue_item_new_install (world, (RCPackage *) info.providers->data);
        rc_queue_item_install_add_dep (install_item, require->dep);

        rc_queue_item_install_add_needed_by (install_item,
                                             require->requiring_package);
        
        *new_items = g_slist_prepend (*new_items, install_item);

    } else if (num_providers > 1) {

        RCQueueItem *branch_item = rc_queue_item_new_branch (world);
        for (iter = info.providers; iter != NULL; iter = iter->next) {
            RCQueueItem *install_item = rc_queue_item_new_install (world, iter->data);
            rc_queue_item_install_add_dep (install_item, require->dep);
            rc_queue_item_branch_add_item (branch_item, install_item);

            rc_queue_item_install_add_needed_by (install_item,
                                                 require->requiring_package);
        }

        *new_items = g_slist_prepend (*new_items, branch_item);

    } else {
        g_assert_not_reached ();
    }

    g_slist_free (info.providers);
    g_hash_table_destroy (info.uniq);
    
 finished:
    rc_queue_item_free (item);
    return TRUE;
}

static void
require_item_destroy (RCQueueItem *item)
{

}

static void
require_item_copy (const RCQueueItem *src, RCQueueItem *dest)
{
    const RCQueueItem_Require *src_require = (const RCQueueItem_Require *) src;
    RCQueueItem_Require *dest_require = (RCQueueItem_Require *) dest;
    
    dest_require->dep               = src_require->dep;
    dest_require->requiring_package = src_require->requiring_package;
    dest_require->upgraded_package  = src_require->upgraded_package;
    dest_require->remove_only       = src_require->remove_only;
}

static char *
require_item_to_string (RCQueueItem *item)
{
    RCQueueItem_Require *require = (RCQueueItem_Require *) item;

    return g_strconcat ("require ",
                        rc_package_relation_to_string (require->dep->relation, 0),
                        " ",
                        rc_package_spec_to_str_static (& require->dep->spec),
                        require->requiring_package ? " for " : NULL,
                        require->requiring_package ? rc_package_to_str_static (require->requiring_package) : NULL,
                        NULL);
}

RCQueueItem *
rc_queue_item_new_require (RCWorld *world, RCPackageDep *dep)
{
    RCQueueItem *item;
    RCQueueItem_Require *require;

    g_return_val_if_fail (dep != NULL, NULL);

    require = g_new0 (RCQueueItem_Require, 1);
    item = (RCQueueItem *) require;

    item->type      = RC_QUEUE_ITEM_TYPE_REQUIRE;
    item->size      = sizeof (RCQueueItem_Require);
    item->world     = world;
    item->process   = require_item_process;
    item->destroy   = require_item_destroy;
    item->copy      = require_item_copy;
    item->to_string = require_item_to_string;

    require->dep = dep;

    return item;
}

void
rc_queue_item_require_add_package (RCQueueItem *item, RCPackage *package)
{
    RCQueueItem_Require *require;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_REQUIRE);
    g_return_if_fail (package != NULL);

    require = (RCQueueItem_Require *) item;

    g_assert (require->requiring_package == NULL);
    require->requiring_package = package;
}

void
rc_queue_item_require_set_remove_only (RCQueueItem *item)
{
    RCQueueItem_Require *require = (RCQueueItem_Require *) item;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_REQUIRE);

    require->remove_only = TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
branch_item_process (RCQueueItem *item,
                     RCResolverContext *context,
                     GSList **new_items)
{
    RCQueueItem_Branch *branch = (RCQueueItem_Branch *) item;
    GSList *iter;
    GSList *live_branches = NULL;
    int branch_count;
    gboolean did_something = TRUE;

    for (iter = branch->possible_items; iter != NULL; iter = iter->next) {

        RCQueueItem *this_item = iter->data;

        if (rc_queue_item_is_satisfied (this_item, context))
            goto finished;

        /* Drop any useless branch items */
        if (! rc_queue_item_is_redundant (this_item, context)) {
            live_branches = g_slist_prepend (live_branches, iter->data);
        }
    }

    branch_count = g_slist_length (live_branches);

    if (branch_count == 0) {
        /* Do nothing */
    } else if (branch_count == 1) {

        RCQueueItem *new_item = rc_queue_item_copy (live_branches->data);
        *new_items = g_slist_prepend (*new_items, new_item);

    } else if (branch_count == g_slist_length (branch->possible_items)) {
        /* Nothing was eliminated, so just pass the branch through (and set it to
           NULL so that it won't get freed when we exit. */

        *new_items = g_slist_prepend (*new_items, item);
        item = NULL;
        did_something = FALSE;

    } else {
        RCQueueItem *new_branch = rc_queue_item_new_branch (rc_queue_item_get_world (item));
        GSList *iter;
        for (iter = live_branches; iter != NULL; iter = iter->next) {
            rc_queue_item_branch_add_item (new_branch, rc_queue_item_copy (iter->data));
        }
        *new_items = g_slist_prepend (*new_items, new_branch);
    }
    
 finished:
    g_slist_free (live_branches);
    rc_queue_item_free (item);

    return did_something;
}

static void
branch_item_destroy (RCQueueItem *item)
{
    RCQueueItem_Branch *branch = (RCQueueItem_Branch *) item;
    
    g_slist_foreach (branch->possible_items,
                     (GFunc) rc_queue_item_free, 
                     NULL);
    g_slist_free (branch->possible_items);
}

static void
branch_item_copy (const RCQueueItem *src, RCQueueItem *dest)
{
    const RCQueueItem_Branch *src_branch = (const RCQueueItem_Branch *) src;
    RCQueueItem_Branch *dest_branch = (RCQueueItem_Branch *) dest;
    GSList *iter;

    for (iter = src_branch->possible_items; iter != NULL; iter = iter->next) {
        RCQueueItem *copy = rc_queue_item_copy (iter->data);
        dest_branch->possible_items =
            g_slist_prepend (dest_branch->possible_items, copy);
    }
}

static char *
branch_item_to_string (RCQueueItem *item)
{
    RCQueueItem_Branch *branch = (RCQueueItem_Branch *) item;
    char *str, *items_str;

    items_str = item_slist_to_string (branch->possible_items);
    str = g_strdup_printf ("branch\n     %s", items_str);
    g_free (items_str);

    return str;
}

RCQueueItem *
rc_queue_item_new_branch (RCWorld *world)
{
    RCQueueItem *item;
    RCQueueItem_Branch *branch;
    
    branch = g_new0 (RCQueueItem_Branch, 1);
    item = (RCQueueItem *) branch;

    item->type      = RC_QUEUE_ITEM_TYPE_BRANCH;
    item->size      = sizeof (RCQueueItem_Branch);
    item->world     = world;
    item->process   = branch_item_process;
    item->destroy   = branch_item_destroy;
    item->copy      = branch_item_copy;
    item->to_string = branch_item_to_string;

    return item;
}

void
rc_queue_item_branch_add_item (RCQueueItem *item, RCQueueItem *subitem)
{
    RCQueueItem_Branch *branch;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_BRANCH);
    g_return_if_fail (subitem != NULL);

    g_assert (item != subitem);

    branch = (RCQueueItem_Branch *) item;

    branch->possible_items = g_slist_prepend (branch->possible_items, subitem);
}

gboolean
rc_queue_item_branch_is_empty (RCQueueItem *item)
{
    RCQueueItem_Branch *branch;

    g_return_val_if_fail (item != NULL, TRUE);
    g_return_val_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_BRANCH, TRUE);

    branch = (RCQueueItem_Branch *) item;
    return branch->possible_items == NULL;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct ConflictProcessInfo {
    RCWorld *world;
    RCPackage *conflicting_package;
    RCPackageDep *dep;
    RCResolverContext *context;
    GSList **new_items;

    char *pkg_str;
    char *dep_str;

    guint actually_an_obsolete : 1;
};

static void
conflict_process_cb (RCPackage *package, RCPackageSpec *spec, gpointer user_data)
{
    struct ConflictProcessInfo *info = (struct ConflictProcessInfo *) user_data;
    RCPackageStatus status;
    char *pkg_str, *spec_str, *msg;
    RCResolverInfo *log_info;

    if (rc_package_spec_equal (& package->spec, & info->conflicting_package->spec))
        return;

    if (! info->context->allow_conflicts_with_virtual_provides
        && rc_package_spec_get_type (info->world, spec) == RC_PACKAGE_SPEC_TYPE_VIRTUAL) {
        return;
    }

    pkg_str = rc_package_spec_to_str (& package->spec);
    spec_str = rc_package_spec_to_str (spec);

    status = rc_resolver_context_get_status (info->context, package);

    switch (status) {
        
    case RC_PACKAGE_STATUS_INSTALLED: {
        RCQueueItem *uninstall;
        RCResolverInfo *log_info;

        uninstall = rc_queue_item_new_uninstall (info->world, package, "conflict");
        rc_queue_item_uninstall_set_dep (uninstall, info->dep);

        ((RCQueueItem_Uninstall *)uninstall)->due_to_obsolete = info->actually_an_obsolete;
        
        log_info = rc_resolver_info_conflicts_with_new (package, info->conflicting_package);
        rc_queue_item_add_info (uninstall, log_info);
        
        *info->new_items = g_slist_prepend (*info->new_items, uninstall);
        break;
    }

    case RC_PACKAGE_STATUS_TO_BE_INSTALLED: {
        RCResolverInfo *log_info;
        
        msg = g_strconcat ("A conflict over ",
                           info->dep_str,
                           " (",
                           spec_str,
                           ") requires the removal of the to-be-installed package ",
                           pkg_str,
                           NULL);

        
        log_info = rc_resolver_info_misc_new (package,
                                              RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                              msg);

        rc_resolver_info_flag_as_error (log_info);
        rc_resolver_info_add_related_package (log_info, info->conflicting_package);

        rc_resolver_context_add_info (info->context, log_info);

        break;
    }

    case RC_PACKAGE_STATUS_UNINSTALLED: 
        rc_resolver_context_set_status (info->context, package,
                                        RC_PACKAGE_STATUS_TO_BE_UNINSTALLED);
        msg = g_strconcat ("Marking ",
                           pkg_str,
                           " as uninstallable due to conflicts over ",
                           info->dep_str,
                           " (",
                           spec_str,
                           ") from ",
                           info->pkg_str,
                           NULL);

        log_info = rc_resolver_info_misc_new (NULL,
                                              RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                              msg);

        rc_resolver_info_add_related_package (log_info, package);
        rc_resolver_info_add_related_package (log_info, info->conflicting_package);
        
        rc_resolver_context_add_info (info->context, log_info);

        break;

    case RC_PACKAGE_STATUS_TO_BE_UNINSTALLED:
    case RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE:
        /* This is the easy case -- we do nothing. */
        break;

    default:
        g_assert_not_reached ();
    }

    g_free (pkg_str);
    g_free (spec_str);
}

static gboolean
conflict_item_process (RCQueueItem *item,
                       RCResolverContext *context, 
                       GSList **new_items)
{
    RCQueueItem_Conflict *conflict = (RCQueueItem_Conflict *) item;
    RCWorld *world = rc_queue_item_get_world (item);

    struct ConflictProcessInfo info;

    info.world                = rc_queue_item_get_world (item);
    info.conflicting_package  = conflict->conflicting_package;
    info.dep                  = conflict->dep;
    info.context              = context;
    info.new_items            = new_items;
    info.pkg_str              = rc_package_spec_to_str (& conflict->conflicting_package->spec);
    info.dep_str              = g_strconcat (rc_package_relation_to_string (conflict->dep->relation, 0),
                                             " ",
                                             rc_package_spec_to_str_static (& conflict->dep->spec),
                                             NULL);
    info.actually_an_obsolete = conflict->actually_an_obsolete;


    rc_world_foreach_providing_package (world, conflict->dep,
                                        RC_WORLD_ANY_CHANNEL,
                                        conflict_process_cb,
                                        &info);
                                        
    g_free (info.pkg_str);
    g_free (info.dep_str);
    rc_queue_item_free (item);

    return TRUE;
}

static void
conflict_item_destroy (RCQueueItem *item)
{
    /* No-op */
}

static void
conflict_item_copy (const RCQueueItem *src, RCQueueItem *dest)
{
    const RCQueueItem_Conflict *src_conflict = (const RCQueueItem_Conflict *) src;
    RCQueueItem_Conflict *dest_conflict = (RCQueueItem_Conflict *) dest;

    dest_conflict->dep = src_conflict->dep;
    dest_conflict->conflicting_package = src_conflict->conflicting_package;
}

static char *
conflict_item_to_string (RCQueueItem *item)
{
    RCQueueItem_Conflict *conflict = (RCQueueItem_Conflict *) item;
    char *str, *package_str;

    package_str = rc_package_to_str (conflict->conflicting_package);

    str = g_strconcat ("conflict ",
                       rc_package_relation_to_string (conflict->dep->relation, 0),
                       " ",
                       rc_package_spec_to_str_static (& conflict->dep->spec),
                       " from ",
                       package_str,
                       NULL);

    g_free (package_str);
    
    return str;
}


RCQueueItem *
rc_queue_item_new_conflict (RCWorld *world, RCPackageDep *dep, RCPackage *package)
{
    RCQueueItem_Conflict *conflict;
    RCQueueItem *item;

    g_return_val_if_fail (dep != NULL, NULL);
    g_return_val_if_fail (package != NULL, NULL);

    conflict = g_new0 (RCQueueItem_Conflict, 1);
    item = (RCQueueItem *) conflict;

    item->type      = RC_QUEUE_ITEM_TYPE_CONFLICT;
    item->size      = sizeof (RCQueueItem_Conflict);
    item->world     = world;
    item->process   = conflict_item_process;
    item->destroy   = conflict_item_destroy;
    item->copy      = conflict_item_copy;
    item->to_string = conflict_item_to_string;

    conflict->dep = dep;
    conflict->conflicting_package = package;

    return item;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct UninstallProcessInfo {
    RCWorld *world;
    RCResolverContext *context;
    RCPackage *uninstalled_package;
    RCPackage *upgraded_package;
    GSList **require_items;
    guint remove_only : 1;
};

static void
uninstall_process_cb (RCPackage *package, RCPackageDep *dep, gpointer user_data)
{
    struct UninstallProcessInfo *info = user_data;
    RCQueueItem *require_item = NULL;

    if (! rc_resolver_context_package_is_present (info->context, package))
        return;

    if (rc_resolver_context_requirement_is_met (info->context, dep))
        return;

    require_item = rc_queue_item_new_require (info->world, dep);
    rc_queue_item_require_add_package (require_item, package);
    if (info->remove_only)
        rc_queue_item_require_set_remove_only (require_item);
    ((RCQueueItem_Require *) require_item)->upgraded_package = info->upgraded_package;
    ((RCQueueItem_Require *) require_item)->lost_package = info->uninstalled_package;

    *info->require_items = g_slist_prepend (*info->require_items, require_item);
}

static gboolean
uninstall_item_process (RCQueueItem *item,
                        RCResolverContext *context,
                        GSList **new_items)
{
    RCQueueItem_Uninstall *uninstall = (RCQueueItem_Uninstall *) item;
    RCWorld *world = rc_queue_item_get_world (item);
    
    RCPackageStatus status;
    char *pkg_str, *dep_str = NULL;
    GSList *iter;
    
    pkg_str = rc_package_spec_to_str (& uninstall->package->spec);

    if (uninstall->dep_leading_to_uninstall)
        dep_str = rc_package_dep_to_str (uninstall->dep_leading_to_uninstall);
    
    status = rc_resolver_context_get_status (context, uninstall->package);

    rc_resolver_context_uninstall_package (context,
                                           uninstall->package,
                                           uninstall->upgraded_to != NULL,
                                           uninstall->due_to_obsolete);
        
    if (status == RC_PACKAGE_STATUS_INSTALLED) {

        rc_resolver_context_set_status (context, uninstall->package,
                                        RC_PACKAGE_STATUS_TO_BE_UNINSTALLED);

        rc_queue_item_log_info (item, context);

#if 0
        msg = g_strconcat ("Uninstalling ",
                           pkg_str,
                           " due to ",
                           uninstall->reason, 
                           ": ", 
                           dep_str, 
                           NULL);
        rc_resolver_context_add_info_str (context,  
                                          uninstall->package, 
                                          RC_RESOLVER_INFO_PRIORITY_USER,
                                          msg);
#endif

        for (iter = uninstall->package->provides; iter != NULL; iter = iter->next) {
            RCPackageDep *dep = iter->data;
            struct UninstallProcessInfo info;
            
            info.world = rc_queue_item_get_world (item);
            info.context = context;
            info.uninstalled_package = uninstall->package;
            info.upgraded_package = uninstall->upgraded_to;
            info.require_items = new_items;
            info.remove_only = uninstall->remove_only;
            
            rc_world_foreach_requiring_package (world, dep,
                                                RC_WORLD_ANY_CHANNEL,
                                                uninstall_process_cb, &info);
        }
    }
    
    g_free (pkg_str);
    g_free (dep_str);
    rc_queue_item_free (item);

    return TRUE;
}

static void
uninstall_item_destroy (RCQueueItem *item)
{
    RCQueueItem_Uninstall *uninstall = (RCQueueItem_Uninstall *) item;
    g_free (uninstall->reason);
}

static void
uninstall_item_copy (const RCQueueItem *src, RCQueueItem *dest)
{
    const RCQueueItem_Uninstall *src_uninstall = (const RCQueueItem_Uninstall *) src;
    RCQueueItem_Uninstall *dest_uninstall = (RCQueueItem_Uninstall *) dest;

    dest_uninstall->package                   = src_uninstall->package;
    dest_uninstall->reason                    = g_strdup (src_uninstall->reason);
    dest_uninstall->dep_leading_to_uninstall  = src_uninstall->dep_leading_to_uninstall;
    dest_uninstall->remove_only               = src_uninstall->remove_only;
    dest_uninstall->upgraded_to               = src_uninstall->upgraded_to;
}

static char *
uninstall_item_to_string (RCQueueItem *item)
{
    RCQueueItem_Uninstall *uninstall = (RCQueueItem_Uninstall *) item;

    return g_strconcat ("uninstall (",
                        uninstall->reason,
                        ") ",
                        rc_package_to_str_static (uninstall->package),
                        " ",
                        uninstall->dep_leading_to_uninstall ?
                        rc_package_dep_to_str_static (uninstall->dep_leading_to_uninstall) : NULL,
                        NULL);
}

RCQueueItem *
rc_queue_item_new_uninstall (RCWorld *world, RCPackage *package, const char *reason)
{
    RCQueueItem_Uninstall *uninstall;
    RCQueueItem *item;

    g_return_val_if_fail (package != NULL, NULL);
    g_return_val_if_fail (reason && *reason, NULL);

    uninstall = g_new0 (RCQueueItem_Uninstall, 1);
    item = (RCQueueItem *) uninstall;

    item->type      = RC_QUEUE_ITEM_TYPE_UNINSTALL;
    item->priority  = 100;
    item->size      = sizeof (RCQueueItem_Uninstall);
    item->world     = world;
    item->process   = uninstall_item_process;
    item->destroy   = uninstall_item_destroy;
    item->copy      = uninstall_item_copy;
    item->to_string = uninstall_item_to_string;
    
    uninstall->package = package;
    uninstall->reason = g_strdup (reason);

    return item;
}

void
rc_queue_item_uninstall_set_dep (RCQueueItem *item, RCPackageDep *dep)
{
    RCQueueItem_Uninstall *uninstall = (RCQueueItem_Uninstall *) item;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_UNINSTALL);
    g_return_if_fail (dep != NULL);

    uninstall->dep_leading_to_uninstall = dep;
}

void
rc_queue_item_uninstall_set_remove_only (RCQueueItem *item)
{
    RCQueueItem_Uninstall *uninstall = (RCQueueItem_Uninstall *) item;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_UNINSTALL);

    uninstall->remove_only = TRUE;
}

void
rc_queue_item_uninstall_set_upgraded_to (RCQueueItem *item,
                                         RCPackage *package)
{
    RCQueueItem_Uninstall *uninstall = (RCQueueItem_Uninstall *) item;

    g_return_if_fail (item != NULL);
    g_return_if_fail (rc_queue_item_type (item) == RC_QUEUE_ITEM_TYPE_UNINSTALL);

    g_assert (uninstall->upgraded_to == NULL);
    uninstall->upgraded_to = package;
}