/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-info.c
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
#include "rc-resolver-info.h"

RCResolverInfoType
rc_resolver_info_type (RCResolverInfo *info)
{
    g_return_val_if_fail (info != NULL, RC_RESOLVER_INFO_TYPE_INVALID);
    
    return info->type;
}

gboolean
rc_resolver_info_merge (RCResolverInfo *info, RCResolverInfo *to_be_merged)
{
    GSList *iter;
    GHashTable *seen_pkgs;

    g_return_val_if_fail (info != NULL && to_be_merged != NULL, FALSE);

    if (info->type != to_be_merged->type
        || info->package != to_be_merged->package)
        return FALSE;

    /* We only merge miscellaneous info items if they have identical
       text messages. */
    if (info->type == RC_RESOLVER_INFO_TYPE_MISC) {
        if (info->msg
            && to_be_merged->msg
            && !strcmp (info->msg, to_be_merged->msg))
            return TRUE;
        return FALSE;
    }
    
    seen_pkgs = g_hash_table_new (NULL, NULL);
    for (iter = info->package_list; iter; iter = iter->next)
        g_hash_table_insert (seen_pkgs, iter->data, (gpointer) 0x1);
    for (iter = to_be_merged->package_list; iter; iter = iter->next) {
        if (g_hash_table_lookup (seen_pkgs, iter->data) == NULL) {
            info->package_list = g_slist_prepend (info->package_list, iter->data);
            g_hash_table_insert (seen_pkgs, iter->data, (gpointer) 0x1);
        }
    }
    g_hash_table_destroy (seen_pkgs);

    return TRUE;
}

RCResolverInfo *
rc_resolver_info_copy (RCResolverInfo *info)
{
    RCResolverInfo *cpy;
    GSList *iter;

    if (info == NULL)
        return NULL;

    cpy = g_new0 (RCResolverInfo, 1);

    cpy->type         = info->type;
    cpy->package      = rc_package_ref (info->package);
    cpy->priority     = info->priority;
    cpy->package_list = NULL;
    cpy->msg          = g_strdup (info->msg);
    cpy->is_error     = info->is_error;
    cpy->is_important = info->is_important;

    for (iter = info->package_list; iter != NULL; iter = iter->next) {
        RCPackage *p = (RCPackage *) iter->data;

        cpy->package_list = g_slist_prepend (cpy->package_list,
                                             rc_package_ref (p));
    }
    cpy->package_list = g_slist_reverse (cpy->package_list);

    return cpy;
}

