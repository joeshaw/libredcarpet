/* FIXME: put the GPL here */

#ifndef _RC_DEBMAN_PRIVATE_H
#define _RC_DEBMAN_PRIVATE_H

struct _RCDebmanPrivate {
    int lock_fd;

    GHashTable *package_hash;
    gboolean hash_valid;

    gchar *status_file;
    gchar *rc_status_file;
};

#endif /* _RC_DEBMAN_PRIVATE_H */
