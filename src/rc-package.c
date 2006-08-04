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

#include "rc-world.h"

#ifdef RC_PACKAGE_FIND_LEAKS
static GHashTable *leaked_packages = NULL;
#endif

GType
rc_package_get_type (void)
{
    static GType boxed_type = 0;

    if (!boxed_type) {
        boxed_type = g_boxed_type_register_static ("RCPackage",
                                        (GBoxedCopyFunc)rc_package_ref,
                                        (GBoxedFreeFunc)rc_package_unref);
    }

    return boxed_type;
}

RCPackage *
rc_package_new (void)
{
    RCPackage *package = g_new0 (RCPackage, 1);

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
            if (!getenv ("RC_DEBUG_PACKAGE_UNREF")) {
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

                g_free (package->key);
            }

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
    dest->install_only  = src->install_only;
    dest->package_set   = src->package_set;
    dest->id            = src->id;
    dest->key           = g_strdup (src->key);

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
    gboolean not_system;

    g_return_val_if_fail (package != NULL, NULL);

    specstr = rc_package_spec_to_str (&package->spec);

    not_system = package->channel && ! rc_channel_is_system (package->channel);

    str = g_strconcat (specstr,
                       not_system ? "[" : NULL,
                       not_system ? rc_channel_get_name (package->channel) : NULL,
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

const char *
rc_package_get_name (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);
    return rc_package_spec_get_name (RC_PACKAGE_SPEC (package));
}

gboolean
rc_package_is_installed (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    if (package->local_package)
        return FALSE;

    return package->channel && rc_channel_is_system (package->channel);
}

gboolean
rc_package_is_local (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return package->local_package;
}

gboolean
rc_package_is_package_set (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return package->package_set;
}

gboolean
rc_package_is_synthetic (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return rc_package_is_package_set (package);
}

gboolean
rc_package_get_install_only (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, FALSE);

    return package->install_only;
}

void
rc_package_set_install_only (RCPackage *package, gboolean val)
{
    g_return_if_fail (package != NULL);

    package->install_only = val;
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
    GSList *l;
    RCPackman *packman = rc_packman_get_global ();

    g_return_if_fail (package != NULL);
    g_return_if_fail (update != NULL);

    g_assert (update->package == NULL || update->package == package);

    if (package->history != NULL) {
        for (l = package->history; l; l = l->next) {
            int result = rc_packman_version_compare (packman, RC_PACKAGE_SPEC (update),
                                                     RC_PACKAGE_SPEC (l->data));

            if (result > 0 || (result == 0 && update->parent != NULL)) {
                package->history = g_slist_insert_before (package->history, l, update);
                break;
            } else if (l->next == NULL ||
                       (result == 0 && update->parent == NULL)) {
                package->history = g_slist_insert_before (package->history, l->next, update);
                break;
            }
        }
    } else {
        package->history = g_slist_append (package->history, update);
    }
}

RCPackageUpdate *
rc_package_get_latest_update (RCPackage *package)
{
    RCWorld *world;
    RCPackageUpdate *latest;
    GSList *l;

    g_return_val_if_fail (package, NULL);
    if (package->history == NULL) {
        return NULL;
    }

    latest = (RCPackageUpdate *) package->history->data;

    /* if the absolute latest is not a patch, just return that */
    if (latest->parent == NULL) {
        return latest;
    }

    world = rc_get_world ();

    for (l = package->history; l; l = l->next) {
        RCPackageUpdate *update = (RCPackageUpdate *) l->data;
        RCPackage *installed;
        
        if (!rc_package_spec_equal (RC_PACKAGE_SPEC (l->data),
                                    RC_PACKAGE_SPEC (latest))) {
            return NULL;
        }

        /* found a non-patch package equal to the latest, so use that */
        if (update->parent == NULL) {
            return update;
        }

        /* see if the required parent for this patch is installed */
        installed = rc_world_find_installed_version (world,
                                                     ((RCPackageUpdate *)l->data)->parent);

        if (installed != NULL &&
            rc_package_spec_equal (RC_PACKAGE_SPEC (installed),
                                   RC_PACKAGE_SPEC (update->parent)))
            return update;
    }

    /* no suitable update found */
    return NULL;
} /* rc_package_get_latest_update */

RCPackageUpdateSList *
rc_package_get_updates (RCPackage *package)
{
    g_return_val_if_fail (package, NULL);

    return package->history;
}

