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
typedef gboolean (*RCPackageFn) (RCPackage *pkg, gpointer data);

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

typedef void     (*RCPackagePairFn) (RCPackage *pkg1,
                                     RCPackage *pkg2,
                                     gpointer data);
typedef gboolean (*RCPackageAndSpecFn) (RCPackage *pkg,
                                        RCPackageSpec *spec,
                                        gpointer data);

struct _RCPackage {
    RCPackageSpec spec;

    gint refs;

    RCArch arch;
    RCPackageSection section;
    
    guint32 file_size;
    guint32 installed_size;

    RCChannel *channel;

    /* Filled in by the package manager or dependency XML */
    RCPackageDepArray *requires_a;
    RCPackageDepArray *provides_a;
    RCPackageDepArray *conflicts_a;
    RCPackageDepArray *obsoletes_a;

    /* For package sets */
    RCPackageDepArray *children_a;

    /* These are here to make the debian folks happy */
    RCPackageDepArray *suggests_a;
    RCPackageDepArray *recommends_a;

    /* Filled in by package info XML */
    gchar *pretty_name;
    gchar *summary;
    gchar *description;

    RCPackageUpdateSList *history;

    /* After downloading this package, fill in the local file name,
       and signature, if appropriate */
    gchar *package_filename;
    gchar *signature_filename;

    gboolean installed;
    gboolean local_package;
    gboolean install_only; /* Only install, don't upgrade this package */
    gboolean package_set;
};

GType      rc_package_get_type (void);
RCPackage *rc_package_new (void);
RCPackage *rc_package_ref   (RCPackage *package);
void       rc_package_unref (RCPackage *package);
RCPackage *rc_package_copy  (RCPackage *package);

void       rc_package_spew_leaks (void);

char       *rc_package_to_str        (RCPackage *package);
const char *rc_package_to_str_static (RCPackage *package);

const char *rc_package_get_name (RCPackage *package);

gboolean   rc_package_is_installed     (RCPackage *package);
gboolean   rc_package_is_package_set   (RCPackage *package);
gboolean   rc_package_is_synthetic     (RCPackage *package);
gboolean   rc_package_is_install_only  (RCPackage *package);

RCPackageSList *rc_package_slist_ref   (RCPackageSList *packages);
void            rc_package_slist_unref (RCPackageSList *packages);

RCPackageSList *rc_package_slist_sort_by_name (RCPackageSList *packages);

RCPackageSList *rc_package_slist_sort_by_pretty_name (RCPackageSList *packages);

RCPackageSList *rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht);
RCPackageSList *rc_package_hash_table_by_string_to_list (RCPackageHashTableBySpec *ht);

void                  rc_package_add_update        (RCPackage *package,
                                                    RCPackageUpdate *update);
RCPackageUpdate      *rc_package_get_latest_update (RCPackage *package);
RCPackageUpdateSList *rc_package_get_updates       (RCPackage *package);

void             rc_package_add_dummy_update  (RCPackage  *package,
                                               const char *package_filename,
                                               guint32     package_size);

RCChannel       *rc_package_get_channel       (RCPackage *package);
void             rc_package_set_channel       (RCPackage *package,
					       RCChannel *channel);

const char      *rc_package_get_filename      (RCPackage  *package);
void             rc_package_set_filename      (RCPackage  *package,
					       const char *filename);

RCPackageSpec   *rc_package_get_spec          (RCPackage  *package);
RCArch           rc_package_get_arch          (RCPackage  *package);
RCPackageSection rc_package_get_section       (RCPackage  *package);
guint32          rc_package_get_file_size     (RCPackage  *package);
guint32          rc_package_get_installed_size (RCPackage *package);
gchar           *rc_package_get_summary       (RCPackage  *package);
gchar           *rc_package_get_description   (RCPackage  *package);
gchar           *rc_package_get_signature_filename (RCPackage *package);
void             rc_package_set_signature_filename (RCPackage  *package,
                                                    const char *filename);


RCPackageDepArray *rc_package_get_requires     (RCPackage *package);
RCPackageDepArray *rc_package_get_provides     (RCPackage *package);
RCPackageDepArray *rc_package_get_conflicts    (RCPackage *package);
RCPackageDepArray *rc_package_get_obsoletes    (RCPackage *package);
RCPackageDepArray *rc_package_get_children     (RCPackage *package);
RCPackageDepArray *rc_package_get_suggests     (RCPackage *package);
RCPackageDepArray *rc_package_get_recommends   (RCPackage *package);

#endif /* _RC_PACKAGE_H */
