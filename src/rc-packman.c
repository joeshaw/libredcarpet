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
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);
    
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
                        gtk_marshal_NONE__NONE,
                        GTK_TYPE_NONE, 0);
    
    gtk_object_class_add_signals (object_class, signals, LAST_SIGNAL);

    /* Subclasses should provide real implementations of these functions, and
       we're just NULLing these for clarity (and paranoia!) */
    klass->rc_packman_real_install = NULL;
    klass->rc_packman_real_remove = NULL;
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

/* Wrappers around all of the virtual functions */

void
rc_packman_install (RCPackman *p, GSList *files)
{
    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return;
    }

    g_assert (_CLASS (p)->rc_packman_real_install);

    p->busy = TRUE;

    _CLASS (p)->rc_packman_real_install (p, files);

    p->busy = FALSE;
}

void
rc_packman_remove (RCPackman *p, RCPackageSList *pkgs)
{
    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if (p->busy) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_BUSY, NULL);
        return;
    }

    g_assert (_CLASS (p)->rc_packman_real_remove);

    p->busy = TRUE;

    _CLASS (p)->rc_packman_real_remove (p, pkgs);

    p->busy = FALSE;
}

RCPackage *
rc_packman_query (RCPackman *p, RCPackage *pkg)
{
    RCPackage *ret = NULL;

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
    g_assert (_CLASS (p)->rc_packman_real_version_compare);

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    return (_CLASS (p)->rc_packman_real_version_compare (p, s1, s2));
}

gboolean
rc_packman_verify (RCPackman *p, RCPackage *pkg)
{
    gboolean ret = FALSE;

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
rc_packman_install_done (RCPackman *p)
{
    gtk_signal_emit ((GtkObject *)p, signals[INSTALL_DONE]);
}

void
rc_packman_package_removed (RCPackman *p,
                            const gchar *filename,
                            gint seqno)
{
    gtk_signal_emit ((GtkObject *)p, signals[PKG_REMOVED], filename, seqno);
}

void
rc_packman_remove_done (RCPackman *p)
{
    gtk_signal_emit ((GtkObject *)p, signals[REMOVE_DONE]);
}

/* Methods to access the error stuff */

void
rc_packman_set_error (RCPackman *p, RCPackmanError error, const gchar *reason)
{
    g_free (p->reason);

    p->error = error;
    p->reason = g_strdup (reason);
}

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
