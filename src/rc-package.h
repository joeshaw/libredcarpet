#ifndef _RC_PACKAGE_H
#define _RC_PACKAGE_H

#include "rc-package-spec.h"
#include "rc-package-dep.h"
#include "rc-package-update.h"

typedef struct _RCPackage RCPackage;

struct _RCPackage {
    RCPackageSpec spec;

    gboolean already_installed;

    /* Filled in by the package manager or dependency XML */
    RCPackageDepSList *requires;
    RCPackageDepSList *provides;
    RCPackageDepSList *conflicts;

    /* These are here to make the debian folks happy */
    RCPackageDepSList *suggests;
    RCPackageDepSList *recommends;
    
    /* Filled in by package info XML */
    gchar *summary;
    gchar *description;

    RCPackageUpdateSList *history;
};

/* Used if key is a string, i.e. name */
typedef GHashTable RCPackageHashTableByString;

/* Used if key is a spec */
typedef GHashTable RCPackageHashTableBySpec;

RCPackage *rc_package_new (void);

RCPackage *rc_package_copy_spec (RCPackage *);

void rc_package_free (RCPackage *rcp);

typedef GSList RCPackageSList;

void rc_package_slist_free (RCPackageSList *rcpsl);

RCPackageSList *rc_package_slist_sort (RCPackageSList *rcpsl);

RCPackageImportance rc_package_get_highest_importance(RCPackage *package);

RCPackageImportance rc_package_get_highest_importance_from_current (
    RCPackage *package, RCPackageSList *system_pkgs);

#endif /* _RC_PACKAGE_H */
