/*
 * rc-package.h: Dealing with individual packages...
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

#ifndef _RC_PACKAGE_H
#define _RC_PACKAGE_H

#include <libredcarpet/rc-package-spec.h>
#include <libredcarpet/rc-package-dep.h>
#include <libredcarpet/rc-package-update.h>

#include <gnome-xml/tree.h>

typedef struct _RCPackage RCPackage;

struct _RCPackage {
    RCPackageSpec spec;

    gboolean already_installed;

    /* Filled in by the package manager or dependency XML */
    RCPackageDepSList *requires;
    RCPackageDepSList *provides;
    RCPackageDepSList *conflicts;

    /* These are here to make the debian folks happy */
    RCPackageDepSList *suggests;
    RCPackageDepSList *recommends;
    
    /* Filled in by package info XML */
    gchar *summary;
    gchar *description;

    RCPackageUpdateSList *history;

    /* Don't upgrade this package */
    gboolean hold;

    /* After downloading this package, fill in the local file name, and
       signature, if appropriate */
    gchar *package_filename;
    gchar *signature_filename;
};

/* Used if key is a string, i.e. name */
typedef GHashTable RCPackageHashTableByString;

/* Used if key is a spec */
typedef GHashTable RCPackageHashTableBySpec;

RCPackage *rc_package_new (void);

RCPackage *rc_package_copy (RCPackage *);

void rc_package_free (RCPackage *rcp);

typedef GSList RCPackageSList;

void rc_package_slist_free (RCPackageSList *rcpsl);

RCPackageSList *rc_package_slist_sort (RCPackageSList *rcpsl);

RCPackageImportance rc_package_get_highest_importance(RCPackage *package);

RCPackageImportance rc_package_get_highest_importance_from_current (
    RCPackage *package, RCPackageSList *system_pkgs);

gint rc_package_compare_func (gconstpointer a, gconstpointer b);

RCPackageSList *rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht);
RCPackageSList *rc_package_hash_table_by_string_to_list (RCPackageHashTableBySpec *ht);

xmlNode *rc_package_to_xml_node (RCPackage *);

RCPackage *rc_xml_node_to_package (xmlNode *);

#endif /* _RC_PACKAGE_H */
