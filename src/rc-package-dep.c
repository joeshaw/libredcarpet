/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-dep.c
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

#include <stdio.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "rc-package-dep.h"
#include "rc-debug-misc.h"
#include "rc-packman.h"
#include "xml-util.h"
#include "rc-debug.h"
#include "rc-deps-util.h"

#include <gnome-xml/tree.h>

#undef DEBUG

#if defined(ENABLE_RPM3) || defined(ENABLE_RPM4)
gboolean rpmish = TRUE;
#else
gboolean rpmish = FALSE;
#endif

extern RCPackman *das_global_packman;

RCPackageDep *
rc_package_dep_new (gchar *name,
                    guint32 epoch,
                    gchar *version,
                    gchar *release,
                    RCPackageRelation relation)
{
    RCPackageDep *rcpd = g_new0 (RCPackageDep, 1);

    rc_package_spec_init (RC_PACKAGE_SPEC (rcpd), name, epoch, version,
                          release);

    rcpd->relation = relation;

    rcpd->pre = FALSE;

    return (rcpd);
} /* rc_package_dep_new */

RCPackageDep *
rc_package_dep_new_from_spec (RCPackageSpec *spec,
                              RCPackageRelation relation)
{
    RCPackageDep *rcpd = rc_package_dep_new (spec->name, spec->epoch,
                                             spec->version,
                                             spec->release,
                                             relation);

    return (rcpd);
} /* rc_package_dep_new_from_spec */

RCPackageDep *
rc_package_dep_copy (RCPackageDep *rcpd)
{
    RCPackageDep *new = g_new0 (RCPackageDep, 1); /* That sucks */

    rc_package_spec_copy ((RCPackageSpec *) new, (RCPackageSpec *) rcpd);

    new->relation = rcpd->relation;
    new->is_or = rcpd->is_or;
    new->pre = rcpd->pre;

    return (new);
} /* rc_package_dep_copy */

void
rc_package_dep_free (RCPackageDep *rcpd)
{
    rc_package_spec_free_members (RC_PACKAGE_SPEC (rcpd));

    g_free (rcpd);
} /* rc_package_dep_free */

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


gboolean
rc_package_dep_slist_verify_relation (RCPackageDepSList *depl,
                                      RCPackageSpec *spec,
                                      RCPackageDepSList **fail_out,
                                      gboolean is_virtual)
{
    gboolean ret = TRUE;

    while (depl) {
        RCPackageDep *d = (RCPackageDep *) depl->data;
        RCPackageRelation unweak_rel = d->relation & ~RC_RELATION_WEAK;

        /* HACK-2: debian can't have versioned deps fulfilled by virtual provides. */
        /* this should be some sort of if (debianish) { .. } */
        if (!rpmish) {
            if (is_virtual && (unweak_rel != RC_RELATION_ANY && unweak_rel != RC_RELATION_NONE)) {
                if (!strcmp (d->spec.name, spec->name)) {
                    if (fail_out && !g_slist_find (*fail_out, d))
                        *fail_out = g_slist_prepend (*fail_out, d);
                    ret = FALSE;
                }
                depl = depl->next;
                continue;
            }
        }

        if (!rc_package_dep_verify_relation (d, spec)) {
            if (fail_out && !g_slist_find (*fail_out, d))
                *fail_out = g_slist_prepend (*fail_out, d);

            ret = FALSE;
        }
        depl = depl->next;
    }

    return ret;
}

/* Note that we make the assumption that if we're in this function,
 * we're checking a dep on the same package (so this function won't
 * check whether they're the same name for an ANY relation
 */
