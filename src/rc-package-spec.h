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

/* Make sure name is always the first element of this struct */
struct _RCPackageSpec {
    gchar *name;
    guint32 epoch;
    gchar *version;
    gchar *release;
};

#define RC_PACKAGE_SPEC(item) ((RCPackageSpec *)(item))

void rc_package_spec_init (RCPackageSpec *rcps,
                           gchar *name,
                           guint32 epoch,
                           gchar *version,
                           gchar *release);

void rc_package_spec_copy (RCPackageSpec *new, RCPackageSpec *old);

void rc_package_spec_free_members (RCPackageSpec *rcps);

gint rc_package_spec_compare_name (void *a, void *b);
gint rc_package_spec_compare (void *a, void *b);

guint rc_package_spec_hash (gconstpointer ptr);

gint rc_package_spec_not_equal (gconstpointer a, gconstpointer b);

gchar *rc_package_spec_to_str (RCPackageSpec *spec);
gchar *rc_package_spec_version_to_str (RCPackageSpec *spec);

gpointer rc_package_spec_slist_find_name (GSList *specs, gchar *name);

#ifdef __GNUC__
__inline__ static
gint rc_package_spec_equal (gconstpointer a, gconstpointer b) {
    RCPackageSpec *one = RC_PACKAGE_SPEC (a);
    RCPackageSpec *two = RC_PACKAGE_SPEC (b);

    g_assert (one);
    g_assert (two);

    if (one->epoch != two->epoch) {
        return (FALSE);
    }

    if (one->name && two->name) {
        if (strcmp (one->name, two->name)) {
            return (FALSE);
        }
    } else if (one->name || two->name) {
        return (FALSE);
    }

    if (one->version && two->version) {
        if (strcmp (one->version, two->version)) {
            return (FALSE);
        }
    } else if (one->version || two->version) {
        return (FALSE);
    }

    if (one->release && two->release) {
        if (strcmp (one->release, two->release)) {
            return (FALSE);
        }
    } else if (one->release || two->release) {
        return (FALSE);
    }

    return (TRUE);
}
#else
gint rc_package_spec_equal (gconstpointer ptra, gconstpointer ptrb);
#endif

#endif /* _RC_PACKAGE_SPEC_H */
