/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-packman.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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

#include <config.h>
#include "rc-packman.h"

#include <stdarg.h>
#include <ctype.h>
#include <sys/stat.h>
#include <unistd.h>

#include "rc-packman-private.h"

#include "rc-marshal.h"

RCPackman *das_global_packman = NULL;

static void rc_packman_class_init (RCPackmanClass *klass);
static void rc_packman_init       (RCPackman *obj);

static GObjectClass *rc_packman_parent;

enum SIGNALS {
    TRANSACT_START,
    TRANSACT_STEP,
    TRANSACT_PROGRESS,
    TRANSACT_DONE,
    DATABASE_CHANGED,
    DATABASE_LOCKED,
    DATABASE_UNLOCKED,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

GType
rc_packman_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCPackmanClass),
            NULL, NULL,
            (GClassInitFunc) rc_packman_class_init,
            NULL, NULL,
            sizeof (RCPackman),
            0,
            (GInstanceInitFunc) rc_packman_init
        };

        type = g_type_register_static (G_TYPE_OBJECT,
                                       "RCPackman",
                                       &type_info,
                                       0);
    }

    return type;
}

static void
rc_packman_finalize (GObject *obj)
{
    RCPackman *packman = RC_PACKMAN (obj);

    g_free (packman->priv->reason);

    g_free (packman->priv);

    if (rc_packman_parent->finalize)
        rc_packman_parent->finalize (obj);
}

static void
rc_packman_class_init (RCPackmanClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;

    object_class->finalize = rc_packman_finalize;

    rc_packman_parent = g_type_class_peek_parent (klass);

    signals[TRANSACT_START] =
        g_signal_new ("transact_start",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCPackmanClass, transact_start),
                      NULL, NULL,
                      rc_marshal_VOID__INT,
                      G_TYPE_NONE, 1,
                      G_TYPE_INT);

    signals[TRANSACT_STEP] =
        g_signal_new ("transact_step",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCPackmanClass, transact_step),
                      NULL, NULL,
                      rc_marshal_VOID__INT_INT_STRING,
                      G_TYPE_NONE, 3,
                      G_TYPE_INT,
                      G_TYPE_INT,
                      G_TYPE_STRING);

    signals[TRANSACT_PROGRESS] =
        g_signal_new ("transact_progress",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCPackmanClass, transact_progress),
                      NULL, NULL,
                      rc_marshal_VOID__LONG_LONG,
                      G_TYPE_NONE, 2,
                      G_TYPE_LONG,
                      G_TYPE_LONG);

    signals[TRANSACT_DONE] =
        g_signal_new ("transact_done",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
                      G_STRUCT_OFFSET (RCPackmanClass, transact_done),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[DATABASE_CHANGED] =
        g_signal_new ("database_changed",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCPackmanClass, database_changed),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[DATABASE_LOCKED] =
        g_signal_new ("database_locked",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCPackmanClass, database_locked),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[DATABASE_UNLOCKED] =
        g_signal_new ("database_unlocked",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (RCPackmanClass, database_unlocked),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
    
    /* Subclasses should provide real implementations of these functions, and
       we're just NULLing these for clarity (and paranoia!) */
    klass->rc_packman_real_transact = NULL;
    klass->rc_packman_real_query = NULL;
    klass->rc_packman_real_query_file = NULL;
    klass->rc_packman_real_query_all = NULL;
    klass->rc_packman_real_version_compare = NULL;
    klass->rc_packman_real_verify = NULL;
    klass->rc_packman_real_find_file = NULL;
    klass->rc_packman_real_lock = NULL;
    klass->rc_packman_real_unlock = NULL;
    klass->rc_packman_real_is_database_changed = NULL;
}

static void
rc_packman_init (RCPackman *packman)
{
    packman->priv = g_new (RCPackmanPrivate, 1);

    packman->priv->error = RC_PACKMAN_ERROR_NONE;
    packman->priv->reason = NULL;
    packman->priv->extension = NULL;

    packman->priv->busy = FALSE;

    packman->priv->capabilities = RC_PACKMAN_CAP_NONE;    

    packman->priv->lock_count = 0;
}

RCPackman *
rc_packman_new (void)
{
    RCPackman *packman =
        RC_PACKMAN (g_object_new (rc_packman_get_type (), NULL));

    return packman;
}

/* Wrappers around all of the virtual functions */

void
rc_packman_transact (RCPackman *packman, RCPackageSList *install_packages,
                     RCPackageSList *remove_packages, gboolean perform)
{
    RCPackmanClass *klass;
    RCPackageSList *iter;

    g_return_if_fail (packman);

    rc_packman_clear_error (packman);

    if (packman->priv->busy) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL, NULL);
        return;
    }

    /* Make sure that no entry in install_packages appears more than
     * once, and that no entry in install_packages is also in
     * remove_packages */

    for (iter = install_packages; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)iter->data;
        RCPackageSList *fpkg;

        fpkg = g_slist_find_custom (
            iter->next, pkg,
            (GCompareFunc) rc_package_spec_compare_name);

        if (fpkg) {
            rc_packman_set_error (
                packman, RC_PACKMAN_ERROR_ABORT,
                "multiple requests to install package '%s'",
                pkg->spec.name);
            return;
        }

        fpkg = g_slist_find_custom (
            remove_packages, pkg,
            (GCompareFunc) rc_package_spec_compare_name);

        if (fpkg) {
            rc_packman_set_error (
                packman, RC_PACKMAN_ERROR_ABORT,
                "requests to install and remove package '%s'",
                pkg->spec.name);
            return;
        }
    }

    /* Make sure that no entry in remove_packages appears more than
     * once */

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)iter->data;
        RCPackageSList *fpkg;

        fpkg = g_slist_find_custom (
            iter->next, pkg,
            (GCompareFunc) rc_package_spec_compare_name);

        if (fpkg) {
            rc_packman_set_error (
                packman, RC_PACKMAN_ERROR_ABORT,
                "multiple requests to remove package '%s'",
                pkg->spec.name);
            return;
        }
    }

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_transact);

    packman->priv->busy = TRUE;

    klass->rc_packman_real_transact (packman, install_packages,
                                     remove_packages, perform);

    packman->priv->busy = FALSE;
}

