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

#include <libxml/tree.h>

#include "rc-debug.h"
#include "rc-dep-or.h"
#include "rc-package-dep.h"
#include "rc-packman.h"
#include "xml-util.h"

/* Our pool of RCPackageDep structures.  We try to return a match from
 * here rather than allocating new ones, when possible */
static GHashTable *global_deps = NULL;

RCPackageRelation
rc_package_relation_from_string (const gchar *relation)
{
    if (!strcmp (relation, "(any)"))
        return RC_RELATION_ANY;
    else if (!strcmp (relation, "="))
        return RC_RELATION_EQUAL;
    else if (!strcmp (relation, "<"))
        return RC_RELATION_LESS;
    else if (!strcmp (relation, "<="))
        return RC_RELATION_LESS_EQUAL;
    else if (!strcmp (relation, ">"))
        return RC_RELATION_GREATER;
    else if (!strcmp (relation, ">="))
        return RC_RELATION_GREATER_EQUAL;
    else if (!strcmp (relation, "!="))
        return RC_RELATION_NOT_EQUAL;
    else if (!strcmp (relation, "!!"))
        return RC_RELATION_NONE;
    else
        return RC_RELATION_INVALID;
}

const gchar *
rc_package_relation_to_string (RCPackageRelation relation, gint words)
{
    /* Weak relations should never find their way to the user */
    if (relation & RC_RELATION_WEAK)
        relation &= ~RC_RELATION_WEAK;

    switch (relation) {
    case RC_RELATION_ANY:
        return "(any)";
    case RC_RELATION_EQUAL:
        return words == 1 ? "equal to" : "=";
    case RC_RELATION_LESS:
        return words == 1 ? "less than" : words == 2 ? "&lt" : "<";
    case RC_RELATION_LESS_EQUAL:
        return words == 1 ? "less than or equal to" :
            words == 2 ? "&lt;=" : "<=";
    case RC_RELATION_GREATER:
        return words == 1 ? "greater than" : words == 2 ? "&gt;" : ">";
    case RC_RELATION_GREATER_EQUAL:
        return words == 1 ? "greater than or equal to" :
            words == 2 ? "&gt;=" : ">=";
    case RC_RELATION_NOT_EQUAL:
        return words == 1 ? "not equal to" : "!=";
    case RC_RELATION_NONE:
        return words == 1 ? "not installed" : "!!";
    default:
        return "(invalid)";
    }
}

/* THE SPEC MUST BE FIRST */
struct _RCPackageDep {
    RCPackageSpec spec;
    gint          refs     : 20;
    gint          relation : 5;
    guint         is_or    : 1;
    guint         is_pre   : 1;
};

RCPackageDep *
rc_package_dep_ref (RCPackageDep *dep)
{
    if (dep) {
        g_assert (dep->refs > 0);
        ++dep->refs;
    }

    return dep;
}

void
rc_package_dep_unref (RCPackageDep *dep)
{

    if (dep) {
        g_assert (dep->refs > 0);
        --dep->refs;

        if (dep->refs == 0) {
            GSList *list;

            /* Remove this dep from the hash table */
            g_assert (global_deps);

            list = g_hash_table_lookup (global_deps,
                                        GINT_TO_POINTER (dep->spec.nameq));
            g_assert (list);
            list = g_slist_remove (list, dep);

            /* If there's still data in the list (ie, there are still
             * other deps with the same spec.name), we need to replace
             * it in the hash.  Otherwise, we need to pull it from the
             * hash. */
            if (list)
                g_hash_table_replace (global_deps,
                                      GINT_TO_POINTER (dep->spec.nameq), list);
            else
                g_hash_table_remove (global_deps,
                                     GINT_TO_POINTER (dep->spec.nameq));

            /* Free the dep */
            rc_package_spec_free_members (RC_PACKAGE_SPEC (dep));
            g_free (dep);
        }
    }
}

/* Actually creates a new RCPackageDep struct when we need one. */
static RCPackageDep *
dep_new (const gchar       *name,
         gboolean           has_epoch,
         guint32            epoch,
         const gchar       *version,
         const gchar       *release,
         RCPackageRelation  relation,
         gboolean           is_pre,
         gboolean           is_or)
{
    RCPackageDep *dep = g_new0 (RCPackageDep, 1);

    rc_package_spec_init (RC_PACKAGE_SPEC (dep), name, has_epoch, epoch,
                          version, release);

    dep->relation = relation;
    dep->is_pre   = is_pre;
    dep->is_or    = is_or;

    dep->refs     = 1;

    return dep;
}

