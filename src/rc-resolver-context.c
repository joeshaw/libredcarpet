/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-context.c
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

#include <config.h>
#include "rc-resolver-context.h"

#include "rc-world.h"
#include "rc-package.h"

const char *
rc_package_status_to_string (RCPackageStatus status)
{
    switch (status) {
    case RC_PACKAGE_STATUS_UNKNOWN:
        return "unknown";
    case RC_PACKAGE_STATUS_INSTALLED:
        return "installed";
    case RC_PACKAGE_STATUS_UNINSTALLED:
        return "uninstalled";
    case RC_PACKAGE_STATUS_TO_BE_INSTALLED:
        return "to be installed";
    case RC_PACKAGE_STATUS_TO_BE_INSTALLED_SOFT:
        return "to be installed (soft)";
    case RC_PACKAGE_STATUS_TO_BE_UNINSTALLED:
        return "to be uninstalled";
    case RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE:
        return "to be uninstalled due to obsolete";

    default:
        return "Huh?";
    }

    g_assert_not_reached ();
    return NULL;
}

RCResolverContext *
rc_resolver_context_new (void)
{
    return rc_resolver_context_new_child (NULL);
}

RCResolverContext *
rc_resolver_context_new_child (RCResolverContext *parent)
{
    RCResolverContext *context = g_new0 (RCResolverContext, 1);

    context->refs = 1;
    context->status = g_hash_table_new (rc_package_spec_hash,
                                        rc_package_spec_equal);
    context->parent = rc_resolver_context_ref (parent);
    
    if (parent) {
        context->world           = parent->world;
        context->download_size   = parent->download_size;
        context->install_size    = parent->install_size;
        context->total_priority  = parent->total_priority;
        context->max_priority    = parent->max_priority;
        context->min_priority    = parent->min_priority;
        context->other_penalties = parent->other_penalties;
    } else {
        context->min_priority = G_MAXINT;
    }

    context->verifying = parent ? parent->verifying : FALSE;
    context->invalid = FALSE;

    return context;
}

RCResolverContext *
rc_resolver_context_ref (RCResolverContext *context)
{
    if (context) {
        g_assert (context->refs > 0);
        ++context->refs;
    }
    
    return context;
}

void
rc_resolver_context_unref (RCResolverContext *context)
{
    if (context) {

        g_assert (context->refs > 0);
        --context->refs;

        if (context->refs == 0) {

            g_hash_table_destroy (context->status);

            g_list_foreach (context->log,
                            (GFunc) rc_resolver_info_free, NULL);
            g_list_free (context->log);

            rc_resolver_context_unref (context->parent);

            g_free (context);
        }
    }
}

RCWorld *
rc_resolver_context_get_world (RCResolverContext *context)
{
    g_return_val_if_fail (context != NULL, NULL);

    if (context->world)
        return context->world;
    else
        return rc_get_world ();
}

void
rc_resolver_context_set_status (RCResolverContext *context,
                                RCPackage *package,
                                RCPackageStatus status)
{
    RCPackageStatus old_status;

    g_return_if_fail (context != NULL);
    g_return_if_fail (package != NULL);
    g_return_if_fail (!context->invalid);

    old_status = rc_resolver_context_get_status (context, package);

    if (rc_package_status_is_to_be_installed (status)
        || rc_package_status_is_to_be_uninstalled (status)) {
        
        if (status != old_status) {
            g_hash_table_insert (context->status,
                                 package,
                                 GINT_TO_POINTER (status));
        }

    } else {

        g_assert_not_reached ();
    
    }

    /* Update our cache if we changed the status of the last
       checked package. */
    if (context->last_checked_package == package)
        context->last_checked_status = status;
}

