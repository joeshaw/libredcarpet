/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-update.c
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

#include <glib.h>
#include <stdlib.h>

#include <gnome-xml/tree.h>

#include "rc-package-update.h"
#include "xml-util.h"

RCPackageUpdate *
rc_package_update_new ()
{
    RCPackageUpdate *update = g_new0 (RCPackageUpdate, 1);

    return (update);
} /* rc_package_update_new */

RCPackageUpdate *
rc_package_update_copy (RCPackageUpdate *old_update)
{
    RCPackageUpdate *new_update;

    g_return_val_if_fail (old_update, NULL);

    new_update = rc_package_update_new ();

    rc_package_spec_copy ((RCPackageSpec *) new_update,
                          (RCPackageSpec *) old_update);

    new_update->package_url = g_strdup (old_update->package_url);
    new_update->package_size = old_update->package_size;

    new_update->installed_size = old_update->installed_size;

    new_update->signature_url = g_strdup (old_update->signature_url);
    new_update->signature_size = old_update->signature_size;

    new_update->md5sum = g_strdup (old_update->md5sum);

    new_update->importance = old_update->importance;

    new_update->description = g_strdup (old_update->description);

    return (new_update);
}

void
rc_package_update_free (RCPackageUpdate *update)
{
    g_return_if_fail (update);

    rc_package_spec_free_members(RC_PACKAGE_SPEC (update));

    g_free (update->package_url);

    g_free (update->signature_url);

    g_free (update->md5sum);

    g_free (update->description);

    g_free (update);
} /* rc_package_update_free */

RCPackageUpdateSList *
rc_package_update_slist_copy (RCPackageUpdateSList *old_slist)
{
    RCPackageUpdateSList *iter;
    RCPackageUpdateSList *new_slist = NULL;

    for (iter = old_slist; iter; iter = iter->next) {
        RCPackageUpdate *old_update = (RCPackageUpdate *)(iter->data);
        RCPackageUpdate *new_update = rc_package_update_copy (old_update);

        new_slist = g_slist_append (new_slist, new_update);
    }

    return (new_slist);
}

void
rc_package_update_slist_free (RCPackageUpdateSList *update_slist)
{
    g_slist_foreach (update_slist, (GFunc) rc_package_update_free, NULL);

    g_slist_free (update_slist);
} /* rc_package_update_slist_free */

RCPackageUpdateSList *
rc_package_update_slist_sort (RCPackageUpdateSList *old_slist)
{
    RCPackageUpdateSList *new_slist = NULL;

    new_slist =
        g_slist_sort (old_slist, (GCompareFunc) rc_package_spec_compare_name);

    return (new_slist);
}

RCPackageUpdate *
rc_xml_node_to_package_update (const xmlNode *node, const RCPackage *package)
{
    RCPackageUpdate *update;
    const xmlNode *iter;
    const gchar *url_prefix = NULL;

    g_return_val_if_fail (node, NULL);

    /* Make sure this is an update node */
    if (g_strcasecmp (node->name, "update")) {
        return (NULL);
    }

    update = rc_package_update_new ();

    update->package = package;

    update->spec.name = g_strdup (package->spec.name);

    if (package->subchannel && package->subchannel->channel &&
        package->subchannel->channel->file_path)
    {
        url_prefix = package->subchannel->channel->file_path;
    }

#if LIBXML_VERSION < 20000
    iter = node->childs;
#else
    iter = node->children;
#endif

    while (iter) {
        if (!g_strcasecmp (iter->name, "epoch")) {
            update->spec.epoch =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "version")) {
            update->spec.version = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "release")) {
            update->spec.release = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "filename")) {
            if (url_prefix) {
                gchar *tmp = xml_get_content (iter);
                update->package_url =
                    g_strconcat (url_prefix, "/", tmp, NULL);
                g_free (tmp);
            } else {
                update->package_url = xml_get_content (iter);
            }
        } else if (!g_strcasecmp (iter->name, "filesize")) {
            update->package_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "installedsize")) {
            update->installed_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "signaturename")) {
            if (url_prefix) {
                gchar *tmp = xml_get_content (iter);
                update->signature_url =
                    g_strconcat (url_prefix, "/", tmp, NULL);
                g_free (tmp);
            } else {
                update->signature_url = xml_get_content (iter);
            }
        } else if (!g_strcasecmp (iter->name, "signaturesize")) {
            update->signature_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "md5sum")) {
            update->md5sum = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "importance")) {
            gchar *tmp = xml_get_content (iter);
            update->importance =
                rc_string_to_package_importance (tmp);
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "description")) {
            update->description = xml_get_content (iter);
        } else {
            /* FIXME: should we complain to the user at this point?  This
               should really never happen. */
        }

        iter = iter->next;
    }

    return (update);
}
