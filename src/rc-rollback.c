/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rollback.c
 *
 * Copyright (C) 2003 Ximian, Inc.
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
#include "rc-rollback.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#include "rc-debug.h"
#include "rc-md5.h"
#include "rc-packman.h"
#include "rc-util.h"
#include "rc-world.h"
#include "xml-util.h"

/* Some file #defines for transaction tracking */
#define RC_ROLLBACK_DIR          "/var/lib/rcd/rollback"
#define RC_ROLLBACK_XML          RC_ROLLBACK_DIR"/rollback.xml"
#define RC_ROLLBACK_CURRENT_DIR  RC_ROLLBACK_DIR"/current-transaction"

struct _RCRollbackInfo {
    time_t timestamp;
    gboolean changes;
    xmlDoc *doc;
};

void
rc_rollback_info_free (RCRollbackInfo *rollback_info)
{
    if (!rollback_info)
        return;

    if (rollback_info->doc)
        xmlFreeDoc (rollback_info->doc);

    g_free (rollback_info);
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
file_changes_to_xml (RCRollbackInfo *rollback_info,
                     RCPackage *package,
                     GError **err)
{
    RCPackman *packman = rc_packman_get_global ();
    xmlNode *changes_node = NULL;
    RCPackageFileSList *files, *iter;
    char *tmp;
    
    files = rc_packman_file_list (packman, package);

    if (rc_packman_get_error (packman)) {
        g_set_error (err, RC_ERROR, RC_ERROR,
                     "Can't get file changes for rollback: %s",
                     rc_packman_get_reason (packman));
        goto ERROR;
    }

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
                g_set_error (err, RC_ERROR, RC_ERROR,
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
            if (!was_removed && S_ISREG (st.st_mode)) {
                char *escapename;
                char *newfile;

                escapename = escape_pathname (file->filename);
                newfile = g_strconcat (RC_ROLLBACK_CURRENT_DIR"/",
                                       escapename, NULL);
                g_free (escapename);

                if (rc_cp (file->filename, newfile) < 0) {
                    g_set_error (err, RC_ERROR, RC_ERROR,
                                 "Unable to copy '%s' to '%s' for "
                                 "transaction tracking", 
                                 file->filename, newfile);
                    g_free (newfile);
                    goto ERROR;
                }

                g_free (newfile);

                rollback_info->changes = TRUE;
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
add_tracked_package (RCRollbackInfo *rollback_info,
                     RCPackage  *old_package,
                     RCPackage  *new_package,
                     GError **err)
{
    xmlNode *root, *package_node;
    char *tmp;

    g_return_if_fail (rollback_info != NULL);
    g_return_if_fail (old_package != NULL || new_package != NULL);

    root = xmlDocGetRootElement (rollback_info->doc);
    package_node = xmlNewNode (NULL, "package");
    xmlAddChild (root, package_node);

    tmp = g_strdup_printf ("%ld", rollback_info->timestamp);
    xmlNewProp (package_node, "timestamp", tmp);
    g_free (tmp);

    xmlNewProp (package_node, "name",
                old_package ? g_quark_to_string (old_package->spec.nameq) : 
                g_quark_to_string (new_package->spec.nameq));

    if (old_package) {
        if (old_package->spec.has_epoch) {
            tmp = g_strdup_printf ("%d", old_package->spec.epoch);
            xmlNewProp (package_node, "old_epoch", tmp);
            g_free (tmp);
        }

        xmlNewProp (package_node, "old_version", old_package->spec.version);
        xmlNewProp (package_node, "old_release", old_package->spec.release);
    }

    if (new_package) {
        if (new_package->spec.has_epoch) {
            tmp = g_strdup_printf ("%d", new_package->spec.epoch);
            xmlNewProp (package_node, "new_epoch", tmp);
            g_free (tmp);
        }

        xmlNewProp (package_node, "new_version", new_package->spec.version);
        xmlNewProp (package_node, "new_release", new_package->spec.release);
    }

    if (old_package && !rc_package_is_synthetic (old_package)) {
        xmlNode *changes_node;
        GError *tmp_error = NULL;

        changes_node = file_changes_to_xml (rollback_info, old_package,
                                            &tmp_error);

        if (changes_node)
            xmlAddChild (package_node, changes_node);

        if (tmp_error)
            g_propagate_error (err, tmp_error);
    }

    return;
}        

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
static gboolean
foreach_package_cb (RCPackage *package, gpointer user_data)
{
    RCPackage **system_package = user_data;

    *system_package = package;

    /* Short-circuit the foreach; see the FIXME above. */
    return FALSE;
}
    
RCRollbackInfo *
rc_rollback_info_new (RCWorld         *world,
                      RCPackageSList  *install_packages,
                      RCPackageSList  *remove_packages,
                      GError         **err)
{
    RCRollbackInfo *rollback_info = NULL;
    RCPackageSList *iter;

    if (!rc_file_exists (RC_ROLLBACK_DIR)) {
        /*
         * Make the dir restrictive; we don't know what kind of files
         * will go in there.
         */
        if (rc_mkdir (RC_ROLLBACK_DIR, 0700) < 0) {
            g_set_error (err, RC_ERROR, RC_ERROR,
                         "Unable to create directory for transaction "
                         "tracking: '"RC_ROLLBACK_DIR"'");
            goto ERROR;
        }
    }

    rollback_info = g_new0 (RCRollbackInfo, 1);
    rollback_info->timestamp = time (NULL);

    if (!rc_file_exists (RC_ROLLBACK_XML) ||
        !(rollback_info->doc = xmlParseFile (RC_ROLLBACK_XML)))
    {
        xmlNode *root;

        rollback_info->doc = xmlNewDoc ("1.0");
        root = xmlNewNode (NULL, "transactions");
        xmlDocSetRootElement (rollback_info->doc, root);
    }

    if (rc_file_exists (RC_ROLLBACK_CURRENT_DIR))
        rc_rmdir (RC_ROLLBACK_CURRENT_DIR);

    if (!rc_mkdir (RC_ROLLBACK_CURRENT_DIR, 0700) < 0) {
        g_set_error (err, RC_ERROR, RC_ERROR,
                     "Unable to create tracking directory "
                     "'"RC_ROLLBACK_CURRENT_DIR"'");
        goto ERROR;
    }

    /*
     * First walk the list of packages to be installed, see if we're
     * doing an upgrade, get the list of files in that package and check
     * to see if the files have changed at all.
     */
    for (iter = install_packages; iter; iter = iter->next) {
        RCPackage *package_to_install = iter->data;
        RCPackage *system_package = NULL;
        GError *tmp_error = NULL;

        rc_world_foreach_package_by_name (world,
                                          g_quark_to_string (package_to_install->spec.nameq),
                                          RC_CHANNEL_SYSTEM,
                                          foreach_package_cb,
                                          &system_package);


        add_tracked_package (rollback_info, system_package,
                             package_to_install, &tmp_error);

        if (tmp_error) {
            g_propagate_error (err, tmp_error);
            goto ERROR;
        }
    }

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package_to_remove = iter->data;
        GError *tmp_error = NULL;

        add_tracked_package (rollback_info, package_to_remove,
                             NULL, &tmp_error);

        if (tmp_error) {
            g_propagate_error (err, tmp_error);
            goto ERROR;
        }
    }

    return rollback_info;

ERROR:
    if (rollback_info)
        rc_rollback_info_free (rollback_info);

    return NULL;
}

static void
strip_whitespace_node_recursive (xmlNode *root)
{
    xmlNode *node, *next;

    for (node = root->xmlChildrenNode; node; node = next) {
        next = node->next;

        if (xmlIsBlankNode (node)) {
            xmlUnlinkNode (node);
            xmlFreeNode (node);
        }
        else
            strip_whitespace_node_recursive (node);
    }
}

void
rc_rollback_info_save (RCRollbackInfo *rollback_info)
{
    /* 
     * FIXME: How can we better handle errors here?  An error fucks any
     * ability of rollback
     */

    /*
     * Strip out whitespace so xmlSaveFormatFile() saves nicely.
     */
    strip_whitespace_node_recursive (
        xmlDocGetRootElement (rollback_info->doc));

    if (xmlSaveFormatFile (RC_ROLLBACK_XML,
                           rollback_info->doc, 1) < 0) {
        rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                  "Unable to open '"RC_ROLLBACK_XML"' for writing; "
                  "transaction cannot be tracked");
        return;
    }

    if (rollback_info->changes) {
        char *tracking_dir = g_strdup_printf (RC_ROLLBACK_DIR"/%ld",
                                              rollback_info->timestamp);

        if (rename (RC_ROLLBACK_CURRENT_DIR, tracking_dir) < 0) {
            rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                      "Unable to move %s to %s for transaction tracking",
                      RC_ROLLBACK_CURRENT_DIR,
                      tracking_dir);
        }

        g_free (tracking_dir);
    }
    else
        rc_rmdir (RC_ROLLBACK_CURRENT_DIR);
}

void
rc_rollback_info_discard (RCRollbackInfo *rollback_info)
{
    rc_rmdir (RC_ROLLBACK_CURRENT_DIR);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

typedef struct {
    char *filename;
    gboolean was_removed;
    uid_t uid;
    gid_t gid;
    mode_t mode;
} FileChange;

struct _RCRollbackAction {
    gboolean is_install;

    time_t timestamp;

    RCPackage       *package;
    RCPackageUpdate *update;

    GSList *file_changes;
};

void
rc_rollback_action_free (RCRollbackAction *action)
{
    if (action->package)
        rc_package_unref (action->package);

    if (action->update)
        rc_package_update_free (action->update);

    g_free (action);
}

void
rc_rollback_action_slist_free (RCRollbackActionSList *actions)
{
    g_slist_foreach (actions, (GFunc) rc_rollback_action_free, NULL);
    g_slist_free (actions);
}

gboolean
rc_rollback_action_is_install (RCRollbackAction *action)
{
    g_return_val_if_fail (action != NULL, FALSE);

    return action->is_install;
}

RCPackage *
rc_rollback_action_get_package (RCRollbackAction *action)
{
    g_return_val_if_fail (action != NULL, NULL);

    return action->package;
}

RCPackageUpdate *
rc_rollback_action_get_package_update (RCRollbackAction *action)
{
    g_return_val_if_fail (action != NULL, NULL);

    return action->update;
}

typedef struct {
    RCPackman *packman;
    RCPackageDep *dep_to_match;

    RCPackage *matching_package;
    RCPackageUpdate *matching_update;
} PackageMatchInfo;

static gboolean
package_match_cb (RCPackage *package, gpointer user_data)
{
    PackageMatchInfo *pmi = user_data;
    RCPackageUpdateSList *iter;

    if (pmi->matching_package) {
        /* Already found a match, we don't care anymore */
        return TRUE;
    }

    /* Make sure this is the package we're looking for */
    if (RC_PACKAGE_SPEC (package)->nameq !=
        RC_PACKAGE_SPEC (pmi->dep_to_match)->nameq)
        return TRUE;

    for (iter = package->history; iter; iter = iter->next) {
        RCPackageUpdate *update = iter->data;
        RCPackageDep *update_dep;
        gboolean match;

        update_dep = rc_package_dep_new_from_spec (RC_PACKAGE_SPEC (update),
                                                   RC_RELATION_EQUAL,
                                                   RC_CHANNEL_ANY,
                                                   FALSE, FALSE);

        match = rc_package_dep_verify_relation (pmi->packman,
                                                pmi->dep_to_match,
                                                update_dep);

        rc_package_dep_unref (update_dep);

        if (match) {
            pmi->matching_package = package;
            pmi->matching_update = update;
            return TRUE;
        }
    }

    return TRUE;
}

static GSList *
get_file_changes (xmlNode *changes_node)
{
    GSList *changes = NULL;
    xmlNode *iter;

    for (iter = changes_node->xmlChildrenNode; iter; iter = iter->next) {
        FileChange *item;
        char *tmp;

        /* Skip comments and text regions and any non-file node */
        if (iter->type != XML_ELEMENT_NODE ||
            g_strcasecmp (iter->name, "file") != 0)
        {
            continue;
        }

        item = g_new0 (FileChange, 1);

        item->filename = xml_get_prop (iter, "filename");
        tmp = xml_get_value (iter, "was_removed");
        if (tmp)
            item->was_removed = TRUE;
        g_free (tmp);

        item->uid = -1;
        item->gid = -1;
        item->mode = -1;

        if (!item->was_removed) {
            tmp = xml_get_value (iter, "uid");
            if (tmp)
                item->uid = atoi (tmp);
            g_free (tmp);

            tmp = xml_get_value (iter, "gid");
            if (tmp)
                item->gid = atoi (tmp);
            g_free (tmp);

            tmp = xml_get_value (iter, "mode");
            if (tmp)
                item->mode = atoi (tmp);
            g_free (tmp);
        }

        changes = g_slist_prepend (changes, item);
    }

    return changes;
}

static RCRollbackAction *
get_action_from_xml_node (xmlNode    *node,
                          time_t      trans_time,
                          GHashTable *action_hash)
{
    RCWorld *world = rc_get_world ();
    char *name, *epoch, *version, *release;
    RCRollbackAction *action, *old_action;
    PackageMatchInfo pmi;
    xmlNode *changes_node;

    name = xml_get_prop (node, "name");
    if (!name) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "No package name available in rollback db");
        return NULL;
    }

    old_action = g_hash_table_lookup (action_hash, name);

    if (old_action) {
        if (old_action->timestamp > trans_time) {
            g_hash_table_remove (action_hash, name);
            rc_rollback_action_free (old_action);
        }
        else
            return NULL;
    }

    version = xml_get_prop (node, "old_version");
    if (!version) {
        /* This was an install and has to be removed */

        RCPackage *package = rc_world_get_package (world,
                                                   RC_CHANNEL_SYSTEM,
                                                   name);

        action = g_new0 (RCRollbackAction, 1);
        action->is_install = FALSE;
        action->timestamp = trans_time;
        action->package = rc_package_ref (package);
        action->update = NULL;
        g_hash_table_insert (action_hash, name, action);

        return action;
    }

    epoch = xml_get_prop (node, "old_epoch");
    release = xml_get_prop (node, "old_release");

    pmi.packman = rc_packman_get_global ();
    pmi.dep_to_match = rc_package_dep_new (name,
                                           epoch != NULL ? TRUE : FALSE,
                                           epoch != NULL ? atoi (epoch) : 0,
                                           version, release,
                                           RC_RELATION_EQUAL,
                                           RC_CHANNEL_ANY,
                                           FALSE, FALSE);
    pmi.matching_package = NULL;
    pmi.matching_update = NULL;

    rc_world_foreach_package (world, RC_CHANNEL_NON_SYSTEM,
                              package_match_cb, &pmi);

    rc_package_dep_unref (pmi.dep_to_match);

    if (!pmi.matching_package) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Unable to find a matching package for %s %s-%s",
                  name, version, release);
        return NULL;
    }

    action = g_new0 (RCRollbackAction, 1);
    action->is_install = TRUE;
    action->timestamp = trans_time;
    action->package = rc_package_ref (pmi.matching_package);
    action->update = rc_package_update_copy (pmi.matching_update);

    changes_node = xml_get_node (node, "changes");
    if (changes_node)
        action->file_changes = get_file_changes (changes_node);
    else
        action->file_changes = NULL;

    g_hash_table_insert (action_hash, name, action);

    return action;
}

