/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-dep.c
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

#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "rc-package-dep.h"
#include "rc-debug-misc.h"
#include "rc-packman.h"
#include "xml-util.h"
#include "rc-debug.h"
#include "rc-dep-or.h"

#include <libxml/tree.h>

#undef DEBUG

extern RCPackman *das_global_packman;

RCPackageDep *
rc_package_dep_new (const gchar *name,
                    gboolean has_epoch,
                    guint32 epoch,
                    const gchar *version,
                    const gchar *release,
                    RCPackageRelation relation)
{
    RCPackageDep *rcpd = g_new0 (RCPackageDep, 1);

    rc_package_spec_init (RC_PACKAGE_SPEC (rcpd), name, has_epoch, epoch,
                          version, release);

    rcpd->relation = relation;
    rcpd->pre      = FALSE;
    rcpd->is_or    = FALSE;

    return (rcpd);
} /* rc_package_dep_new */

RCPackageDep *
rc_package_dep_new_from_spec (RCPackageSpec *spec,
                              RCPackageRelation relation)
{
    RCPackageDep *rcpd = rc_package_dep_new (
        spec->name, spec->has_epoch, spec->epoch, spec->version,
        spec->release, relation);

    return (rcpd);
} /* rc_package_dep_new_from_spec */

static void
rc_package_dep_copy_with_dest (RCPackageDep *dest,
                               RCPackageDep *src)
{
    rc_package_spec_copy (RC_PACKAGE_SPEC (dest), RC_PACKAGE_SPEC (src));

    dest->relation = src->relation;
    dest->is_or = src->is_or;
    dest->pre = src->pre;
}

RCPackageDep *
rc_package_dep_copy (RCPackageDep *rcpd)
{
    RCPackageDep *new = g_new0 (RCPackageDep, 1); /* That sucks */

    rc_package_dep_copy_with_dest (new, rcpd);

    return (new);
} /* rc_package_dep_copy */

static void
rc_package_dep_free_members (RCPackageDep *dep)
{
    rc_package_spec_free_members (RC_PACKAGE_SPEC (dep));
}

void
rc_package_dep_free (RCPackageDep *rcpd)
{
    rc_package_dep_free_members (rcpd);

    g_free (rcpd);
} /* rc_package_dep_free */

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCPackageDepSList *
rc_package_dep_slist_copy (RCPackageDepSList *old)
{
    RCPackageDepSList *iter;
    RCPackageDepSList *new = NULL;

    for (iter = old; iter; iter = iter->next) {
        RCPackageDep *old_dep = (RCPackageDep *)(iter->data);
        RCPackageDep *dep = rc_package_dep_copy (old_dep);

        new = g_slist_append (new, dep);
    }

    return (new);
} /* rc_package_dep_slist_copy */

void
rc_package_dep_slist_free (RCPackageDepSList *rcpdsl)
{
    g_slist_foreach (rcpdsl, (GFunc) rc_package_dep_free, NULL);

    g_slist_free (rcpdsl);
} /* rc_package_dep_slist_free */

char *
rc_package_dep_to_str (RCPackageDep *dep)
{
    char *spec_str;
    char *str;

    g_return_val_if_fail (dep != NULL, NULL);

    spec_str = rc_package_spec_to_str (&dep->spec);
    str = g_strconcat (rc_package_relation_to_string (dep->relation, 0),
                       spec_str,
                       NULL);
    g_free (spec_str);

    return str;
} /* rc_package_dep_to_str */

const char *
rc_package_dep_to_str_static (RCPackageDep *dep)
{
    static char *str = NULL;

    g_return_val_if_fail (dep != NULL, NULL);

    g_free (str);
    str = rc_package_dep_to_str (dep);
    return str;
} /* rc_package_dep_to_str_static */

/* Dep verification */

