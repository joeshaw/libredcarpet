/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-spec.h
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

#ifndef _RC_PACKAGE_SPEC_H
#define _RC_PACKAGE_SPEC_H

#include <glib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _RCPackageSpec RCPackageSpec;

struct _RCPackageSpec {
    GQuark nameq;
    gchar *version;
    gchar *release;
    guint has_epoch : 1;
    guint epoch : 31;
};

#define RC_PACKAGE_SPEC(item) ((RCPackageSpec *)(item))

void rc_package_spec_init (RCPackageSpec *rcps,
                           const gchar *name,
                           gboolean has_epoch,
                           guint32 epoch,
                           const gchar *version,
                           const gchar *release);

void rc_package_spec_copy (RCPackageSpec *nuevo, RCPackageSpec *old);

void rc_package_spec_free_members (RCPackageSpec *rcps);

const char *rc_package_spec_get_name (RCPackageSpec *rcps);

gint rc_package_spec_compare_name (void *a, void *b);

guint rc_package_spec_hash (gconstpointer ptr);

gint rc_package_spec_not_equal (gconstpointer a, gconstpointer b);

gchar *rc_package_spec_to_str (RCPackageSpec *spec);
gchar *rc_package_spec_version_to_str (RCPackageSpec *spec);
const gchar *rc_package_spec_to_str_static (RCPackageSpec *spec);
const gchar *rc_package_spec_version_to_str_static (RCPackageSpec *spec);

gpointer rc_package_spec_slist_find_name (GSList *specs, const gchar *name);

gint rc_package_spec_equal (gconstpointer ptra, gconstpointer ptrb);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RC_PACKAGE_SPEC_H */
