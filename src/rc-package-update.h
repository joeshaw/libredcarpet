/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-update.h
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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

#include <libxml/tree.h>

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

    gchar *license;

    /* refers to the parent package for SuSE patch RPMs */
    RCPackage *parent;
};

RCPackageUpdate *rc_package_update_new (void);

RCPackageUpdate *rc_package_update_copy (RCPackageUpdate *old_update);

void rc_package_update_free (RCPackageUpdate *update);

RCPackageSpec *rc_package_update_get_spec    (RCPackageUpdate *update);
const RCPackage     *rc_package_update_get_package (RCPackageUpdate *update);

RCPackageImportance rc_package_update_get_importance (RCPackageUpdate *update);
void                rc_package_update_set_importance (RCPackageUpdate *update,
                                                      RCPackageImportance value);

const gchar *rc_package_update_get_package_url (RCPackageUpdate *update);
void         rc_package_update_set_package_url (RCPackageUpdate *update,
                                                const gchar *value);

const gchar *rc_package_update_get_signature_url (RCPackageUpdate *update);
void         rc_package_update_set_signature_url (RCPackageUpdate *update,
                                                  const gchar *value);

const gchar *rc_package_update_get_md5sum    (RCPackageUpdate *update);
void         rc_package_update_set_md5sum    (RCPackageUpdate *update,
                                              const gchar *value);

const gchar *rc_package_update_get_description (RCPackageUpdate *update);
void         rc_package_update_set_description (RCPackageUpdate *update,
                                                const gchar *value);

const gchar *rc_package_update_get_license   (RCPackageUpdate *update);
void         rc_package_update_set_license   (RCPackageUpdate *update,
                                              const gchar *value);

guint32 rc_package_update_get_package_size   (RCPackageUpdate *update);
void    rc_package_update_set_package_size   (RCPackageUpdate *update,
                                              guint32 value);

guint32 rc_package_update_get_installed_size (RCPackageUpdate *update);
void    rc_package_update_set_installed_size (RCPackageUpdate *update,
                                              guint32 value);

guint32 rc_package_update_get_signature_size (RCPackageUpdate *update);
void    rc_package_update_set_signature_size (RCPackageUpdate *update,
                                              guint32 value);

guint   rc_package_update_get_hid            (RCPackageUpdate *update);
void    rc_package_update_set_hid            (RCPackageUpdate *update,
                                              guint value);


RCPackage *rc_package_update_get_parent      (RCPackageUpdate *update);

RCPackageUpdateSList
*rc_package_update_slist_copy (RCPackageUpdateSList *old_update);

void rc_package_update_slist_free (RCPackageUpdateSList *update_slist);

RCPackageUpdateSList
*rc_package_update_slist_sort (RCPackageUpdateSList *old_slist);

#endif /* _RC_PACKAGE_UPDATE_H */