RCPackageStatus
rc_resolver_context_get_status (RCResolverContext *context,
                                RCPackage *package)
{
    RCResolverContext *c = context;
    RCPackageStatus status = RC_PACKAGE_STATUS_UNKNOWN;

    g_return_val_if_fail (context != NULL, RC_PACKAGE_STATUS_UNKNOWN);
    g_return_val_if_fail (package != NULL, RC_PACKAGE_STATUS_UNKNOWN);

    /* We often end up getting the status of the same package several times in a row.
       By caching the status of the last checked package, we can in practice eliminate
       the need for any hash table lookups in about 50% of our calls to get_status. */
    if (context->last_checked_package
        && rc_package_spec_equal (&package->spec, &context->last_checked_package->spec)) {
        return context->last_checked_status;
    }

    while (status == RC_PACKAGE_STATUS_UNKNOWN 
           && c) {
        status = (RCPackageStatus) GPOINTER_TO_INT (g_hash_table_lookup (c->status, package));
        c = c->parent;
    }

    if (status == RC_PACKAGE_STATUS_UNKNOWN) {
        status = rc_package_is_installed (package) 
            ? RC_PACKAGE_STATUS_INSTALLED : RC_PACKAGE_STATUS_UNINSTALLED;
    }

    context->last_checked_package = package;
    context->last_checked_status = status;

    return status;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_resolver_context_install_package (RCResolverContext *context,
                                     RCPackage *package,
                                     gboolean is_soft,
                                     int other_penalty)
{
    RCPackageStatus status;
    int priority;
    char *msg;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    status = rc_resolver_context_get_status (context, package);

    if (rc_package_status_is_to_be_uninstalled (status)) {
        msg = g_strconcat ("Can't install ",
                           rc_package_spec_to_str_static (& package->spec),
                           " since it is already marked as needing to be uninstalled",
                           NULL);

        rc_resolver_context_add_error_str (context,
                                           package,
                                           msg);
        return FALSE;
    }

    if (rc_package_status_is_to_be_installed (status))
        return TRUE;

    if (rc_resolver_context_is_parallel_install (context, package)) {
        msg = g_strconcat ("Can't install ",
                           rc_package_spec_to_str_static (& package->spec),
                           ", since a package of the same name "
                           "is already marked as needing to be installed",
                           NULL);

        rc_resolver_context_add_error_str (context,
                                           package,
                                           msg);
        return FALSE;
    }

    rc_resolver_context_set_status (context, package,
                                    is_soft ? 
                                    RC_PACKAGE_STATUS_TO_BE_INSTALLED_SOFT : 
                                    RC_PACKAGE_STATUS_TO_BE_INSTALLED);

    if (status == RC_PACKAGE_STATUS_UNINSTALLED) {
        /* FIXME: Incomplete */
        context->download_size += package->file_size;
        context->install_size += package->installed_size;

        if (package->local_package)
            priority = 0;
        else {
            priority = rc_resolver_context_get_channel_priority (
                context, package->channel);
        }

        if (priority < context->min_priority)
            context->min_priority = priority;
        if (priority > context->max_priority)
            context->max_priority = priority;

        context->other_penalties += other_penalty;

    }
        
    return TRUE;
}

gboolean
rc_resolver_context_upgrade_package (RCResolverContext *context,
                                     RCPackage *package,
                                     RCPackage *old_package,
                                     gboolean is_soft,
                                     int other_penalty)
{
    RCPackageStatus status;
    int priority;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);
    g_return_val_if_fail (old_package != NULL, FALSE);

    status = rc_resolver_context_get_status (context, package);

    if (rc_package_status_is_to_be_uninstalled (status))
        return FALSE;

    if (rc_package_status_is_to_be_installed (status))
        return TRUE;

    rc_resolver_context_set_status (context, package,
                                    is_soft ? 
                                    RC_PACKAGE_STATUS_TO_BE_INSTALLED_SOFT :
                                    RC_PACKAGE_STATUS_TO_BE_INSTALLED);

    if (status == RC_PACKAGE_STATUS_UNINSTALLED) {
      
        context->download_size += package->file_size;
        

        /* FIXME: Incomplete */
        /* We should change installed_size to reflect the difference in
           installed size between the old and new versions. */

        

        if (package->local_package)
            priority = 0;
        else {
            priority = rc_resolver_context_get_channel_priority (
                context, package->channel);
        }

        if (priority < context->min_priority)
            context->min_priority = priority;
        if (priority > context->max_priority)
            context->max_priority = priority;

        context->other_penalties += other_penalty;
    }

    return TRUE;
}

