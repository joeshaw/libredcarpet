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

#include "rc-packman.h"
#include "rc-packman-private.h"

static void rc_packman_class_init (RCPackmanClass *klass);
static void rc_packman_init       (RCPackman *obj);

static GtkObjectClass *rc_packman_parent;

enum SIGNALS {
    PKG_PROGRESS,
    PKG_INSTALLED,
    INSTALL_DONE,
    PKG_REMOVED,
    REMOVE_DONE,
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

void
rc_packman_set_error (RCPackman *p, RCPackmanError error, const gchar *reason)
{
    g_free (p->reason);

    p->error = error;
    p->reason = g_strdup (reason);
}

static void
rc_packman_fake_install (RCPackman *p, GSList *filenames)
{
    gtk_signal_emit ((GtkObject *)p, signals[INSTALL_DONE],
                     RC_PACKMAN_ABORT);

    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);
}

static void
rc_packman_fake_remove (RCPackman *p, RCPackageSList *pkgs)
{
    gtk_signal_emit ((GtkObject *)p, signals[REMOVE_DONE],
                     RC_PACKMAN_ABORT);

    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);
}

static RCPackage *
rc_packman_fake_query (RCPackman *p, RCPackage *pkg)
{
    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);

    return (pkg);
}

static RCPackage *
rc_packman_fake_query_file (RCPackman *p, gchar *filename)
{
    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);

    return (NULL);
}

static RCPackageSList *
rc_packman_fake_query_all (RCPackman *p)
{
    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);

    return (NULL);
}

static gint
rc_packman_fake_version_compare (RCPackman *p,
                                 RCPackageSpec *s1,
                                 RCPackageSpec *s2)
{
    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);

    return 0;
}

static gboolean
rc_packman_fake_verify (RCPackman *p, RCPackage *d)
{
    rc_packman_set_error (p, RC_PACKMAN_NO_BACKEND, NULL);

    return (TRUE);
}

#define gtk_marshal_NONE__STRING_INT_INT gtk_marshal_NONE__POINTER_INT_INT
#define gtk_marshal_NONE__INT_STRING     gtk_marshal_NONE__INT_POINTER
#define gtk_marshal_NONE__STRING_INT     gtk_marshal_NONE__POINTER_INT

static void
rc_packman_class_init (RCPackmanClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;

    object_class->destroy = rc_packman_destroy;

    rc_packman_parent = gtk_type_class (gtk_object_get_type ());

    signals[PKG_PROGRESS] =
        gtk_signal_new("pkg_progress",
                       GTK_RUN_LAST,
                       object_class->type,
                       GTK_SIGNAL_OFFSET(RCPackmanClass, pkg_progress),
                       gtk_marshal_NONE__STRING_INT_INT,
                       GTK_TYPE_NONE, 3,
                       GTK_TYPE_STRING,
                       GTK_TYPE_INT,
                       GTK_TYPE_INT);

    signals[PKG_INSTALLED] =
        gtk_signal_new ("pkg_installed",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, pkg_installed),
                        gtk_marshal_NONE__STRING_INT,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_STRING,
                        GTK_TYPE_INT);
    
    signals[INSTALL_DONE] =
        gtk_signal_new ("install_done",
                        GTK_RUN_LAST | GTK_RUN_NO_RECURSE,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, install_done),
                        gtk_marshal_NONE__INT,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_INT);
    
    signals[PKG_REMOVED] =
        gtk_signal_new ("pkg_removed",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, pkg_removed),
                        gtk_marshal_NONE__STRING_INT,
                        GTK_TYPE_NONE, 2,
                        GTK_TYPE_STRING,
                        GTK_TYPE_INT);

    signals[REMOVE_DONE] =
        gtk_signal_new ("remove_done",
                        GTK_RUN_LAST,
                        object_class->type,
                        GTK_SIGNAL_OFFSET (RCPackmanClass, remove_done),
                        gtk_marshal_NONE__INT,
                        GTK_TYPE_NONE, 1,
                        GTK_TYPE_INT);
    
    gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

    /* Subclasses should provide real implementations of these functions */
    klass->rc_packman_real_install = rc_packman_fake_install;
    klass->rc_packman_real_remove = rc_packman_fake_remove;
    klass->rc_packman_real_query = rc_packman_fake_query;
    klass->rc_packman_real_query_file = rc_packman_fake_query_file;
    klass->rc_packman_real_query_all = rc_packman_fake_query_all;
    klass->rc_packman_real_version_compare =
        rc_packman_fake_version_compare;
    klass->rc_packman_real_verify = rc_packman_fake_verify;
}

