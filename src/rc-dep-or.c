/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * rc-dep-or.c
 *
 * Copyright (C) 2000, 2001   Ximian, Inc.
 *
 * Authors: Vladimir Vukicevic <vladimir@ximian.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>
#include <string.h>
#include "rc-dep-or.h"

#include "rc-world.h"
#include "rc-debman-general.h"

static GHashTable *or_dep_storage = NULL;

RCDepOr *
rc_dep_or_new (RCPackageDepSList *deps)
{
    RCDepOr *out_or;
    gchar *depstr = rc_dep_or_dep_slist_to_string (deps);

    if (!or_dep_storage) {
        or_dep_storage = g_hash_table_new (g_str_hash, g_str_equal);
    }

    out_or = g_hash_table_lookup (or_dep_storage, depstr);
    if (!out_or) {
        out_or = g_new0 (RCDepOr, 1);
        out_or->or_dep = depstr;
        out_or->ref = 1;
        out_or->split_ors = rc_package_dep_slist_copy (deps);
        g_hash_table_insert (or_dep_storage, depstr, out_or);
    } else {
        out_or->ref++;
        g_free (depstr);
    }

    return out_or;
}

RCDepOr *
rc_dep_or_new_by_string (gchar *depstr)
{
    RCDepOr *out_or;
    if (!or_dep_storage) {
        or_dep_storage = g_hash_table_new (g_str_hash, g_str_equal);
    }

    out_or = g_hash_table_lookup (or_dep_storage, depstr);
    if (!out_or) {
        out_or = g_new0 (RCDepOr, 1);
        out_or->or_dep = g_strdup(depstr);
        out_or->ref = 1;
        out_or->split_ors = rc_dep_string_to_or_dep_slist (depstr);
        g_hash_table_insert (or_dep_storage, out_or->or_dep, out_or);
    } else {
        out_or->ref++;
    }

    return out_or;
}

RCPackageDep *
rc_dep_or_new_provide (RCDepOr *dor)
{
    RCPackageDep *new_dep;

    new_dep = rc_package_dep_new (dor->or_dep, 0, NULL, NULL, RC_RELATION_ANY);
    new_dep->is_or = TRUE;
    dor->created_provides = g_slist_prepend (dor->created_provides, new_dep);

    return new_dep;
}

RCPackageDep *
rc_dep_or_new_provide_by_string (gchar *dor_name)
{
    RCDepOr *dor;

    dor = g_hash_table_lookup (or_dep_storage, dor_name);
    if (!dor) {
        return NULL;
    }

    return rc_dep_or_new_provide (dor);
}

void
rc_dep_or_free (RCDepOr *dor)
{
    if (dor->ref) {
        dor->ref--;
    } else {
        g_warning ("attempting to free already free RCDepOr object!");
    }
}


/*
 * libesd0 (>= 0.2.20) | libesd-alsa0 (>= 0.2.20)
 * imlib1 | gdk-imlib1
 * ddd | xxgdb
 * perl5 | perl (>= 5.003)
 * exim | smail | sendmail | mail-transport-agent
 *   ->
 * (||libesd0&>=&0.2.20|libesd-alsa0&>=&0.2.20)
 */


gchar *
rc_dep_or_dep_slist_to_string (RCPackageDepSList *dep)
{
    GString *gstr;
    char *out_str;

    gstr = g_string_sized_new (50);

    g_string_append (gstr, "(||");

    while (dep) {
        RCPackageDep *pdi = (RCPackageDep *) dep->data;

        if (pdi->relation & RC_RELATION_WEAK) {
            g_error ("Bork bork bork! munge or dep with weak dep!");
        }

        g_string_append (gstr, pdi->spec.name);

        if (pdi->relation != RC_RELATION_ANY) {
            const gchar *rel = rc_package_relation_to_string (pdi->relation, FALSE);
            g_string_append (gstr, "&");
            g_string_append (gstr, rel);
            g_string_append (gstr, "&");

            if (pdi->spec.epoch) {
                gchar *s = g_strdup_printf ("%d:", pdi->spec.epoch);
                g_string_append (gstr, s);
                g_free (s);
            }

            g_string_append (gstr, pdi->spec.version);

            if (pdi->spec.release) {
                g_string_append (gstr, "-");
                g_string_append (gstr, pdi->spec.release);
            }
        }

        if (dep->next) {
            g_string_append (gstr, "|");
        }
        dep = dep->next;
    }

    g_string_append (gstr, ")");

    out_str = g_strdup (gstr->str);
    g_string_free (gstr, TRUE);

    return out_str;
}

