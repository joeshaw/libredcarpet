/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package.h
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

#ifndef _RC_PACKAGE_H
#define _RC_PACKAGE_H

#include <glib.h>

#include <libxml/tree.h>

typedef struct _RCPackage RCPackage;

typedef GSList RCPackageSList;

/* Used if key is a string, i.e. name */
typedef GHashTable RCPackageHashTableByString;

/* Used if key is a spec */
typedef GHashTable RCPackageHashTableBySpec;

#include "rc-package-spec.h"
#include "rc-package-section.h"
#include "rc-package-dep.h"
#include "rc-package-update.h"
#include "rc-channel.h"

typedef void     (*RCPackageFn) (RCPackage *, gpointer);
typedef void     (*RCPackagePairFn) (RCPackage *, RCPackage *, gpointer);
typedef void     (*RCPackageAndSpecFn) (RCPackage *, RCPackageSpec *, gpointer);
typedef gboolean (*RCPackageAndSpecCheckFn) (RCPackage *, RCPackageSpec *, gpointer);

struct _RCPackage {
    RCPackageSpec spec;

    RCPackageSection section;

    gboolean installed;

    guint32 file_size;
    guint32 installed_size;

    const RCChannel *channel;

    /* Filled in by the package manager or dependency XML */
    RCPackageDepSList *requires;
    RCPackageDepSList *provides;
    RCPackageDepSList *conflicts;
    RCPackageDepSList *obsoletes;

    /* These are here to make the debian folks happy */
    RCPackageDepSList *suggests;
    RCPackageDepSList *recommends;

    /* Filled in by package info XML */
    gchar *summary;
    gchar *description;

    RCPackageUpdateSList *history;

    /* Don't upgrade this package */
    gboolean hold;

    /* After downloading this package, fill in the local file name,
       and signature, if appropriate */
    gchar *package_filename;
    gchar *signature_filename;
};

RCPackage *rc_package_new (void);

RCPackage *rc_package_copy (RCPackage *package);

void rc_package_free (RCPackage *package);

char       *rc_package_to_str        (RCPackage *package);
const char *rc_package_to_str_static (RCPackage *package);

gboolean rc_package_is_installed (RCPackage *package);

void rc_package_slist_free (RCPackageSList *packages);

RCPackageSList *rc_package_slist_sort_by_name (RCPackageSList *packages);
RCPackageSList *rc_package_slist_sort_by_spec (RCPackageSList *packages);
RCPackageSList *rc_package_slist_sort_by_spec_reverse (RCPackageSList *packages);
RCPackageSList *rc_package_slist_sort_by_pretty_name (RCPackageSList *packages);

RCPackageSList *rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht);
RCPackageSList *rc_package_hash_table_by_string_to_list (RCPackageHashTableBySpec *ht);

RCPackageUpdate *rc_package_get_latest_update(RCPackage *package);

GSList *rc_package_slist_find_duplicates (RCPackageSList *pkgs);
RCPackageSList *rc_package_slist_remove_older_duplicates (RCPackageSList *packages,
                                                          RCPackageSList **removed_packages);

RCPackage *rc_xml_node_to_package (const xmlNode *, const RCChannel *channel);
xmlNode   *rc_package_to_xml_node (RCPackage *);

#endif /* _RC_PACKAGE_H */
