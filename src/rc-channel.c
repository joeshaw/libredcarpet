/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-channel.c
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

#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

#include <gnome-xml/tree.h>

#include "rc-channel.h"
#include "rc-util.h"
#include "rc-package-info.h"
#include "rc-debug.h"
#include "xml-util.h"

RCSubchannel *
rc_subchannel_new (void)
{
    RCSubchannel *subchannel;

    RC_ENTRY;

    subchannel = g_new0 (RCSubchannel, 1);

    RC_EXIT;

    return (subchannel);
} /* rc_subchannel_new */

static gboolean
remove_helper (gchar *name, RCPackage *package, gpointer user_data)
{
    RC_ENTRY;

    rc_package_free (package);

    RC_EXIT;

    return (TRUE);
}

void
rc_subchannel_free (RCSubchannel *subchannel)
{
    RC_ENTRY;

    g_free (subchannel->name);

    /* Everything in this table is a pointer into the elements of the
       packages hash table, so it'll get free'd in a few lines */
    if (subchannel->dep_table) {
        rc_hash_table_slist_free (subchannel->dep_table);
        g_hash_table_destroy (subchannel->dep_table);
    }
    if (subchannel->dep_name_table) {
        rc_hash_table_slist_free (subchannel->dep_name_table);
        g_hash_table_destroy (subchannel->dep_name_table);
    }

    g_hash_table_foreach_remove (subchannel->packages,
                                 (GHRFunc) remove_helper, NULL);

    g_hash_table_destroy (subchannel->packages);

    g_free (subchannel);

    RC_EXIT;
} /* rc_subchannel_free */

void
rc_subchannel_slist_free(RCSubchannelSList *subchannel_slist)
{
    RC_ENTRY;

    g_slist_foreach(subchannel_slist, (GFunc) rc_subchannel_free, NULL);

    g_slist_free(subchannel_slist);

    RC_EXIT;
} /* rc_subchannel_slist_free */

RCChannel *
rc_channel_new (void)
{
    RCChannel *channel;

    RC_ENTRY;

    channel = g_new0 (RCChannel, 1);

    channel->type = RC_CHANNEL_TYPE_HELIX; /* default */

    return (channel);

    RC_EXIT;
} /* rc_channel_new */

void
rc_channel_free (RCChannel *channel)
{
    RC_ENTRY;

    g_free (channel->name);
    g_free (channel->description);

    g_slist_foreach (channel->distro_target, (GFunc)g_free, NULL);
    g_slist_free (channel->distro_target);

    g_free (channel->path);
    g_free (channel->file_path);

    g_free (channel->pkginfo_file);
    g_free (channel->pkgset_file);

    g_free (channel->subs_url);
    g_free (channel->unsubs_url);

    g_free (channel->icon_file);

    rc_subchannel_slist_free (channel->subchannels);

    rc_package_set_slist_free (channel->package_sets);

    g_free (channel);

    RC_EXIT;
} /* rc_channel_free */

void
rc_channel_slist_free(RCChannelSList *channel_slist)
{
    RC_ENTRY;

    g_slist_foreach(channel_slist, (GFunc) rc_channel_free, NULL);
    
    g_slist_free(channel_slist);

    RC_EXIT;
} /* rc_channel_slist_free */

