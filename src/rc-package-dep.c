#include "rc-package-dep.h"

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

void
rc_package_dep_item_free (RCPackageDepItem *rcpdi)
{
    rc_package_spec_free_members (RC_PACKAGE_SPEC (rcpdi));

    g_free (rcpdi);
} /* rc_package_dep_item_free */

RCPackageDep *rc_package_dep_new_with_item (RCPackageDepItem *rcpdi)
{
    RCPackageDep *rcpd;
    rcpd = g_slist_append (NULL, rcpdi);
    return rcpd;
} /* rc_package_dep_new_with_item */

void
rc_package_dep_free (RCPackageDep *rcpd)
{
    g_slist_foreach (rcpd, (GFunc) rc_package_dep_item_free, NULL);

    g_slist_free (rcpd);
} /* rc_package_dep_free */

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
