#include "rc-package-update.h"

RCPackageUpdate *
rc_package_update_new ()
{
    RCPackageUpdate *rcpu = g_new0 (RCPackageUpdate, 1);

    return (rcpu);
} /* rc_package_update_new */

RCPackageUpdate *
rc_package_update_copy (RCPackageUpdate *old)
{
    RCPackageUpdate *new = rc_package_update_new ();

    rc_package_spec_copy ((RCPackageSpec *) old, (RCPackageSpec *) new);

    new->importance = old->importance;
    new->url = g_strdup (old->url);
    new->md5sum = g_strdup (old->md5sum);
    new->description = g_strdup (old->description);
    new->installed_size = old->installed_size;
    new->package_size = old->package_size;
    new->time = old->time;

    return (new);
}

void
rc_package_update_free (RCPackageUpdate *rcpu)
{
    rc_package_spec_free_members(RC_PACKAGE_SPEC(rcpu));

    g_free (rcpu->url);
    g_free (rcpu->md5sum);
    g_free (rcpu->description);

    g_free (rcpu);
} /* rc_package_update_free */

RCPackageUpdateSList *
rc_package_update_slist_copy (RCPackageUpdateSList *old)
{
    RCPackageUpdateSList *iter;
    RCPackageUpdateSList *new_list = NULL;

    for (iter = old; iter; iter = iter->next) {
        RCPackageUpdate *old_update = (RCPackageUpdate *)(iter->data);
        RCPackageUpdate *new = rc_package_update_copy (old_update);

        new_list = g_slist_append (new_list, new);
    }

    return (new_list);
}

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
