#ifndef _RC_PACKAGE_UPDATE_H
#define _RC_PACKAGE_UPDATE_H

#include <time.h>

#include <glib.h>

#include "rc-package-spec.h"

typedef enum _RCPackageImportance RCPackageImportance;

enum _RCPackageImportance {
    RC_IMPORTANCE_INVALID = -1,

    RC_IMPORTANCE_NECESSARY,
    RC_IMPORTANCE_URGENT,
    RC_IMPORTANCE_SUGGESTED,
    RC_IMPORTANCE_FEATURE,
    RC_IMPORTANCE_MINOR,

    RC_IMPORTANCE_MAX
};

typedef struct _RCPackageUpdate RCPackageUpdate;

struct _RCPackageUpdate {
    RCPackageSpec spec;

    RCPackageImportance importance;
    gchar *url;                 /* URL for the filename of this pkg */
    gchar *md5sum;
    gchar *description;
    guint32 installed_size;
    guint32 package_size;
    time_t time;
};

RCPackageUpdate *rc_package_update_new (void);

void rc_package_update_free (RCPackageUpdate *rcpu);

typedef GSList RCPackageUpdateSList;

void rc_package_update_slist_free (RCPackageUpdateSList *rcpusl);

RCPackageUpdateSList *rc_package_update_slist_sort (RCPackageUpdateSList *l);

#endif /* _RC_PACKAGE_UPDATE_H */
