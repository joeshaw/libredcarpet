#ifndef _RC_PACKAGE_UPDATE_H
#define _RC_PACKAGE_UPDATE_H

#include <glib.h>

typedef struct _RCPackageUpdate RCPackageUpdate;

typedef GSList RCPackageUpdateSList;

#include "rc-package.h"
#include "rc-package-spec.h"
#include "rc-package-importance.h"

#include <gnome-xml/tree.h>

#include <time.h>

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

RCPackageUpdate *rc_package_update_copy (RCPackageUpdate *old_update);

void rc_package_update_free (RCPackageUpdate *update);

RCPackageUpdateSList
*rc_package_update_slist_copy (RCPackageUpdateSList *old_update);

void rc_package_update_slist_free (RCPackageUpdateSList *update_slist);

RCPackageUpdateSList
*rc_package_update_slist_sort (RCPackageUpdateSList *old_slist);

xmlNode *rc_package_update_to_xml_node (RCPackageUpdate *update);

RCPackageUpdate *rc_xml_node_to_package_update (const xmlNode *,
                                                const RCPackage *package);

#endif /* _RC_PACKAGE_UPDATE_H */
