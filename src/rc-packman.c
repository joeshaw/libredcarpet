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

#include "rc-packman.h"
#include "rc-packman-private.h"

static void rc_packman_class_init (RCPackmanClass *klass);
static void rc_packman_init       (RCPackman *obj);

static GtkObjectClass *rc_packman_parent;

enum SIGNALS {
    CONFIGURE_PROGRESS,
    CONFIGURE_STEP,
    CONFIGURE_DONE,
    TRANSACTION_PROGRESS,
    TRANSACTION_STEP,
    TRANSACTION_DONE,
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
    RCPackman *hp = RC_PACKMAN (obj);

    g_free (hp->reason);

    if (GTK_OBJECT_CLASS(rc_packman_parent)->destroy)
        (* GTK_OBJECT_CLASS(rc_packman_parent)->destroy) (obj);
}

static void
rc_packman_class_init (RCPackmanClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;

    object_class->destroy = rc_packman_destroy;

    rc_packman_parent = gtk_type_class (gtk_object_get_type ());

    signals[CONFIGURE_PROGRESS] =
        gtk_signal_new ("configure_progress",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, configure_progress),
                        gtk_marshal_NONE__INT_INT,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_INT,
                        GTK_TYPE_INT);

    signals[CONFIGURE_STEP] =
        gtk_signal_new ("configure_step",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, configure_step),
                        gtk_marshal_NONE__INT_INT,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_INT,
                        GTK_TYPE_INT);

    signals[CONFIGURE_DONE] =
        gtk_signal_new ("configure_done",
                        GTK_RUN_LAST | GTK_RUN_NO_RECURSE,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, configure_done),
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);

    signals[TRANSACTION_PROGRESS] =
        gtk_signal_new ("transaction_progress",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass,
                                           transaction_progress),
                        gtk_marshal_NONE__INT_INT,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_INT,
                        GTK_TYPE_INT);

    signals[TRANSACTION_STEP] =
        gtk_signal_new ("transaction_step",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, transaction_step),
                        gtk_marshal_NONE__INT_INT,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_INT,
                        GTK_TYPE_INT);

    signals[TRANSACTION_DONE] =
        gtk_signal_new ("transaction_done",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, transaction_done),
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
}

static void
rc_packman_init (RCPackman *obj)
{
    obj->error = RC_PACKMAN_ERROR_NONE;

    obj->reason = NULL;

    obj->busy = FALSE;
}

RCPackman *
rc_packman_new (void)
{
    RCPackman *new =
        RC_PACKMAN (gtk_type_new (rc_packman_get_type ()));

    new->busy = FALSE;

    return new;
}

void
rc_packman_query_interface (RCPackman *p, gboolean *pre_config,
                            gboolean *pkg_progress, gboolean *post_config)
{
    if (pre_config) {
        *pre_config = p->pre_config;
    }
    if (pkg_progress) {
        *pkg_progress = p->pkg_progress;
    }
    if (post_config) {
        *post_config = p->post_config;
    }
}

/* Wrappers around all of the virtual functions */

void
rc_packman_transact (RCPackman *p, GSList *install_pkgs,
                     RCPackageSList *remove_pkgs)
{
    g_return_if_fail (p);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return;
    }

    g_assert (_CLASS (p)->rc_packman_real_transact);

    p->busy = TRUE;

    _CLASS (p)->rc_packman_real_transact (p, install_pkgs, remove_pkgs);

    p->busy = FALSE;
}

RCPackage *
rc_packman_query (RCPackman *p, RCPackage *pkg)
{
    RCPackage *ret = NULL;

    g_return_val_if_fail(p, NULL);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return (pkg);
    }

    g_assert (_CLASS (p)->rc_packman_real_query);

    p->busy = TRUE;

    ret = _CLASS (p)->rc_packman_real_query (p, pkg);

    p->busy = FALSE;

    return (ret);
}

RCPackageSList *
rc_packman_query_list (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter;

    g_return_val_if_fail(p, NULL);

    g_free (p->reason);
    p->error = RC_PACKMAN_ERROR_NONE;

    for (iter = pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        rc_packman_query (p, pkg);

        if (p->error) {
            return (pkgs);
        }
    }
    return pkgs;
}

