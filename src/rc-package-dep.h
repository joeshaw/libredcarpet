/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-dep.h
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

#ifndef _RC_PACKAGE_DEP_H
#define _RC_PACKAGE_DEP_H

#include <glib.h>

#define RELATION_ANY 0
#define RELATION_EQUAL (1 << 0)
#define RELATION_LESS (1 << 1)
#define RELATION_GREATER (1 << 2)
#define RELATION_NONE (1 << 3)

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
} RCPackageRelation;

typedef struct _RCPackageDep RCPackageDep;

typedef struct _RCPackageDepArray RCPackageDepArray;

typedef GSList RCPackageDepSList;

struct _RCPackageDepArray {
    RCPackageDep **data;
    guint len;
};

/* These are included later, so as to avoid circular #include hell */

#include <libxml/tree.h>

#include "rc-package.h"
#include "rc-package-spec.h"
#include "rc-packman.h"
#include "rc-channel.h"

typedef gboolean (*RCPackageAndDepFn) (RCPackage *pkg, 
                                       RCPackageDep *dep, 
                                       gpointer data);

RCPackageRelation
rc_package_relation_from_string (const gchar *relation);

const gchar *
rc_package_relation_to_string (RCPackageRelation relation,
                               gint              words);

RCPackageDep *
rc_package_dep_ref (RCPackageDep *dep);

void
rc_package_dep_unref (RCPackageDep *dep);

RCPackageDep *
rc_package_dep_new (const char        *name,
                    gboolean           has_epoch,
                    guint32            epoch,
                    const char        *version,
                    const char        *release,
                    RCPackageRelation  relation,
                    RCChannel         *channel,
                    gboolean           pre,
                    gboolean           is_or);

RCPackageDep *
rc_package_dep_new_from_spec (RCPackageSpec     *spec,
                              RCPackageRelation  relation,
                              RCChannel         *channel,
                              gboolean           pre,
                              gboolean           is_or);

RCPackageSpec *
rc_package_dep_get_spec (RCPackageDep *dep);

RCPackageRelation
rc_package_dep_get_relation (RCPackageDep *dep);

RCChannel *
rc_package_dep_get_channel (RCPackageDep *dep);

gboolean
rc_package_dep_is_or (RCPackageDep *dep);

gboolean
rc_package_dep_is_pre (RCPackageDep *dep);

char *
rc_package_dep_to_string (RCPackageDep *dep);

const char *
rc_package_dep_to_string_static (RCPackageDep *dep);

RCPackageSList *
rc_package_dep_slist_copy (RCPackageSList *list);

void
rc_package_dep_slist_free (RCPackageDepSList *list);

/* Consumes the list */
RCPackageDepArray *
rc_package_dep_array_from_slist (RCPackageDepSList **list);

RCPackageDepSList *
rc_package_dep_array_to_slist (RCPackageDepArray *array);

RCPackageDepArray *
rc_package_dep_array_copy (RCPackageDepArray *array);

void
rc_package_dep_array_free (RCPackageDepArray *array);

gboolean
rc_package_dep_verify_relation (RCPackman    *packman,
                                RCPackageDep *dep,
                                RCPackageDep *prov);

void
rc_package_dep_spew_cache (void);

#endif /* _RC_PACKAGE_DEP_H */
