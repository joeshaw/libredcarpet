/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _RC_PACKMAN_H
#define _RC_PACKMAN_H

#include <gtk/gtk.h>

#include "rc-package.h"

#define GTK_TYPE_RC_PACKMAN        (rc_packman_get_type ())
#define RC_PACKMAN(obj)            (GTK_CHECK_CAST ((obj), \
                                    GTK_TYPE_RC_PACKMAN, RCPackman))
#define RC_PACKMAN_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                    GTK_TYPE_RC_PACKMAN, \
                                    RCPackmanClass))
#define IS_RC_PACKMAN(obj)         (GTK_CHECK_TYPE ((obj), \
                                    GTK_TYPE_RC_PACKMAN))
#define IS_RC_PACKMAN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
                                    GTK_TYPE_RC_PACKMAN))

#define _CLASS(p) (RC_PACKMAN_CLASS(GTK_OBJECT(p)->klass))

typedef struct _RCPackman      RCPackman;
typedef struct _RCPackmanClass RCPackmanClass;

typedef enum _RCPackmanError {
    /* No error */
    RC_PACKMAN_ERROR_NONE = 0,
    /* Packman object is busy in another operation */
    RC_PACKMAN_ERROR_BUSY,
    /* The requested operation was aborted due to detected error */
    RC_PACKMAN_ERROR_ABORT,
    /* An error occured at the package manager level or lower, and the
       requested operation was aborted */
    RC_PACKMAN_ERROR_FAIL,
    /* An error from which we cannot and should not attempt to recover */
    RC_PACKMAN_ERROR_FATAL,
} RCPackmanError;

struct _RCPackman {
    GtkObject parent;

    /* Stores the error code from the last packman operation (0 is a
       successful operation, everything else is a problem) */
    guint error;

    /* If an operation (eg installation) fails, the reason for failure will be
       stored here */
    gchar *reason;

    /* Current status of the object */
    gboolean busy;

    /* Flags for the supported interface */
    gboolean pre_config;
    gboolean pkg_progress;
    gboolean post_config;
};

struct _RCPackmanClass {
    GtkObjectClass parent_class;

    /* Signals */

    void (*configure_progress)(RCPackman *, gint amount, gint total);
    void (*configure_step)(RCPackman *, gint seqno, gint total);
    void (*configure_done)(RCPackman *);

    void (*transaction_progress)(RCPackman *, gint amount, gint total);
    void (*transaction_step)(RCPackman *, gint seqno, gint total);
    void (*transaction_done)(RCPackman *);

    /* Virtual functions */

    void (*rc_packman_real_transact)(RCPackman *p,
                                     RCPackageSList *install_pkgs,
                                     RCPackageSList *remove_pkgs);

    RCPackage *(*rc_packman_real_query)(RCPackman *p, RCPackage *pkg);

    RCPackage *(*rc_packman_real_query_file)(RCPackman *p, gchar *filename);

    RCPackageSList *(*rc_packman_real_query_all)(RCPackman *p);

    gint (*rc_packman_real_version_compare)(RCPackman *p,
                                            RCPackageSpec *s1,
                                            RCPackageSpec *s2);

    gboolean (*rc_packman_real_verify)(RCPackman *p,
                                       RCPackage *pkg);
};

guint rc_packman_get_type (void);

RCPackman *rc_packman_new (void);

/* Query the supported interfaces of the backend.  First parameter, does the
   packman give config_progress signals?  Second parameter, does the packman
   give pkg_progress signals?  Thirdly, does the packman require a post-install
   step? */

void rc_packman_query_interface (RCPackman *p, gboolean *config, gboolean *pkg,
                                 gboolean *post);

void rc_packman_transact (RCPackman *p, RCPackageSList *install_pkgs,
                          RCPackageSList *remove_pkgs);

/* Queries the system package database for a given RCPackage.  Fills in any
   missing information, including version, release, epoch, dependency lists,
   etc.  Sets the spec->installed field if it finds the package.

   Note: on systems which support installing multiple version of the same file,
   this function should return the latest version which matches the given
   criteria.  However, if the version information is supplied, it must only
   query packages which meet that criteria. */
RCPackage *rc_packman_query (RCPackman *p, RCPackage *pkg);

RCPackageSList *rc_packman_query_list (RCPackman *p, RCPackageSList *pkgs);

/* Uses the system package manager to examine a given filename, and returns
   an RCPackage with the name, version, release, epoch, and dependency fields
   filled in.  This function does not touch the system installed package
   database, so spec->installed must always be FALSE (even if the identical
   package to this file is already installed!). */
RCPackage *rc_packman_query_file (RCPackman *p, gchar *filename);

RCPackageSList *rc_packman_query_file_list (RCPackman *p, GSList *filenames);

/* Queries the system package database and returns a list of all packages
   installed on the system.  This function will return all instances of a
   package installed, unlike _query. */
RCPackageSList *rc_packman_query_all (RCPackman *p);

/* Use the system package manager style comparison to do a strcmp-semantics
   like comparison on the two RCPackageSpec's. */
gint rc_packman_version_compare (RCPackman *p,
                                 RCPackageSpec *s1,
                                 RCPackageSpec *s2);

/* Don't worry about this function yet, I haven't decided firmly on how to
   handle this stuff. */
gboolean rc_packman_verify (RCPackman *p, RCPackage *pkg);

/* Return the object's error code from the last operation. */
guint rc_packman_get_error (RCPackman *p);

/* Return a possible descriptive string about the last error that occured. */
gchar *rc_packman_get_reason (RCPackman *p);

/* Wrappers to emit signals */

void rc_packman_config_progress (RCPackman *p, gint amount, gint total);

void rc_packman_configure_step (RCPackman *p, gint seqno, gint total);

void rc_packman_configure_done (RCPackman *p);

void rc_packman_transaction_progress (RCPackman *p, gint amount, gint total);

void rc_packman_transaction_step (RCPackman *p, gint seqno, gint total);

void rc_packman_transaction_done (RCPackman *p);

/* Helper function to build RCPackageSList's easily */

RCPackageSList *rc_package_slist_add_package (RCPackageSList *pkgs,
                                              gchar *name, guint32 epoch,
                                              gchar *version, gchar *release,
                                              gboolean installed,
                                              guint32 installed_size);

#endif /* _RC_PACKMAN_H */