void
rc_package_add_dummy_update (RCPackage  *package,
                             const char *package_filename,
                             guint32     package_size)
{
    RCPackageUpdate *update;

    g_return_if_fail (package != NULL);
    g_return_if_fail (package_filename && *package_filename);

    update = rc_package_update_new ();
    
    rc_package_spec_copy (&update->spec, &package->spec);

    update->package_url  = g_strdup (package_filename);
    update->package_size = package_size;
    update->importance   = RC_IMPORTANCE_SUGGESTED;
    
    rc_package_add_update (package, update);
}

RCChannel *
rc_package_get_channel (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return package->channel;
}

void
rc_package_set_channel (RCPackage *package, RCChannel *channel)
{
    g_return_if_fail (package != NULL);

    package->channel = rc_channel_ref (channel);
}

const char *
rc_package_get_filename (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return package->package_filename;
}

void
rc_package_set_filename (RCPackage *package, const char *filename)
{
    g_return_if_fail (package != NULL);

    g_free (package->package_filename);
    package->package_filename = g_strdup (filename);
}

RCPackageSpec *
rc_package_get_spec (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return &package->spec;
}

RCPackageSection
rc_package_get_section (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, RC_SECTION_MISC);

    return package->section;
}

void
rc_package_set_section (RCPackage  *package, RCPackageSection value)
{
    g_return_if_fail (package != NULL);

    package->section = value;
}

guint32
rc_package_get_file_size (RCPackage  *package)
{
    g_return_val_if_fail (package != NULL, 0);

    return package->file_size;
}

void
rc_package_set_file_size (RCPackage  *package, guint32 value)
{
    g_return_if_fail (package != NULL);

    package->file_size = value;
}

guint32
rc_package_get_installed_size (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, 0);

    return package->installed_size;
}

void
rc_package_set_installed_size (RCPackage *package, guint32 value)
{
    g_return_if_fail (package != NULL);

    package->installed_size = value;
}

const gchar *
rc_package_get_summary (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return package->summary;
}

void
rc_package_set_summary (RCPackage  *package, const gchar *value)
{
    g_return_if_fail (package != NULL);

    g_free (package->summary);
    package->summary = g_strdup (value);
}

const gchar *
rc_package_get_description (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return package->description;
}

void
rc_package_set_description (RCPackage  *package, const gchar *value)
{
    g_return_if_fail (package != NULL);

    g_free (package->description);
    package->description = g_strdup (value);
}

const gchar *
rc_package_get_signature_filename (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return package->signature_filename;
}

void
rc_package_set_signature_filename (RCPackage *package,
                                   const char *filename)
{
    g_return_if_fail (package != NULL);

    g_free (package->signature_filename);
    package->signature_filename = g_strdup (filename);
}

RCPackageDepSList *
rc_package_get_requires (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->requires_a);
}

void
rc_package_set_requires (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->requires_a);
    package->requires_a = rc_package_dep_array_from_slist (&value);
}

RCPackageDepSList *
rc_package_get_provides (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->provides_a);
}

void
rc_package_set_provides (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->provides_a);
    package->provides_a = rc_package_dep_array_from_slist (&value);
}

RCPackageDepSList *
rc_package_get_conflicts (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->conflicts_a);
}

void
rc_package_set_conflicts (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->conflicts_a);
    package->conflicts_a = rc_package_dep_array_from_slist (&value);
}

RCPackageDepSList *
rc_package_get_obsoletes (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->obsoletes_a);
}

void
rc_package_set_obsoletes (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->obsoletes_a);
    package->obsoletes_a = rc_package_dep_array_from_slist (&value);
}

RCPackageDepSList *
rc_package_get_children (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->children_a);
}

void
rc_package_set_children (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->children_a);
    package->children_a = rc_package_dep_array_from_slist (&value);
}

RCPackageDepSList *
rc_package_get_suggests (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->suggests_a);
}

void
rc_package_set_suggests (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->suggests_a);
    package->suggests_a = rc_package_dep_array_from_slist (&value);
}

RCPackageDepSList *
rc_package_get_recommends (RCPackage *package)
{
    g_return_val_if_fail (package != NULL, NULL);

    return rc_package_dep_array_to_slist (package->recommends_a);
}

void
rc_package_set_recommends (RCPackage *package, RCPackageDepSList *value)
{
    g_return_if_fail (package != NULL);

    rc_package_dep_array_free (package->recommends_a);
    package->recommends_a = rc_package_dep_array_from_slist (&value);
}

guint64
rc_package_get_id (RCPackage *package)
{
    return package->id;
}

void
rc_package_set_id (RCPackage *package, guint64 id)
{
    package->id = id;
}

