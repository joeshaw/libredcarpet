#include <stdio.h>

#include <libredcarpet/rc-package-dep.h>
#include <libredcarpet/xml-util.h>

#include <gnome-xml/tree.h>

/* #define DEBUG 50 */

RCPackageDepItem *
rc_package_dep_item_new (gchar *name,
                         guint32 epoch,
                         gchar *version,
                         gchar *release,
                         RCPackageRelation relation)
{
    RCPackageDepItem *rcpdi = g_new0 (RCPackageDepItem, 1);

    rc_package_spec_init (RC_PACKAGE_SPEC (rcpdi), name, epoch, version,
                          release, FALSE, 0, 0, 0);

    rcpdi->relation = relation;

    return (rcpdi);
} /* rc_package_dep_item_new */

RCPackageDepItem *
rc_package_dep_item_new_from_spec (RCPackageSpec *spec,
                                   RCPackageRelation relation)
{
    RCPackageDepItem *rcpdi = rc_package_dep_item_new (spec->name, spec->epoch,
                                                       spec->version,
                                                       spec->release,
                                                       relation);

    return (rcpdi);
} /* rc_package_dep_item_new_from_spec */

RCPackageDepItem *
rc_package_dep_item_copy (RCPackageDepItem *rcpdi)
{
    RCPackageDepItem *new = g_new0 (RCPackageDepItem, 1); /* That sucks */

    rc_package_spec_copy ((RCPackageSpec *) rcpdi, (RCPackageSpec *) new);

    new->relation = rcpdi->relation;

    return (new);
}

void
rc_package_dep_item_free (RCPackageDepItem *rcpdi)
{
    rc_package_spec_free_members (RC_PACKAGE_SPEC (rcpdi));

    g_free (rcpdi);
} /* rc_package_dep_item_free */

RCPackageDep *
rc_package_dep_new_with_item (RCPackageDepItem *rcpdi)
{
    RCPackageDep *rcpd;
    rcpd = g_slist_append (NULL, rcpdi);
    return rcpd;
} /* rc_package_dep_new_with_item */

RCPackageDep *
rc_package_dep_copy (RCPackageDep *old)
{
    RCPackageDep *iter;
    RCPackageDep *dep = NULL;

    for (iter = old; iter; iter = iter->next) {
        RCPackageDepItem *di = (RCPackageDepItem *)(iter->data);
        RCPackageDepItem *new = rc_package_dep_item_copy (di);

        dep = g_slist_append (dep, new);
    }

    return (dep);
}

void
rc_package_dep_free (RCPackageDep *rcpd)
{
    g_slist_foreach (rcpd, (GFunc) rc_package_dep_item_free, NULL);

    g_slist_free (rcpd);
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
}

void
rc_package_dep_slist_free (RCPackageDepSList *rcpdsl)
{
    g_slist_foreach (rcpdsl, (GFunc) rc_package_dep_free, NULL);

    g_slist_free (rcpdsl);
} /* rc_package_dep_slist_free */

/*
 * Helper function to verify  if the Dep or DepItem is fulfilled by the given spec
 */

gboolean
rc_package_dep_verify_relation (RCPackageDep *dep, RCPackageSpec *spec)
{
    /* Succeeds if any of the deps succeed */
    while (dep) {
        if (rc_package_dep_item_verify_relation ((RCPackageDepItem *) (dep->data), spec))
            return TRUE;
        dep = dep->next;
    }

    return FALSE;
}

/* Returns true if ALL and deps verify; if fail_out isn't NULL,
 * returns the first dep that failed in there.
 */
gboolean
rc_package_dep_verify_and_relation (RCPackageDep *dep, RCPackageSpec *spec,
                                    RCPackageDepItem **fail_out)
{
    while (dep) {
        if (!rc_package_dep_item_verify_relation ((RCPackageDepItem *) (dep->data), spec)) {
            if (fail_out) *fail_out = (RCPackageDepItem *) (dep->data);
            return FALSE;
        }
        dep = dep->next;
    }

    return TRUE;
}