/* Check to see whether two RCPackageDep structs are exactly equal.
 * This is used to decide if we need to actually allocate a new one or
 * not.  Since we looked up the dep in a hash table by name, we
 * already know the name is the same, so we don't need to check
 * that. */
static gboolean
dep_equal (RCPackageDep      *dep,
           gboolean           has_epoch,
           guint32            epoch,
           const gchar       *version,
           const gchar       *release,
           RCPackageRelation  relation,
           gboolean           is_pre,
           gboolean           is_or)
{
    if (dep->spec.has_epoch != has_epoch)
        return FALSE;
    if (dep->spec.epoch != epoch)
        return FALSE;
    if ((dep->spec.version && !version) ||
        (!dep->spec.version && version))
        return FALSE;
    if (version && strcmp (dep->spec.version, version))
        return FALSE;
    if ((dep->spec.release && !release) ||
        (!dep->spec.release && release))
        return FALSE;
    if (release && strcmp (dep->spec.release, release))
        return FALSE;
    if (dep->relation != relation)
        return FALSE;
    if (dep->is_pre != is_pre)
        return FALSE;
    if (dep->is_or != is_or)
        return FALSE;

    return TRUE;
}

RCPackageDep *
rc_package_dep_new (const gchar       *name,
                    gboolean           has_epoch,
                    guint32            epoch,
                    const gchar       *version,
                    const gchar       *release,
                    RCPackageRelation  relation,
                    gboolean           is_pre,
                    gboolean           is_or)
{
    GSList *list;

    if (!global_deps)
        global_deps = g_hash_table_new (NULL, NULL);

    list = g_hash_table_lookup (global_deps,
                                GINT_TO_POINTER (g_quark_try_string (name)));

    if (!list) {
        /* We haven't even created an RCPackageDep with the same name yet.
         * Create the dep, create a new list, and shove it into our hash
         * table. */
        RCPackageDep *dep;

        dep = dep_new (name, has_epoch, epoch, version, release, relation,
                       is_pre, is_or);
        list = g_slist_append (NULL, dep);
        g_hash_table_insert (global_deps, GINT_TO_POINTER (dep->spec.nameq),
                             list);

        return dep;
    } else {
        /* We've already created some RCPackageDeps with this name.
         * One of them might be us! */
        GSList *iter;
        RCPackageDep *dep;

        iter = list;
        while (iter) {
            dep = iter->data;
            if (dep_equal (dep, has_epoch, epoch, version, release,
                           relation, is_pre, is_or))
            {
                /* This is the exact dep we were looking for! */
                rc_package_dep_ref (dep);
                return dep;
            }
            iter = iter->next;
        }

        /* Nope, nothing is exactly like us.  Create ourselves, add us
         * to our name's list, and stuff it back into the hash
         * table. */
        dep = dep_new (name, has_epoch, epoch, version, release, relation,
                       is_pre, is_or);
        list = g_slist_prepend (list, dep);
        g_hash_table_replace (global_deps, GINT_TO_POINTER (dep->spec.nameq),
                              list);

        return dep;
    }
}

RCPackageDep *
rc_package_dep_new_from_spec (RCPackageSpec     *spec,
                              RCPackageRelation  relation,
                              gboolean           is_pre,
                              gboolean           is_or)
{
    return rc_package_dep_new (
        g_quark_to_string (spec->nameq), spec->has_epoch, spec->epoch,
        spec->version, spec->release, relation, is_pre, is_or);
}

RCPackageRelation
rc_package_dep_get_relation (RCPackageDep *dep)
{
    return dep->relation;
}

gboolean
rc_package_dep_is_or (RCPackageDep *dep)
{
    return dep->is_or;
}

gboolean
rc_package_dep_is_pre (RCPackageDep *dep)
{
    return dep->is_pre;
}

char *
rc_package_dep_to_string (RCPackageDep *dep)
{
    char *spec_str;
    char *str;

    g_return_val_if_fail (dep != NULL, NULL);

    spec_str = rc_package_spec_to_str (&dep->spec);
    str = g_strconcat (rc_package_relation_to_string (dep->relation, 0),
                       spec_str, NULL);
    g_free (spec_str);

    return str;
}

