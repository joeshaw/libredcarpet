/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-packman.h
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#include <glib-object.h>

#define RC_TYPE_PACKMAN            (rc_packman_get_type ())
#define RC_PACKMAN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    RC_TYPE_PACKMAN, RCPackman))
#define RC_PACKMAN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                    RC_TYPE_PACKMAN, \
                                    RCPackmanClass))
#define RC_IS_PACKMAN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    RC_TYPE_PACKMAN))
#define RC_IS_PACKMAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                    RC_TYPE_PACKMAN))
#define RC_PACKMAN_GET_CLASS(obj)  (RC_PACKMAN_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCPackman        RCPackman;
typedef struct _RCPackmanClass   RCPackmanClass;
typedef struct _RCPackmanPrivate RCPackmanPrivate;
    
typedef enum {
    /* No error */
    RC_PACKMAN_ERROR_NONE = 0,
    /* The requested operation failed, but is non-fatal to program execution */
    RC_PACKMAN_ERROR_ABORT,
    /* An error from which we cannot and should not attempt to recover */
    RC_PACKMAN_ERROR_FATAL,
} RCPackmanError;

typedef enum {
    RC_PACKMAN_STEP_UNKNOWN,
    RC_PACKMAN_STEP_PREPARE,
    RC_PACKMAN_STEP_INSTALL,
    RC_PACKMAN_STEP_REMOVE,
    RC_PACKMAN_STEP_CONFIGURE,
} RCPackmanStep;

#define RC_PACKMAN_CAP_NONE                  (0)
#define RC_PACKMAN_CAP_PROVIDE_ALL_VERSIONS  (1 << 0)
#define RC_PACKMAN_CAP_SELF_CONFLICT         (1 << 1)
#define RC_PACKMAN_CAP_LEGACY_EPOCH_HANDLING (1 << 2)

#include "rc-package.h"
#include "rc-verification.h"


struct _RCPackman {
    GObject parent;

    RCPackmanPrivate *priv;
};

struct _RCPackmanClass {
    GObjectClass parent_class;

    void (*transact_start)(RCPackman *packman, gint total_steps);
    void (*transact_step)(RCPackman *packman, gint seqno, RCPackmanStep step,
                          gchar *name);
    void (*transact_progress)(RCPackman *packman, gulong amount, gulong total);
    void (*transact_done)(RCPackman *packman);

    void (*database_changed)(RCPackman *packman);
    void (*database_locked)(RCPackman *packman);
    void (*database_unlocked)(RCPackman *packman);

    /* Virtual functions */

    void (*rc_packman_real_transact)(RCPackman *packman,
                                     RCPackageSList *install_packages,
                                     RCPackageSList *remove_packages,
                                     gboolean perform);

    RCPackageSList *(*rc_packman_real_query)(RCPackman *packman,
                                             const char *name);

    RCPackage *(*rc_packman_real_query_file)(RCPackman *packman,
                                             const gchar *filename);

    RCPackageSList *(*rc_packman_real_query_all)(RCPackman *packman);

    gint (*rc_packman_real_version_compare)(RCPackman *packman,
                                            RCPackageSpec *spec1,
                                            RCPackageSpec *spec2);

    gboolean (*rc_packman_real_parse_version)(RCPackman   *packman,
                                              const gchar *input,
                                              gboolean    *has_epoch,
                                              guint32     *epoch,
                                              gchar      **version,
                                              gchar      **release);

    RCVerificationSList *(*rc_packman_real_verify)(RCPackman *packman,
                                                   RCPackage *package,
                                                   guint32    type);

    RCPackage *(*rc_packman_real_find_file)(RCPackman *packman,
                                            const gchar *filename);

    gboolean (*rc_packman_real_lock)(RCPackman *packman);

    void (*rc_packman_real_unlock)(RCPackman *packman);

    gboolean (*rc_packman_real_is_database_changed)(RCPackman *packman);
};

GType rc_packman_get_type (void);

RCPackman *rc_packman_new (void);

void rc_packman_transact (RCPackman *packman,
                          RCPackageSList *install_packages,
                          RCPackageSList *remove_packages,
                          gboolean perform);

RCPackageSList *rc_packman_query (RCPackman *packman, const char *name);

RCPackage *rc_packman_query_file (RCPackman *packman, const gchar *filename);

RCPackageSList *rc_packman_query_file_list (RCPackman *packman,
                                            GSList *filenames);

RCPackageSList *rc_packman_query_all (RCPackman *packman);

gint rc_packman_version_compare (RCPackman *packman,
                                 RCPackageSpec *spec1,
                                 RCPackageSpec *spec2);

gboolean rc_packman_parse_version (RCPackman   *packman,
                                   const gchar *input,
                                   gboolean    *has_epoch,
                                   guint32     *epoch,
                                   gchar      **version,
                                   gchar      **release);

RCVerificationSList *rc_packman_verify (RCPackman *packman,
                                        RCPackage *package,
                                        guint32    type);

RCPackage *rc_packman_find_file (RCPackman *packman, const gchar *filename);

gboolean rc_packman_is_locked (RCPackman *packman);

gboolean rc_packman_lock (RCPackman *packman);

void rc_packman_unlock (RCPackman *packman);

gboolean rc_packman_is_database_changed (RCPackman *packman);

const gchar *rc_packman_get_file_extension(RCPackman *packman);

guint32 rc_packman_get_capabilities(RCPackman *packman);

guint rc_packman_get_error (RCPackman *packman);

const gchar *rc_packman_get_reason (RCPackman *packman);

void rc_packman_set_packman (RCPackman *packman);

#endif /* _RC_PACKMAN_H */