RCPackageSList *
rc_packman_query (RCPackman *packman, const char *name)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_query);

    return (klass->rc_packman_real_query (packman, name));
}

RCPackage *
rc_packman_query_file (RCPackman *packman, const gchar *filename)
{
    RCPackmanClass *klass;
    RCPackage *package;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_query_file);

    package = klass->rc_packman_real_query_file (packman, filename);

    if (package) {
        /* Get the file size if the rc_packman_real_query_file didn't do
           so already. */
        if (package->file_size == 0) {
            struct stat statbuf;
            if (stat (filename, &statbuf) == 0)
                package->file_size = statbuf.st_size;
        }

        /* Set the local package bit */
        package->local_package = TRUE;
    }

    return package;
}

RCPackageSList *
rc_packman_query_file_list (RCPackman *packman, GSList *filenames)
{
    GSList *iter;
    RCPackageSList *ret = NULL;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    for (iter = filenames; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);
        RCPackage *package;

        package = rc_packman_query_file (packman, filename);

        if (packman->priv->error) {
            rc_package_unref (package);
            return (ret);
        }

        ret = g_slist_append (ret, package);
    }

    return (ret);
}

RCPackageSList *
rc_packman_query_all (RCPackman *packman)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_query_all);

    return (klass->rc_packman_real_query_all (packman));
}

gint
rc_packman_version_compare (RCPackman *packman,
                            RCPackageSpec *spec1,
                            RCPackageSpec *spec2)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, 0);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_version_compare);

    return (klass->rc_packman_real_version_compare (packman, spec1, spec2));
}

RCVerificationSList *
rc_packman_verify (RCPackman *packman, RCPackage *package, guint32 type)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_verify);

    return (klass->rc_packman_real_verify (packman, package, type));
}

RCPackage *
rc_packman_find_file (RCPackman *packman, const gchar *filename)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_find_file);

    return (klass->rc_packman_real_find_file (packman, filename));
}

gboolean
rc_packman_is_locked (RCPackman *packman)
{
    g_return_val_if_fail (packman, FALSE);

    return packman->priv->lock_count > 0;
}

gboolean
rc_packman_lock (RCPackman *packman)
{
    RCPackmanClass *klass;
    gboolean success;

    g_return_val_if_fail (packman, FALSE);

    rc_packman_clear_error (packman);

    g_assert (packman->priv->lock_count >= 0);

    if (packman->priv->lock_count == 0) {
        klass = RC_PACKMAN_GET_CLASS (packman);
        g_assert (klass->rc_packman_real_lock);
        success = klass->rc_packman_real_lock (packman);

        if (success)
            g_signal_emit (packman, signals[DATABASE_LOCKED], 0);

    } else {
        success = TRUE;
    }

    if (success)
        ++packman->priv->lock_count;

    return success;
}