gboolean
rc_package_dep_verify_relation (RCPackageDep *dep,
                                RCPackageDep *prov)
{
    RCPackageSpec newdepspec;
    RCPackageSpec newprovspec;
    gint compare_ret = 0;

    gint unweak_deprel = dep->relation & ~RC_RELATION_WEAK;
    gint unweak_provrel = prov->relation & ~RC_RELATION_WEAK;
    
    g_assert (dep);
    g_assert (prov);

    /* No dependency can be met by a different token name */
    if (strcmp(dep->spec.name, prov->spec.name)) {
        return FALSE;
    }

    /* WARNING: RC_RELATION_NONE is NOT handled */

    /* No specific version in the req, so return */
    if (unweak_deprel == RC_RELATION_ANY) {
        return TRUE;
    }

    /* No specific version in the prov.  In RPM this means it will satisfy
     * any version, but debian it will not satisfy a versioned dep */
    if (unweak_provrel == RC_RELATION_ANY) {
        if (rc_packman_get_capabilities(das_global_packman) &
            RC_PACKMAN_CAP_PROVIDE_ALL_VERSIONS)
        {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    
    /* WARNING: This is partially broken, because you cannot
     * tell the difference between an epoch of zero and no epoch */
    /* NOTE: when the code is changed, foo->spec.epoch will be an
     * existance test for the epoch and foo->spec.epoch > 0 will 
     * be a test for > 0.  Right now these are the same, but
     * please leave the code as-is. */
    
    if (dep->spec.has_epoch && prov->spec.has_epoch) {
        /* HACK: This sucks, but I don't know a better way to compare 
         * elements one at a time */
        newdepspec.epoch = dep->spec.epoch;
        newdepspec.has_epoch = dep->spec.has_epoch;
        newprovspec.epoch = prov->spec.epoch;
        newprovspec.has_epoch = prov->spec.has_epoch;
        newdepspec.version = newprovspec.version = NULL;
        newdepspec.release = newprovspec.release = NULL;
        newdepspec.name = newprovspec.name = NULL;
        compare_ret = rc_packman_version_compare (das_global_packman, 
                                                  &newprovspec, &newdepspec);
    } else if (prov->spec.has_epoch && prov->spec.epoch > 0 ) {
        if (rc_packman_get_capabilities(das_global_packman) &
            RC_PACKMAN_CAP_LEGACY_EPOCH_HANDLING)
        {
            compare_ret = 0;
        } else {
            compare_ret = 1;
        }
    } else if (dep->spec.has_epoch && dep->spec.epoch > 0 ) {
        compare_ret = -1;
    }

    if (compare_ret == 0) {
        newdepspec.epoch = newprovspec.epoch = 0;
        newdepspec.has_epoch = newprovspec.has_epoch = 0;
        newdepspec.version = dep->spec.version;
        newprovspec.version = prov->spec.version;
        if (dep->spec.release && prov->spec.release) {
            newdepspec.release = dep->spec.release;
            newprovspec.release = prov->spec.release;
        } else {
            newdepspec.release = newprovspec.release = NULL;
        }
        newdepspec.name = newprovspec.name = NULL;
        compare_ret = rc_packman_version_compare (das_global_packman,
                                                  &newprovspec, &newdepspec);
    }

    if (compare_ret < 0 && ((unweak_provrel & RC_RELATION_GREATER) ||
                            (unweak_deprel & RC_RELATION_LESS)))
    {
        return TRUE;
    } else if (compare_ret > 0 && ((unweak_provrel & RC_RELATION_LESS) ||
                                   (unweak_deprel & RC_RELATION_GREATER)))
    {
        return TRUE;
    } else if (compare_ret == 0 && (
            ((unweak_provrel & RC_RELATION_EQUAL) &&
             (unweak_deprel & RC_RELATION_EQUAL)) ||
            ((unweak_provrel & RC_RELATION_LESS) &&
             (unweak_deprel & RC_RELATION_LESS)) ||
            ((unweak_provrel & RC_RELATION_GREATER) &&
             (unweak_deprel & RC_RELATION_GREATER))))
    {
        return TRUE;
    }
    
    return FALSE;
} /* rc_package_dep_verify_relation */

RCPackageRelation
rc_string_to_package_relation (const gchar *relation)
{
    if (!strcmp (relation, "(any)")) {
        return (RC_RELATION_ANY);
    } else if (!strcmp (relation, "=")) {
        return (RC_RELATION_EQUAL);
    } else if (!strcmp (relation, "<")) {
        return (RC_RELATION_LESS);
    } else if (!strcmp (relation, "<=")) {
        return (RC_RELATION_LESS_EQUAL);
    } else if (!strcmp (relation, ">")) {
        return (RC_RELATION_GREATER);
    } else if (!strcmp (relation, ">=")) {
        return (RC_RELATION_GREATER_EQUAL);
    } else if (!strcmp (relation, "!=")) {
        return (RC_RELATION_NOT_EQUAL);
    } else if (!strcmp (relation, "!!")) {
        return (RC_RELATION_NONE);
    } else {
        return (RC_RELATION_INVALID);
    }
} /* rc_string_to_package_relation */

const gchar *
rc_package_relation_to_string (RCPackageRelation relation, gint words)
{
    if (relation & RC_RELATION_WEAK)
        /* this should never get back to the user */
        return rc_package_relation_to_string (relation & ~RC_RELATION_WEAK, words);

    switch (relation) {
    case RC_RELATION_ANY:
        return ("(any)");
        break;
    case RC_RELATION_EQUAL:
        return (words == 1 ? "equal to" : "=");
        break;
    case RC_RELATION_LESS:
        return (words == 1 ? "less than" : words == 2 ? "&lt" : "<");
        break;
    case RC_RELATION_LESS_EQUAL:
        return (words == 1 ? "less than or equal to" : words == 2 ? "&lt;=" : "<=");
        break;
    case RC_RELATION_GREATER:
        return (words == 1 ? "greater than" : words == 2 ? "&gt;" : ">");
        break;
    case RC_RELATION_GREATER_EQUAL:
        return (words == 1 ? "greater than or equal to" : words == 2 ? "&gt;=" : ">=");
        break;
    case RC_RELATION_NOT_EQUAL:
        return (words == 1 ? "not equal to" : "!=");
        break;
    case RC_RELATION_NONE:
        return (words == 1 ? "not installed" : "!!");
        break;
    default:
        return ("(invalid)");
        break;
    }
} /* rc_package_dep_slist_remove_duplicates */

RCPackageDepSList *
rc_package_dep_slist_remove_duplicates (RCPackageDepSList *deps)
{
    RCPackageDepSList *out = NULL;
    RCPackageDepSList *it;
    gchar *last_name = NULL;

    deps = g_slist_sort (deps, (GCompareFunc) rc_package_spec_compare_name);

    it = deps;
    while (it) {
        RCPackageDep *dep = (RCPackageDep *) it->data;
        if (!last_name || strcmp (last_name, dep->spec.name)) {
            last_name = dep->spec.name;
            out = g_slist_prepend (out, dep);
        }
        it = it->next;
    }

    g_slist_free (deps);
    return out;
} /* rc_package_dep_slist_remove_duplicates */

/* Consumes *list */
RCPackageDepArray *
rc_package_dep_array_from_slist (RCPackageDepSList **list)
{
    RCPackageDepArray *array;
    RCPackageDepSList *iter;
    int i;

    array = g_new0 (RCPackageDepArray, 1);
    array->len = g_slist_length (*list);
    array->data = g_new0 (RCPackageDep, array->len);

    i = 0;
    iter = *list;

    while (iter) {
        memcpy (array->data + i, iter->data, sizeof (RCPackageDep));
        iter = iter->next;
        i++;
    }
    
    g_slist_free (*list);

    *list = NULL;

    return array;
}

void
rc_package_dep_array_free (RCPackageDepArray *array)
{
    int i;

    if (!array)
        return;

    for (i = 0; i < array->len; i++)
        rc_package_dep_free_members (array->data + i);

    g_free (array->data);

    g_free (array);
}

RCPackageDepArray *
rc_package_dep_array_copy (RCPackageDepArray *old)
{
    int i;
    RCPackageDepArray *new;

    if (!old)
        return NULL;

    new = g_new0 (RCPackageDepArray, 1);
    new->len = old->len;
    new->data = g_new0 (RCPackageDep, old->len);

    for (i = 0; i < old->len; i++)
        rc_package_dep_copy_with_dest (new->data + i,
                                       old->data + i);

    return new;
}
