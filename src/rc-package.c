/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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

            rc_package_dep_array_free (package->requires_a);
            rc_package_dep_array_free (package->provides_a);
            rc_package_dep_array_free (package->conflicts_a);
            rc_package_dep_array_free (package->obsoletes_a);

            rc_package_dep_array_free (package->children_a);

            rc_package_dep_array_free (package->suggests_a);
            rc_package_dep_array_free (package->recommends_a);

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

RCPackage *
rc_package_copy (RCPackage *src)
{
    RCPackage *dest;

    if (src == NULL)
        return NULL;

    dest = rc_package_new ();

    rc_package_spec_copy (&dest->spec, &src->spec);

    dest->arch           = src->arch;
    dest->section        = src->section;
    dest->file_size      = src->file_size;
    dest->installed_size = src->installed_size;
    dest->channel        = rc_channel_ref (src->channel);

    dest->requires_a   = rc_package_dep_array_copy (src->requires_a);
    dest->provides_a   = rc_package_dep_array_copy (src->provides_a);
    dest->conflicts_a  = rc_package_dep_array_copy (src->conflicts_a);
    dest->obsoletes_a  = rc_package_dep_array_copy (src->obsoletes_a);
    dest->children_a   = rc_package_dep_array_copy (src->children_a);
    dest->suggests_a   = rc_package_dep_array_copy (src->suggests_a);
    dest->recommends_a = rc_package_dep_array_copy (src->recommends_a);

    dest->pretty_name = g_strdup (src->pretty_name);
    dest->summary     = g_strdup (src->summary);
    dest->description = g_strdup (dest->description);

    dest->history = rc_package_update_slist_copy (src->history);

    dest->installed     = src->installed;
    dest->local_package = src->local_package;
    dest->hold          = src->hold;

    return dest;
}

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

    if (package->local_package)
        return FALSE;
    else
        return package->channel == NULL || package->installed;
}

gboolean
rc_package_is_package_set (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return package->children_a != NULL && package->children_a->len > 0;
}

gboolean
rc_package_is_synthetic (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return rc_package_is_package_set (package);
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

RCPackageUpdateSList *
rc_package_slist_sort_by_name (RCPackageSList *packages)
{
    return g_slist_sort (packages, (GCompareFunc) rc_package_spec_compare_name);
} /* rc_package_slist_sort_by_name */

static gint
rc_package_compare_pretty_name (void *a, void *b)
{
    RCPackage *ap, *bp;
    const char *one, *two;

    ap = (RCPackage *) a;
    bp = (RCPackage *) b;

    one = ap->pretty_name ? ap->pretty_name : g_quark_to_string (ap->spec.nameq);
    two = bp->pretty_name ? bp->pretty_name : g_quark_to_string (bp->spec.nameq);

    return strcmp (one, two);
}

RCPackageUpdateSList *
rc_package_slist_sort_by_pretty_name (RCPackageSList *packages)
{
    return g_slist_sort (packages, (GCompareFunc) rc_package_compare_pretty_name);
} /* rc_package_slist_sort_by_pretty_name */

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