gboolean
rc_package_dep_item_verify_relation (RCPackageDepItem *dep, RCPackageSpec *spec)
{
    gboolean use_newspecspec = FALSE;
    RCPackageSpec newspecspec;

    gint compare_ret;

    g_assert (dep);
    g_assert (spec);

#if DEBUG > 10
    fprintf (stderr, "--> Checking: %s-%d:%s-%s (rel %d) vs. %s-%d:%s-%s: ",
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

    if (dep->spec.version == NULL && dep->spec.release == NULL) {
        if (strcmp (dep->spec.name, spec->name) == 0) {
#if DEBUG > 10
            fprintf (stderr, "PASS (nullrv)\n");
#endif
            return TRUE;
        }
    }

    if (spec->version == NULL && spec->release == NULL)
    {
        /* If it's the same name and the relation isn't looking for a version less than blah */
        if ((strcmp (dep->spec.name, spec->name) == 0) && !(dep->relation & RC_RELATION_LESS)) {
#if DEBUG > 10
            fprintf (stderr, "PASS (nullrv)\n");
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
 *
 */
    
    if (dep->relation == RC_RELATION_EQUAL && dep->spec.release == NULL) {
        /* it's not depending on any particular release */
        newspecspec.name = spec->name;
        newspecspec.epoch = spec->epoch;
        newspecspec.version = spec->version;
        newspecspec.release = NULL;

        use_newspecspec = TRUE;
    }

    compare_ret = rc_package_spec_compare (&dep->spec,
                                           use_newspecspec ? &newspecspec : spec);
    
    /* Note that we make the assumption that if we're in this function,
     * we're checking a dep on the same package (so this function won't
     * check whether they're the same name for an ANY relation
     */
    if (dep->relation == RC_RELATION_ANY) {
#if DEBUG > 10
            fprintf (stderr, "PASS\n");
#endif
        return TRUE;
    }

    if ((dep->relation & RC_RELATION_EQUAL) && (compare_ret == 0)) {
#if DEBUG > 10
            fprintf (stderr, "PASS\n");
#endif
        return TRUE;
    }
    /* If the realtion is greater, then we want the compare to tell us that the
     * dep is LESS than the spec
     */
    if ((dep->relation & RC_RELATION_GREATER) && (compare_ret < 0)) {
#if DEBUG > 10
            fprintf (stderr, "PASS\n");
#endif
        return TRUE;
    }
    if ((dep->relation & RC_RELATION_LESS) && (compare_ret > 0)) {
#if DEBUG > 10
            fprintf (stderr, "PASS\n");
#endif
        return TRUE;
    }

#if DEBUG > 10
            fprintf (stderr, "FAIL\n");
#endif
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
    } else {
        return (RC_RELATION_INVALID);
    }
}

const gchar *
rc_package_relation_to_string (RCPackageRelation relation, gboolean words)
{
    switch (relation) {
    case RC_RELATION_ANY:
        return ("(any)");
        break;
    case RC_RELATION_EQUAL:
        return (words ? "equal to" : "=");
        break;
    case RC_RELATION_LESS:
        return (words ? "less than" : "<");
        break;
    case RC_RELATION_LESS_EQUAL:
        return (words ? "less than or equal to" : "<=");
        break;
    case RC_RELATION_GREATER:
        return (words ? "greater than" : ">");
        break;
    case RC_RELATION_GREATER_EQUAL:
        return (words ? "greater than or equal to" : ">=");
        break;
    case RC_RELATION_NOT_EQUAL:
        return (words ? "not equal to" : "!=");
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
rc_package_dep_item_is_subset (RCPackageDepItem *a, RCPackageDepItem *b)
{
    gint arel;
    gint brel;

    gint compare;

    g_return_val_if_fail (a, FALSE);
    g_return_val_if_fail (b, FALSE);

    arel = a->relation;
    brel = b->relation;

    compare = rc_package_spec_compare (&a->spec, &b->spec);


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

static xmlNode *
rc_package_dep_item_to_xml_node (RCPackageDepItem *dep_item)
{
    xmlNode *dep_node;

    dep_node = xmlNewNode (NULL, "dep");

    xmlSetProp (dep_node, "name", dep_item->spec.name);

    if (dep_item->relation != RC_RELATION_ANY) {
        xmlSetProp (dep_node, "op",
                    rc_package_relation_to_string (dep_item->relation, FALSE));

        if (dep_item->spec.epoch > 0) {
            gchar *tmp;

            tmp = g_strdup_printf ("%d", dep_item->spec.epoch);
            xmlSetProp (dep_node, "epoch", tmp);
            g_free (tmp);
        }

        if (dep_item->spec.version) {
            xmlSetProp (dep_node, "version", dep_item->spec.version);
        }

        if (dep_item->spec.release) {
            xmlSetProp (dep_node, "release", dep_item->spec.release);
        }
    }

    return (dep_node);
}

xmlNode *
rc_package_dep_to_xml_node (RCPackageDep *dep)
{
    if (!dep->next) {
        /* The simple case, a single non-OR dependency */

        return (rc_package_dep_item_to_xml_node
                ((RCPackageDepItem *)dep->data));
    } else {
        xmlNode *or_node;
        RCPackageDep *dep_iter;

        or_node = xmlNewNode (NULL, "or");

        for (dep_iter = dep; dep_iter; dep_iter = dep_iter->next) {
            RCPackageDepItem *dep_item = (RCPackageDepItem *)(dep_iter->data);
            xmlAddChild (or_node, rc_package_dep_item_to_xml_node (dep_item));
        }

        return (or_node);
    }
}

static RCPackageDepItem *
rc_xml_node_to_package_dep_item (xmlNode *node)
{
    RCPackageDepItem *dep_item;
    gchar *tmp;

    if (g_strcasecmp (node->name, "dep")) {
        return (NULL);
    }

    /* FIXME: this sucks */
    dep_item = g_new0 (RCPackageDepItem, 1);

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
rc_xml_node_to_package_dep (xmlNode *node)
{
    RCPackageDep *dep = NULL;

    if (!g_strcasecmp (node->name, "dep")) {
        dep = g_slist_append (dep, rc_xml_node_to_package_dep_item (node));
        return (dep);
    } else if (!g_strcasecmp (node->name, "or")) {
#if LIBXML_VERSION < 20000
        xmlNode *iter = node->childs;
#else
        xmlNode *iter = node->children;
#endif

        while (iter) {
            dep = g_slist_append (dep, rc_xml_node_to_package_dep_item (iter));
            iter = iter->next;
        }
    }

    return (dep);
}