gboolean
rc_resolver_context_uninstall_package (RCResolverContext *context,
                                       RCPackage *package,
                                       gboolean part_of_upgrade,
                                       gboolean due_to_obsolete)
{
    RCPackageStatus status, new_status;
    char *msg;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    status = rc_resolver_context_get_status (context, package);

    if (status == RC_PACKAGE_STATUS_TO_BE_INSTALLED) {
        msg = g_strconcat ("Can't uninstall the to-be-installed package ",
                           rc_package_spec_to_str_static (& package->spec),
                           NULL);
        rc_resolver_context_add_error_str (context,
                                           package,
                                           msg);
        return FALSE;
    }

    if (rc_package_status_is_to_be_uninstalled (status)) {
        return TRUE;
    }
    
    if (status == RC_PACKAGE_STATUS_UNINSTALLED) {
        msg = g_strconcat ("Marking package ",
                           rc_package_spec_to_str_static (& package->spec),
                           " as uninstallable",
                           NULL);
        rc_resolver_context_add_info_str (context,
                                          package,
                                          RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                          msg);
    }


    if (due_to_obsolete)
        new_status = RC_PACKAGE_STATUS_TO_BE_UNINSTALLED_DUE_TO_OBSOLETE;
    else
        new_status = RC_PACKAGE_STATUS_TO_BE_UNINSTALLED;

    rc_resolver_context_set_status (context, package, new_status);
    
    if (status == RC_PACKAGE_STATUS_INSTALLED) {
        /* FIXME: incomplete */
    }

    return TRUE;
}

gboolean
rc_resolver_context_package_is_present (RCResolverContext *context,
                                        RCPackage *package)
{
    RCPackageStatus status;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    status = rc_resolver_context_get_status (context, package);
    g_return_val_if_fail (status != RC_PACKAGE_STATUS_UNKNOWN, FALSE);

    return status == RC_PACKAGE_STATUS_INSTALLED
        || rc_package_status_is_to_be_installed (status);
}

gboolean
rc_resolver_context_package_is_absent (RCResolverContext *context,
                                       RCPackage *package)
{
    RCPackageStatus status;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    status = rc_resolver_context_get_status (context, package);
    g_return_val_if_fail (status != RC_PACKAGE_STATUS_UNKNOWN, FALSE);

    return status == RC_PACKAGE_STATUS_UNINSTALLED 
        || rc_package_status_is_to_be_uninstalled (status);
}

struct MarkedPackageInfo {
    RCMarkedPackageFn fn;
    gpointer user_data;
};

static void
marked_pkg_cb (gpointer key, gpointer val, gpointer user_data)
{
    RCPackage *package = key;
    RCPackageStatus status = (RCPackageStatus) GPOINTER_TO_INT (val);
    struct MarkedPackageInfo *info = user_data;

    info->fn (package, status, info->user_data);
}

