/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
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

#include <glib.h>

typedef enum _RCPackageSection RCPackageSection;

typedef struct _RCPackage RCPackage;

typedef GSList RCPackageSList;

/* Used if key is a string, i.e. name */
typedef GHashTable RCPackageHashTableByString;

/* Used if key is a spec */
typedef GHashTable RCPackageHashTableBySpec;

#include <libredcarpet/rc-package-spec.h>
#include <libredcarpet/rc-package-dep.h>
#include <libredcarpet/rc-package-update.h>
#include <libredcarpet/rc-channel.h>

#include <gnome-xml/tree.h>

/*
 * RCPackageSection stuff (doesn't really deserve its own files
 */

enum _RCPackageSection {
    SECTION_OFFICE = 0,
    SECTION_IMAGING,
    SECTION_PIM,
    SECTION_GAME,
    SECTION_MULTIMEDIA,
    SECTION_INTERNET,
    SECTION_UTIL,
    SECTION_SYSTEM,
    SECTION_DOC,
    SECTION_DEVEL,
    SECTION_DEVELUTIL,
    SECTION_LIBRARY,
    SECTION_XAPP,
    SECTION_MISC,
    SECTION_LAST
};

const gchar *rc_package_section_to_string (RCPackageSection section);

RCPackageSection rc_string_to_package_section (gchar *section);

/*
 * RCPackage proper
 */

struct _RCPackage {
    RCPackageSpec spec;

    RCPackageSection section;

    gboolean installed;

    guint32 installed_size;

    const RCSubchannel *subchannel;

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

RCPackage *rc_package_new (void);

RCPackage *rc_package_copy (RCPackage *);

void rc_package_free (RCPackage *rcp);

void rc_package_slist_free (RCPackageSList *rcpsl);

RCPackageSList *rc_package_slist_sort (RCPackageSList *rcpsl);

RCPackageImportance rc_package_get_highest_importance(RCPackage *package);

RCPackageImportance rc_package_get_highest_importance_from_current (
    RCPackage *package, RCPackageSList *system_pkgs);

gint rc_package_compare_func (gconstpointer a, gconstpointer b);

RCPackageSList *rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht);
RCPackageSList *rc_package_hash_table_by_string_to_list (RCPackageHashTableBySpec *ht);

RCPackageUpdate *rc_package_get_latest_update(RCPackage *package);

xmlNode *rc_package_to_xml_node (RCPackage *);

RCPackage *rc_xml_node_to_package (const xmlNode *,
                                   const RCSubchannel *subchannel);

#endif /* _RC_PACKAGE_H */
