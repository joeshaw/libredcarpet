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
#include <errno.h>

#include "rc-marshal.h"
#include "rc-md5.h"
#include "rc-packman-private.h"

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

/* Some file #defines for transaction tracking */
#define RC_PACKMAN_TRACKING_DIR          "/var/lib/rcd/tracking"
#define RC_PACKMAN_TRACKING_XML          RC_PACKMAN_TRACKING_DIR"/tracking.xml"
#define RC_PACKMAN_TRACKING_CURRENT_DIR  RC_PACKMAN_TRACKING_DIR"/current"

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
    g_free (packman->priv->repackage_dir);

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
    klass->rc_packman_real_file_list = NULL;
}

static void
rc_packman_init (RCPackman *packman)
{
    packman->priv = g_new0 (RCPackmanPrivate, 1);

    packman->priv->error = RC_PACKMAN_ERROR_NONE;
    packman->priv->reason = NULL;

    packman->priv->extension = NULL;
    packman->priv->repackage_dir = NULL;

    packman->priv->capabilities = RC_PACKMAN_CAP_NONE;    

    packman->priv->lock_count = 0;
    packman->priv->transaction_tracking = FALSE;
}

RCPackman *
rc_packman_new (void)
{
    RCPackman *packman =
        RC_PACKMAN (g_object_new (rc_packman_get_type (), NULL));

    return packman;
}

typedef struct {
    RCPackman *packman;
    time_t timestamp;
    gboolean changes;
    xmlDoc *doc;
} TrackingInfo;

static void
tracking_info_free (TrackingInfo *tracking_info)
{
    if (!tracking_info)
        return;

    if (tracking_info->packman)
        g_object_unref (tracking_info->packman);

    if (tracking_info->doc)
        xmlFreeDoc (tracking_info->doc);

    g_free (tracking_info);
}

static char *
escape_pathname (const char *in_path)
{
    char *out_path;
    char **parts;

    parts = g_strsplit (in_path, "/", 0);
    out_path = g_strjoinv ("%2F", parts);
    g_strfreev (parts);

    return out_path;
}

static xmlNode *
file_changes_to_xml (TrackingInfo *tracking_info, RCPackage *package)
{
    xmlNode *changes_node = NULL;
    RCPackageFileSList *files, *iter;
    char *tmp;
    
    files = rc_packman_file_list (tracking_info->packman, package);

    for (iter = files; iter; iter = iter->next) {
        RCPackageFile *file = iter->data;
        struct stat st;
        xmlNode *file_node;
        gboolean was_removed = FALSE;

        file_node = xmlNewNode (NULL, "file");
        xmlNewProp (file_node, "filename", file->filename);

        errno = 0;
        if (stat (file->filename, &st) < 0) {
            if (errno == ENOENT) {
                xmlNewTextChild (file_node, NULL, "was_removed", "1");
                was_removed = TRUE;
            } else {
                rc_packman_set_error (tracking_info->packman,
                                      RC_PACKMAN_ERROR_ABORT,
                                      "Unable to stat '%s' in package '%s' "
                                      "for transaction tracking",
                                      file->filename,
                                      g_quark_to_string (package->spec.nameq));
                goto ERROR;
            }
        } else {
            /* Only care about size of regular files */
            if (S_ISREG (st.st_mode) && file->size != st.st_size) {
                tmp = g_strdup_printf ("%ld", st.st_size);
                xmlNewTextChild (file_node, NULL, "size", tmp);
                g_free (tmp);
            }

            if (file->uid != st.st_uid) {
                tmp = g_strdup_printf ("%d", st.st_uid);
                xmlNewTextChild (file_node, NULL, "uid", tmp);
                g_free (tmp);
            }

            if (file->gid != st.st_gid) {
                tmp = g_strdup_printf ("%d", st.st_gid);
                xmlNewTextChild (file_node, NULL, "gid", tmp);
                g_free (tmp);
            }

            if (file->mode != st.st_mode) {
                tmp = g_strdup_printf ("%d", st.st_mode);
                xmlNewTextChild (file_node, NULL, "mode", tmp);
                g_free (tmp);
            }

            /* Only care about mtime of regular files */
            if (S_ISREG (st.st_mode) && file->mtime != st.st_mtime) {
                tmp = g_strdup_printf ("%ld", st.st_mtime);
                xmlNewTextChild (file_node, NULL, "mtime", tmp);
                g_free (tmp);
            }

            /* Only md5sum regular files */
            if (S_ISREG (st.st_mode)) {
                tmp = rc_md5_digest (file->filename);
                if (strcmp (file->md5sum, tmp) != 0)
                    xmlNewTextChild (file_node, NULL, "md5sum", tmp);
                g_free (tmp);
            }
        }

        if (file_node->xmlChildrenNode) {
            if (!was_removed) {
                char *escapename;
                char *newfile;

                escapename = escape_pathname (file->filename);
                newfile = g_strconcat (RC_PACKMAN_TRACKING_CURRENT_DIR"/",
                                       escapename, NULL);
                g_free (escapename);

                if (rc_cp (file->filename, newfile) < 0) {
                    rc_packman_set_error (tracking_info->packman,
                                          RC_PACKMAN_ERROR_ABORT,
                                          "Unable to copy '%s' to '%s' for "
                                          "transaction tracking", 
                                          file->filename, newfile);
                    g_free (newfile);
                    goto ERROR;
                }

                g_free (newfile);

                tracking_info->changes = TRUE;
            }

            if (!changes_node)
                changes_node = xmlNewNode (NULL, "changes");

            xmlAddChild (changes_node, file_node);
        } else {
            xmlFreeNode (file_node);
        }
    }

    rc_package_file_slist_free (files);

    return changes_node;

ERROR:
    if (changes_node)
        xmlFreeNode (changes_node);

    rc_package_file_slist_free (files);

    return NULL;
}    

