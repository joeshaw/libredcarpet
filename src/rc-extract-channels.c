/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-extract-channels.c
 *
 * Copyright (C) 2000-2003 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#include <config.h>
#include "rc-extract-channels.h"

#include "rc-debug.h"
#include "rc-distro.h"
#include "xml-util.h"

gint
rc_extract_channels_from_helix_buffer (const guint8 *data, int len,
                                       RCChannelFn callback,
                                       gpointer user_data)
{
    xmlDoc *doc;
    xmlNode *node;
    int count = 0;

    doc = rc_parse_xml_from_buffer (data, len);

    if (!doc) {
        rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                  "Unable to uncompress or parse channel list.");
        return -1;
    }

    node = xmlDocGetRootElement (doc);

    for (node = node->xmlChildrenNode; node; node = node->next) {
        char *tmp;
        gchar *name, *alias, *id_str, *description;
        RCChannelType type = RC_CHANNEL_TYPE_HELIX;
        int subd_priority = 0, unsubd_priority = 0;
        RCChannel *channel;

        /* Skip comments */
        if (node->type == XML_COMMENT_NODE || node->type == XML_TEXT_NODE) {
            continue;
        }

        name = xml_get_prop(node, "name");
            
        alias = xml_get_prop(node, "alias");
        if (alias == NULL)
            alias = g_strdup ("");

        id_str = xml_get_prop(node, "bid");

        if (!id_str)
            id_str = xml_get_prop(node, "id");

        description = xml_get_prop (node, "description");

        channel = rc_channel_new (id_str, name, alias, description);

        g_free (name);
        g_free (alias);
        g_free (id_str);
        g_free (description);

        id_str = xml_get_prop(node, "id");
        rc_channel_set_legacy_id (channel, id_str);
        g_free (id_str);
        
        tmp = xml_get_prop (node, "distro_target");
        if (tmp) {
            gchar **targets, **iter;

            targets = g_strsplit (tmp, ":", 0);
            g_free (tmp);

            for (iter = targets; iter && *iter; iter++)
                rc_channel_add_distro_target (channel, *iter);

            g_strfreev (targets);
        }

        tmp = xml_get_prop(node, "type");
        if (tmp) {
            if (g_strcasecmp (tmp, "helix") == 0)
                type = RC_CHANNEL_TYPE_HELIX;
            else if (g_strcasecmp (tmp, "debian") == 0)
                type = RC_CHANNEL_TYPE_DEBIAN;
            else if (g_strcasecmp (tmp, "aptrpm") == 0)
                type = RC_CHANNEL_TYPE_APTRPM;
            else
                type = RC_CHANNEL_TYPE_UNKNOWN;
            g_free (tmp);
        }

        rc_channel_set_type (channel, type);

        /* Priorities determine affinity amongst channels in dependency
           resolution. */
        tmp = xml_get_prop(node, "priority");
        if (tmp) {
            subd_priority = rc_channel_priority_parse (tmp);
            g_free (tmp);
        }
            
        tmp = xml_get_prop(node, "priority_when_unsubscribed");
        if (tmp) {
            unsubd_priority = rc_channel_priority_parse (tmp);
            g_free (tmp);
        }

        rc_channel_set_priorities (channel, subd_priority, unsubd_priority);

        tmp = xml_get_prop (node, "path");
        rc_channel_set_path (channel, tmp);
        g_free (tmp);

        tmp = xml_get_prop (node, "file_path");
        rc_channel_set_file_path (channel, tmp);
        g_free (tmp);
        
        tmp = xml_get_prop (node, "icon");
        rc_channel_set_icon_file (channel, tmp);
        g_free (tmp);

        tmp = xml_get_prop(node, "pkginfo_file");
        rc_channel_set_pkginfo_file (channel, tmp);
        g_free (tmp);

#if 0            
        tmp = xml_get_prop(node, "last_update");
        if (tmp)
            channel->last_update = atol(tmp);
        g_free(tmp);
#endif

        if (callback)
            callback (channel, user_data);

        rc_channel_unref (channel);

        ++count;
    }

    xmlFreeDoc (doc);

    return count;
}

gint
rc_extract_channels_from_helix_file (const char *filename,
                                     RCChannelFn callback,
                                     gpointer user_data)
{
    RCBuffer *buf;
    int count;

    g_return_val_if_fail (filename != NULL, -1);

    buf = rc_buffer_map_file (filename);
    if (buf == NULL)
        return -1;

    count = rc_extract_channels_from_helix_buffer (buf->data, buf->size,
                                                   callback, user_data);
    rc_buffer_unmap_file (buf);

    return count;
}
