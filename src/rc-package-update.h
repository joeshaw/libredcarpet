/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-update.h
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

#ifndef _RC_PACKAGE_UPDATE_H
#define _RC_PACKAGE_UPDATE_H

#include <glib.h>
#include <time.h>

#include <gnome-xml/tree.h>

typedef struct _RCPackageUpdate RCPackageUpdate;

typedef GSList RCPackageUpdateSList;

#include "rc-package.h"
#include "rc-package-spec.h"
#include "rc-package-importance.h"
#include "rc-util.h"

struct _RCPackageUpdate {
    RCPackageSpec spec;

    const RCPackage *package;

    gchar *package_url;
    guint32 package_size;

    guint32 installed_size;

    gchar *signature_url;
    guint32 signature_size;

    gchar *md5sum;

    RCPackageImportance importance;

    gchar *description;

    guint hid;
};

RCPackageUpdate *rc_package_update_new (void);

RCPackageUpdate *rc_package_update_copy (RCPackageUpdate *old_update);

void rc_package_update_free (RCPackageUpdate *update);

RCPackageUpdateSList
*rc_package_update_slist_copy (RCPackageUpdateSList *old_update);

void rc_package_update_slist_free (RCPackageUpdateSList *update_slist);

RCPackageUpdateSList
*rc_package_update_slist_sort (RCPackageUpdateSList *old_slist);

RCPackageUpdate *rc_xml_node_to_package_update (const xmlNode *,
                                                const RCPackage *package);

#endif /* _RC_PACKAGE_UPDATE_H */
