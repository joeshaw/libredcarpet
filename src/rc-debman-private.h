/* FIXME: put the GPL here */

#ifndef _RC_DEBMAN_PRIVATE_H
#define _RC_DEBMAN_PRIVATE_H

struct _RCDebmanPrivate {
    int lock_fd;

    GHashTable *pkg_hash;
    gboolean hash_valid;
};

#endif /* _RC_DEBMAN_PRIVATE_H */