RCPackageDepSList *
rc_dep_string_to_or_dep_slist (gchar *munged)
{
    gchar *s, *p, *zz;
    RCPackageDepSList *out_dep = NULL;
    gboolean have_more = TRUE;

    s = munged;
    if (strncmp (s, "(||", 3)) {
        g_warning ("'%s' is not a munged or string!\n", munged);
        return NULL;
    }

    s += 3;

    zz = strchr (s, ')');

    /* s now points to the start of the first thing */
    do {
        RCPackageDep *cur_item;
        char *z;
        gchar *name;

        /* grab the name */

        cur_item = g_new0 (RCPackageDep, 1);

        z = strchr (s, '|');
        p = strchr (s, '&');

        if (!z) {
            have_more = FALSE;
        }
        else {
            /* We don't want to get a p from a later element. */
            if (p > z)
                p = NULL;
        }

        name = g_strndup (s, p ? p - s : (z ? z - s : zz - s));
        cur_item->spec.name = name;

        if (p) {
            char *e;
            char op[4];
            char *vstr;

            /* We need to parse version things */
            p++;
            e = strchr (p, '&');
            if (!e) {
                /* Bad. */
                g_error ("Couldn't parse ver str");
            }

            /* text between p and e is an operator */
            strncpy (op, p, e - p);
            op[e - p] = 0;
            cur_item->relation = rc_string_to_package_relation (op);

            e++;
            if (z) {
                p = z;
            } else {
                p = zz;
            }

            /* e .. p is the epoch:version-release */
            vstr = g_strndup (e, p - e);
            rc_debman_parse_version (vstr, &cur_item->spec.epoch,
                                     &cur_item->spec.version,
                                     &cur_item->spec.release);
            g_free (vstr);
        }

        out_dep = g_slist_append (out_dep, cur_item);
        s = z + 1;

        if (p == zz)
            have_more = FALSE;
    } while (have_more);

    return out_dep;
}

/*
 * Or fun
 */

struct _RCOrFixerHelperStruct {
    RCPackageSList *system_packages;
    RCChannelSList *channels;
    GHashTable *name_provide_hash;
    RCPackageDepSList *found_ors;
};

static void
process_one_package_helper (RCPackage *pkg, RCPackageDepSList **dl)
{
    RCPackageDepSList *dep_iter;

    /* No provides, since nothing can provide an OR */

    dep_iter = pkg->requires;
    while (dep_iter) {
        RCPackageDep *dep = (RCPackageDep *) dep_iter->data;
        
        if (dep->is_or)
            *dl = g_slist_prepend (*dl, dep);
        dep_iter = dep_iter->next;
    }

    dep_iter = pkg->conflicts;
    while (dep_iter) {
        RCPackageDep *dep = (RCPackageDep *) dep_iter->data;
        
        if (dep->is_or)
            *dl = g_slist_prepend (*dl, dep);
        dep_iter = dep_iter->next;
    }

    dep_iter = pkg->suggests;
    while (dep_iter) {
        RCPackageDep *dep = (RCPackageDep *) dep_iter->data;
        
        if (dep->is_or)
            *dl = g_slist_prepend (*dl, dep);
        dep_iter = dep_iter->next;
    }

    dep_iter = pkg->recommends;
    while (dep_iter) {
        RCPackageDep *dep = (RCPackageDep *) dep_iter->data;
        
        if (dep->is_or)
            *dl = g_slist_prepend (*dl, dep);
        dep_iter = dep_iter->next;
    }
}

static void
process_package_slist_helper (RCPackageSList *pkgs, RCPackageDepSList **dl)
{
    RCPackageSList *pkgs_iter;
    pkgs_iter = pkgs;
    while (pkgs_iter) {
        RCPackage *pkg = (RCPackage *) pkgs_iter->data;
        process_one_package_helper (pkg, dl);
        pkgs_iter = pkgs_iter->next;
    }
}

