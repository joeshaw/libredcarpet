/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-spec.c
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

#include <glib.h>
#include <ctype.h>
#include <stdio.h>

/* #define DEBUG 50 */

#include "rc-package-spec.h"
#include "rc-packman.h"

void
rc_package_spec_init (RCPackageSpec *rcps,
                      gchar *name,
                      guint32 epoch,
                      gchar *version,
                      gchar *release)
{
    g_assert (rcps);

    rcps->name = g_strdup (name);
    rcps->epoch = epoch;
    rcps->version = g_strdup (version);
    rcps->release = g_strdup (release);
} /* rc_package_spec_init */

void
rc_package_spec_copy (RCPackageSpec *new, RCPackageSpec *old)
{
    rc_package_spec_init (new, old->name, old->epoch, old->version,
                          old->release);
}

void
rc_package_spec_free_members (RCPackageSpec *rcps)
{
    g_free (rcps->name);
    g_free (rcps->version);
    g_free (rcps->release);
} /* rc_package_spec_free_members */

gint
rc_package_spec_compare_name (void *a, void *b)
{
    RCPackageSpec *one, *two;

    one = (RCPackageSpec *) a;
    two = (RCPackageSpec *) b;

    return (strcmp (one->name, two->name));
}

gchar *
rc_package_spec_to_str (RCPackageSpec *spec)
{
    gchar *buf;
    if (spec->epoch) {
        buf = g_strdup_printf ("%s%s%d:%s%s%s",
                               (spec->name ? spec->name  : ""),
                               (spec->version ? "-" : ""),
                               spec->epoch,
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    } else {
        buf = g_strdup_printf ("%s%s%s%s%s",
                               (spec->name ? spec->name  : ""),
                               (spec->version ? "-" : ""),
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    }

    return buf;
}

gchar *
rc_package_spec_version_to_str (RCPackageSpec *spec)
{
    gchar *buf;
    if (spec->epoch) {
        buf = g_strdup_printf ("%d:%s%s%s",
                               spec->epoch,
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    } else {
        buf = g_strdup_printf ("%s%s%s",
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    }
    return buf;
}

    
guint rc_package_spec_hash (gconstpointer ptr)
{
    RCPackageSpec *spec = (RCPackageSpec *) ptr;
    guint ret;
    gchar *b = rc_package_spec_to_str (spec);
    ret = g_str_hash (b);
    g_free (b);
    return ret;
}

#ifndef __GNUC__
gint rc_package_spec_equal (gconstpointer a, gconstpointer b)
{
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
#endif

gint rc_package_spec_not_equal (gconstpointer a, gconstpointer b)
{
    return ! rc_package_spec_equal (a, b);
}

extern RCPackman *das_global_packman;

gint
rc_package_spec_compare (void *a, void *b)
{
    RCPackage *apkg = a;
    RCPackage *bpkg = b;

    return rc_packman_version_compare (das_global_packman, &apkg->spec, &bpkg->spec);
}


static gint
spec_find_by_name (gconstpointer a, gconstpointer b)
{
    const RCPackageSpec *s = (const RCPackageSpec *) a;
    const gchar *name = (const gchar *) b;

    if (s->name) {
        return strcmp (s->name, name);
    } else {
        return -1;
    }
}

gpointer
rc_package_spec_slist_find_name (GSList *specs, gchar *name)
{
    GSList *lnk;

    lnk = g_slist_find_custom (specs,
                               name,
                               spec_find_by_name);
    if (lnk)
        return lnk->data;
    else
        return NULL;
}