static void
add_tracked_package (TrackingInfo *tracking_info,
                     RCPackage  *old_package,
                     RCPackage  *new_package)
{
    xmlNode *root, *package_node;
    char *tmp;

    g_return_if_fail (tracking_info != NULL);
    g_return_if_fail (old_package != NULL || new_package != NULL);

    root = xmlDocGetRootElement (tracking_info->doc);
    package_node = xmlNewNode (NULL, "package");
    xmlAddChild (root, package_node);

    tmp = g_strdup_printf ("%ld", tracking_info->timestamp);
    xmlNewProp (package_node, "timestamp", tmp);
    g_free (tmp);

    xmlNewProp (package_node, "name",
                old_package ? g_quark_to_string (old_package->spec.nameq) : 
                g_quark_to_string (new_package->spec.nameq));

    if (old_package) {
        xmlNewProp (package_node, "old_version", old_package->spec.version);
        xmlNewProp (package_node, "old_release", old_package->spec.release);
    }

    if (new_package) {
        xmlNewProp (package_node, "new_version", new_package->spec.version);
        xmlNewProp (package_node, "new_release", new_package->spec.release);
    }

    if (old_package) {
        xmlNode *changes_node;

        changes_node = file_changes_to_xml (tracking_info, old_package);

        if (changes_node)
            xmlAddChild (package_node, changes_node);
    }

    return;
}        
    
static TrackingInfo *
rc_packman_track_transaction (RCPackman      *packman,
                              RCPackageSList *install_packages,
                              RCPackageSList *remove_packages)
{
    TrackingInfo *tracking_info = NULL;
    RCPackageSList *iter;

    if (!rc_file_exists (RC_PACKMAN_TRACKING_DIR)) {
        /*
         * Make the dir restrictive; we don't know what kind of files
         * will go in there.
         */
        if (rc_mkdir (RC_PACKMAN_TRACKING_DIR, 0700) < 0) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "Unable to create directory for "
                                  "transaction tracking "
                                  "'"RC_PACKMAN_TRACKING_DIR"'");
            goto ERROR;
        }
    }

    tracking_info = g_new0 (TrackingInfo, 1);
    tracking_info->packman = g_object_ref (packman);
    tracking_info->timestamp = time (NULL);

    if (!rc_file_exists (RC_PACKMAN_TRACKING_XML) ||
        !(tracking_info->doc = xmlParseFile (RC_PACKMAN_TRACKING_XML)))
    {
        xmlNode *root;

        tracking_info->doc = xmlNewDoc ("1.0");
        root = xmlNewNode (NULL, "transactions");
        xmlDocSetRootElement (tracking_info->doc, root);
    }

    if (rc_file_exists (RC_PACKMAN_TRACKING_CURRENT_DIR))
        rc_rmdir (RC_PACKMAN_TRACKING_CURRENT_DIR);

    if (!rc_mkdir (RC_PACKMAN_TRACKING_CURRENT_DIR, 0700) < 0) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "Unable to create tracking directory "
                              RC_PACKMAN_TRACKING_CURRENT_DIR);
        goto ERROR;
    }

    /*
     * First walk the list of packages to be installed, see if we're
     * doing an upgrade, get the list of files in that package and check
     * to see if the files have changed at all.
     */
    for (iter = install_packages; iter; iter = iter->next) {
        RCPackage *package_to_install = iter->data;
        RCPackageSList *system_packages;
        RCPackage *system_package = NULL;

        system_packages = rc_packman_query (
            packman,
            g_quark_to_string (package_to_install->spec.nameq));


        if (system_packages) {
            /*
             * FIXME: What do we do in the case when multiple packages by the
             * same name are installed?  Right now we just pick the first one,
             * which is probably not the right thing to do.
             *
             * I think that we'd probably want to track all versions and then
             * install all the versions when we rollback and apply the changes,
             * but we currently don't have a way of doing an install (as
             * opposed to an upgrade) to allow this.  (Obviously this is only
             * a problem for RPM systems; dpkg doesn't allow it)
             */
            system_package = system_packages->data;
        }

        add_tracked_package (tracking_info, system_package,
                             package_to_install);

        if (rc_packman_get_error (packman))
            goto ERROR;
    }

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package_to_remove = iter->data;

        add_tracked_package (tracking_info, package_to_remove, NULL);

        if (rc_packman_get_error (packman))
            goto ERROR;
    }

    return tracking_info;

