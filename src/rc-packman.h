/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-packman.h
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _RC_PACKMAN_H
#define _RC_PACKMAN_H

#include <gtk/gtk.h>

#define RC_TYPE_PACKMAN            (rc_packman_get_type ())
#define RC_PACKMAN(obj)            (GTK_CHECK_CAST ((obj), \
                                    RC_TYPE_PACKMAN, RCPackman))
#define RC_PACKMAN_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                    RC_TYPE_PACKMAN, \
                                    RCPackmanClass))
#define RC_IS_PACKMAN(obj)         (GTK_CHECK_TYPE ((obj), \
                                    RC_TYPE_PACKMAN))
#define RC_IS_PACKMAN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
                                    RC_TYPE_PACKMAN))
#define RC_PACKMAN_GET_CLASS(obj)  (RC_PACKMAN_CLASS (GTK_OBJECT (obj)->klass))

typedef struct _RCPackman        RCPackman;
typedef struct _RCPackmanClass   RCPackmanClass;
    
typedef enum _RCPackmanError RCPackmanError;

enum _RCPackmanError {
    /* No error */
    RC_PACKMAN_ERROR_NONE = 0,
    /* The requested operation failed, but is non-fatal to program execution */
    RC_PACKMAN_ERROR_ABORT,
    /* An error from which we cannot and should not attempt to recover */
    RC_PACKMAN_ERROR_FATAL,
};

typedef enum _RCPackmanFeatures RCPackmanFeatures;

enum _RCPackmanFeatures {
    RC_PACKMAN_FEATURE_PRE_CONFIG      = 1 << 0,
    RC_PACKMAN_FEATURE_POST_CONFIG     = 1 << 1,
    RC_PACKMAN_FEATURE_CONFIG_PROGRESS = 1 << 2,
    RC_PACKMAN_FEATURE_PKG_PROGRESS    = 1 << 3,
};

#include "rc-packman-private.h"
#include "rc-package.h"
#include "rc-verification.h"

struct _RCPackman {
    GtkObject parent;

    RCPackmanPrivate *priv;
};

struct _RCPackmanClass {
    GtkObjectClass parent_class;

    /* Signals */

    /* The basic idea for these is:
     *
     * _start
     *   total is the number of expected steps for the phase
     * _step
     *   tells you which step it's about to begin, and the name of the
     *   package being mangled
     * _progress
     *   for a given step, tells you some measure of the percentage
     *   complete (for example, amount bytes of total)
     * _done
     *   the phase is complete
     */

    void (*configure_start)(RCPackman *packman, gint total);
    void (*configure_step)(RCPackman *packman, gchar *name, gint seqno);
    void (*configure_progress)(RCPackman *packman, gint amount, gint total);
    void (*configure_done)(RCPackman *packman);

    /* In _step install is TRUE for a package being installed, FALSE
     * for a package being removed */

    void (*transact_start)(RCPackman *packman, gint total);
    void (*transact_step)(RCPackman *packman, gboolean install, gchar *name,
                         gint seqno);
    void (*transact_progress)(RCPackman *packman, gint amount, gint total);
    void (*transact_done)(RCPackman *packman);

    /* Virtual functions */

    void (*rc_packman_real_transact)(RCPackman *packman,
                                     RCPackageSList *install_packages,
                                     RCPackageSList *remove_packages);

    RCPackage *(*rc_packman_real_query)(RCPackman *packman,
                                        RCPackage *package);

    RCPackage *(*rc_packman_real_query_file)(RCPackman *packman,
                                             const gchar *filename);

    RCPackageSList *(*rc_packman_real_query_all)(RCPackman *packman);

    gint (*rc_packman_real_version_compare)(RCPackman *packman,
                                            RCPackageSpec *spec1,
                                            RCPackageSpec *spec2);

    RCVerificationSList *(*rc_packman_real_verify)(RCPackman *packman,
                                                   RCPackage *package);

    RCPackage *(*rc_packman_real_find_file)(RCPackman *packman,
                                            const gchar *filename);
};

guint rc_packman_get_type (void);

RCPackman *rc_packman_new (void);

RCPackmanFeatures rc_packman_get_features (RCPackman *packman);

void rc_packman_transact (RCPackman *packman,
                          RCPackageSList *install_packages,
                          RCPackageSList *remove_packages);

RCPackage *rc_packman_query (RCPackman *packman, RCPackage *package);

RCPackageSList *rc_packman_query_list (RCPackman *packman,
                                       RCPackageSList *packages);

RCPackage *rc_packman_query_file (RCPackman *packman, const gchar *filename);

RCPackageSList *rc_packman_query_file_list (RCPackman *packman,
                                            GSList *filenames);

RCPackageSList *rc_packman_query_all (RCPackman *packman);

gint rc_packman_version_compare (RCPackman *packman,
                                 RCPackageSpec *spec1,
                                 RCPackageSpec *spec2);

RCVerificationSList *rc_packman_verify (RCPackman *packman,
                                        RCPackage *package);

RCPackage *rc_packman_find_file (RCPackman *packman, const gchar *filename);

const gchar *rc_packman_get_file_extension(RCPackman *packman);

guint rc_packman_get_error (RCPackman *packman);

const gchar *rc_packman_get_reason (RCPackman *packman);

void rc_packman_set_libdir (const gchar *libdir);

#endif /* _RC_PACKMAN_H */