static void
rc_packman_init (RCPackman *obj)
{
    /* RCPackman *hp = RC_PACKMAN (obj); */
}

RCPackman *
rc_packman_new (void)
{
    RCPackman *new =
        RC_PACKMAN (gtk_type_new (rc_packman_get_type ()));

    new->status = RC_PACKMAN_IDLE;

    return new;
}

/* Wrappers around all of the virtual functions */

void
rc_packman_install (RCPackman *p, GSList *files)
{
    g_assert (_CLASS (p)->rc_packman_real_install);

    _CLASS (p)->rc_packman_real_install (p, files);
}

void
rc_packman_remove (RCPackman *p, RCPackageSList *pkgs)
{
    g_assert (_CLASS (p)->rc_packman_real_remove);

    _CLASS (p)->rc_packman_real_remove (p, pkgs);
}

RCPackage *
rc_packman_query (RCPackman *p, RCPackage *pkg)
{
    g_assert (_CLASS (p)->rc_packman_real_query);

    return (_CLASS (p)->rc_packman_real_query (p, pkg));
}

RCPackageSList *
rc_packman_query_list (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter;

    g_free (p->reason);
    p->error = RC_PACKMAN_NONE;

    for (iter = pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        rc_packman_query (p, pkg);

        if (p->error) {
            return (pkgs);
        }
    }
}

RCPackage *
rc_packman_query_file (RCPackman *p, gchar *filename)
{
    g_assert (_CLASS (p)->rc_packman_real_query_file);

    return (_CLASS (p)->rc_packman_real_query_file (p, filename));
}

RCPackageSList *
rc_packman_query_file_list (RCPackman *p, GSList *filenames)
{
    GSList *iter;
    RCPackageSList *ret = NULL;

    g_free (p->reason);
    p->error = RC_PACKMAN_NONE;

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
    g_assert (_CLASS (p)->rc_packman_real_query_all);

    return (_CLASS (p)->rc_packman_real_query_all (p));
}

gint
rc_packman_version_compare (RCPackman *p,
                            RCPackageSpec *s1,
                            RCPackageSpec *s2)
{
    g_assert (_CLASS (p)->rc_packman_real_version_compare);

    return (_CLASS (p)->rc_packman_real_version_compare (p, s1, s2));
}

gboolean
rc_packman_verify (RCPackman *p, RCPackage *pkg)
{
    g_assert (_CLASS (p)->rc_packman_real_verify);

    return (_CLASS (p)->rc_packman_real_verify (p, pkg));
}

/* Public functions to emit signals */

void
rc_packman_package_progress(RCPackman *p, const gchar *filename,
                            gint amount, gint total)
{
    gtk_signal_emit(
        GTK_OBJECT(p), signals[PKG_PROGRESS], filename, amount, total);
} /* rc_packman_package_progress */

void
rc_packman_package_installed (RCPackman *p,
                              const gchar *filename,
                              gint seqno)
{
    gtk_signal_emit ((GtkObject *)p, signals[PKG_INSTALLED], filename, seqno);
    /* Why is this here? --Joe */
    while (gtk_events_pending ()) {
	gtk_main_iteration ();
    }
}

void
rc_packman_install_done (RCPackman *p, RCPackmanOperationStatus status)
{
    gtk_signal_emit ((GtkObject *)p, signals[INSTALL_DONE], status);
}

void
rc_packman_package_removed (RCPackman *p,
                            const gchar *filename,
                            gint seqno)
{
    gtk_signal_emit ((GtkObject *)p, signals[PKG_REMOVED], filename, seqno);
}

void
rc_packman_remove_done (RCPackman *p, RCPackmanOperationStatus status)
{
    gtk_signal_emit ((GtkObject *)p, signals[REMOVE_DONE], status);
}

/* Methods to access the error stuff */

guint
rc_packman_get_error (RCPackman *p)
{
    return (p->error);
}

gchar *
rc_packman_get_reason (RCPackman *p)
{
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
