#include "rc-package.h"

RCPackage *
rc_package_new (void)
{
    RCPackage *rcp = g_new0 (RCPackage, 1);

    return (rcp);
} /* rc_package_new */

RCPackage *
rc_package_copy_spec (RCPackage *orig)
{
    RCPackage *rcp = rc_package_new ();

    rc_package_spec_copy ((RCPackageSpec *) orig, (RCPackageSpec *) rcp);

    return (rcp);
}

void
rc_package_free (RCPackage *rcp)
{
    rc_package_spec_free_members (RC_PACKAGE_SPEC (rcp));

    rc_package_dep_slist_free (rcp->requires);
    rc_package_dep_slist_free (rcp->provides);
    rc_package_dep_slist_free (rcp->conflicts);

    g_free (rcp->summary);
    g_free (rcp->description);

    rc_package_update_slist_free (rcp->history);

    g_free (rcp);
} /* rc_package_free */

void
rc_package_slist_free (RCPackageSList *rcpsl)
{
    g_slist_foreach (rcpsl, (GFunc) rc_package_free, NULL);

    g_slist_free (rcpsl);
} /* rc_package_slist_free */

RCPackageUpdateSList *
rc_package_slist_sort (RCPackageSList *rcpusl)
{
    RCPackageSList *list = NULL;

    list = g_slist_sort (rcpusl, (GCompareFunc) rc_package_spec_compare_name);

    return (list);
}

RCPackageImportance 
rc_package_get_highest_importance(RCPackage *package)
{
    GSList *i;
    RCPackageImportance highest = RC_IMPORTANCE_INVALID;

    /* FIXME: This probably isn't the right value to return, but
       I can't think of anything better to return. */
    g_return_val_if_fail(package, RC_IMPORTANCE_INVALID);

    i = package->history;
    while (i) {
	RCPackageUpdate *up = i->data;

	if (up->importance == RC_IMPORTANCE_MAX)
	    highest = RC_IMPORTANCE_NECESSARY;
	else if (up->importance < highest || highest == RC_IMPORTANCE_INVALID)
	    highest = up->importance;

	i = i->next;
    }

    return highest;
} /* rc_package_get_highest_importance */

RCPackageImportance 
rc_package_get_highest_importance_from_current(RCPackage *package,
					       RCPackageSList *system_pkgs)
{
    GSList *i;
    RCPackage *system_package = NULL;
    RCPackageImportance highest = RC_IMPORTANCE_INVALID;

    /* FIXME: This probably isn't the right value to return, but
       I can't think of anything better to return. */
    g_return_val_if_fail(package, RC_IMPORTANCE_INVALID);

    i = system_pkgs;
    while (i) {
	RCPackageSpec *s = i->data;

	if (g_strcasecmp(s->name, RC_PACKAGE_SPEC(package)->name) == 0) {
	    system_package = (RCPackage *) s;
	    break;
	}

	i = i->next;
    }

    if (!system_package)
	return RC_IMPORTANCE_INVALID;

    i = package->history;
    while (i) {
	RCPackageUpdate *up = i->data;

	if (rc_package_spec_compare(
	    RC_PACKAGE_SPEC(up), RC_PACKAGE_SPEC(system_package)) > 0) {
	    if (up->importance == RC_IMPORTANCE_MAX)
		highest = RC_IMPORTANCE_NECESSARY;
	    else if (up->importance < highest || 
		     highest == RC_IMPORTANCE_INVALID)
		highest = up->importance;
	}
	
	i = i->next;
    }

    return highest;
} /* rc_package_get_highest_importance_from_current */
