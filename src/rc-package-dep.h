/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-dep.h
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

#ifndef _RC_PACKAGE_DEP_H
#define _RC_PACKAGE_DEP_H

#include <glib.h>

#define RELATION_ANY 0
#define RELATION_EQUAL (1 << 0)
#define RELATION_LESS (1 << 1)
#define RELATION_GREATER (1 << 2)
#define RELATION_NONE (1 << 3)
#define RELATION_WEAK (1 << 4)

/* This enum is here so that gdb can give us pretty strings */

typedef enum _RCPackageRelation RCPackageRelation;

enum _RCPackageRelation {
    RC_RELATION_INVALID            = -1,
    RC_RELATION_ANY                = RELATION_ANY,
    RC_RELATION_EQUAL              = RELATION_EQUAL,
    RC_RELATION_LESS               = RELATION_LESS,
    RC_RELATION_LESS_EQUAL         = RELATION_LESS | RELATION_EQUAL,
    RC_RELATION_GREATER            = RELATION_GREATER,
    RC_RELATION_GREATER_EQUAL      = RELATION_GREATER | RELATION_EQUAL,
    RC_RELATION_NOT_EQUAL          = RELATION_LESS | RELATION_GREATER,
    RC_RELATION_NONE               = RELATION_NONE,

/* A weak relation is one that has to be satisfied only if there is
 * a strong relation in the set, or if the spec is otherwise
 * installed/to be installed. Otherwise it should be ignored. (i.e.
 * it should never add new packages)
 */
    RC_RELATION_WEAK               = RELATION_WEAK,
    RC_RELATION_WEAK_ANY           = RELATION_WEAK | RELATION_ANY,
    RC_RELATION_WEAK_EQUAL         = RELATION_WEAK | RELATION_EQUAL,
    RC_RELATION_WEAK_LESS          = RELATION_WEAK | RELATION_LESS,
    RC_RELATION_WEAK_LESS_EQUAL    = RELATION_WEAK | RELATION_LESS | RELATION_EQUAL,
    RC_RELATION_WEAK_GREATER       = RELATION_WEAK | RELATION_GREATER,
    RC_RELATION_WEAK_GREATER_EQUAL = RELATION_WEAK | RELATION_GREATER | RELATION_EQUAL,
    RC_RELATION_WEAK_NOT_EQUAL     = RELATION_WEAK | RELATION_LESS | RELATION_GREATER,
    RC_RELATION_WEAK_NONE          = RELATION_WEAK | RELATION_NONE,
};

typedef struct _RCPackageDepItem RCPackageDepItem;

typedef GSList RCPackageDep;
typedef GSList RCPackageDepSList;

/* These are included later, so as to avoid circular #include hell */

#include <gnome-xml/tree.h>

#include "rc-package-spec.h"

/* THE SPEC MUST BE FIRST */
struct _RCPackageDepItem {
    RCPackageSpec spec;
    RCPackageRelation relation;
};

typedef GSList RCPackageDepItemSList;

RCPackageDepItem *rc_package_dep_item_new (gchar *name,
                                           guint32 epoch,
                                           gchar *version,
                                           gchar *release,
                                           RCPackageRelation relation);

RCPackageDepItem *rc_package_dep_item_new_from_spec (
    RCPackageSpec *spec,
    RCPackageRelation relation);

RCPackageDepItem *rc_package_dep_item_copy (RCPackageDepItem *rcpdi);

void rc_package_dep_item_free (RCPackageDepItem *rcpdi);

gboolean rc_package_dep_item_equal (RCPackageDepItem *a, RCPackageDepItem *b);

RCPackageDep *rc_package_dep_new_with_item (RCPackageDepItem *rcpdi);

RCPackageDep *rc_package_dep_copy (RCPackageDep *orig);

void rc_package_dep_free (RCPackageDep *rcpd);

RCPackageDepSList *rc_package_dep_slist_copy (RCPackageDepSList *old);

void rc_package_dep_slist_free (RCPackageDepSList *rcpdsl);

/* Dep verification */
gboolean rc_package_dep_verify_relation (RCPackageDep *dep,
                                         RCPackageSpec *spec);
gboolean rc_package_dep_verify_and_slist_relation (RCPackageDepSList *depl,
                                                   RCPackageSpec *spec,
                                                   RCPackageDep **fail_out);
gboolean rc_package_dep_verify_and_relation (RCPackageDep *depl,
                                             RCPackageSpec *spec,
                                             RCPackageDep **fail_out,
                                             gboolean is_virtual);
gboolean rc_package_dep_item_verify_relation (RCPackageDepItem *dep, RCPackageSpec *spec);

gint rc_package_dep_is_item_subset (RCPackageDep *a, RCPackageDepItem *b);
gint rc_package_dep_item_is_subset (RCPackageDepItem *a, RCPackageDepItem *b);


RCPackageDepItem *rc_package_dep_item_invert (RCPackageDepItem *depi);
RCPackageDep *rc_package_dep_invert (RCPackageDep *dep);
void rc_package_dep_weaken (RCPackageDep *dep);
gboolean rc_package_dep_is_fully_weak (RCPackageDep *dep);
gboolean rc_package_dep_slist_is_fully_weak (RCPackageDepSList *deps);

gboolean rc_package_dep_slist_has_item (RCPackageDepSList *deps, RCPackageDepItem *di);
void rc_package_dep_system_is_rpmish (gboolean is_rpm);


RCPackageRelation rc_string_to_package_relation (const gchar *relation);
const gchar *rc_package_relation_to_string (RCPackageRelation relation,
                                            gboolean words);

xmlNode *rc_package_dep_to_xml_node (const RCPackageDep *);
RCPackageDep *rc_xml_node_to_package_dep (const xmlNode *);

#endif /* _RC_PACKAGE_DEP_H */
