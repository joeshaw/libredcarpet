/*
 * rc-dep-or.h
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifndef _RC_DEP_OR_H_
#define _RC_DEP_OR_H_

#include <glib.h>

#include "rc-package.h"
#include "rc-package-dep.h"

typedef struct _RCDepOr RCDepOr;

struct _RCDepOr {
    gchar *or_dep;
    RCPackageDepSList *split_ors;
    RCPackageDepSList *created_provides;
    gint ref;
};

RCDepOr *rc_dep_or_new (RCPackageDepSList *deps);
RCDepOr *rc_dep_or_new_by_string (gchar *depstr);

RCPackageDep *rc_dep_or_new_provide_by_string (gchar *dor_name);
RCPackageDep *rc_dep_or_new_provide (RCDepOr *dor);
void rc_dep_or_free (RCDepOr *dor);

gchar *rc_dep_or_dep_slist_to_string (RCPackageDepSList *dep);
RCPackageDepSList *rc_dep_string_to_or_dep_slist (const gchar *munged);

#endif /* _RC_DEP_OR_H_ */