gboolean
rc_package_dep_verify_relation (RCPackageDep *dep,
                                RCPackageSpec *spec)
{
    gboolean use_newspecspec = FALSE;
    RCPackageSpec newspecspec;
    gint compare_ret;

    gint unweak_rel = dep->relation & ~RC_RELATION_WEAK;

    g_assert (dep);
    g_assert (spec);

#if DEBUG > 10
    rc_debug (RC_DEBUG_LEVEL_DEBUG, "--> Checking: %s-%d:%s-%s (rel %d) vs. %s-%d:%s-%s: ",
             dep->spec.name, dep->spec.epoch, dep->spec.version, dep->spec.release,
             dep->relation, spec->name, spec->epoch, spec->version, spec->release);
#endif

/*
 * Some gunk arises here, since the system packages will usually be
 * non-versioned, at least on RPM. So when we're comparing, we
 * have to assume that if we don't know the version (the spec), that it
 * matches. This is kind of gross, but we can't do anything about it.
 * Same with if we're checking a dep that doesn't have a specified version
 * and release.
 *
 * This means that if a package wants "foo >= 1.2.5", and the system provides
 * "foo", that this will pass. I'm not sure if that's a desired mode of
 * operation.
 */

    if (strcmp (dep->spec.name, spec->name)) {
        if (unweak_rel == RC_RELATION_NONE) {
            return TRUE;
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS (diffname) NONE relation\n");
#endif
        } else {
            return FALSE;
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "FAIL (samename) NONE relation\n");
#endif
        }
    }

    if (dep->spec.version == NULL && dep->spec.release == NULL) {
        if (unweak_rel == RC_RELATION_NONE) {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "FAIL (nullrv) NONE relation\n");
#endif
            return FALSE;
        } else {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS (nullrv)\n");
#endif
            return TRUE;
        }
    }

    if (spec->version == NULL && spec->release == NULL)
    {
        /* If it's the same name and the relation isn't looking for a version less than blah */
        if (!(unweak_rel & RC_RELATION_LESS)) {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS (nullrv)\n");
#endif
            return TRUE;
        } else {
            return FALSE;
        }
    }

    /*
     * HACK --
     *
     * There are cases where a RPM will require an exact version = (i.e. for the -devel)
     * but will not include the release in that requirement. We ship such packages --
     * our packages always use helix as the release, but our dependencies don't
     * reflect that. So, if we're looking for an exact match, we ignore the release when
     * comparing.
     */
    if (rpmish) {
        if ((dep->spec.release == NULL || dep->spec.epoch == 0)) {
            /* it's not depending on any particular release */
            newspecspec.name = spec->name;
            newspecspec.epoch = dep->spec.epoch ? spec->epoch : 0;
            newspecspec.version = spec->version;
            newspecspec.release = dep->spec.release ? spec->release : NULL;
            
            use_newspecspec = TRUE;
        }
    }

    compare_ret = rc_packman_version_compare (das_global_packman, &dep->spec,
                                              use_newspecspec ? &newspecspec : spec);
    
    if (unweak_rel == RC_RELATION_ANY) {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS\n");
#endif
        return TRUE;
    }

    if ((unweak_rel & RC_RELATION_EQUAL) && (compare_ret == 0)) {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS\n");
#endif
        return TRUE;
    }
    /* If the realtion is greater, then we want the compare to tell us that the
     * dep is LESS than the spec
     */
    if ((unweak_rel & RC_RELATION_GREATER) && (compare_ret < 0)) {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS\n");
#endif
        return TRUE;
    }
    if ((unweak_rel & RC_RELATION_LESS) && (compare_ret > 0)) {
#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "PASS\n");
#endif
        return TRUE;
    }

#if DEBUG > 10
            rc_debug (RC_DEBUG_LEVEL_DEBUG, "FAIL\n");
#endif
    return FALSE;
}


gboolean
rc_package_dep_verify_package_relation (RCPackageDep *dep, RCPackage *pkg)
{
    RCPackageDepSList *prov_iter;

    /* if the package is the same name as the dep, then we use it to verify */
    if (strcmp (dep->spec.name, pkg->spec.name) == 0) {
        return rc_package_dep_verify_relation (dep, &pkg->spec);
    }

    /* otherwise, we hunt for a provide with the same name */
    prov_iter = pkg->provides;
    while (prov_iter) {
        RCPackageDep *prov = (RCPackageDep *) prov_iter->data;

        if (strcmp (dep->spec.name, prov->spec.name) == 0) {
            return rc_package_dep_verify_relation (dep, &prov->spec);
        }

        prov_iter = prov_iter->next;
    }

    /* nothing provided with this name, can't be true */
    return FALSE;
}

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
}

