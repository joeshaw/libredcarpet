/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 * rc-dep-or.c
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

#include <glib.h>
#include <string.h>
#include "rc-dep-or.h"

#include "rc-world.h"
#include "rc-debman-general.h"

static GHashTable *or_dep_storage = NULL;

RCDepOr *
rc_dep_or_new (RCPackageDepSList *deps)
{
    RCDepOr *out_or;
    gchar *depstr = rc_dep_or_dep_slist_to_string (deps);

    if (!or_dep_storage) {
        or_dep_storage = g_hash_table_new (g_str_hash, g_str_equal);
    }

    out_or = g_hash_table_lookup (or_dep_storage, depstr);
    if (!out_or) {
        out_or = g_new0 (RCDepOr, 1);
        out_or->or_dep = depstr;
        out_or->ref = 1;
        out_or->split_ors = rc_package_dep_slist_copy (deps);
        g_hash_table_insert (or_dep_storage, depstr, out_or);
    } else {
        out_or->ref++;
        g_free (depstr);
    }

    return out_or;
}

RCDepOr *
rc_dep_or_new_by_string (gchar *depstr)
{
    RCDepOr *out_or;
    if (!or_dep_storage) {
        or_dep_storage = g_hash_table_new (g_str_hash, g_str_equal);
    }

    out_or = g_hash_table_lookup (or_dep_storage, depstr);
    if (!out_or) {
        out_or = g_new0 (RCDepOr, 1);
        out_or->or_dep = g_strdup(depstr);
        out_or->ref = 1;
        out_or->split_ors = rc_dep_string_to_or_dep_slist (depstr);
        g_hash_table_insert (or_dep_storage, out_or->or_dep, out_or);
    } else {
        out_or->ref++;
    }

    return out_or;
}

RCPackageDep *
rc_dep_or_new_provide (RCDepOr *dor)
{
    RCPackageDep *new_dep;

    new_dep = rc_package_dep_new (dor->or_dep, 0, 0, NULL, NULL,
                                  RC_RELATION_ANY, RC_CHANNEL_ANY,
                                  FALSE, TRUE);
    dor->created_provides = g_slist_prepend (dor->created_provides, new_dep);

    return new_dep;
}

RCPackageDep *
rc_dep_or_new_provide_by_string (gchar *dor_name)
{
    RCDepOr *dor;

    dor = g_hash_table_lookup (or_dep_storage, dor_name);
    if (!dor) {
        return NULL;
    }

    return rc_dep_or_new_provide (dor);
}

void
rc_dep_or_free (RCDepOr *dor)
{
    if (dor->ref) {
        dor->ref--;
    } else {
        g_warning ("attempting to free already free RCDepOr object!");
    }
}


/*
 * libesd0 (>= 0.2.20) | libesd-alsa0 (>= 0.2.20)
 * imlib1 | gdk-imlib1
 * ddd | xxgdb
 * perl5 | perl (>= 5.003)
 * exim | smail | sendmail | mail-transport-agent
 *   ->
 * (||libesd0&>=&0.2.20|libesd-alsa0&>=&0.2.20)
 */


gchar *
rc_dep_or_dep_slist_to_string (RCPackageDepSList *dep)
{
    GString *gstr;
    char *out_str;

    gstr = g_string_sized_new (50);

    g_string_append (gstr, "(||");

    while (dep) {
        RCPackageDep *pdi = (RCPackageDep *) dep->data;
        RCPackageRelation relation = rc_package_dep_get_relation (pdi);

        g_string_append (gstr, g_quark_to_string (
                             RC_PACKAGE_SPEC (pdi)->nameq));

        if (relation != RC_RELATION_ANY) {
            const gchar *rel = rc_package_relation_to_string (relation, FALSE);
            g_string_append (gstr, "&");
            g_string_append (gstr, rel);
            g_string_append (gstr, "&");

            if (RC_PACKAGE_SPEC (pdi)->epoch) {
                gchar *s = g_strdup_printf (
                    "%d:", RC_PACKAGE_SPEC (pdi)->epoch);
                g_string_append (gstr, s);
                g_free (s);
            }

            g_string_append (gstr, RC_PACKAGE_SPEC (pdi)->version);

            if (RC_PACKAGE_SPEC (pdi)->release) {
                g_string_append (gstr, "-");
                g_string_append (gstr, RC_PACKAGE_SPEC (pdi)->release);
            }
        }

        if (dep->next) {
            g_string_append (gstr, "|");
        }
        dep = dep->next;
    }

    g_string_append (gstr, ")");

    out_str = g_strdup (gstr->str);
    g_string_free (gstr, TRUE);

    return out_str;
}

RCPackageDepSList *
rc_dep_string_to_or_dep_slist (const gchar *munged)
{
    const gchar *s, *p, *zz;
    RCPackageDepSList *out_dep = NULL;
    gboolean have_more = TRUE;

    s = munged;
    if (strncmp (s, "(||", 3)) {
        g_warning ("'%s' is not a munged or string!\n", munged);
        return NULL;
    }

    s += 3;

    zz = strchr (s, ')');

    if (!zz)
        return NULL;

    /* s now points to the start of the first thing */
    do {
        RCPackageDep *cur_item;
        char *z;
        gchar *name;
        guint32 epoch;
        gchar *version = NULL;
        gchar *release = NULL;
        RCPackageRelation relation = RC_RELATION_ANY;

        /* grab the name */

        z = strchr (s, '|');
        p = strchr (s, '&');

        if (!z) {
            have_more = FALSE;
        }
        else {
            /* We don't want to get a p from a later element. */
            if (p > z)
                p = NULL;
        }

        name = g_strndup (s, p ? p - s : (z ? z - s : zz - s));

        if (p) {
            char *e;
            char op[4];
            char *vstr;

            /* We need to parse version things */
            p++;
            e = strchr (p, '&');
            if (!e) {
                /* Bad. */
                g_error ("Couldn't parse ver str");
            }

            /* text between p and e is an operator */
            strncpy (op, p, e - p);
            op[e - p] = 0;
            relation = rc_package_relation_from_string (op);

            e++;
            if (z) {
                p = z;
            } else {
                p = zz;
            }

            /* e .. p is the epoch:version-release */
            vstr = g_strndup (e, p - e);
            rc_debman_parse_version (vstr, &epoch, &version, &release);
            g_free (vstr);
        }

        cur_item = rc_package_dep_new (name, 1, epoch, version, release,
                                       relation, RC_CHANNEL_ANY,
                                       FALSE, FALSE);
        out_dep = g_slist_append (out_dep, cur_item);
        s = z + 1;

        if (p == zz)
            have_more = FALSE;
    } while (have_more);

    return out_dep;
}
