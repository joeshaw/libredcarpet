#ifndef _RC_PACKAGE_SPEC_H
#define _RC_PACKAGE_SPEC_H

#include <glib.h>

typedef enum _RCPackageSection RCPackageSection;

enum _RCPackageSection {
    SECTION_OFFICE = 0,
    SECTION_IMAGING,
    SECTION_PIM,
    SECTION_GAME,
    SECTION_MULTIMEDIA,
    SECTION_INTERNET,
    SECTION_UTIL,
    SECTION_SYSTEM,
    SECTION_DOC,
    SECTION_DEVEL,
    SECTION_DEVELUTIL,
    SECTION_LIBRARY,
    SECTION_XAPP,
    SECTION_MISC,
    SECTION_LAST
};

typedef struct _RCPackageSpec RCPackageSpec;

struct _RCPackageSpec {
    gchar *name;
    guint32 epoch;
    gchar *version;
    gchar *release;

    RCPackageSection section;

    gboolean installed;
    guint32 installed_size;
    guint32 channel;
    guint32 subchannel;
};

#define RC_PACKAGE_SPEC(item) ((RCPackageSpec *)(item))

void rc_package_spec_init (RCPackageSpec *rcps,
                           gchar *name,
                           guint32 epoch,
                           gchar *version,
                           gchar *release,
                           gboolean installed,
                           guint32 installed_size,
                           guint32 channel,
                           guint32 subchannel);

void rc_package_spec_copy (RCPackageSpec *old, RCPackageSpec *new);

void rc_package_spec_free_members (RCPackageSpec *rcps);

gint rc_package_spec_compare_name (void *a, void *b);

/* Hash functions: hash and compare */
guint rc_package_spec_hash (gconstpointer ptr);
gint rc_package_spec_compare (gconstpointer ptra, gconstpointer ptrb);
gint rc_package_spec_equal (gconstpointer ptra, gconstpointer ptrb);

gchar *rc_package_spec_to_str (RCPackageSpec *spec);

const gchar *rc_package_section_to_string (RCPackageSection section);

RCPackageSection rc_string_to_package_section (gchar *section);

#endif /* _RC_PACKAGE_SPEC_H */