const char *
rc_package_dep_to_string_static (RCPackageDep *dep)
{
    static char *str = NULL;

    g_return_val_if_fail (dep != NULL, NULL);

    if (str)
        g_free (str);
    str = rc_package_dep_to_string (dep);

    return str;
}

RCPackageDepSList *
rc_package_dep_slist_copy (RCPackageDepSList *list)
{
    RCPackageDepSList *iter;
    RCPackageDepSList *new = NULL;

    for (iter = list; iter; iter = iter->next) {
        RCPackageDep *dep = iter->data;

        rc_package_dep_ref (dep);
        new = g_slist_prepend (new, dep);
    }

    new = g_slist_reverse (new);

    return new;
}

void
rc_package_dep_slist_free (RCPackageDepSList *list)
{
    g_slist_foreach (list, (GFunc)rc_package_dep_unref, NULL);
    g_slist_free (list);
}

RCPackageDepArray *
rc_package_dep_array_from_slist (RCPackageDepSList **list)
{
    RCPackageDepArray *array;
    RCPackageDepSList *iter;
    int i;

    array = g_new0 (RCPackageDepArray, 1);

    if (!list || !*list) {
        array->len = 0;
        array->data = NULL;

        return array;
    }

    array->len = g_slist_length (*list);
    array->data = g_new0 (RCPackageDep *, array->len);

    i = 0;
    iter = *list;

    while (iter) {
        array->data[i] = iter->data;
        iter = iter->next;
        i++;
    }
    
    g_slist_free (*list);

    *list = NULL;

    return array;
}

RCPackageDepArray *
rc_package_dep_array_copy (RCPackageDepArray *array)
{
    int i;
    RCPackageDepArray *new;

    if (!array)
        return NULL;

    new = g_new0 (RCPackageDepArray, 1);
    new->len = array->len;
    new->data = g_new0 (RCPackageDep *, array->len);

    for (i = 0; i < array->len; i++) {
        new->data[i] = array->data[i];
        rc_package_dep_ref (new->data[i]);
    }

    return new;
}

void
rc_package_dep_array_free (RCPackageDepArray *array)
{
    int i;

    if (!array)
        return;

    for (i = 0; i < array->len; i++)
        rc_package_dep_unref (array->data[i]);

    g_free (array->data);

    g_free (array);
}

gboolean
rc_package_dep_verify_relation (RCPackman    *packman,
                                RCPackageDep *dep,
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
    if (dep->spec.nameq != prov->spec.nameq) {
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
        if (rc_packman_get_capabilities(packman) &
            RC_PACKMAN_CAP_PROVIDE_ALL_VERSIONS)
        {
            return TRUE;
        } else {
            return FALSE;
        }
    }
    
    if (dep->spec.has_epoch && prov->spec.has_epoch) {
        /* HACK: This sucks, but I don't know a better way to compare 
         * elements one at a time */
        newdepspec.epoch = dep->spec.epoch;
        newdepspec.has_epoch = dep->spec.has_epoch;
        newprovspec.epoch = prov->spec.epoch;
        newprovspec.has_epoch = prov->spec.has_epoch;
        newdepspec.version = newprovspec.version = NULL;
        newdepspec.release = newprovspec.release = NULL;
        newdepspec.nameq = newprovspec.nameq = 0;
        compare_ret = rc_packman_version_compare (packman, 
                                                  &newprovspec, &newdepspec);
    } else if (prov->spec.has_epoch && prov->spec.epoch > 0 ) {
        if (rc_packman_get_capabilities(packman) &
            RC_PACKMAN_CAP_IGNORE_ABSENT_EPOCHS)
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

        if (rc_packman_get_capabilities (packman) &
            RC_PACKMAN_CAP_ALWAYS_VERIFY_RELEASE ||
            (dep->spec.release && prov->spec.release))
        {
            newdepspec.release = dep->spec.release;
            newprovspec.release = prov->spec.release;
        } else {
            newdepspec.release = newprovspec.release = NULL;
        }

        newdepspec.nameq = newprovspec.nameq = 0;
        compare_ret = rc_packman_version_compare (packman,
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
}
