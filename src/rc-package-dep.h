#ifndef _RC_PACKAGE_DEP_H
#define _RC_PACKAGE_DEP_H

#include <glib.h>

#include <gnome-xml/tree.h>

#include <libredcarpet/rc-package-spec.h>

#define RELATION_ANY 0
#define RELATION_EQUAL (1 << 0)
#define RELATION_LESS (1 << 1)
#define RELATION_GREATER (1 << 2)

/* This enum is here so that gdb can give us pretty strings */
typedef enum _RCPackageRelation RCPackageRelation;

enum _RCPackageRelation {
    RC_RELATION_INVALID = -1,
    RC_RELATION_ANY = RELATION_ANY,
    RC_RELATION_EQUAL = RELATION_EQUAL,
    RC_RELATION_LESS = RELATION_LESS,
    RC_RELATION_LESS_EQUAL = RELATION_LESS | RELATION_EQUAL,
    RC_RELATION_GREATER = RELATION_GREATER,
    RC_RELATION_GREATER_EQUAL = RELATION_GREATER | RELATION_EQUAL,
    RC_RELATION_NOT_EQUAL = RELATION_LESS | RELATION_GREATER
};

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

RCPackageDepItem *rc_package_dep_item_copy (RCPackageDepItem *rcpdi);

void rc_package_dep_item_free (RCPackageDepItem *rcpdi);

typedef GSList RCPackageDep;

RCPackageDep *rc_package_dep_new_with_item (RCPackageDepItem *rcpdi);

RCPackageDep *rc_package_dep_copy (RCPackageDep *orig);

void rc_package_dep_free (RCPackageDep *rcpd);

typedef GSList RCPackageDepSList;

RCPackageDepSList *rc_package_dep_slist_copy (RCPackageDepSList *old);

void rc_package_dep_slist_free (RCPackageDepSList *rcpdsl);

/* Dep verification */
gboolean rc_package_dep_verify_relation (RCPackageDep *dep, RCPackageSpec *spec);
gboolean rc_package_dep_verify_and_relation (RCPackageDep *dep, RCPackageSpec *spec,
                                             RCPackageDepItem **fail_out);
gboolean rc_package_dep_item_verify_relation (RCPackageDepItem *dep, RCPackageSpec *spec);

gint rc_package_dep_item_is_subset (RCPackageDepItem *a, RCPackageDepItem *b);

RCPackageRelation rc_string_to_package_relation (const gchar *relation);

const gchar *rc_package_relation_to_string (RCPackageRelation relation,
                                            gboolean words);

xmlNode *rc_package_dep_to_xml_node (RCPackageDep *);

RCPackageDep *rc_xml_node_to_package_dep (xmlNode *);

#endif /* _RC_PACKAGE_DEP_H */
