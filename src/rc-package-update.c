#include "rc-package-update.h"

RCPackageUpdate *
rc_package_update_new ()
{
    RCPackageUpdate *rcpu = g_new0 (RCPackageUpdate, 1);

    return (rcpu);
} /* rc_package_update_new */

void
rc_package_update_free (RCPackageUpdate *rcpu)
{
    rc_package_spec_free_members(RC_PACKAGE_SPEC(rcpu));

    g_free (rcpu->filename);
    g_free (rcpu->md5sum);
    g_free (rcpu->description);

    g_free (rcpu);
} /* rc_package_update_free */

void
rc_package_update_slist_free (RCPackageUpdateSList *rcpusl)
{
    g_slist_foreach (rcpusl, (GFunc) rc_package_update_free, NULL);

    g_slist_free (rcpusl);
} /* rc_package_update_slist_free */

RCPackageUpdateSList *
rc_package_update_slist_sort (RCPackageUpdateSList *rcpusl)
{
    RCPackageUpdateSList *list = NULL;

    list = g_slist_sort (rcpusl, (GCompareFunc) rc_package_spec_compare_name);

    return (list);
}
