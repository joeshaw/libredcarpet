/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package.h
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#define RC_PACKAGE_FIND_LEAKS

typedef struct _RCPackage RCPackage;
typedef void (*RCPackageFn) (struct _RCPackage *, gpointer);

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
#include "rc-arch.h"

typedef void     (*RCPackagePairFn) (RCPackage *, RCPackage *, gpointer);
typedef void     (*RCPackageAndSpecFn) (RCPackage *, RCPackageSpec *, gpointer);
typedef gboolean (*RCPackageAndSpecCheckFn) (RCPackage *, RCPackageSpec *, gpointer);

struct _RCPackage {
    RCPackageSpec spec;
    RCArch arch;

    RCPackageSection section;

    gint refs;

    guint32 file_size;
    guint32 installed_size;

    RCChannel *channel;

    /* Filled in by the package manager or dependency XML */
    RCPackageDepArray *requires_a;
    RCPackageDepArray *provides_a;
    RCPackageDepArray *conflicts_a;
    RCPackageDepArray *obsoletes_a;

    /* These are here to make the debian folks happy */
    RCPackageDepArray *suggests_a;
    RCPackageDepArray *recommends_a;

    /* Filled in by package info XML */
    gchar *summary;
    gchar *description;

    RCPackageUpdateSList *history;

    /* After downloading this package, fill in the local file name,
       and signature, if appropriate */
    gchar *package_filename;
    gchar *signature_filename;

    guint installed     : 1;
    guint local_package : 1;
    guint hold          : 1; /* Don't upgrade this package */
};

RCPackage *rc_package_new (void);
RCPackage *rc_package_copy (RCPackage *package);
RCPackage *rc_package_ref   (RCPackage *package);
void       rc_package_unref (RCPackage *package);

void       rc_package_spew_leaks (void);

char       *rc_package_to_str        (RCPackage *package);
const char *rc_package_to_str_static (RCPackage *package);

gboolean   rc_package_is_installed     (RCPackage *package);
RCPackage *rc_package_get_best_upgrade (RCPackage *package,
                                        gboolean subscribed_only);

RCPackageSList *rc_package_slist_ref   (RCPackageSList *packages);
void            rc_package_slist_unref (RCPackageSList *packages);
RCPackageSList *rc_package_slist_copy  (RCPackageSList *packages);

RCPackageSList *rc_package_slist_sort_by_name (RCPackageSList *packages);

RCPackageSList *rc_package_slist_sort_by_pretty_name (RCPackageSList *packages);

RCPackageSList *rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht);
RCPackageSList *rc_package_hash_table_by_string_to_list (RCPackageHashTableBySpec *ht);

void             rc_package_add_update        (RCPackage *package,
                                               RCPackageUpdate *update);
RCPackageUpdate *rc_package_get_latest_update (RCPackage *package);

#endif /* _RC_PACKAGE_H */
