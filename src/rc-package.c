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

#include <stdlib.h>
#include <string.h>

#include "rc-package.h"
#include "rc-pretty-name.h"
#include "xml-util.h"

RCPackage *
rc_package_new (void)
{
    RCPackage *package = g_new0 (RCPackage, 1);

    package->section = RC_SECTION_MISC;

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

    package->section = old_package->section;

    package->installed = old_package->installed;

    package->installed_size = old_package->installed_size;

    package->channel = old_package->channel;

    package->requires = rc_package_dep_slist_copy (old_package->requires);
    package->provides = rc_package_dep_slist_copy (old_package->provides);
    package->conflicts = rc_package_dep_slist_copy (old_package->conflicts);

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

void
rc_package_free (RCPackage *package)
{
    g_return_if_fail (package);

    rc_package_spec_free_members (RC_PACKAGE_SPEC (package));

    rc_package_dep_slist_free (package->requires);
    rc_package_dep_slist_free (package->provides);
    rc_package_dep_slist_free (package->conflicts);

    rc_package_dep_slist_free (package->suggests);
    rc_package_dep_slist_free (package->recommends);

    g_free (package->summary);
    g_free (package->description);

    rc_package_update_slist_free (package->history);

    g_free (package->package_filename);
    g_free (package->signature_filename);

    g_free (package);
} /* rc_package_free */

char *
rc_package_to_str (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return g_strconcat (rc_package_spec_to_str_static (&package->spec),
                        package->channel ? "[" : NULL,
                        package->channel ? package->channel->name : "",
                        "]",
                        NULL);
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

void
rc_package_slist_free (RCPackageSList *packages)
{
    g_slist_foreach (packages, (GFunc) rc_package_free, NULL);

    g_slist_free (packages);
} /* rc_package_slist_free */

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

RCPackageUpdate *
rc_package_get_latest_update(RCPackage *package)
{
    g_return_val_if_fail (package, NULL);
    g_return_val_if_fail (package->history, NULL);

    return (RCPackageUpdate *) package->history->data;
} /* rc_package_get_latest_update */

RCPackage *
rc_xml_node_to_package (const xmlNode *node, const RCChannel *channel)
{
    RCPackage *package;
    const xmlNode *iter;

    if (g_strcasecmp (node->name, "package")) {
        return (NULL);
    }

    package = rc_package_new ();

    package->channel = channel;

    iter = node->xmlChildrenNode;

    while (iter) {
        if (!g_strcasecmp (iter->name, "name")) {
            package->spec.name = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "summary")) {
            package->summary = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "description")) {
            package->description = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "section")) {
            gchar *tmp = xml_get_content (iter);
            package->section = rc_string_to_package_section (tmp);
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "filesize")) {
            gchar *tmp = xml_get_content (iter);
            package->file_size = tmp && *tmp ? atoi (tmp) : 0;
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "installedsize")) {
            gchar *tmp = xml_get_content (iter);
            package->installed_size = tmp && *tmp ? atoi (tmp) : 0;
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "history")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                RCPackageUpdate *update;

                update = rc_xml_node_to_package_update (iter2, package);

                package->history =
                    g_slist_append (package->history, update);

                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "requires")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                package->requires =
                    g_slist_append (package->requires,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "recommends")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                package->recommends =
                    g_slist_append (package->recommends,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "suggests")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                package->suggests =
                    g_slist_append (package->suggests,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "conflicts")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                package->conflicts =
                    g_slist_append (package->conflicts,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "provides")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                package->provides =
                    g_slist_append (package->provides,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else {
            /* FIXME: do we want to bitch to the user here? */
        }

        iter = iter->next;
    }

    if (package->history && package->history->data) {

        /* If possible, we grab the version info from the most
           recent update. */

        RCPackageUpdate *update = package->history->data;

        package->spec.epoch   = update->spec.epoch;
        package->spec.version = update->spec.version;
        package->spec.release = update->spec.release;

    } else {

        /* Otherwise, try to find where the package provides itself,
           and use that version info. */
        
        RCPackageDepSList *iter3;

        for (iter3 = package->provides; iter3 != NULL; iter3 = iter3->next) {
            RCPackageDep *dep = (RCPackageDep *) iter3->data;
            
            if (dep->relation == RC_RELATION_EQUAL
                && ! strcmp (dep->spec.name, package->spec.name)) {
                package->spec.epoch   = dep->spec.epoch;
                package->spec.version = dep->spec.version;
                package->spec.release = dep->spec.release;
                break;
            }
        }
    }

    return (package);
}


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