gboolean
rc_package_dep_equal (RCPackageDep *a, RCPackageDep *b)
{
    if (a->relation != b->relation)
        return FALSE;
    if (!rc_package_spec_equal (&a->spec, &b->spec))
        return FALSE;

    return TRUE;
}

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
}

/* Returns:
 *  1 if b is a more restrictive version of a
 *  0 if b is not more restrictive than a [a is more restrictive]
 * -1 if b and a must both be considered together (cannot be merged)
 */

gint
rc_package_dep_slist_is_item_subset (RCPackageDepSList *a, RCPackageDep *b)
{
    while (a) {
        RCPackageDep *ad = (RCPackageDep *) a->data;
        gint r;

        r = rc_package_dep_is_subset (ad, b);
        /* if b is more restrictive than ai, then b is more restrictive than b */
        if (r == 1) return 1;
        if (r == -1) return -1;
        a = a->next;
    }
    return 0;
}
        

/* This wrapper is here so that we can print the return code, since it can come out
 * many different places
 */
static gint rc_package_dep_is_subset_real (RCPackageDep *a, RCPackageDep *b);

gint rc_package_dep_is_subset (RCPackageDep *a, RCPackageDep *b)
{
    gint ret;
    ret = rc_package_dep_is_subset_real (a, b);
#if 0
    rc_debug (RC_DEBUG_LEVEL_DEBUG, "IS_SUBSET: A: %s B: %s  --> %d\n", debug_rc_package_dep_str (a),
             debug_rc_package_dep_str (b), ret);
#endif
    return ret;
}