RCRollbackActionSList *
rc_rollback_get_actions (time_t when)
{
    RCRollbackActionSList *actions, *iter, *next;
    xmlDoc *doc;
    xmlNode *root, *node;
    GHashTable *action_hash;

    /* File doesn't exist yet; that's ok. */
    if (!rc_file_exists (RC_ROLLBACK_XML))
        return NULL;

    doc = xmlParseFile (RC_ROLLBACK_XML);
    if (!doc) {
        rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                  "Unable to parse rollback XML file");
        return NULL;
    }

    root = xmlDocGetRootElement (doc);
    if (g_strcasecmp (root->name, "transactions") != 0) {
        rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                  "Unknown root element in rollback XML file: %s",
                  root->name);
        return NULL;
    }

    action_hash = g_hash_table_new (g_str_hash, g_str_equal);

    for (node = root->xmlChildrenNode; node; node = node->next) {
        char *timestamp;
        time_t trans_time;

        /* Skip comments and text regions and any non-package node */
        if (node->type != XML_ELEMENT_NODE ||
            g_strcasecmp (node->name, "package") != 0)
        {
            continue;
        }

        timestamp = xml_get_prop (node, "timestamp");
        trans_time = atoll (timestamp);
        g_free (timestamp);

        if (!trans_time) {
            rc_debug (RC_DEBUG_LEVEL_WARNING,
                      "Unable to parse timestamp: %s", timestamp);
        }

        if (trans_time && when <= trans_time)
            get_action_from_xml_node (node, trans_time, action_hash);
    }

    actions = rc_hash_values_to_list (action_hash);
    g_hash_table_destroy (action_hash);

    /*
     * Remove any removal elements with NULL packages.  This is for packages
     * whose installs would have been rolled back but which have since
     * been removed from the system.
     */
    for (iter = actions; iter; iter = next) {
        RCRollbackAction *action = iter->data;
        
        next = iter->next;

        if (!action->is_install && !action->package) {
            actions = g_slist_remove (actions, action);
            rc_rollback_action_free (action);
        }
    }
    
    return actions;
}