ERROR:
    if (tracking_info)
        tracking_info_free (tracking_info);

    return NULL;
}

void
rc_packman_save_tracking_info (TrackingInfo *tracking_info)
{
    /* 
     * FIXME: How can we better handle errors here?  An error fucks any
     * ability of rollback
     */

    if (xmlSaveFormatFile (RC_PACKMAN_TRACKING_XML,
                           tracking_info->doc, 1) < 0) {
        rc_packman_set_error (tracking_info->packman, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open '%s' for writing; transaction "
                              "cannot be tracked");
        return;
    }

    if (tracking_info->changes) {
        char *tracking_dir = g_strdup_printf (RC_PACKMAN_TRACKING_DIR"/%ld",
                                              tracking_info->timestamp);

        if (rename (RC_PACKMAN_TRACKING_CURRENT_DIR, tracking_dir) < 0) {
            rc_packman_set_error (tracking_info->packman,
                                  RC_PACKMAN_ERROR_ABORT,
                                  "Unable to move %s to %s for transaction "
                                  "tracking",
                                  RC_PACKMAN_TRACKING_CURRENT_DIR,
                                  tracking_dir);
        }

        g_free (tracking_dir);
    }
    else
        rc_rmdir (RC_PACKMAN_TRACKING_CURRENT_DIR);
}

/* Wrappers around all of the virtual functions */

void
rc_packman_transact (RCPackman       *packman,
                     RCPackageSList  *install_packages,
                     RCPackageSList  *remove_packages,
                     int              flags)
{
    RCPackmanClass *klass;
    RCPackageSList *iter;
    TrackingInfo *tracking_info = NULL;

    g_return_if_fail (packman);

    rc_packman_clear_error (packman);

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
                g_quark_to_string (pkg->spec.nameq));
            return;
        }

        fpkg = g_slist_find_custom (
            remove_packages, pkg,
            (GCompareFunc) rc_package_spec_compare_name);

        if (fpkg) {
            rc_packman_set_error (
                packman, RC_PACKMAN_ERROR_ABORT,
                "requests to install and remove package '%s'",
                g_quark_to_string (pkg->spec.nameq));
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
                g_quark_to_string (pkg->spec.nameq));
            return;
        }
    }

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_transact);

    if (packman->priv->transaction_tracking) {
        tracking_info = rc_packman_track_transaction (packman,
                                                      install_packages,
                                                      remove_packages);
    }

    if (!rc_packman_get_error (packman)) {
        klass->rc_packman_real_transact (packman, install_packages,
                                         remove_packages, flags);
    }

    if (packman->priv->transaction_tracking) {
        if (!rc_packman_get_error (packman))
            rc_packman_save_tracking_info (tracking_info);
        else
            rc_rmdir (RC_PACKMAN_TRACKING_CURRENT_DIR);

        tracking_info_free (tracking_info);
    }
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

gboolean
rc_packman_parse_version (RCPackman    *packman,
                          const gchar  *input,
                          gboolean     *has_epoch,
                          guint32      *epoch,
                          char        **version,
                          char        **release)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, FALSE);
    g_return_val_if_fail (input, FALSE);
    g_return_val_if_fail (has_epoch, FALSE);
    g_return_val_if_fail (epoch, FALSE);
    g_return_val_if_fail (version, FALSE);
    g_return_val_if_fail (release, FALSE);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_parse_version);

    return (klass->rc_packman_real_parse_version (
                packman, input, has_epoch, epoch, version, release));
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

RCPackageFileSList *
rc_packman_file_list (RCPackman *packman, RCPackage *package)
{
    RCPackmanClass *klass;

    g_return_val_if_fail (packman, NULL);

    rc_packman_clear_error (packman);

    klass = RC_PACKMAN_GET_CLASS (packman);

    g_assert (klass->rc_packman_real_file_list);

    return klass->rc_packman_real_file_list (packman, package);
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

    if (spec1->nameq != spec2->nameq) {
        const char *one, *two;

        one = g_quark_to_string (spec1->nameq);
        two = g_quark_to_string (spec2->nameq);

        rc = strcmp (one ? one : "",
                     two ? two : "");

        if (rc) {
            return rc;
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
rc_packman_set_repackage_dir (RCPackman   *packman,
                              const gchar *repackage_dir)
{
    g_return_if_fail (packman);

    g_free (packman->priv->repackage_dir);
    packman->priv->repackage_dir = g_strdup (repackage_dir);
}

const gchar *
rc_packman_get_repackage_dir (RCPackman *packman)
{
    g_return_val_if_fail (packman, NULL);

    return packman->priv->repackage_dir;
}

void
rc_packman_set_transaction_tracking (RCPackman *packman, gboolean enabled)
{
    g_return_if_fail (packman);

    packman->priv->transaction_tracking = enabled;
}