static gint
rc_package_dep_is_subset_real (RCPackageDep *a, RCPackageDep *b)
{
    RCPackageRelation arel;
    RCPackageRelation brel;

    gint compare;

    g_return_val_if_fail (a, FALSE);
    g_return_val_if_fail (b, FALSE);

    arel = a->relation;
    brel = b->relation;

    /* If you have a weak and a strong, they cannot be merged */
    if ((arel ^ brel) & RC_RELATION_WEAK)
        return -1;

    /* On rpm systems, we don't bother merging two relations with
     * different epochs; the semtantics are too bizzare
     */
    if (rpmish && (a->spec.epoch != b->spec.epoch))
        return -1;

    if (brel == RC_RELATION_ANY && !(arel & RC_RELATION_NONE))
        return 0;               /* merge ANY b with a */
    if (arel == RC_RELATION_ANY && !(brel & RC_RELATION_ANY))
        return 1;              /* don't merge ANY a with !ANY b */

    compare = rc_packman_version_compare (das_global_packman, &a->spec, &b->spec);

    if (arel == brel) {
        gint rel = arel;

        if (rel == RC_RELATION_ANY)
            return 0;

        if (compare == 0)
            return 0;           /* they're exactly the same */

        if ((rel == RC_RELATION_LESS) ||
            (rel == RC_RELATION_LESS_EQUAL))
        {
            return (compare >= 0);
        }

        if ((rel == RC_RELATION_GREATER) ||
            (rel == RC_RELATION_GREATER_EQUAL))
        {
            return (compare <= 0);
        }
    }

    if (arel == brel == RC_RELATION_EQUAL)
    {
        if (compare == 0)
            return 0;       /* A can be used instead of B */
        else
            return -1;      /* both must be considered together */
    }

    
    if (arel == RC_RELATION_EQUAL)
    {
        if (compare > 0 && brel & RC_RELATION_GREATER)
            return 0;
        if (compare < 0 && brel & RC_RELATION_LESS)
            return 0;
        if (compare == 0 && brel & RC_RELATION_EQUAL)
            return 0;
    }

    if (arel == RC_RELATION_NOT_EQUAL)
    {
        if (compare == 0 && brel == RC_RELATION_GREATER)
            return 1;
        if (compare == 0 && brel == RC_RELATION_LESS)
            return 1;
        if (compare < 0 && ((brel == RC_RELATION_GREATER) || (brel == RC_RELATION_GREATER_EQUAL)))
            return 1;
        if (compare > 0 && ((brel == RC_RELATION_LESS) || (brel == RC_RELATION_LESS_EQUAL)))
            return 1;
        if (compare != 0 && brel == RC_RELATION_EQUAL)
            return 1;
    }

    if (brel == RC_RELATION_EQUAL)
    {
        if (compare == 0 && arel & RC_RELATION_EQUAL)
            return 1;
        if (compare < 0 && arel & RC_RELATION_GREATER)
            return 1;
        if (compare > 0 && arel & RC_RELATION_LESS)
            return 1;
    }

    if (brel == RC_RELATION_NOT_EQUAL)
    {
        if (compare == 0 && arel == RC_RELATION_GREATER)
            return 0;
        if (compare == 0 && arel == RC_RELATION_LESS)
            return 0;
        if (compare > 0 && ((arel == RC_RELATION_GREATER) || (arel == RC_RELATION_GREATER_EQUAL)))
            return 0;
        if (compare < 0 && ((arel == RC_RELATION_LESS) || (arel == RC_RELATION_LESS_EQUAL)))
            return 0;
        if (compare != 0 && arel == RC_RELATION_EQUAL)
            return 1;
    }

    if (((arel & RC_RELATION_LESS) && (brel & RC_RELATION_GREATER)) ||
        ((brel & RC_RELATION_LESS) && (arel & RC_RELATION_GREATER)))
    {
        /* These must be considered together */
        return -1;
    }

    /* So now we have both either greaters or both either less than's */

    if (arel == RC_RELATION_LESS_EQUAL) {
        if (brel == RC_RELATION_LESS) {
            /* a <= 1.0, b < 1.0 */
            if (compare >= 0) return 1;
            else return 0;
        } else if (brel == RC_RELATION_LESS_EQUAL) {
            /* a <= 1.0, b <= 0.5 */
            if (compare == 0) return 0;
            if (compare > 0) return 1;
            else return 0;
        }
    }

    if (arel == RC_RELATION_LESS) {
        if (brel == RC_RELATION_LESS) {
            /* a < 1.0, b < 0.5 */
            if (compare == 0) return 0;
            if (compare > 0) return 1;
            else return 0;
        } else if (brel == RC_RELATION_LESS_EQUAL) {
            /* a < 1.0, b <= 1.0 */
            if (compare > 0) return 1;
            else return 0;
        } else if (brel == RC_RELATION_NOT_EQUAL && compare == 0) {
            /* a < 1.0, b != 1.0 */
            return 0;
        }
    }

    if (arel == RC_RELATION_GREATER_EQUAL) {
        if (brel == RC_RELATION_GREATER) {
            /* a >= 1.0, b > 1.0 */
            if (compare <= 0) return 1;
            else return 0;
        } else if (brel == RC_RELATION_GREATER_EQUAL) {
            /* a >= 1.0, b >= 1.0 */
            if (compare == 0) return 0;
            else if (compare < 0) return 1;
            else return 0;
        }
    }

    if (arel == RC_RELATION_GREATER) {
        if (brel == RC_RELATION_GREATER) {
            /* a > 1.0, b > 1.0 */
            if (compare == 0) return 0;
            if (compare < 0) return 1;
            else return 0;
        } else if (brel == RC_RELATION_GREATER_EQUAL) {
            /* a > 1.0, b >= 1.0 */
            if (compare < 0) return 1;
            else return 0;
        } else if (brel == RC_RELATION_NOT_EQUAL && compare == 0) {
            /* > 1.0, != 1.0 */
            return 0;
        }
    }

    return -1;                  /* always safe -- means we won't guarantee anything about these 2 */
}


RCPackageDep *
rc_package_dep_invert (RCPackageDep *dep)
{
    RCPackageDep *out;
    RCPackageRelation out_rel;

    out = rc_package_dep_copy (dep);
    if (dep->relation == RC_RELATION_ANY) {
        out_rel = RC_RELATION_NONE;
    } else if (dep->relation == RC_RELATION_NONE) {
        out_rel = RC_RELATION_ANY;
    } else if (dep->relation == RC_RELATION_NOT_EQUAL) {
        out_rel = RC_RELATION_EQUAL;
    } else if (dep->relation == RC_RELATION_EQUAL) {
        out_rel = RC_RELATION_NOT_EQUAL;
    } else {
        if (dep->relation & RC_RELATION_EQUAL) {
            out_rel = 0;
        } else {
            out_rel = RC_RELATION_EQUAL;
        }

        if (dep->relation & RC_RELATION_LESS) {
            out_rel |= RC_RELATION_GREATER;
        } else if (dep->relation & RC_RELATION_GREATER) {
            out_rel |= RC_RELATION_LESS;
        }
    }

    out->relation = out_rel;

    return out;
}