void
rc_resolver_context_foreach_marked_package (RCResolverContext *context,
                                            RCMarkedPackageFn fn,
                                            gpointer user_data)
{
    struct MarkedPackageInfo info;

    g_return_if_fail (context != NULL);
    g_return_if_fail (fn != NULL);

    info.fn = fn;
    info.user_data = user_data;

    while (context) {
        g_hash_table_foreach (context->status, marked_pkg_cb, &info);
        context = context->parent;
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct InstallInfo {
    RCWorld *world;
    RCMarkedPackageFn fn;
    gpointer user_data;
    int count;
};

static void
install_pkg_cb (RCPackage *package,
                RCPackageStatus status,
                gpointer user_data)
{
    struct InstallInfo *info = user_data;

    if (rc_package_status_is_to_be_installed (status)
        && ! rc_package_is_installed (package)
        && rc_world_find_installed_version (info->world, package) == NULL) {

        if (info->fn)
            info->fn (package, status, info->user_data);
        ++info->count;
    }
}

int
rc_resolver_context_foreach_install (RCResolverContext *context,
                                     RCMarkedPackageFn fn,
                                     gpointer user_data)
{
    struct InstallInfo info;

    g_return_val_if_fail (context != NULL, -1);

    info.world = rc_resolver_context_get_world (context);
    info.fn = fn;
    info.user_data = user_data;
    info.count = 0;

    rc_resolver_context_foreach_marked_package (context,
                                                install_pkg_cb,
                                                &info);

    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct UpgradeInfo {
    RCWorld *world;
    RCMarkedPackagePairFn fn;
    gpointer user_data;
    RCResolverContext *context;
    int count;
};

static void
upgrade_pkg_cb (RCPackage *package,
                RCPackageStatus status,
                gpointer user_data)
{
    struct UpgradeInfo *info = user_data;
    RCPackage *to_be_upgraded;
    RCPackageStatus tbu_status;

    if (rc_package_status_is_to_be_installed (status)
        && ! rc_package_is_installed (package)) {
        
        to_be_upgraded = rc_world_find_installed_version (info->world, package);
        if (to_be_upgraded) {
            tbu_status = rc_resolver_context_get_status (info->context, to_be_upgraded);
            
            if (info->fn)
                info->fn (package, status, 
                          to_be_upgraded, tbu_status,
                          info->user_data);

            ++info->count;
        }
    }
}

int
rc_resolver_context_foreach_upgrade (RCResolverContext *context,
                                     RCMarkedPackagePairFn fn,
                                     gpointer user_data)
{
    struct UpgradeInfo info;

    g_return_val_if_fail (context != NULL, -1);

    info.world = rc_resolver_context_get_world (context);
    info.fn = fn;
    info.user_data = user_data;
    info.context = context;
    info.count = 0;

    rc_resolver_context_foreach_marked_package (context,
                                                upgrade_pkg_cb,
                                                &info);

    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct UninstallInfo {
    RCMarkedPackageFn fn;
    gpointer user_data;
    GHashTable *upgrade_hash;
    int count;
};

static void
uninstall_pkg_cb (RCPackage *package,
                  RCPackageStatus status,
                  gpointer user_data)
{
    struct UninstallInfo *info = user_data;

    if (rc_package_status_is_to_be_uninstalled (status)
        && g_hash_table_lookup (info->upgrade_hash, GINT_TO_POINTER (package->spec.nameq)) == NULL) {
        if (info->fn)
            info->fn (package, status, info->user_data);
        ++info->count;
    }
}

static void
build_upgrade_hash_cb (RCPackage *package_add,
                       RCPackageStatus status_add,
                       RCPackage *package_del,
                       RCPackageStatus status_del,
                       gpointer user_data)
{
    GHashTable *upgrade_hash = user_data;

    g_hash_table_insert (upgrade_hash,
                         GINT_TO_POINTER (package_del->spec.nameq),
                         package_del);
}

int
rc_resolver_context_foreach_uninstall (RCResolverContext *context,
                                       RCMarkedPackageFn fn,
                                       gpointer user_data)
{
    struct UninstallInfo info;

    g_return_val_if_fail (context != NULL, -1);
    
    info.fn = fn;
    info.user_data = user_data;
    info.upgrade_hash = g_hash_table_new (NULL, NULL);
    info.count = 0;

    rc_resolver_context_foreach_upgrade (context,
                                         build_upgrade_hash_cb,
                                         info.upgrade_hash);

    rc_resolver_context_foreach_marked_package (context,
                                                uninstall_pkg_cb,
                                                &info);

    g_hash_table_destroy (info.upgrade_hash);

    return info.count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
install_count_cb (RCPackage      *pkg,
                  RCPackageStatus status,
                  gpointer        user_data)
{
    if (pkg->channel != NULL) {
        int *count = user_data;
        ++*count;
    }
}

int
rc_resolver_context_install_count (RCResolverContext *context)
{
    int count = 0;

    g_return_val_if_fail (context != NULL, -1);

    rc_resolver_context_foreach_install (context, install_count_cb, &count);

    return count;
}

static void
uninstall_count_cb (RCPackage       *pkg,
                    RCPackageStatus status,
                    gpointer        user_data)
{
    if (pkg->channel == NULL) {
        int *count = user_data;
        ++*count;
    }
}

int
rc_resolver_context_uninstall_count (RCResolverContext *context)
{
    int count = 0;

    g_return_val_if_fail (context != NULL, -1);

    rc_resolver_context_foreach_uninstall (context, uninstall_count_cb, &count);

    return count;
}

int
rc_resolver_context_upgrade_count (RCResolverContext *context)
{
    g_return_val_if_fail (context != NULL, -1);

    return rc_resolver_context_foreach_upgrade (context, NULL, NULL);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_resolver_context_is_valid (RCResolverContext *context)
{
    g_return_val_if_fail (context != NULL, FALSE);
    return ! context->invalid;
}

gboolean
rc_resolver_context_is_invalid (RCResolverContext *context)
{
    g_return_val_if_fail (context != NULL, TRUE);
    return context->invalid;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_resolver_context_add_info (RCResolverContext *context,
                              RCResolverInfo *info)
{
    g_return_if_fail (context != NULL);
    g_return_if_fail (info != NULL);

    context->log = g_list_prepend (context->log, info);
    /* context->propagated_importance = FALSE; */

    if (rc_resolver_info_is_error (info)) {

        if (! context->invalid) {
            RCResolverInfo *info;
            info = rc_resolver_info_misc_new (NULL,
                                              RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                              g_strdup ("Marking this resolution attempt as invalid."));
            rc_resolver_info_flag_as_error (info);
            context->log = g_list_prepend (context->log, info);
        }

        context->invalid = TRUE;
    }
}

void
rc_resolver_context_add_info_str (RCResolverContext *context,
                                  RCPackage *package, int priority,
                                  char *msg)
{
    RCResolverInfo *info;

    g_return_if_fail (context != NULL);
    g_return_if_fail (msg != NULL);

    info = rc_resolver_info_misc_new (package, priority, msg);
    rc_resolver_context_add_info (context, info);
}

void
rc_resolver_context_add_error_str (RCResolverContext *context,
                                   RCPackage *package,
                                   char *msg)
{
    RCResolverInfo *info;

    g_return_if_fail (context != NULL);
    g_return_if_fail (msg != NULL);

    info = rc_resolver_info_misc_new (package,
                                      RC_RESOLVER_INFO_PRIORITY_VERBOSE,
                                      msg);

    rc_resolver_info_flag_as_error (info);

    rc_resolver_context_add_info (context, info);
}

#if 0
static void
rc_resolver_context_propagate_importance (RCResolverContext *context)
{
    GHashTable *important_hash = NULL;
    GSList *important_list = NULL, *pkg_iter;
    GList *log_iter;
    gboolean did_something;
    int propagation_count = 1;

    g_return_if_fail (context != NULL);
    
    if (context->propagated_importance)
        return;

    important_hash = g_hash_table_new (NULL, NULL);

    do {
        did_something = FALSE;

        ++propagation_count;

        for (log_iter = context->log; log_iter != NULL; log_iter = log_iter->next) {
            RCResolverInfo *info = log_iter->data;
            
            /* If info is not important, but mentions a package that we have declared
               to be important, flag info as important. */
            if (! rc_resolver_info_is_important (info)) {
                
                gboolean mentions_important = FALSE;

                pkg_iter = important_list;
                while (pkg_iter && ! mentions_important) {
                    RCPackage *pkg = pkg_iter->data;
                    if (rc_resolver_info_mentions (info, pkg))
                        mentions_important = TRUE;
                    pkg_iter = pkg_iter->next;
                }
                
                if (mentions_important) {
                    rc_resolver_info_flag_as_important (info);
                    did_something = TRUE;
                }
            }

            /* If info is important, walk across the package_list and
               and store those in our important package hash/list. */
            if (rc_resolver_info_is_important (info)) {

                if (info->package
                    && g_hash_table_lookup (important_hash, info->package) == NULL) {
                    g_hash_table_insert (important_hash, info->package, info->package);
                    important_list = g_slist_prepend (important_list, info->package);
                    did_something = TRUE;
                }

                
                pkg_iter = info->package_list;
                while (pkg_iter != NULL) {
                    RCPackage *pkg = pkg_iter->data;
                    
                    if (g_hash_table_lookup (important_hash, pkg) == NULL) {
                        g_hash_table_insert (important_hash, pkg, pkg);
                        important_list = g_slist_prepend (important_list, pkg);
                        did_something = TRUE;
                    }
                    
                    pkg_iter = pkg_iter->next;
                }

            } /* if (rc_resolver_info_is_important (info)) ... */

        }

        if (propagation_count > 10000) {
            g_warning ("Excessive looping in rc_resolver_context_propagate_importance!");
            did_something = FALSE;
        }

    } while (did_something);

    g_hash_table_destroy (important_hash);
    g_slist_free (important_list);

    context->propagated_importance = TRUE;
}
#endif

/*
  We call a package mentioned by an error info an "error-package".
  We call a package mentioned by an important info an "important-package".
  
  The rules:
  (1) An info item that mentions an error-package is important.
  (2) An info item is about an important-package is important.
*/
static void
mark_important_info (GSList *info_slist)
{
    GHashTable *important_hash, *error_hash;
    gboolean did_something;
    GSList *important_slist = NULL, *error_slist = NULL;
    GSList *info_iter, *pkg_iter;
    int pass_num = 1;

    important_hash = g_hash_table_new (NULL, NULL);
    error_hash     = g_hash_table_new (NULL, NULL);

    /* First of all, store all error-packages in a hash. */
    for (info_iter = info_slist; info_iter != NULL; info_iter = info_iter->next) {
        RCResolverInfo *info = info_iter->data;
            
        if (info && rc_resolver_info_is_error (info)) {

            if (info->package
                && g_hash_table_lookup (error_hash, info->package) == NULL) {
                g_hash_table_insert (error_hash, info->package, (gpointer)1);
                error_slist = g_slist_prepend (error_slist, info->package);
            }

            for (pkg_iter = info->package_list; pkg_iter != NULL; pkg_iter = pkg_iter->next) {
                RCPackage *pkg = pkg_iter->data;
                if (g_hash_table_lookup (error_hash, pkg) == NULL) {
                    g_hash_table_insert (error_hash, pkg, (gpointer)1);
                    error_slist = g_slist_prepend (error_slist, pkg);
                }
            }
        }
    }             
        
    
    do {
        
        ++pass_num;
        g_assert (pass_num < 10000);

        did_something = FALSE;

        for (info_iter = info_slist; info_iter != NULL; info_iter = info_iter->next) {
            RCResolverInfo *info = info_iter->data;
            
            if (info && ! rc_resolver_info_is_important (info)) {
                gboolean should_be_important = FALSE;

                for (pkg_iter = error_slist;
                     pkg_iter != NULL && ! should_be_important;
                     pkg_iter = pkg_iter->next) {
                    RCPackage *pkg = pkg_iter->data;

                    if (rc_resolver_info_mentions (info, pkg)) {
                        should_be_important = TRUE;
                    }
                }

                for (pkg_iter = important_slist;
                     pkg_iter != NULL && ! should_be_important;
                     pkg_iter = pkg_iter->next) {
                    RCPackage *pkg = pkg_iter->data;
                    
                    if (rc_resolver_info_is_about (info, pkg)) {
                        should_be_important = TRUE;
                        break;
                    }
                }

                if (should_be_important) {
                    did_something = TRUE;
                    rc_resolver_info_flag_as_important (info);
                    for (pkg_iter = info->package_list; pkg_iter != NULL; pkg_iter = pkg_iter->next) {
                        RCPackage *pkg = pkg_iter->data;
                        if (g_hash_table_lookup (important_hash, pkg) == NULL) {
                            g_hash_table_insert (important_hash, pkg, (gpointer)1);
                            important_slist = g_slist_prepend (important_slist, pkg);
                        }
                    }
                }
            }
        }

    } while (did_something);

    g_hash_table_destroy (error_hash);
    g_hash_table_destroy (important_hash);
    g_slist_free (error_slist);
    g_slist_free (important_slist);
}

void
rc_resolver_context_foreach_info (RCResolverContext *context,
                                  RCPackage *package, int priority,
                                  RCResolverInfoFn fn,
                                  gpointer user_data)
{
    GList *iter;
    GSList *info_slist = NULL, *info_iter;

    g_return_if_fail (context != NULL);
    g_return_if_fail (fn != NULL);

    /* Assemble a list of copies of all of the info objects */
    while (context) {
        for (iter = context->log; iter != NULL; iter = iter->next) {
            RCResolverInfo *info = iter->data;
            if ((package == NULL || info->package == package)
                && info->priority >= priority) {
                info_slist = g_slist_prepend (info_slist, 
                                              rc_resolver_info_copy (info));
            }
        }
        context = context->parent;
    }

    /* Merge info objects */
    for (info_iter = info_slist; info_iter != NULL; info_iter = info_iter->next) {
        RCResolverInfo *info1 = info_iter->data;
        GSList *subiter;
        if (info1 != NULL) {
            for (subiter = info_iter->next; subiter != NULL; subiter = subiter->next) {
                RCResolverInfo *info2 = subiter->data;
                if (info2 && rc_resolver_info_merge (info1, info2)) {
                    rc_resolver_info_free (info2);
                    subiter->data = NULL;
                }
            }
        }
    }

    mark_important_info (info_slist);

    /* Walk across the list of info objects and invoke our callback */
    for (info_iter = info_slist; info_iter != NULL; info_iter = info_iter->next) {
        RCResolverInfo *info = info_iter->data;
        if (info != NULL) {
            fn (info, user_data);
            rc_resolver_info_free (info);
        }
    }

    g_slist_free (info_slist);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
spew_pkg_cb (RCPackage *pkg,
             RCPackageStatus status,
             gpointer user_data)
{
    g_print ("  %s (%s)\n",
             rc_package_to_str_static (pkg),
             rc_package_status_to_string (status));
}

void
spew_pkg2_cb (RCPackage *pkg1,
              RCPackageStatus status1,
              RCPackage *pkg2,
              RCPackageStatus status2,
              gpointer user_data)
{
    char *s1, *s2;

    s1 = rc_package_to_str (pkg1);
    s2 = rc_package_to_str (pkg2);

    g_print ("  %s (%s) => %s (%s)\n",
             s2,
             rc_package_status_to_string (status2),
             s1,
             rc_package_status_to_string (status1));

    g_free (s1);
    g_free (s2);
}

void
rc_resolver_context_spew (RCResolverContext *context)
{
    g_print ("TO INSTALL:\n");
    rc_resolver_context_foreach_install (context, spew_pkg_cb, NULL);
    g_print ("\n");

    g_print ("TO REMOVE:\n");
    rc_resolver_context_foreach_uninstall (context, spew_pkg_cb, NULL);
    g_print ("\n");

    g_print ("TO UPGRADE:\n");
    rc_resolver_context_foreach_upgrade (context, spew_pkg2_cb, NULL);
    g_print ("\n");
}
 
void
spew_info_cb (RCResolverInfo *info, gpointer user_data)
{
    char *msg = rc_resolver_info_to_string (info);
    if (rc_resolver_info_is_error (info))
        g_print ("[ERROR] ");
    else if (rc_resolver_info_is_important (info))
        g_print ("[>>>>>] ");
    g_print ("%s\n", msg);
    g_free (msg);
}

void
rc_resolver_context_spew_info (RCResolverContext *context)
{
    g_return_if_fail (context != NULL);
    rc_resolver_context_foreach_info (context, NULL, -1, spew_info_cb, NULL);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct RequirementMetInfo {
    RCResolverContext *context;
    gboolean flag;
};

static gboolean
requirement_met_cb (RCPackage *package, RCPackageSpec *spec, gpointer user_data)
{
    struct RequirementMetInfo *info = user_data;

    if (rc_resolver_context_package_is_present (info->context, package)) {
            info->flag = TRUE;
    }

    return ! info->flag;
}

gboolean
rc_resolver_context_requirement_is_met (RCResolverContext *context,
                                        RCPackageDep *dep)
{
    struct RequirementMetInfo info;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (dep != NULL, FALSE);

    info.context = context;
    info.flag = FALSE;

    rc_world_check_providing_package (rc_resolver_context_get_world (context),
                                      dep, RC_WORLD_ANY_CHANNEL, FALSE,
                                      requirement_met_cb, &info);
    
    return info.flag;
}

static gboolean
requirement_possible_cb (RCPackage *package, RCPackageSpec *spec, gpointer user_data)
{
    struct RequirementMetInfo *info = user_data;
    RCPackageStatus status = rc_resolver_context_get_status (info->context, package);

    if (! rc_package_status_is_to_be_uninstalled (status)) {
        info->flag = TRUE;
    }

    return ! info->flag;
}

gboolean
rc_resolver_context_requirement_is_possible (RCResolverContext *context,
                                             RCPackageDep *dep)
{
    struct RequirementMetInfo info;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (dep != NULL, FALSE);

    info.context = context;
    info.flag = FALSE;

    rc_world_check_providing_package (rc_resolver_context_get_world (context),
                                      dep, RC_WORLD_ANY_CHANNEL, FALSE, 
                                      requirement_possible_cb, &info);
    
    return info.flag;
}

gboolean
rc_resolver_context_package_is_possible (RCResolverContext *context,
                                         RCPackage *package)
{
    int i;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    if (package->requires_a)
        for (i = 0; i < package->requires_a->len; i++) {
            RCPackageDep *dep = package->requires_a->data[i];
            if (! rc_resolver_context_requirement_is_possible (context, dep)) {
                return FALSE;
            }
        }

    return TRUE;
}

struct DupNameCheckInfo {
    RCPackageSpec *spec;
    gboolean flag;
};

static void
dup_name_check_cb (RCPackage *package, RCPackageStatus status, gpointer user_data)
{
    struct DupNameCheckInfo *info = user_data;

    if (! info->flag
        && rc_package_status_is_to_be_installed (status)
        && info->spec->nameq == package->spec.nameq
        && rc_package_spec_not_equal (info->spec, & package->spec)) {
        info->flag = TRUE;
    }
}

gboolean
rc_resolver_context_is_parallel_install (RCResolverContext *context,
                                         RCPackage *package)
{
    struct DupNameCheckInfo info;

    g_return_val_if_fail (context != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    info.spec = & package->spec;
    info.flag = FALSE;
    rc_resolver_context_foreach_marked_package (context, dup_name_check_cb, &info);

    return info.flag;
}

gint
rc_resolver_context_get_channel_priority (RCResolverContext *context,
                                          const RCChannel *channel)
{
    RCResolverContext *c;
    gboolean is_current = FALSE, is_subscribed = FALSE;
    int priority;

    g_return_val_if_fail (context != NULL, 0);
    g_return_val_if_fail (channel != NULL, 0);

    for (c = context; c != NULL; c = c->parent) {
        if (c->current_channel) {
            is_current = (c->current_channel == channel);
            break;
        }
    }

    if (! is_current) {
        is_subscribed = rc_channel_subscribed (channel);
    }

    priority = rc_channel_get_priority (channel, is_subscribed, is_current);

    return priority;
}
