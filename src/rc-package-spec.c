#include <ctype.h>
#include <stdio.h>

/* #define DEBUG 50 */

#include <libredcarpet/rc-package-spec.h>

void
rc_package_spec_init (RCPackageSpec *rcps,
                      gchar *name,
                      guint32 epoch,
                      gchar *version,
                      gchar *release)
{
    g_assert (rcps);

    rcps->name = g_strdup (name);
    rcps->epoch = epoch;
    rcps->version = g_strdup (version);
    rcps->release = g_strdup (release);
} /* rc_package_spec_init */

void
rc_package_spec_copy (RCPackageSpec *new, RCPackageSpec *old)
{
    rc_package_spec_init (new, old->name, old->epoch, old->version,
                          old->release);
}

void
rc_package_spec_free_members (RCPackageSpec *rcps)
{
    g_free (rcps->name);
    g_free (rcps->version);
    g_free (rcps->release);
} /* rc_package_spec_free_members */

gint
rc_package_spec_compare_name (void *a, void *b)
{
    RCPackageSpec *one, *two;

    one = (RCPackageSpec *) a;
    two = (RCPackageSpec *) b;

    return (strcmp (one->name, two->name));
}

gchar *
rc_package_spec_to_str (RCPackageSpec *spec)
{
    gchar *buf;
    if (spec->epoch) {
        buf = g_strdup_printf ("%s%s%d:%s%s%s",
                               (spec->name ? spec->name  : ""),
                               (spec->version ? "-" : ""),
                               spec->epoch,
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    } else {
        buf = g_strdup_printf ("%s%s%s%s%s",
                               (spec->name ? spec->name  : ""),
                               (spec->version ? "-" : ""),
                               (spec->version ? spec->version : ""),
                               (spec->release ? "-" : ""),
                               (spec->release ? spec->release : ""));
    }

    return buf;
}

    
guint rc_package_spec_hash (gconstpointer ptr)
{
    RCPackageSpec *spec = (RCPackageSpec *) ptr;
    guint ret;
    gchar *b = rc_package_spec_to_str (spec);
    ret = g_str_hash (b);
    g_free (b);
    return ret;
}

gint rc_package_spec_equal (gconstpointer a, gconstpointer b)
{
    RCPackageSpec *one = RC_PACKAGE_SPEC (a);
    RCPackageSpec *two = RC_PACKAGE_SPEC (b);

    g_assert (one);
    g_assert (two);

    if (one->epoch != two->epoch) {
        return (FALSE);
    }

    if (one->name && two->name) {
        if (strcmp (one->name, two->name)) {
            return (FALSE);
        }
    } else if (one->name || two->name) {
        return (FALSE);
    }

    if (one->version && two->version) {
        if (strcmp (one->version, two->version)) {
            return (FALSE);
        }
    } else if (one->version || two->version) {
        return (FALSE);
    }

    if (one->release && two->release) {
        if (strcmp (one->release, two->release)) {
            return (FALSE);
        }
    } else if (one->release || two->release) {
        return (FALSE);
    }

    return (TRUE);
}