RCPackage *
rc_packman_query_file (RCPackman *p, gchar *filename)
{
    RCPackage *ret = NULL;

    g_return_val_if_fail(p, NULL);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return (NULL);
    }

    g_assert (_CLASS (p)->rc_packman_real_query_file);

    p->busy = TRUE;

    ret =  _CLASS (p)->rc_packman_real_query_file (p, filename);

    p->busy = FALSE;

    return (ret);
}

RCPackageSList *
rc_packman_query_file_list (RCPackman *p, GSList *filenames)
{
    GSList *iter;
    RCPackageSList *ret = NULL;

    g_return_val_if_fail(p, NULL);

    g_free (p->reason);
    p->error = RC_PACKMAN_ERROR_NONE;

    for (iter = filenames; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);
        RCPackage *pkg;

        pkg = rc_packman_query_file (p, filename);

        if (p->error) {
            rc_package_free (pkg);
            return (ret);
        }

        ret = g_slist_append (ret, pkg);
    }

    return (ret);
}

RCPackageSList *
rc_packman_query_all (RCPackman *p)
{
    RCPackageSList *ret = NULL;

    g_return_val_if_fail(p, NULL);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return (NULL);
    }

    g_assert (_CLASS (p)->rc_packman_real_query_all);

    p->busy = TRUE;

    ret =  _CLASS (p)->rc_packman_real_query_all (p);

    p->busy = FALSE;

    return (ret);
}

gint
rc_packman_version_compare (RCPackman *p,
                            RCPackageSpec *s1,
                            RCPackageSpec *s2)
{
    g_return_val_if_fail(p, 0);

    g_assert (_CLASS (p)->rc_packman_real_version_compare);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    return (_CLASS (p)->rc_packman_real_version_compare (p, s1, s2));
}

gboolean
rc_packman_verify (RCPackman *p, RCPackage *pkg)
{
    gboolean ret = FALSE;

    g_return_val_if_fail(p, FALSE);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return (FALSE);
    }

    g_assert (_CLASS (p)->rc_packman_real_verify);

    p->busy = TRUE;

    ret = _CLASS (p)->rc_packman_real_verify (p, pkg);

    p->busy = FALSE;

    return (ret);
}

/* Public functions to emit signals */

void
rc_packman_configure_progress (RCPackman *p, gint amount, gint total)
{
    g_return_if_fail (p);
}

void
rc_packman_configure_step (RCPackman *p, gint seqno, gint total)
{
    g_return_if_fail (p);

    gtk_signal_emit ((GtkObject *)p, signals[CONFIGURE_STEP], seqno, total);
}

void
rc_packman_configure_done (RCPackman *p)
{
    g_return_if_fail (p);

    gtk_signal_emit ((GtkObject *)p, signals[CONFIGURE_DONE]);
}

void
rc_packman_transaction_progress (RCPackman *p, gint amount, gint total)
{
    g_return_if_fail (p);

    gtk_signal_emit ((GtkObject *)p, signals[TRANSACTION_PROGRESS],
                     amount, total);
}

void
rc_packman_transaction_step (RCPackman *p, gint seqno, gint total)
{
    g_return_if_fail (p);

    gtk_signal_emit ((GtkObject *)p, signals[TRANSACTION_STEP], seqno, total);
}

void
rc_packman_transaction_done (RCPackman *p)
{
    g_return_if_fail (p);

    gtk_signal_emit ((GtkObject *)p, signals[TRANSACTION_DONE]);
}

/* Methods to access the error stuff */

void
rc_packman_set_error (RCPackman *p, RCPackmanError error, const gchar *reason)
{
    g_return_if_fail(p);

    g_free (p->reason);

    p->error = error;
    p->reason = g_strdup (reason);
}

guint
rc_packman_get_error (RCPackman *p)
{
    g_return_val_if_fail(p, RC_PACKMAN_ERROR_FAIL);

    return (p->error);
}

gchar *
rc_packman_get_reason (RCPackman *p)
{
    g_return_val_if_fail(p, "No packman object");

    if (p->reason)
        return (p->reason);

    return ("");
}

/* Helper function to build RCPackageSList's easily */

RCPackageSList *
rc_package_slist_add_package (RCPackageSList *pkgs, gchar *name,
                              guint32 epoch, gchar *version, gchar *release,
                              gboolean installed, guint32 installed_size)
{
    RCPackage *pkg = rc_package_new ();

    rc_package_spec_init (RC_PACKAGE_SPEC (pkg), name, epoch, version, release,
                          installed, installed_size, 0, 0);

    pkgs = g_slist_append (pkgs, pkg);

    return (pkgs);
}
