#ifndef _RC_PACKAGE_DEP_H
#define _RC_PACKAGE_DEP_H

#include <glib.h>

#include "rc-package-spec.h"

typedef guint RCPackageRelation;

#define RC_RELATION_ANY 0
#define RC_RELATION_EQUAL (1 << 0)
#define RC_RELATION_LESS (1 << 1)
#define RC_RELATION_GREATER (1 << 2)

typedef struct _RCPackageDepItem RCPackageDepItem;

struct _RCPackageDepItem {
    RCPackageSpec spec;
    RCPackageRelation relation;
};

RCPackageDepItem *rc_package_dep_item_new (gchar *name,
                                           guint32 epoch,
                                           gchar *version,
                                           gchar *release,
                                           RCPackageRelation relation);

RCPackageDepItem *rc_package_dep_item_new_from_spec (
    RCPackageSpec *spec,
    RCPackageRelation relation);

void rc_package_dep_item_free (RCPackageDepItem *rcpdi);

typedef GSList RCPackageDep;

RCPackageDep *rc_package_dep_new_with_item (RCPackageDepItem *rcpdi);

void rc_package_dep_free (RCPackageDep *rcpd);

typedef GSList RCPackageDepSList;

void rc_package_dep_slist_free (RCPackageDepSList *rcpdsl);

/* Dep verification */
gboolean rc_package_dep_verify_relation (RCPackageDep *dep, RCPackageSpec *spec);
gboolean rc_package_dep_item_verify_relation (RCPackageDepItem *dep, RCPackageSpec *spec);

#endif /* _RC_PACKAGE_DEP_H */
