/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-packman.c
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

#include <gtk/gtk.h>
#include <stdarg.h>
#include <ctype.h>

#include "rc-packman.h"
#include "rc-packman-private.h"

gchar *rc_libdir = LIBDIR;

void
rc_packman_set_libdir (const gchar *libdir)
{
    rc_libdir = g_strdup (libdir);
}

static void rc_packman_class_init (RCPackmanClass *klass);
static void rc_packman_init       (RCPackman *obj);

static GtkObjectClass *rc_packman_parent;

enum SIGNALS {
    TRANSACT_START,
    TRANSACT_STEP,
    TRANSACT_PROGRESS,
    TRANSACT_DONE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

guint
rc_packman_get_type (void)
{
    static guint type = 0;

    if (!type) {
        GtkTypeInfo type_info = {
            "RCPackman",
            sizeof (RCPackman),
            sizeof (RCPackmanClass),
            (GtkClassInitFunc) rc_packman_class_init,
            (GtkObjectInitFunc) rc_packman_init,
            (GtkArgSetFunc) NULL,
            (GtkArgGetFunc) NULL,
        };

        type = gtk_type_unique (gtk_object_get_type (), &type_info);
    }

    return type;
}

static void
rc_packman_destroy (GtkObject *obj)
{
    RCPackman *packman = RC_PACKMAN (obj);

    g_free (packman->priv->reason);

    g_free (packman->priv);

    if (GTK_OBJECT_CLASS(rc_packman_parent)->destroy)
        (* GTK_OBJECT_CLASS(rc_packman_parent)->destroy) (obj);
}

typedef void (*GtkSignal_NONE__LONG_LONG) (GtkObject *object, 
                                           glong arg1,
                                           glong arg2,
                                           gpointer user_data);
static void gtk_marshal_NONE__LONG_LONG (GtkObject    *object, 
                                         GtkSignalFunc func, 
                                         gpointer      func_data, 
                                         GtkArg       *args)
{
    GtkSignal_NONE__LONG_LONG rfunc;
    rfunc = (GtkSignal_NONE__LONG_LONG) func;
    (* rfunc) (object,
               GTK_VALUE_LONG(args[0]),
               GTK_VALUE_LONG(args[1]),
               func_data);
}

static void
rc_packman_class_init (RCPackmanClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;

    object_class->destroy = rc_packman_destroy;

    rc_packman_parent = gtk_type_class (gtk_object_get_type ());

    signals[TRANSACT_START] =
        gtk_signal_new ("transact_start",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, transact_start),
                        gtk_marshal_NONE__INT,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_INT);

    signals[TRANSACT_STEP] =
        gtk_signal_new ("transact_step",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, transact_step),
                        gtk_marshal_NONE__INT_INT_POINTER,
                        GTK_TYPE_NONE, 3,
                        GTK_TYPE_INT,
                        GTK_TYPE_INT,
                        GTK_TYPE_STRING);

    signals[TRANSACT_PROGRESS] =
        gtk_signal_new ("transact_progress",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, transact_progress),
                        gtk_marshal_NONE__LONG_LONG,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_LONG,
                        GTK_TYPE_LONG);

    signals[TRANSACT_DONE] =
        gtk_signal_new ("transact_done",
                        GTK_RUN_LAST | GTK_RUN_NO_RECURSE,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, transact_done),
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);
    
    gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

    /* Subclasses should provide real implementations of these functions, and
       we're just NULLing these for clarity (and paranoia!) */
    klass->rc_packman_real_transact = NULL;
    klass->rc_packman_real_query = NULL;
    klass->rc_packman_real_query_file = NULL;
    klass->rc_packman_real_query_all = NULL;
    klass->rc_packman_real_version_compare = NULL;
    klass->rc_packman_real_verify = NULL;
    klass->rc_packman_real_find_file = NULL;
}

static void
rc_packman_init (RCPackman *packman)
{
    packman->priv = g_new (RCPackmanPrivate, 1);

    packman->priv->error = RC_PACKMAN_ERROR_NONE;
    packman->priv->reason = NULL;
    packman->priv->extension = NULL;

    packman->priv->busy = FALSE;
}

RCPackman *
rc_packman_new (void)
{
    RCPackman *packman =
        RC_PACKMAN (gtk_type_new (rc_packman_get_type ()));

    return packman;
}

/* Wrappers around all of the virtual functions */

void
rc_packman_transact (RCPackman *packman, RCPackageSList *install_packages,
                     RCPackageSList *remove_packages)
{
    RCPackmanClass *klass;

    g_return_if_fail (packman);

    rc_packman_clear_error (packman);

    if (packman->priv->busy) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL, NULL);
        return;
    }

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_transact);

    packman->priv->busy = TRUE;

    klass->rc_packman_real_transact (packman, install_packages,
                                     remove_packages);

    packman->priv->busy = FALSE;
}

RCPackage *
rc_packman_query (RCPackman *packman, RCPackage *package)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_query);

    return (klass->rc_packman_real_query (packman, package));
}

RCPackageSList *
rc_packman_query_list (RCPackman *packman, RCPackageSList *packages)
{
    RCPackageSList *iter;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        rc_packman_query (packman, package);

        if (rc_packman_get_error (packman)) {
            return (packages);
        }
    }

    return (packages);
}

RCPackage *
rc_packman_query_file (RCPackman *packman, const gchar *filename)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_query_file);

    return (klass->rc_packman_real_query_file (packman, filename));
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
            rc_package_free (package);
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
rc_packman_verify (RCPackman *packman, RCPackage *package)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_verify);

    return (klass->rc_packman_real_verify (packman, package));
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
