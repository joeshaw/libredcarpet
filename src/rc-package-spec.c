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

#include "rc-package-spec.h"
#include "rc-packman.h"

void
rc_package_spec_init (RCPackageSpec *spec,
                      gchar         *name,
                      guint32        epoch,
                      gchar         *version,
                      gchar         *release)
{
    g_assert (spec);

    if (spec->version) {
        g_free (spec->version);
    }

    if (spec->release) {
        g_free (spec->release);
    }

    spec->nameq = g_quark_from_string (name);
    spec->epoch = epoch;
    spec->version = g_strdup (version);
    spec->release = g_strdup (release);
} /* rc_package_spec_init */

void
rc_package_spec_copy (RCPackageSpec *dest, RCPackageSpec *src)
{
    g_assert (src);
    g_assert (dest);

    rc_package_spec_init (
        dest,
        g_quark_to_string (src->nameq),
        src->epoch,
        src->version,
        src->release);
} /* rc_package_spec_copy */

void
rc_package_spec_free_members (RCPackageSpec *spec)
{
    g_assert (spec);

    if (spec->version) {
        g_free (spec->version);
    }

    if (spec->release) {
        g_free (spec->release);
    }
} /* rc_package_spec_free_members */

extern RCPackman *das_global_packman;

gint
rc_package_spec_compare (void *a, void *b)
{
    RCPackageSpec *one, *two;

    g_assert (a);
    g_assert (b);

    one = RC_PACKAGE_SPEC (a);
    two = RC_PACKAGE_SPEC (b);

    return (rc_packman_version_compare (das_global_packman, one, two));
} /* rc_package_spec_compare */

gint
rc_package_spec_compare_name (void *a, void *b)
{
    RCPackageSpec *one, *two;

    g_assert (a);
    g_assert (b);

    one = RC_PACKAGE_SPEC (a);
    two = RC_PACKAGE_SPEC (b);

    return (strcmp (g_quark_to_string (one->nameq),
                    g_quark_to_string (two->nameq)));
} /* rc_package_spec_compare_name */

gint rc_package_spec_equal (gconstpointer a, gconstpointer b)
{
    RCPackageSpec *one = RC_PACKAGE_SPEC (a);
    RCPackageSpec *two = RC_PACKAGE_SPEC (b);

    g_assert (one);
    g_assert (two);

    if (one->epoch != two->epoch) {
        return (FALSE);
    }

    if (one->nameq != two->nameq) {
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
} /* rc_package_spec_equal */

gint rc_package_spec_not_equal (gconstpointer a, gconstpointer b)
{
    return (!rc_package_spec_equal (a, b));
} /* rc_package_spec_not_equal */

gchar *
rc_package_spec_to_str (RCPackageSpec *spec)
{
    gchar *buf;

    g_assert (spec);
    g_assert (spec->nameq);
    g_assert (spec->version);

    if (spec->epoch) {
        buf = g_strdup_printf ("%s%s%d:%s%s%s",
                               g_quark_to_string (spec->nameq),
                               spec->version,
                               spec->epoch,
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    } else {
        buf = g_strdup_printf ("%s%s%s%s%s",
                               g_quark_to_string (spec->nameq),
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

    g_assert (spec);
    g_assert (spec->version);

    if (spec->epoch) {
        buf = g_strdup_printf ("%d:%s%s%s",
                               spec->epoch,
                               spec->version,
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    } else {
        buf = g_strdup_printf ("%s%s%s",
                               spec->version,
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    }

    return buf;
}

const gchar *
rc_package_spec_to_str_static (RCPackageSpec *spec)
{
    static gchar *buf = NULL;
    if (!buf) {
        buf = g_malloc (128);
    }

    g_assert (spec);
    g_assert (spec->nameq);
    g_assert (spec->version);

    if (spec->epoch) {
        snprintf (buf, 128, "%s%s%d:%s%s%s",
                  g_quark_to_string (spec->nameq),
                  spec->version,
                  spec->epoch,
                  (spec->version ? spec->version : ""),
                  (spec->release ? "-" : ""),
                  (spec->release ? spec->release : ""));
    } else {
        snprintf (buf, 128, "%s%s%s%s%s",
                  g_quark_to_string (spec->nameq),
                  spec->version,
                  (spec->version ? spec->version : ""),
                  (spec->release ? "-" : ""),
                  (spec->release ? spec->release : ""));
    }

    return buf;
}

const gchar *
rc_package_spec_version_to_str_static (RCPackageSpec *spec)
{
    static gchar *buf = NULL;
    if (!buf) {
        buf = g_malloc (128);
    }

    g_assert (spec);
    g_assert (spec->nameq);
    g_assert (spec->version);

    if (spec->epoch) {
        snprintf (buf, 128, "%d:%s%s%s",
                  spec->epoch,
                  spec->version,
                  (spec->release ? "-" : ""),
                  (spec->release ? spec->release : ""));
    } else {
        snprintf (buf, 128, "%s%s%s",
                  spec->version,
                  (spec->release ? "-" : ""),
                  (spec->release ? spec->release : ""));
    }

    return buf;
}

guint rc_package_spec_hash (gconstpointer ptr)
{
    RCPackageSpec *spec;

    g_assert (ptr);

    spec = RC_PACKAGE_SPEC (ptr);

    return (g_str_hash (
                rc_package_spec_to_str_static (RC_PACKAGE_SPEC (ptr))));
}

static gint
spec_find_by_name (gconstpointer a, gconstpointer b)
{
    const RCPackageSpec *spec;
    GQuark nameq;

    spec = RC_PACKAGE_SPEC (a);
    nameq = GPOINTER_TO_UINT (b);

    return (nameq == spec->nameq);
}

gpointer
rc_package_spec_slist_find_name (GSList *specs, gchar *name)
{
    GSList *lnk;

    lnk = g_slist_find_custom (
        specs,
        GUINT_TO_POINTER (g_string_to_quark (name)),
        spec_find_by_name);

    if (lnk) {
        return (lnk->data);
    } else {
        return (NULL);
    }
}