RCChannelSList *
rc_channel_parse_xml(char *xmlbuf, int compressed_length)
{
    RCChannelSList *channel_slist = NULL;
    xmlDoc *doc;
    xmlNode *root, *node;

    RC_ENTRY;

    if (compressed_length) {
        GByteArray *buf;

        if (rc_uncompress_memory (xmlbuf, compressed_length, &buf)) {
            rc_debug (RC_DEBUG_LEVEL_CRITICAL, "%s: uncompression failed\n",
                      __FUNCTION__);

            RC_EXIT;

            return (NULL);
        }

        doc = xmlParseMemory (buf->data, buf->len - 1);
        g_byte_array_free (buf, TRUE);
    } else {
        doc = xmlParseMemory (xmlbuf, strlen(xmlbuf));
    }

    if (!doc) {
        rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                  "%s: unable to parse channel list\n", __FUNCTION__);

        RC_EXIT;

        return (NULL);
    }

    root = xmlDocGetRootElement (doc);

    if (!root) {
        rc_debug (RC_DEBUG_LEVEL_CRITICAL, "%s: channels.xml has no root\n",
                  __FUNCTION__);

        xmlFreeDoc(doc);

        RC_EXIT;

        return (NULL);
    }

    node = root->xmlChildrenNode;

    while (node) {
        char *tmp;
        gchar **targets;
        gchar **iter;
        RCChannel *channel;

        /* Skip comments */
        if (node->type == XML_COMMENT_NODE) {
            node = node->next;
            continue;
        }

        channel = rc_channel_new();

        channel->name = xml_get_prop(node, "name");
        channel->path = xml_get_prop(node, "path");
        channel->file_path = xml_get_prop(node, "file_path");
        channel->icon_file = xml_get_prop(node, "icon");
        channel->description = xml_get_prop(node, "description");

        channel->distro_target = NULL;

        tmp = xml_get_prop (node, "distro_target");
        targets = g_strsplit (tmp, ":", 0);
        g_free (tmp);

        for (iter = targets; *iter; iter++) {
            channel->distro_target =
                g_slist_append (channel->distro_target, *iter);
        }

        g_free (targets);

        channel->pkginfo_file = xml_get_prop(node, "pkginfo_file");
        channel->pkgset_file = xml_get_prop(node, "pkgset_file");

        tmp = xml_get_prop(node, "mirrored");
        if (tmp) {
            channel->mirrored = TRUE;
            g_free(tmp);
        }
        else {
            channel->mirrored = FALSE;
        }
        tmp = xml_get_prop(node, "available_select");
        if (tmp) {
            channel->available_select = TRUE;
            g_free(tmp);
        }
        else {
            channel->available_select = FALSE;
        }
        tmp = xml_get_prop(node, "pkginfo_compressed");
        if (tmp) {
            channel->pkginfo_compressed = TRUE;
            g_free (tmp);
        } else {
            channel->pkginfo_compressed = FALSE;
        }
        tmp = xml_get_prop(node, "pkgset_compressed");
        if (tmp) {
            channel->pkgset_compressed = TRUE;
            g_free (tmp);
        } else {
            channel->pkgset_compressed = FALSE;
        }
            
        channel->subs_url = xml_get_prop(node, "subs_url");
        channel->unsubs_url = xml_get_prop(node, "unsubs_url");
        tmp = xml_get_prop(node, "id");
        channel->id = atoi(tmp);
        g_free(tmp);
        tmp = xml_get_prop(node, "last_update");
        if (tmp)
            channel->last_update = atol(tmp);
        g_free(tmp);
        tmp = xml_get_prop(node, "type");
        if (tmp) {
            if (g_strcasecmp (tmp, "helix") == 0)
                channel->type = RC_CHANNEL_TYPE_HELIX;
            else if (g_strcasecmp (tmp, "debian") == 0)
                channel->type = RC_CHANNEL_TYPE_DEBIAN;
            else if (g_strcasecmp (tmp, "redhat") == 0)
                channel->type = RC_CHANNEL_TYPE_REDHAT;
            else
                channel->type = RC_CHANNEL_TYPE_UNKNOWN;
            g_free (tmp);
        }

        if (channel->pkginfo_file == NULL) {
            /* default */
            channel->pkginfo_file = g_strdup ("packageinfo.xml.gz");
            channel->pkginfo_compressed = TRUE;
        }
        if (channel->pkgset_file == NULL) {
            /* default */
            channel->pkgset_file = g_strdup ("packageset.xml.gz");
            channel->pkgset_compressed = TRUE;
        }

        channel_slist = g_slist_append (channel_slist, channel);

        node = node->next;
    }

    xmlFreeDoc (doc);

    RC_EXIT;

    return channel_slist;
} /* rc_channel_parse_xml */

#if 0
RCChannelSList *
rc_channel_parse_apt_sources(const char *filename)
{
    RCChannelSlist *cl = NULL;
    int fd;
    int bytes;
    char buf[16384];
    GByteArray *data;
    char **lines;
    char *l;
    int id = -1;

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
	g_warning("Unable to open file %s", filename);
	return NULL;
    }

    data = g_byte_array_new();

    do {
	bytes = read(fd, buf, 16384);
	if (bytes)
	    g_byte_array_append(data, buf, bytes);
    } while (bytes > 0);

    fclose(fd);
    g_byte_array_append(data, "\0", 1);
    lines = g_strsplit(data->data, "\n", 0);
    g_byte_array_free(data);
    
    for (l = lines; l; l++) {
	char **s;
	char c;

	if (*l == '#')
	    continue;

	s = g_strsplit(l, " ", 0);
	if (g_strcasecmp(s[0], "deb") != 0) {
	    g_strfreev(s);
	    continue;
	}

	/* So, we need to see if the last character of the distribution field
	   in the sources line ends with a slash. If it does, then it is an
	   absolutely path. Otherwise, it is a distribution name. */
	if (s[2][strlen(s[2])-1] == '/') {
	    RCChannel *channel;

	    channel = rc_channel_new();

	    channel->id = id;
	    channel->path = g_strconcat(s[1], "/", s[2], NULL);
	    channel->name = g_strdup(channel->path);
	    channel->description = g_strdup(channel->path);
	    channel->pkginfo_file = g_strdup("Packages.gz");
	    channel->pkginfo_compressed = TRUE;
	    channel->type = RC_CHANNEL_TYPE_DEBIAN;

	    id--;
	}

	/* FIXME: Write the rest of me. Bad Joey! */
    }
} /* rc_channel_parse_apt_sources */
#endif	    

