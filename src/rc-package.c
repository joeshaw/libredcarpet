/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package.c
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <config.h>
#include "rc-package.h"

#include <stdlib.h>
#include <string.h>

#include "rc-pretty-name.h"
#include "rc-channel-private.h"
#include "rc-world.h"
#include "rc-arch.h"

#ifdef RC_PACKAGE_FIND_LEAKS
static GHashTable *leaked_packages = NULL;
#endif

RCPackage *
rc_package_new (void)
{
    RCPackage *package = g_new0 (RCPackage, 1);

    package->arch = RC_ARCH_UNKNOWN;

    package->section = RC_SECTION_MISC;
    package->refs    = 1;

#ifdef RC_PACKAGE_FIND_LEAKS
    if (leaked_packages == NULL)
        leaked_packages = g_hash_table_new (NULL, NULL);

    g_hash_table_insert (leaked_packages, package, package);
#endif

    return (package);
} /* rc_package_new */

RCPackage *
rc_package_copy (RCPackage *old_package)
{
    RCPackage *package;

    g_return_val_if_fail (old_package, NULL);

    package = rc_package_new ();

    rc_package_spec_copy (RC_PACKAGE_SPEC (package),
                          RC_PACKAGE_SPEC (old_package));

    package->arch = old_package->arch;

    package->section = old_package->section;

    package->installed = old_package->installed;

    package->installed_size = old_package->installed_size;

    package->channel = rc_channel_ref (old_package->channel);

    package->requires = rc_package_dep_slist_copy (old_package->requires);
    package->provides = rc_package_dep_slist_copy (old_package->provides);
    package->conflicts = rc_package_dep_slist_copy (old_package->conflicts);
    package->obsoletes = rc_package_dep_slist_copy (old_package->obsoletes);

    package->suggests = rc_package_dep_slist_copy (old_package->suggests);
    package->recommends = rc_package_dep_slist_copy (old_package->recommends);

    package->summary = g_strdup (old_package->summary);
    package->description = g_strdup (old_package->description);

    package->history = rc_package_update_slist_copy (old_package->history);

    package->hold = old_package->hold;

    package->package_filename = g_strdup (old_package->package_filename);
    package->signature_filename = g_strdup (old_package->signature_filename);

    return (package);
} /* rc_package_copy */

RCPackage *
rc_package_ref (RCPackage *package)
{
    if (package) {
        g_assert (package->refs > 0);
        ++package->refs;
    }

    return package;
} /* rc_package_ref */

void
rc_package_unref (RCPackage *package)
{
    if (package) {

        g_assert (package->refs > 0);
        --package->refs;

        if (package->refs == 0) {

            rc_channel_unref ((RCChannel *) package->channel);

            rc_package_update_slist_free (package->history);

            rc_package_spec_free_members (RC_PACKAGE_SPEC (package));

            rc_package_dep_slist_free (package->requires);
            rc_package_dep_slist_free (package->provides);
            rc_package_dep_slist_free (package->conflicts);
            rc_package_dep_slist_free (package->obsoletes);

            rc_package_dep_slist_free (package->suggests);
            rc_package_dep_slist_free (package->recommends);

            g_free (package->summary);
            g_free (package->description);
            
            g_free (package->package_filename);
            g_free (package->signature_filename);

#ifdef RC_PACKAGE_FIND_LEAKS
            g_assert (leaked_packages);
            g_hash_table_remove (leaked_packages, package);
#endif

            g_free (package);
        }
    }
} /* rc_package_unref */

#ifdef RC_PACKAGE_FIND_LEAKS
static void
leaked_pkg_cb (gpointer key, gpointer val, gpointer user_data)
{
    RCPackage *pkg = key;

    g_print (">!> Leaked %s (refs=%d)\n",
             rc_package_to_str_static (pkg),
             pkg->refs);
}
#endif

void
rc_package_spew_leaks (void)
{
#ifdef RC_PACKAGE_FIND_LEAKS
    if (leaked_packages) 
        g_hash_table_foreach (leaked_packages,
                              leaked_pkg_cb,
                              NULL);
#endif
}

char *
rc_package_to_str (RCPackage *package)
{
    char *str, *specstr;

    g_return_val_if_fail (package != NULL, NULL);

    specstr = rc_package_spec_to_str (&package->spec);

    str = g_strconcat (specstr,
                       package->channel ? "[" : NULL,
                       package->channel ? rc_channel_get_name (package->channel) : NULL,
                       "]",
                       NULL);

    g_free (specstr);

    return str;
}

const char *
rc_package_to_str_static (RCPackage *package)
{
    static char *str = NULL;

    g_free (str);
    str = rc_package_to_str (package);
    return str;
}

gboolean
rc_package_is_installed (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return package->channel == NULL || package->installed;
}

RCPackage *
rc_package_get_best_upgrade (RCPackage *package, gboolean subscribed_only)
{
    g_return_val_if_fail (package != NULL, NULL);

    if (package->channel == NULL || package->channel->world)
        return NULL;

    return rc_world_get_best_upgrade (package->channel->world, package,
                                      subscribed_only);
}

RCPackageSList *
rc_package_slist_ref (RCPackageSList *packages)
{
    g_slist_foreach (packages, (GFunc) rc_package_ref, NULL);

    return packages;
} /* rc_package_slist_ref */

