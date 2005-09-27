/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-query-packages.c
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
#include "rc-query-packages.h"

#include <stdlib.h>

static gboolean
name_match (RCQueryPart *part,
            gpointer     data)
{
    RCPackage *pkg = data;
    return rc_query_match_string_ci (
        part, g_quark_to_string (pkg->spec.nameq));
}

static gboolean
summary_match (RCQueryPart *part,
               gpointer     data)
{
    RCPackage *pkg = data;

    if (!pkg->summary)
        return FALSE;

    return rc_query_match_string_ci (part, pkg->summary);
}

static gboolean
description_match (RCQueryPart *part,
                   gpointer     data)
{
    RCPackage *pkg = data;

    if (!pkg->description)
        return FALSE;

    return rc_query_match_string_ci (part, pkg->description);
}

static gboolean
text_match (RCQueryPart *part,
            gpointer     data)
{
    RCPackage *pkg = data;

    return rc_query_match_string_ci (
        part, g_quark_to_string (pkg->spec.nameq))
        || (pkg->summary && rc_query_match_string_ci (part, pkg->summary))
        || (pkg->description && rc_query_match_string_ci (part,
                                                           pkg->description));
}

static gboolean
channel_match (RCQueryPart *part,
               gpointer     data) 
{
    RCPackage *pkg = data;

    return rc_channel_equal_id (pkg->channel, part->query_str);
}

static gboolean
installed_match (RCQueryPart *part,
                 gpointer     data)
{
    RCPackage *pkg = data;
    return rc_query_match_bool (part, rc_package_is_installed (pkg));
}

static gboolean
name_installed_check_cb (RCPackage *pkg, gpointer user_data)
{
    gboolean *installed = user_data;

    *installed = TRUE;

    return FALSE;
}

static gboolean
name_installed_match (RCQueryPart *part,
                      gpointer     data)
{
    RCPackage *pkg = data;
    gboolean name_installed = FALSE;
    const char *name;

    name = rc_package_get_name (pkg);
    rc_world_foreach_package_by_name (rc_get_world (),
                                      name,
                                      RC_CHANNEL_SYSTEM,
                                      name_installed_check_cb,
                                      &name_installed);

    return rc_query_match_bool (part, name_installed);
} /* name_installed_match */

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct InstalledCheck {
    RCPackage *pkg;
    gboolean installed;
};

static gboolean
installed_check_cb (RCPackage *sys_pkg,
                    gpointer user_data)
{
    struct InstalledCheck *check = user_data;
   
    if (check->installed)
        return FALSE;

    if (rc_package_spec_equal (RC_PACKAGE_SPEC (sys_pkg),
                               RC_PACKAGE_SPEC (check->pkg)))
        check->installed = TRUE;

    return TRUE;
}

static gboolean
package_installed_match (RCQueryPart *part,
                         gpointer     data)
{
    RCPackage *pkg = data;
    gboolean installed;


    if (rc_package_is_installed (pkg)) {
        
        installed = TRUE;

    } else {
        struct InstalledCheck check;
        const char *name;

        check.pkg = pkg;
        check.installed = FALSE;

        name = g_quark_to_string (RC_PACKAGE_SPEC (pkg)->nameq);
        rc_world_foreach_package_by_name (rc_get_world (),
                                          name,
                                          RC_CHANNEL_SYSTEM,
                                          installed_check_cb,
                                          &check);

        installed = check.installed;
    }

    return rc_query_match_bool (part, installed);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static gboolean
needs_upgrade_match (RCQueryPart *part,
                     gpointer     data)
{
    RCPackage *pkg = data;
    return rc_query_match_bool (part,
                                 rc_package_is_installed (pkg) /* must be installed */
                                 && rc_world_get_best_upgrade (rc_get_world (), pkg, TRUE) != NULL);
}

static gboolean
importance_validate (RCQueryPart *part)
{
    RCPackageImportance imp = rc_string_to_package_importance (part->query_str);

    if (imp == RC_IMPORTANCE_INVALID)
        return FALSE;

    part->data = GINT_TO_POINTER (imp);
    return TRUE;
}

static gboolean
importance_match (RCQueryPart *part, 
                  gpointer     data)
{
    RCPackage *pkg = data;
    RCPackageImportance imp;

    /* Installed packages automatically fail. */
    if (rc_package_is_installed (pkg))
        return FALSE;

    imp = ((RCPackageUpdate *) pkg->history->data)->importance;
    
    return rc_query_type_int_compare (part->type,
                                       GPOINTER_TO_INT (part->data), (gint) imp);
}


/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static RCQueryEngine query_packages_engine[] = {
    { "name",
      NULL, NULL, NULL,
      name_match },
    
    { "summary",
      NULL, NULL, NULL,
      summary_match },

    { "description",
      NULL, NULL, NULL, 
      description_match },

    { "text", /* name or summary or description */
      NULL, NULL, NULL,
      text_match },

    { "channel",
      NULL, NULL, NULL,
      channel_match },

    { "installed",  /* This is a system package, essentially. */
      rc_query_validate_bool, NULL, NULL,
      installed_match },

    { "name-installed", /* Any package by this name installed */
      rc_query_validate_bool, NULL, NULL,
      name_installed_match },

    /* This package is a system package, or appears to be the
       in-channel version of an installed package. */
    { "package-installed",  
      rc_query_validate_bool, NULL, NULL,
      package_installed_match },

    { "needs_upgrade",
      rc_query_validate_bool, NULL, NULL,
      needs_upgrade_match },

    { "importance",
      importance_validate, NULL, NULL,
      importance_match },

    { NULL, NULL, NULL, NULL, NULL }
};

struct QueryInfo {
    RCQueryPart *query_parts;
    RCPackageSList *result;
};

static gboolean
match_package_fn (RCPackage *pkg, gpointer user_data)
{
    struct QueryInfo *info = user_data;

    if (rc_query_match (info->query_parts, 
                        query_packages_engine, 
                        pkg)) {
        info->result = g_slist_prepend (info->result, pkg);
    }

    return TRUE;
}

RCPackageSList *
rc_query_packages (RCWorld *world, GSList *query)
{
    struct QueryInfo info;
    int query_len, i;
    GSList *iter;
    RCQueryPart *query_parts;

    g_return_val_if_fail (world != NULL, NULL);

    query_len = g_slist_length (query);
    query_parts = g_new0 (RCQueryPart, query_len + 1);

    i = 0;
    for (iter = query; iter; iter = iter->next)
        query_parts[i++] = *(RCQueryPart *) iter->data;
    query_parts[query_len].type = RC_QUERY_LAST;

    if (! rc_query_begin (query_parts, query_packages_engine)) {
        g_free (query_parts);
        return NULL;
    }

    info.query_parts = query_parts;
    info.result = NULL;

    rc_world_foreach_package (world,
                              RC_CHANNEL_ANY,
                              match_package_fn,
                              &info);

    rc_query_end (query_parts, query_packages_engine);

    g_free (query_parts);

    return g_slist_reverse (info.result);
}
