#ifndef _RC_PACKAGE_UPDATE_H
#define _RC_PACKAGE_UPDATE_H

#include <glib.h>

typedef enum _RCPackageImportance RCPackageImportance;

typedef struct _RCPackageUpdate RCPackageUpdate;

typedef GSList RCPackageUpdateSList;

#include <time.h>

#include <gnome-xml/tree.h>

#include <libredcarpet/rc-package.h>
#include <libredcarpet/rc-package-spec.h>

enum _RCPackageImportance {
    RC_IMPORTANCE_INVALID = -1,

    RC_IMPORTANCE_NECESSARY,
    RC_IMPORTANCE_URGENT,
    RC_IMPORTANCE_SUGGESTED,
    RC_IMPORTANCE_FEATURE,
    RC_IMPORTANCE_MINOR,

    RC_IMPORTANCE_NEW,

    RC_IMPORTANCE_MAX
};

struct _RCPackageUpdate {
    RCPackageSpec spec;

    const RCPackage *package;

    gchar *package_url;
    guint32 package_size;

    guint32 installed_size;

    gchar *signature_url;
    guint32 signature_size;

    gchar *md5sum;

    RCPackageImportance importance;

    gchar *description;
};

RCPackageUpdate *rc_package_update_new (void);

RCPackageUpdate *rc_package_update_copy (RCPackageUpdate *old);

void rc_package_update_free (RCPackageUpdate *rcpu);

RCPackageUpdateSList *rc_package_update_slist_copy (RCPackageUpdateSList *old);

void rc_package_update_slist_free (RCPackageUpdateSList *rcpusl);

RCPackageUpdateSList *rc_package_update_slist_sort (RCPackageUpdateSList *l);

RCPackageImportance rc_string_to_package_importance (gchar *importance);

const gchar *rc_package_importance_to_string (RCPackageImportance importance);

xmlNode *rc_package_update_to_xml_node (RCPackageUpdate *);

RCPackageUpdate *rc_xml_node_to_package_update (const xmlNode *,
                                                const RCPackage *package);

#endif /* _RC_PACKAGE_UPDATE_H */
