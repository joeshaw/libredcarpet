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

typedef enum {
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
    /* Note that WEAK_ANY doesn't really make sense, since to get this, it would require
     * an original dep of none, which no packaging system can express as a dependency */
    RC_RELATION_WEAK_ANY           = RELATION_WEAK | RELATION_ANY,
    RC_RELATION_WEAK_EQUAL         = RELATION_WEAK | RELATION_EQUAL,
    RC_RELATION_WEAK_LESS          = RELATION_WEAK | RELATION_LESS,
    RC_RELATION_WEAK_LESS_EQUAL    = RELATION_WEAK | RELATION_LESS | RELATION_EQUAL,
    RC_RELATION_WEAK_GREATER       = RELATION_WEAK | RELATION_GREATER,
    RC_RELATION_WEAK_GREATER_EQUAL = RELATION_WEAK | RELATION_GREATER | RELATION_EQUAL,
    RC_RELATION_WEAK_NOT_EQUAL     = RELATION_WEAK | RELATION_LESS | RELATION_GREATER,
    RC_RELATION_WEAK_NONE          = RELATION_WEAK | RELATION_NONE,
} RCPackageRelation;

typedef struct _RCPackageDep RCPackageDep;

typedef GSList RCPackageDepSList;

/* These are included later, so as to avoid circular #include hell */

#include <libxml/tree.h>

#include "rc-package-spec.h"
#include "rc-package.h"

typedef void (*RCPackageAndDepFn) (RCPackage *, RCPackageDep *, gpointer);

/* THE SPEC MUST BE FIRST */
struct _RCPackageDep {
    RCPackageSpec spec;
    gint relation : 28;
    guint is_or : 1;
    guint pre   : 1;
};

RCPackageDep *rc_package_dep_new (const gchar *name,
                                  gboolean has_epoch,
                                  guint32 epoch,
                                  const gchar *version,
                                  const gchar *release,
                                  RCPackageRelation relation);

RCPackageDep *rc_package_dep_new_from_spec (
    RCPackageSpec *spec,
    RCPackageRelation relation);

RCPackageDep *rc_package_dep_copy (RCPackageDep *rcpdi);

void rc_package_dep_free (RCPackageDep *rcpdi);

RCPackageDepSList *rc_package_dep_slist_copy (RCPackageDepSList *old);

void rc_package_dep_slist_free (RCPackageDepSList *rcpdsl);

char       *rc_package_dep_to_str (RCPackageDep *dep);
const char *rc_package_dep_to_str_static (RCPackageDep *dep);

/* Dep verification */
gboolean rc_package_dep_verify_relation (RCPackageDep *dep,
                                         RCPackageDep *prov);

RCPackageRelation rc_string_to_package_relation (const gchar *relation);
const gchar *rc_package_relation_to_string (RCPackageRelation relation,
                                            gint words);
RCPackageDepSList *rc_package_dep_slist_remove_duplicates (RCPackageDepSList *deps);

#endif /* _RC_PACKAGE_DEP_H */