int
rc_channel_get_id_by_name (RCChannelSList *channels, char *name)
{
    RCChannelSList *iter;

    RC_ENTRY;
      
    iter = channels;

    while (iter) {
        RCChannel *channel = iter->data;

        if (g_strcasecmp (channel->name, name) == 0) {
            RC_EXIT;

            return channel->id;
        }

        iter = iter->next;
    }

    RC_EXIT;

    return (-1);
} /* rc_channel_get_id_by_name */

RCChannel *
rc_channel_get_by_id (RCChannelSList *channels, int id)
{
    RCChannelSList *iter;
    
    RC_ENTRY;
    
    iter = channels;

    while (iter) {
        RCChannel *channel = iter->data;

        if (channel->id == id) {
            RC_EXIT;

            return (channel);
        }

        iter = iter->next;
    }

    RC_EXIT;

    return (NULL);
} /* rc_channel_get_by_id */

RCChannel *
rc_channel_get_by_name(RCChannelSList *channels, char *name)
{
    int id;
    RCChannel *channel;
    
    RC_ENTRY;
    
    id = rc_channel_get_id_by_name(channels, name);
    channel = rc_channel_get_by_id(channels, id);

    RC_EXIT;
    return channel;
} /* rc_channel_get_by_name */

gint
rc_channel_compare_func (gconstpointer a, gconstpointer b)
{
    RCChannel *one = (RCChannel *)a;
    RCChannel *two = (RCChannel *)b;

    if (one->id == two->id) {
        return (TRUE);
    }

    return (FALSE);
}

RCSubchannel *
rc_channel_get_subchannel (RCChannel *channel, guint preference)
{
    RCSubchannelSList *iter;

    for (iter = channel->subchannels; iter; iter = iter->next) {
        RCSubchannel *subchannel = (RCSubchannel *)(iter->data);

        if (subchannel->preference == preference) {
            return (subchannel);
        }
    }

    return (NULL);
}

static RCSubchannel *
rc_xml_node_to_subchannel (xmlNode *node, const RCChannel *channel)
{
    RCSubchannel *subchannel;
    const xmlNode *iter;

    if (g_strcasecmp (node->name, "subchannel")) {
        return (NULL);
    }

    subchannel = rc_subchannel_new ();

    subchannel->channel = channel;

    subchannel->name = xml_get_prop (node, "name");
    subchannel->preference =
        xml_get_guint32_prop_default (node, "preference", 0);

    subchannel->packages = g_hash_table_new (g_str_hash, g_str_equal);
    subchannel->dep_table = g_hash_table_new (rc_package_spec_hash,
                                              rc_package_spec_equal);
    subchannel->dep_name_table = g_hash_table_new (g_str_hash, g_str_equal);

    g_hash_table_freeze (subchannel->packages);
    g_hash_table_freeze (subchannel->dep_table);
    g_hash_table_freeze (subchannel->dep_name_table);

#if LIBXML_VERSION < 20000
    iter = node->childs;
#else
    iter = node->children;
#endif

    while (iter) {
        RCPackage *package;
        const RCPackageDepSList *prov_iter;

        package = rc_xml_node_to_package (iter, subchannel);

        for (prov_iter = package->provides; prov_iter;
             prov_iter = prov_iter->next)
        {
            const RCPackageDep *dep = (RCPackageDep *)(prov_iter->data);

            rc_hash_table_slist_insert_unique (subchannel->dep_table,
                                               &dep->spec, package, NULL);
            rc_hash_table_slist_insert_unique (subchannel->dep_name_table,
                                               dep->spec.name, package, NULL);
        }
            
        g_hash_table_insert (subchannel->packages,
                             (gpointer) package->spec.name,
                             (gpointer) package);

        iter = iter->next;
    }

    g_hash_table_thaw (subchannel->packages);
    g_hash_table_thaw (subchannel->dep_table);
    g_hash_table_thaw (subchannel->dep_name_table);

    return (subchannel);
}

guint
rc_xml_node_to_channel (RCChannel *channel, xmlNode *node)
{
    xmlNode *iter;

    if (g_strcasecmp (node->name, "channel")) {
        return (1);
    }

#if LIBXML_VERSION < 20000
    iter = node->childs;
#else
    iter = node->children;
#endif

    while (iter) {
        channel->subchannels =
            g_slist_append (channel->subchannels,
                            rc_xml_node_to_subchannel (iter, channel));
        iter = iter->next;
    }

    return (0);
}