void
rc_packman_unlock (RCPackman *packman)
{
    RCPackmanClass *klass;

    g_return_if_fail (packman);

    rc_packman_clear_error (packman);

    g_assert (packman->priv->lock_count >= 0);

    if (packman->priv->lock_count == 0)
        return;

    if (packman->priv->lock_count == 1) {
        klass = RC_PACKMAN_GET_CLASS (packman);
        g_assert (klass->rc_packman_real_unlock);
        klass->rc_packman_real_unlock (packman);

        g_signal_emit (packman, signals[DATABASE_UNLOCKED], 0);
    }

    --packman->priv->lock_count;
}

gboolean
rc_packman_is_database_changed (RCPackman *packman)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, FALSE);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_is_database_changed);

    return (klass->rc_packman_real_is_database_changed (packman));
}

void
rc_packman_set_file_extension(RCPackman *packman, const gchar *extension)
{
    g_return_if_fail(packman);
    
    g_free(packman->priv->extension);
    packman->priv->extension = NULL;
    
    if (extension)
        packman->priv->extension = g_strdup(extension);
}

const gchar *
rc_packman_get_file_extension(RCPackman *packman)
{
    g_return_val_if_fail (packman, NULL);

    return packman->priv->extension;
}

void
rc_packman_set_capabilities(RCPackman *packman, const guint32 caps)
{
    g_return_if_fail(packman);
    packman->priv->capabilities = caps;
}

guint32
rc_packman_get_capabilities(RCPackman *packman)
{
    g_return_val_if_fail (packman, 0);

    return packman->priv->capabilities;
}

/* Methods to access the error stuff */

void
rc_packman_clear_error (RCPackman *packman)
{
    g_return_if_fail (packman);

    g_free (packman->priv->reason);

    packman->priv->error = RC_PACKMAN_ERROR_NONE;
    packman->priv->reason = NULL;
}

void
rc_packman_set_error (RCPackman *packman, RCPackmanError error,
                      const gchar *format, ...)
{
    va_list args;
    gchar *reason;

    g_return_if_fail (packman);
    g_return_if_fail (format);

    va_start (args, format);

    if (error > packman->priv->error) {
        packman->priv->error = error;
    }

    reason = g_strdup_vprintf (format, args);

    if (packman->priv->reason) {
        gchar *tmp = packman->priv->reason;

        packman->priv->reason = g_strconcat (reason, ": ", tmp, NULL);

        g_free (reason);
        g_free (tmp);
    } else {
        packman->priv->reason = reason;
    }

    va_end (args);
}

guint
rc_packman_get_error (RCPackman *packman)
{
    g_return_val_if_fail (packman, RC_PACKMAN_ERROR_FATAL);

    return (packman->priv->error);
}

const gchar *
rc_packman_get_reason (RCPackman *packman)
{
    g_return_val_if_fail (packman, "No packman object");

    if (packman->priv->reason) {
        return (packman->priv->reason);
    }

    return (NULL);
}

gint
rc_packman_generic_version_compare (RCPackageSpec *spec1,
                                    RCPackageSpec *spec2,
                                    int (*vercmp)(const char *, const char *))
{
    gint rc;

    g_assert (spec1);
    g_assert (spec2);

    rc = spec1->epoch - spec2->epoch;

    if (rc) {
        return (rc);
    }

    if (spec1->name || spec2->name) {
        rc = strcmp (spec1->name ? spec1->name : "",
                     spec2->name ? spec2->name : "");

        if (rc) {
            return (rc);
        }
    }

    if (spec1->version || spec2->version) {
        rc = vercmp (spec1->version ? spec1->version : "",
                     spec2->version ? spec2->version : "");

        if (rc) {
            return (rc);
        }
    }

    if (spec1->release || spec2->release) {
        rc = vercmp (spec1->release ? spec1->release : "",
                     spec2->release ? spec2->release : "");

        if (rc) {
            return (rc);
        }
    }

    return (0);
}

void
rc_packman_set_packman (RCPackman *packman)
{
    das_global_packman = packman;
}