RCPackageDepSList *
rc_package_dep_slist_invert (RCPackageDepSList *depl)
{
    RCPackageDepSList *new_depl = NULL;

    while (depl) {
        RCPackageDep *dep = (RCPackageDep *) depl->data;
        new_depl = g_slist_append (new_depl, rc_package_dep_invert (dep));
        depl = depl->next;
    }

    return new_depl;
}

void
rc_package_dep_weaken (RCPackageDep *dep)
{
    dep->relation |= RC_RELATION_WEAK;
}

void
rc_package_dep_slist_weaken (RCPackageDepSList *depl)
{
    while (depl) {
        RCPackageDep *dep = (RCPackageDep *) depl->data;
        dep->relation |= RC_RELATION_WEAK;
        depl = depl->next;
    }
}

gboolean
rc_package_dep_is_fully_weak (RCPackageDep *dep)
{
    if (!(dep->relation & RC_RELATION_WEAK))
        return FALSE;
    else
        return TRUE;
}

gboolean
rc_package_dep_slist_is_fully_weak (RCPackageDepSList *deps)
{
    while (deps) {
        RCPackageDep *dep = (RCPackageDep *) deps->data;
        if (!rc_package_dep_is_fully_weak (dep))
            return FALSE;
        deps = deps->next;
    }
    return TRUE;
}

gboolean
rc_package_dep_slist_has_dep (RCPackageDepSList *deps, RCPackageDep *pd)
{
    while (deps) {
        RCPackageDep *dep = (RCPackageDep *) deps->data;
        if (dep->relation == pd->relation &&
            rc_package_spec_equal (&dep->spec, &pd->spec))
        {
            return TRUE;
        }

        deps = deps->next;
    }

    return FALSE;
}

void
rc_package_dep_system_is_rpmish (gboolean is_rpm)
{
    rpmish = is_rpm;
}

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
}

static RCPackageDep *
rc_xml_node_to_package_dep_internal (const xmlNode *node)
{
    RCPackageDep *dep_item;
    gchar *tmp;

    if (g_strcasecmp (node->name, "dep")) {
        return (NULL);
    }

    /* FIXME: this sucks */
    dep_item = g_new0 (RCPackageDep, 1);

    dep_item->spec.name = xml_get_prop (node, "name");
    tmp = xml_get_prop (node, "op");
    if (tmp) {
        dep_item->relation = rc_string_to_package_relation (tmp);
        dep_item->spec.epoch =
            xml_get_guint32_value_default (node, "epoch", 0);
        dep_item->spec.version =
            xml_get_prop (node, "version");
        dep_item->spec.release =
            xml_get_prop (node, "release");
        g_free (tmp);
    } else {
        dep_item->relation = RC_RELATION_ANY;
    }

    return (dep_item);
}

RCPackageDep *
rc_xml_node_to_package_dep (const xmlNode *node)
{
    RCPackageDep *dep = NULL;

    if (!g_strcasecmp (node->name, "dep")) {
        dep = rc_xml_node_to_package_dep_internal (node);
        return (dep);
    } else if (!g_strcasecmp (node->name, "or")) {
        RCPackageDepSList *or_dep_slist = NULL;
        RCDepOr *or;
#if LIBXML_VERSION < 20000
        xmlNode *iter = node->childs;
#else
        xmlNode *iter = node->children;
#endif

        while (iter) {
            or_dep_slist = g_slist_append (or_dep_slist,
                                           rc_xml_node_to_package_dep_internal (iter));
            iter = iter->next;
        }

        or = rc_dep_or_new (or_dep_slist);
        dep = rc_dep_or_new_provide (or);
    }

    return (dep);
}