void
rc_resolver_info_free (RCResolverInfo *info)
{
    if (info) {
        rc_package_slist_unref (info->package_list);
        g_slist_free (info->package_list);
        rc_package_unref (info->package);
        g_free (info->msg);
        g_free (info);
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

char *
rc_resolver_info_to_str (RCResolverInfo *info)
{
    char *msg = NULL;
    char *pkgs = NULL;

    g_return_val_if_fail (info != NULL, NULL);
    
    switch (info->type) {

    case RC_RESOLVER_INFO_TYPE_NEEDED_BY:
        pkgs = rc_resolver_info_packages_to_str (info, FALSE);
        msg = g_strdup_printf ("needed by %s", pkgs);
        g_free (pkgs);
        break;

    case RC_RESOLVER_INFO_TYPE_CONFLICTS_WITH:
        pkgs = rc_resolver_info_packages_to_str (info, FALSE);
        msg = g_strdup_printf ("conflicts with %s", pkgs);
        g_free (pkgs);
        break;

    case RC_RESOLVER_INFO_TYPE_OBSOLETES:
        pkgs = rc_resolver_info_packages_to_str (info, FALSE);
        msg = g_strdup_printf ("replaces %s", pkgs);
        g_free (pkgs);
        break;

    case RC_RESOLVER_INFO_TYPE_DEPENDS_ON:
        pkgs = rc_resolver_info_packages_to_str (info, FALSE);
        msg = g_strdup_printf ("depended on %s", pkgs);
        g_free (pkgs);
        break;

    case RC_RESOLVER_INFO_TYPE_MISC:
        msg = g_strconcat (info->action ? "Action: " : "",
                           info->action ? info->action : "",
                           info->action ? "\n" : "",
                           info->trigger ? "Trigger: " : "",
                           info->trigger ? info->trigger : "",
                           info->trigger ? "\n" : "",
                           info->msg,
                           NULL);
        break;

    default:
        msg = g_strdup ("<unknown type>");
    }

    if (info->type != RC_RESOLVER_INFO_TYPE_MISC
        && info->package != NULL) {
        char *new_msg = g_strconcat (rc_package_spec_to_str_static (&info->package->spec),
                                     ": ",
                                     msg,
                                     NULL);
        g_free (msg);
        msg = new_msg;
    }

    return msg;
}

char *
rc_resolver_info_packages_to_str (RCResolverInfo *info,
                                  gboolean names_only)
{
    char **strv;
    char *str;
    GSList *iter;
    int i;

    g_return_val_if_fail (info != NULL, NULL);

    if (info->package_list == NULL)
        return g_strdup ("");

    strv = g_new0 (char *, g_slist_length (info->package_list) + 1);

    i = 0;
    for (iter = info->package_list; iter != NULL; iter = iter->next) {
        RCPackage *pkg = iter->data;
        strv[i] = names_only ? pkg->spec.name : rc_package_spec_to_str (&pkg->spec);
        ++i;
    }

    str = g_strjoinv (", ", strv);

    if (names_only) {
        g_free (strv);
    } else {
        g_strfreev (strv);
    }

    return str;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_resolver_info_is_about (RCResolverInfo *info,
                           RCPackage *package)
{
    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    return info->package && !strcmp (package->spec.name, info->package->spec.name);
}

gboolean
rc_resolver_info_mentions (RCResolverInfo *info,
                           RCPackage *package)
{
    GSList *iter;

    g_return_val_if_fail (info != NULL, FALSE);
    g_return_val_if_fail (package != NULL, FALSE);

    if (rc_resolver_info_is_about (info, package))
        return TRUE;

    /* Search package_list for any mention of the package. */

    iter = info->package_list;
    while (iter != NULL) {
        RCPackage *this_pkg = iter->data;
        if (this_pkg && !strcmp (package->spec.name, this_pkg->spec.name))
            return TRUE;
        iter = iter->next;
    }
    
    return FALSE;
}

void
rc_resolver_info_add_related_package (RCResolverInfo *info,
                                      RCPackage *package)
{
    g_return_if_fail (info != NULL);
    
    if (package == NULL)
        return;

    if (! rc_resolver_info_mentions (info, package)) {

        info->package_list = g_slist_prepend (info->package_list,
                                              rc_package_ref (package));
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_resolver_info_is_error (RCResolverInfo *info)
{
    g_return_val_if_fail (info != NULL, FALSE);
    return info->is_error;
}

void
rc_resolver_info_flag_as_error (RCResolverInfo *info)
{
    g_return_if_fail (info != NULL);
    info->is_error = TRUE;
}

gboolean
rc_resolver_info_is_important (RCResolverInfo *info)
{
    g_return_val_if_fail (info != NULL, FALSE);
    return info->is_important || info->is_error;
}

void
rc_resolver_info_flag_as_important (RCResolverInfo *info)
{
    g_return_if_fail (info != NULL);
    info->is_important = TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCResolverInfo *
rc_resolver_info_misc_new (RCPackage *package,
                           int priority,
                           char *msg)
{
    RCResolverInfo *info;

    g_return_val_if_fail (msg != NULL, NULL);

    info = g_new0 (RCResolverInfo, 1);

    info->type     = RC_RESOLVER_INFO_TYPE_MISC;
    info->package  = rc_package_ref (package);
    info->priority = priority;
    info->msg      = msg;

    return info;
}

void
rc_resolver_info_misc_add_action (RCResolverInfo *info,
                                  char *action_msg)
{
    g_return_if_fail (info != NULL);
    g_return_if_fail (info->type != RC_RESOLVER_INFO_TYPE_MISC);

    g_free (info->action);
    info->action = action_msg;
}

void
rc_resolver_info_misc_add_trigger (RCResolverInfo *info,
                                   char *trigger_msg)
{
    g_return_if_fail (info != NULL);
    g_return_if_fail (info->type != RC_RESOLVER_INFO_TYPE_MISC);
    
    g_free (info->trigger);
    info->trigger = trigger_msg;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCResolverInfo *
rc_resolver_info_needed_by_new (RCPackage *package)
{
    RCResolverInfo *info;

    g_return_val_if_fail (package != NULL, NULL);

    info = g_new0 (RCResolverInfo, 1);

    info->type     = RC_RESOLVER_INFO_TYPE_NEEDED_BY;
    info->package  = rc_package_ref (package);
    info->priority = RC_RESOLVER_INFO_PRIORITY_USER;

    return info;
}

void
rc_resolver_info_needed_add (RCResolverInfo *info,
                             RCPackage *needed_by)
{
    g_return_if_fail (info != NULL);
    g_return_if_fail (info->type == RC_RESOLVER_INFO_TYPE_NEEDED_BY);
    g_return_if_fail (needed_by != NULL);

    info->package_list = g_slist_prepend (info->package_list,
                                          rc_package_ref (needed_by));
}

void
rc_resolver_info_needed_add_slist (RCResolverInfo *info,
                                   GSList *slist)
{
    g_return_if_fail (info != NULL);
    g_return_if_fail (info->type == RC_RESOLVER_INFO_TYPE_NEEDED_BY);

    while (slist) {
        RCPackage *p = (RCPackage *) slist->data;

        info->package_list = g_slist_prepend (info->package_list,
                                              rc_package_ref (p));
        slist = slist->next;
    }
}

RCResolverInfo *
rc_resolver_info_conflicts_with_new (RCPackage *package,
                                     RCPackage *conflicts_with)
{
    RCResolverInfo *info;

    g_return_val_if_fail (package != NULL, NULL);
    g_return_val_if_fail (conflicts_with != NULL, NULL);

    info = g_new0 (RCResolverInfo, 1);
    
    info->type     = RC_RESOLVER_INFO_TYPE_CONFLICTS_WITH;
    info->package  = rc_package_ref (package);
    info->priority = RC_RESOLVER_INFO_PRIORITY_USER;

    info->package_list = g_slist_prepend (info->package_list,
                                          rc_package_ref (conflicts_with));

    return info;
}

RCResolverInfo *
rc_resolver_info_obsoletes_new (RCPackage *package,
                                RCPackage *obsoletes)
{
    RCResolverInfo *info;

    g_return_val_if_fail (package != NULL, NULL);
    g_return_val_if_fail (obsoletes != NULL, NULL);

    info = g_new0 (RCResolverInfo, 1);
    
    info->type     = RC_RESOLVER_INFO_TYPE_OBSOLETES;
    info->package  = rc_package_ref (package);
    info->priority = RC_RESOLVER_INFO_PRIORITY_USER;

    info->package_list = g_slist_prepend (info->package_list,
                                          rc_package_ref (obsoletes));

    return info;
}

RCResolverInfo *
rc_resolver_info_depends_on_new (RCPackage *package,
                                 RCPackage *dependency)
{
    RCResolverInfo *info;

    g_return_val_if_fail (package != NULL, NULL);
    g_return_val_if_fail (dependency != NULL, NULL);

    info = g_new0 (RCResolverInfo, 1);
    
    info->type     = RC_RESOLVER_INFO_TYPE_DEPENDS_ON;
    info->package  = rc_package_ref (package);
    info->priority = RC_RESOLVER_INFO_PRIORITY_USER;

    info->package_list = g_slist_prepend (info->package_list,
                                          rc_package_ref (dependency));

    return info;
}
