/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public License
 *    as published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *    GNU Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _RC_PACKMAN_H
#define _RC_PACKMAN_H

#include <gtk/gtk.h>

#include "common.h"

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

typedef enum _RCPackmanOperationStatus {
    RC_PACKMAN_COMPLETE = 0,  /* Operation(s) successfully carried out */
    RC_PACKMAN_FAIL,          /* Lowlevel error occured, operation failed */
    RC_PACKMAN_ABORT,         /* Error detected, operation aborted */
} RCPackmanOperationStatus;

typedef enum _RCPackmanStatus {
    RC_PACKMAN_IDLE,      /* Object is not performing any operations */
    RC_PACKMAN_INSTALL,   /* Object is installing packages */
    RC_PACKMAN_REMOVE,    /* Object is removing packages */
    RC_PACKMAN_QUERY,     /* Object is querying installed packages */
} RCPackmanStatus;

typedef enum _RCPackmanError {
    RC_PACKMAN_NONE = 0,         /* No error */
    RC_PACKMAN_NO_BACKEND,       /* You're not using a subclass of RCPackman */
    RC_PACKMAN_NOT_IMPLEMENTED,  /* Current object doesn't support operation */
    RC_PACKMAN_OPERATION_FAILED, /* Package system specific failure */
} RCPackmanError;

struct _RCPackman {
    GtkObject parent;

    /* Stores the error code from the last packman operation (0 is a
       successful operation, everything else is a problem) */
    guint error;

    /* If an operation (eg installation) fails, the reason for failure will be
     * stored here */
    gchar *reason;

    /* Current status of the object */
    guint status;
};

struct _RCPackmanClass {
    GtkObjectClass parent_class;

    /* Signals */

    void (*pkg_progress)(RCPackman *, gchar *file, gint amount, gint total);

    void (*pkg_installed)(RCPackman *, gchar *file, gint sequence);
    void (*install_done)(RCPackman *, RCPackmanOperationStatus status);

    void (*pkg_removed)(RCPackman *, gchar *name, gint sequence);
    void (*remove_done)(RCPackman *, RCPackmanOperationStatus status);

    /* Virtual functions */

    void (*rc_packman_real_install)(RCPackman *p, GSList *filenames);

    void (*rc_packman_real_remove)(RCPackman *p, RCPackageSList *pkgs);

    RCPackage *(*rc_packman_real_query)(RCPackman *p, RCPackage *pkg);

    RCPackage *(*rc_packman_real_query_file)(RCPackman *p, gchar *filename);

    RCPackageSList *(*rc_packman_real_query_all)(RCPackman *p);

    gint (*rc_packman_real_version_compare)(RCPackman *p,
                                            RCPackageSpec *s1,
                                            RCPackageSpec *s2);

    RCPackageSList *(*rc_packman_real_depends)(RCPackman *p,
                                               RCPackageSList *pkgs);

    RCPackageSList *(*rc_packman_real_depends_files)(RCPackman *p,
                                                     GSList *files);

    gboolean (*rc_packman_real_verify)(RCPackman *p,
                                       RCPackage *pkg);
};

guint rc_packman_get_type (void);

RCPackman *rc_packman_new (void);

/* Given a list of filenames of packages to install, use the system package
   manager to install them.  Sets the error code (and possibly reason field)
   after install.  Should make every effort to insure that the listed packages
   are either all installed, or none installed. */

void rc_packman_install (RCPackman *p, GSList *filenames);

/* Given a list of RCPackages, remove the listed packages from the system using
   the system package manager.  Sets the error code (and possibly reason field)
   after removal.  Should make every effort to insure that the listed packages
   are either all removed, or none removed. */

void rc_packman_remove (RCPackman *p, RCPackageSList *pkgs);

/* Queries the system package database for a given RCPackage.  Fills in any
   missing information (for instance, if only the name field is filled in, will
   return with the version, release, epoch fields filled in).  Most
   importantly, this function should /never/ change non-NULL information in the
   RCPackage (including dependency info), and should set the installed field in
   the RCPackageSpec.

   Note: on systems which support installing multiple version of the same file,
   this function should return the latest version which matches the given
   criteria.  However, if the version information is supplied, it must only
   query packages which meet that criteria. */

RCPackage *rc_packman_query (RCPackman *p, RCPackage *pkg);

/* Uses the system package manager to examine a given filename, and returns
   an RCPackage with the name, version, release, epoch fields filled in.  This
   function does not touch the dependencies information.  This function does
   not touch the system installed package database, so spec->installed must
   always be FALSE (even if the identical package to this file is already
   installed!). */

RCPackage *rc_packman_query_file (RCPackman *p, gchar *filename);

/* Queries the system package database and returns a list of all packages
   installed on the system.  This function will return all instances of a
   package installed, unlike _query. */

RCPackageSList *rc_packman_query_all (RCPackan *p);

/* Use the system package manager style comparison to do a strcmp-semantics
   like comparison on the two RCPackageSpec's. */

gint rc_packman_version_compare (RCPackman *p,
                                 RCPackageSpec *s1,
                                 RCPackageSpec *s2);

/* Given a list of RCPackage's, fill in the dependency fields in all of the
   RCPackages. */

RCPackageSList *rc_packman_depends (RCPackman *p, RCPackageSList *pkgs);

/* Given a list of filenames, create an RCPackageSpec (a query), and then fill
   in the dependency information as well. */

RCPackageSList *rc_packman_depends_files (RCPackman *p, GSList *pkgs);

/* Don't worry about this function yet, I haven't decided firmly on how to
   handle this stuff. */

gboolean rc_packman_verify (RCPackman *p, RCPackage *pkg);

/* Return the object's error code from the last operation. */

guint rc_packman_get_error (RCPackman *p);

/* Return a possible descriptive string about the last error that occured. */

gchar *rc_packman_get_reason (RCPackman *p);

/* Wrappers to emit signals */

void rc_packman_package_progress(RCPackman *p,
                                 const gchar *filename,
                                 gint amount,
                                 gint total);

void rc_packman_package_installed (RCPackman *p,
                                   const gchar *filename,
                                   gint seqno);

void rc_packman_install_done (RCPackman *p,
                              RCPackmanOperationStatus status);

void rc_packman_package_removed (RCPackman *p,
                                 const gchar *filename,
                                 gint seqno);

void rc_packman_remove_done (RCPackman *p,
                             RCPackmanOperationStatus status);

/* Helper function to build RCPackageSList's easily */

RCPackageSList *rc_package_slist_add_package (RCPackageSList *pkgs,
                                              gchar *name, gchar *epoch,
                                              gchar *version, gchar *release,
                                              gboolean installed,
                                              guint32 installed_size);

#endif /* _RC_PACKMAN_H */