static void
package_process_helper_fn (RCPackage *pkg, gpointer user_data)
{
    struct _RCOrFixerHelperStruct *rof = (struct _RCOrFixerHelperStruct *) user_data;
    RCPackageDepSList *prov_iter;

    process_one_package_helper (pkg, &rof->found_ors);

    prov_iter = pkg->provides;
    while (prov_iter) {
        RCPackageDep *prov = (RCPackageDep *) prov_iter->data;
        rc_hash_table_slist_insert_unique (rof->name_provide_hash, prov->spec.name, pkg, NULL);
        prov_iter = prov_iter->next;
    }
}

gint
rc_dep_or_process_system_and_channels (RCPackageSList *system_packages,
                                       RCChannelSList *channels)
{
    RCPackageDepSList *dep_iter;
    RCPackageSList *pkg_it;
    RCChannelSList *ch_iter;

    struct _RCOrFixerHelperStruct rof = { NULL };

    if (!rof.name_provide_hash)
        rof.name_provide_hash = g_hash_table_new (g_str_hash, g_str_equal);

    process_package_slist_helper (system_packages, &rof.found_ors);

    rc_world_foreach_package (rc_get_world (),
                              RC_WORLD_ANY_CHANNEL,
                              package_process_helper_fn, 
                              &rof);

    /* At this point, we have a list of everything that's possibly an OR */
    rof.found_ors = rc_package_dep_slist_remove_duplicates (rof.found_ors);

    /* Now we need to figure out what provides each of these things */

    /* Build a helpher hash of all provides by name */
    pkg_it = system_packages;
    while (pkg_it) {
        RCPackage *pkg = (RCPackage *) pkg_it->data;
        RCPackageDepSList *prov_iter = pkg->provides;
        while (prov_iter) {
            RCPackageDep *prov = (RCPackageDep *) prov_iter->data;
            rc_hash_table_slist_insert_unique (rof.name_provide_hash, prov->spec.name, pkg, NULL);
            prov_iter = prov_iter->next;
        }
        pkg_it = pkg_it->next;
    }

    dep_iter = rof.found_ors;
    while (dep_iter) {
        RCPackageDep *dep = (RCPackageDep *) dep_iter->data;
        RCDepOr *dor;
        GSList *or_it;

        /* this will do a lookup and ref since it already exists */
        dor = rc_dep_or_new_by_string (dep->spec.name);

        or_it = dor->split_ors;
        while (or_it) {
            RCPackageDep *or_dep = (RCPackageDep *) or_it->data;
            RCPackageSList *what_provides;

            what_provides = g_hash_table_lookup (rof.name_provide_hash,
                                                 or_dep->spec.name);
            while (what_provides) {
                RCPackage *pkg = (RCPackage *) what_provides->data;
                RCPackageDepSList *provs = pkg->provides;
                gboolean pkg_ok = FALSE;

                while (provs) {
                    RCPackageDep *prov = (RCPackageDep *) provs->data;
                    if (rc_package_dep_verify_relation (or_dep, prov)) {
                        pkg_ok = TRUE;
                        break;
                    }
                    provs = provs->next;
                }
                if (pkg_ok) {
                    RCPackageDep *prov_dep = rc_dep_or_new_provide (dor);
//                    rc_debug (0, "Package %s provides %s\n", rc_package_spec_to_str (&pkg->spec), dor->or_dep);
                    pkg->provides = g_slist_prepend (pkg->provides, prov_dep);
                } else if (strstr (dor->or_dep, pkg->spec.name)) {
//                    rc_debug (0, "--- Package %s maybe should provide %s, but it doesn't!\n", rc_package_spec_to_str (&pkg->spec), dor->or_dep);
                }

                what_provides = what_provides->next;
            }

            or_it = or_it->next;
        }

        dep_iter = dep_iter->next;
    }

    rc_hash_table_slist_free (rof.name_provide_hash);

    g_hash_table_destroy (rof.name_provide_hash);

    return TRUE;
}

