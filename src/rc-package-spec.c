/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-spec.c
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

#include <glib.h>
#include <ctype.h>
#include <stdio.h>

/* #define DEBUG 50 */

#include "rc-world.h"
#include "rc-package-spec.h"
#include "rc-packman.h"

void
rc_package_spec_init (RCPackageSpec *rcps,
                      const gchar *name,
                      gboolean has_epoch,
                      guint32 epoch,
                      const gchar *version,
                      const gchar *release)
{
    g_assert (rcps);

    rcps->nameq = g_quark_from_string (name);
    rcps->has_epoch = has_epoch ? 1 : 0;
    rcps->epoch = epoch;
    rcps->version = g_strdup (version);
    rcps->release = g_strdup (release);
} /* rc_package_spec_init */

void
rc_package_spec_copy (RCPackageSpec *new, RCPackageSpec *old)
{
    rc_package_spec_init (new, g_quark_to_string (old->nameq), old->has_epoch,
                          old->epoch, old->version, old->release);
}

void
rc_package_spec_free_members (RCPackageSpec *rcps)
{
    g_free (rcps->version);
    g_free (rcps->release);
} /* rc_package_spec_free_members */

const char *
rc_package_spec_get_name (RCPackageSpec *rcps)
{
    g_return_val_if_fail (rcps != NULL, NULL);
    return g_quark_to_string (rcps->nameq);
}

gint
rc_package_spec_compare_name (void *a, void *b)
{
    RCPackageSpec *one, *two;

    one = (RCPackageSpec *) a;
    two = (RCPackageSpec *) b;

    return (strcmp (g_quark_to_string (one->nameq),
                    g_quark_to_string (two->nameq)));
}

gchar *
rc_package_spec_version_to_str (RCPackageSpec *spec)
{
    gchar epoch_buf[11];

    if (spec->has_epoch) {
        g_snprintf (epoch_buf, 11, "%d:", spec->epoch);
    } else {
        epoch_buf[0] = 0;
    }

    return (g_strdup_printf (
                "%s%s%s%s",
                epoch_buf,
                (spec->version ? spec->version : ""),
                (spec->release && *spec->release ? "-" : ""),
                (spec->release ? spec->release : "")));
}

const gchar *
rc_package_spec_version_to_str_static (RCPackageSpec *spec)
{
    static gchar *buf = NULL;

    if (buf)
        g_free (buf);

    buf = rc_package_spec_version_to_str (spec);

    return buf;
}

gchar *
rc_package_spec_to_str (RCPackageSpec *spec)
{
    const char *name_str = g_quark_to_string (spec->nameq);
    const char *vers_str = rc_package_spec_version_to_str_static (spec);
    
    if (vers_str && *vers_str)
        return g_strdup_printf ("%s-%s", name_str, vers_str);
    else
        return g_strdup_printf (name_str);
}

const gchar *
rc_package_spec_to_str_static (RCPackageSpec *spec)
{
    static gchar *buf = NULL;

    if (buf)
        g_free (buf);

    buf = rc_package_spec_to_str (spec);

    return buf;
}
    
guint rc_package_spec_hash (gconstpointer ptr)
{
    RCPackageSpec *spec = (RCPackageSpec *) ptr;
    guint ret = spec->epoch + 1;
    const char *spec_strs[3], *p;
    int i;

    spec_strs[0] = g_quark_to_string (spec->nameq);
    spec_strs[1] = spec->version;
    spec_strs[2] = spec->release;

    for (i = 0; i < 3; ++i) {
        p = spec_strs[i];
        if (p) {
            for (p += 1; *p != '\0'; ++p) {
                ret = (ret << 5) - ret + *p;
            }
        } else {
            ret = ret * 17;
        }
    }

    return ret;
}

gint rc_package_spec_equal (gconstpointer a, gconstpointer b)
{
    RCPackageSpec *one = RC_PACKAGE_SPEC (a);
    RCPackageSpec *two = RC_PACKAGE_SPEC (b);

    g_assert (one);
    g_assert (two);

    /* Why isn't there a logical XOR in C? */
    if (!((one->has_epoch && two->has_epoch) ||
          (!one->has_epoch && !two->has_epoch)))
    {
        return (FALSE);
    }

    if (one->has_epoch && (one->epoch != two->epoch)) {
        return (FALSE);
    }

    if (one->nameq != two->nameq)
        return FALSE;

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

gint rc_package_spec_not_equal (gconstpointer a, gconstpointer b)
{
    return ! rc_package_spec_equal (a, b);
}

static gint
spec_find_by_name (gconstpointer a, gconstpointer b)
{
    const RCPackageSpec *s = (const RCPackageSpec *) a;
    const gchar *name = (const gchar *) b;

    if (s->nameq) {
        return strcmp (g_quark_to_string (s->nameq), name);
    } else {
        return -1;
    }
}

gpointer
rc_package_spec_slist_find_name (GSList *specs, const gchar *name)
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
