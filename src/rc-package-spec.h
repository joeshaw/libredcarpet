/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-spec.h
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

#ifndef _RC_PACKAGE_SPEC_H
#define _RC_PACKAGE_SPEC_H

#include <glib.h>
#include <string.h>

typedef struct _RCPackageSpec RCPackageSpec;

struct _RCPackageSpec {
    GQuark   nameq;
    guint32  epoch;
    gchar   *version;
    gchar   *release;
};

#define RC_PACKAGE_SPEC(item) ((RCPackageSpec *)(item))

void rc_package_spec_init (RCPackageSpec *spec,
                           gchar         *name,
                           guint32        epoch,
                           gchar         *version,
                           gchar         *release);

void rc_package_spec_copy (RCPackageSpec *dest, RCPackageSpec *src);

void rc_package_spec_free_members (RCPackageSpec *spec);

/* FIXME: any use of this function is fundamentally broken and should
 * be fixed ASAP.  There can be no concept of comparing RCPackageSpec
 * objects without knowing what the backend package manager is;
 * therefore, you should use the rc_packman_version_compare function,
 * with an appropriate reference to an RCPackman object. */
gint rc_package_spec_compare (void *a, void *b);

/* Don't use this function if you can help it, since it's slow; if all
 * you want to know is if two RCPackageSpec's are the same or
 * different, use rc_package_spec_[not_]equal instead. */
gint rc_package_spec_compare_name (void *a, void *b);

gint rc_package_spec_equal (gconstpointer a, gconstpointer b);
gint rc_package_spec_not_equal (gconstpointer a, gconstpointer b);

gchar *rc_package_spec_to_str (RCPackageSpec *spec);
gchar *rc_package_spec_version_to_str (RCPackageSpec *spec);
const gchar *rc_package_spec_to_str_static (RCPackageSpec *spec);
const gchar *rc_package_spec_version_to_str_static (RCPackageSpec *spec);

guint rc_package_spec_hash (gconstpointer ptr);

gpointer rc_package_spec_slist_find_name (GSList *specs, gchar *name);

#endif /* _RC_PACKAGE_SPEC_H */
