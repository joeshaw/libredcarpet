/*
 * rc-package.c: Dealing with individual packages...
 *
 * Copyright (C) 2000 Helix Code, Inc.
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

#include "rc-package.h"

RCPackage *
rc_package_new (void)
{
    RCPackage *rcp = g_new0 (RCPackage, 1);

    return (rcp);
} /* rc_package_new */

RCPackage *
rc_package_copy (RCPackage *old_pkg)
{
    RCPackage *pkg = rc_package_copy_spec (old_pkg);

    pkg->already_installed = old_pkg->already_installed;

    pkg->requires = rc_package_dep_slist_copy (old_pkg->requires);
    pkg->provides = rc_package_dep_slist_copy (old_pkg->provides);
    pkg->conflicts = rc_package_dep_slist_copy (old_pkg->conflicts);
    pkg->suggests = rc_package_dep_slist_copy (old_pkg->suggests);
    pkg->recommends = rc_package_dep_slist_copy (old_pkg->recommends);

    pkg->summary = g_strdup (old_pkg->summary);
    pkg->description = g_strdup (old_pkg->description);

    pkg->history = rc_package_update_slist_copy (old_pkg->history);

    pkg->hold = old_pkg->hold;

    return (pkg);
}

RCPackage *
rc_package_copy_spec (RCPackage *orig)
{
    RCPackage *rcp = rc_package_new ();

    rc_package_spec_copy ((RCPackageSpec *) orig, (RCPackageSpec *) rcp);

    return (rcp);
}

void
rc_package_free (RCPackage *rcp)
{
    rc_package_spec_free_members (RC_PACKAGE_SPEC (rcp));

    rc_package_dep_slist_free (rcp->requires);
    rc_package_dep_slist_free (rcp->provides);
    rc_package_dep_slist_free (rcp->conflicts);
    rc_package_dep_slist_free (rcp->suggests);
    rc_package_dep_slist_free (rcp->recommends);

    g_free (rcp->summary);
    g_free (rcp->description);

    rc_package_update_slist_free (rcp->history);

    g_free (rcp);
} /* rc_package_free */

void
rc_package_slist_free (RCPackageSList *rcpsl)
{
    g_slist_foreach (rcpsl, (GFunc) rc_package_free, NULL);

    g_slist_free (rcpsl);
} /* rc_package_slist_free */

RCPackageUpdateSList *
rc_package_slist_sort (RCPackageSList *rcpusl)
{
    RCPackageSList *list = NULL;

    list = g_slist_sort (rcpusl, (GCompareFunc) rc_package_spec_compare_name);

    return (list);
}

RCPackageImportance 
rc_package_get_highest_importance(RCPackage *package)
{
    GSList *i;
    RCPackageImportance highest = RC_IMPORTANCE_INVALID;

    /* FIXME: This probably isn't the right value to return, but
       I can't think of anything better to return. */
    g_return_val_if_fail(package, RC_IMPORTANCE_INVALID);

    i = package->history;
    while (i) {
	RCPackageUpdate *up = i->data;

	if (up->importance == RC_IMPORTANCE_MAX)
	    highest = RC_IMPORTANCE_NECESSARY;
	else if (up->importance < highest || highest == RC_IMPORTANCE_INVALID)
	    highest = up->importance;

	i = i->next;
    }

    return highest;
} /* rc_package_get_highest_importance */

RCPackageImportance 
rc_package_get_highest_importance_from_current(RCPackage *package,
					       RCPackageSList *system_pkgs)
{
    GSList *i;
    RCPackage *system_package = NULL;
    RCPackageImportance highest = RC_IMPORTANCE_INVALID;

    /* FIXME: This probably isn't the right value to return, but
       I can't think of anything better to return. */
    g_return_val_if_fail(package, RC_IMPORTANCE_INVALID);

    i = system_pkgs;
    while (i) {
	RCPackageSpec *s = i->data;

	if (g_strcasecmp(s->name, RC_PACKAGE_SPEC(package)->name) == 0) {
	    system_package = (RCPackage *) s;
	    break;
	}

	i = i->next;
    }

    if (!system_package)
	return RC_IMPORTANCE_INVALID;

    i = package->history;
    while (i) {
	RCPackageUpdate *up = i->data;

	if (rc_package_spec_compare(
	    RC_PACKAGE_SPEC(up), RC_PACKAGE_SPEC(system_package)) > 0) {
	    if (up->importance == RC_IMPORTANCE_MAX)
		highest = RC_IMPORTANCE_NECESSARY;
	    else if (up->importance < highest || 
		     highest == RC_IMPORTANCE_INVALID)
		highest = up->importance;
	}
	
	i = i->next;
    }

    return highest;
} /* rc_package_get_highest_importance_from_current */

gint
rc_package_compare_func (gconstpointer a, gconstpointer b)
{
    RCPackage *one = (RCPackage *)(a);
    RCPackage *two = (RCPackage *)(b);

    return rc_package_spec_compare (&one->spec, &two->spec);
}

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