void
rc_package_slist_unref (RCPackageSList *packages)
{
    g_slist_foreach (packages, (GFunc) rc_package_unref, NULL);
} /* rc_package_slist_unref */

RCPackageSList *
rc_package_slist_copy (RCPackageSList *packages)
{
    RCPackageSList *iter, *ret = NULL;

    for (iter = packages; iter; iter = iter->next) {
        ret = g_slist_prepend (ret,
                               rc_package_copy ((RCPackage *)(iter->data)));
    }

    ret = g_slist_reverse (ret);

    return ret;
}

RCPackageUpdateSList *
rc_package_slist_sort_by_name (RCPackageSList *packages)
{
    return (g_slist_sort (packages, (GCompareFunc) rc_package_spec_compare_name));
} /* rc_package_slist_sort_by_name */

static int pretty_name_package_strcmp (const RCPackage *a, const RCPackage *b)
{
    return g_strcasecmp (rc_pretty_name_lookup (a->spec.name), rc_pretty_name_lookup (b->spec.name));
}

RCPackageUpdateSList *
rc_package_slist_sort_by_pretty_name (RCPackageSList *packages)
{
    return (g_slist_sort (packages, (GCompareFunc) pretty_name_package_strcmp));
} /* rc_package_slist_sort_by_pretty_name */

RCPackageSList *
rc_package_slist_sort_by_spec (RCPackageSList *packages)
{
    return (g_slist_sort (packages, (GCompareFunc) rc_package_spec_compare));
} /* rc_package_slist_sort_by_spec */

static gboolean
c_sucks_rc_package_spec_compare_reverse (gpointer a, gpointer b)
{
    return rc_package_spec_compare (b, a);
}

RCPackageSList *
rc_package_slist_sort_by_spec_reverse (RCPackageSList *packages)
{
    return (g_slist_sort (packages, (GCompareFunc) c_sucks_rc_package_spec_compare_reverse));
} /* rc_package_slist_sort_by_spec_reverse */

static void
util_hash_to_list (gpointer a, gpointer b, gpointer c)
{
    GSList **l = (GSList **) c;
    *l = g_slist_append (*l, b);
}

RCPackageSList *
rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht)
{
    RCPackageSList *l = NULL;

    g_hash_table_foreach (ht, util_hash_to_list, &l);

    return l;
}

RCPackageSList *
rc_package_hash_table_by_string_to_list (RCPackageHashTableByString *ht)
{
    RCPackageSList *l = NULL;

    g_hash_table_foreach (ht, util_hash_to_list, &l);

    return l;
}

void
rc_package_add_update (RCPackage *package,
                       RCPackageUpdate *update)
{
    g_return_if_fail (package != NULL);
    g_return_if_fail (update != NULL);

    g_assert (update->package == NULL || update->package == package);
    update->package = package;

    package->history = g_slist_append (package->history, update);
}

RCPackageUpdate *
rc_package_get_latest_update(RCPackage *package)
{
    g_return_val_if_fail (package, NULL);
    if (package->history == NULL)
        return NULL;

    return (RCPackageUpdate *) package->history->data;
} /* rc_package_get_latest_update */

GSList *
rc_package_slist_find_duplicates (RCPackageSList *pkgs)
{
    RCPackageSList *sorted_slist, *iter;
    RCPackage *prev_pkg = NULL;
    GSList *out_list = NULL;
    RCPackageSList *cur_dupes = NULL;

    sorted_slist = rc_package_slist_sort_by_name (g_slist_copy (pkgs));

    iter = sorted_slist;
    while (iter) {
        RCPackage *cur_pkg = (RCPackage *) iter->data;

        if (prev_pkg) {
            if (!strcmp (prev_pkg->spec.name, cur_pkg->spec.name)) {
                if (!cur_dupes) {
                    cur_dupes = g_slist_append (cur_dupes, prev_pkg);
                }

                cur_dupes = g_slist_append (cur_dupes, cur_pkg);
            } else if (cur_dupes) {
                out_list = g_slist_append (out_list, cur_dupes);
                cur_dupes = NULL;
            }
        }

        prev_pkg = cur_pkg;
        iter = iter->next;
    }

    if (cur_dupes) {
        /* This means that the last set of packages in the list was a duplicate */
        out_list = g_slist_append (out_list, cur_dupes);
    }

    g_slist_free (sorted_slist);

    return out_list;
}

RCPackageSList *
rc_package_slist_remove_older_duplicates (RCPackageSList *packages, RCPackageSList **removed_packages)
{
    GSList *dupes;

    packages = rc_package_slist_sort_by_spec (packages);
    dupes = rc_package_slist_find_duplicates (packages);

    while (dupes) {
        RCPackageSList *cur_dupe = (RCPackageSList *) dupes->data;

        cur_dupe = g_slist_reverse (cur_dupe);

        while (cur_dupe) {
            RCPackage *dup_pkg = (RCPackage *) cur_dupe->data;
            if (cur_dupe->next) {
                packages = g_slist_remove (packages, dup_pkg);
                if (removed_packages) {
                    *removed_packages = g_slist_prepend (*removed_packages, dup_pkg);
                }
            }
            cur_dupe = cur_dupe->next;
        }

        dupes = dupes->next;
    }

    return packages;
}
