#ifndef _RC_PACKAGE_SPEC_H
#define _RC_PACKAGE_SPEC_H

#include <glib.h>

typedef struct _RCPackageSpec RCPackageSpec;

/* Make sure name is always the first element of this struct */
struct _RCPackageSpec {
    gchar *name;
    guint32 epoch;
    gchar *version;
    gchar *release;
};

#define RC_PACKAGE_SPEC(item) ((RCPackageSpec *)(item))

void rc_package_spec_init (RCPackageSpec *rcps,
                           gchar *name,
                           guint32 epoch,
                           gchar *version,
                           gchar *release);

void rc_package_spec_copy (RCPackageSpec *new, RCPackageSpec *old);

void rc_package_spec_free_members (RCPackageSpec *rcps);

gint rc_package_spec_compare_name (void *a, void *b);

/* Hash functions: hash and compare */
guint rc_package_spec_hash (gconstpointer ptr);
gint rc_package_spec_equal (gconstpointer ptra, gconstpointer ptrb);

gchar *rc_package_spec_to_str (RCPackageSpec *spec);

#endif /* _RC_PACKAGE_SPEC_H */