void
rc_rollback_action_slist_dump (RCRollbackActionSList *actions)
{
    RCRollbackActionSList *iter;

    for (iter = actions; iter; iter = iter->next) {
        RCRollbackAction *action = iter->data;

        if (action->is_install) {
            printf ("install - %s\n", rc_package_spec_to_str_static (
                        RC_PACKAGE_SPEC (action->update)));
        }
        else {
            printf ("remove - %s\n", rc_package_spec_to_str_static (
                        RC_PACKAGE_SPEC (action->package)));
        }
    }
}

void
rc_rollback_restore_files (RCRollbackActionSList *actions)
{
    RCRollbackActionSList *iter;

    for (iter = actions; iter; iter = iter->next) {
        RCRollbackAction *action = iter->data;
        char *change_dir;
        GSList *citer;

        change_dir = g_strdup_printf (RC_ROLLBACK_DIR"/%ld",
                                      action->timestamp);

        for (citer = action->file_changes; citer; citer = citer->next) {
            FileChange *change = citer->data;

            if (change->was_removed)
                unlink (change->filename);
            else {
                if (S_ISREG (change->mode)) {
                    char *tmp;
                    char *backup_filename;

                    tmp = escape_pathname (change->filename);
                    backup_filename = g_strconcat (change_dir, "/", tmp, NULL);
                    g_free (tmp);

                    if (rc_cp (backup_filename, change->filename) < 0) {
                        rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                                  "Unable to copy saved '%s' to '%s'!",
                                  backup_filename, change->filename);
                    }

                    g_free (backup_filename);
                }

                chown (change->filename, change->uid, change->gid);
            
                if (change->mode != -1)
                    chmod (change->filename, change->mode);
            }
        }
    }
}
            
