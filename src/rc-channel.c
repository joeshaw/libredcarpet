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
#include <ctype.h>
#include <glib.h>

#include <gnome-xml/tree.h>

#include "rc-world.h"
#include "rc-channel.h"
#include "rc-util.h"
#include "rc-package-info.h"
#include "rc-debug.h"
#include "xml-util.h"

#define DEFAULT_CHANNEL_PRIORITY 1600
#define DEFAULT_CURRENT_CHANNEL_PRIORITY 12800
#define UNSUBSCRIBED_CHANNEL_ADJUSTMENT(x) ((x)/2)
#define BAD_CHANNEL_PRIORITY     0 

static struct ChannelPriorityTable {
    const char *str;
    int priority;
} channel_priority_table[] = { { "private",     6400 },
                               { "ximian",      3200 },
                               { "distro",      1600 },
                               { "third_party",  800 },
                               { "preview",      400 },
                               { "untested",     200 },
                               { "snapshot",     100 },
                               { NULL,             0 } };

int
rc_channel_priority_parse (const char *priority_str)
{
    const char *c;
    int i;
    gboolean is_numeric = TRUE;

    if (priority_str && *priority_str) {

        c = priority_str;
        while (*c && is_numeric) {
            if (! isdigit (*c))
                is_numeric = FALSE;
        }
        if (is_numeric) {
            return atoi (priority_str);
        }
        
        for (i=0; channel_priority_table[i].str != NULL; ++i) {
            if (! g_strcasecmp (channel_priority_table[i].str, priority_str))
                return channel_priority_table[i].priority;
        }

    }

    return DEFAULT_CHANNEL_PRIORITY;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCChannel *
rc_channel_new (void)
{
    RCChannel *channel;

    RC_ENTRY;

    channel = g_new0 (RCChannel, 1);

    channel->type = RC_CHANNEL_TYPE_HELIX; /* default */

    channel->priority = -1;
    channel->priority_unsubd = -1;
    channel->priority_current = -1;

    channel->packages = g_hash_table_new (g_str_hash, g_str_equal);
    channel->dep_table = g_hash_table_new (rc_package_spec_hash,
                                           rc_package_spec_equal);
    channel->dep_name_table = g_hash_table_new (g_str_hash, g_str_equal);


    return (channel);

    RC_EXIT;
} /* rc_channel_new */

static gboolean
remove_helper (gchar *name, RCPackage *package, gpointer user_data)
{
    RC_ENTRY;

    rc_package_free (package);

    RC_EXIT;

    return (TRUE);
}

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

    g_free (channel->subs_file);
    g_free (channel->unsubs_file);

    g_free (channel->icon_file);

    if (channel->dep_table) {
        rc_hash_table_slist_free (channel->dep_table);
        g_hash_table_destroy (channel->dep_table);
    }

    if (channel->dep_name_table) {
        rc_hash_table_slist_free (channel->dep_name_table);
        g_hash_table_destroy (channel->dep_name_table);
    }

    if (channel->packages) {
        g_hash_table_foreach_remove (channel->packages,
                                     (GHRFunc) remove_helper, NULL);
        g_hash_table_destroy (channel->packages);
    }

    rc_package_set_slist_free (channel->package_sets);

    g_free (channel);

    RC_EXIT;
} /* rc_channel_free */

int
rc_channel_get_priority (const RCChannel *channel,
                         gboolean is_subscribed,
                         gboolean is_current)
{
    int priority;

    g_return_val_if_fail (channel != NULL, BAD_CHANNEL_PRIORITY);

    if (is_current) {
        priority = channel->priority_current;
        if (priority < 0)
            priority = DEFAULT_CURRENT_CHANNEL_PRIORITY;
    } else {
        priority = channel->priority;
        if (priority < 0)
            priority = DEFAULT_CHANNEL_PRIORITY;

        if (!is_subscribed) {
            if (channel->priority_unsubd > 0) {
                priority = channel->priority_unsubd;
            } else {
                priority = UNSUBSCRIBED_CHANNEL_ADJUSTMENT (priority);
            }
        }
    }

    return priority;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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
        if (node->type == XML_COMMENT_NODE || node->type == XML_TEXT_NODE) {
            node = node->next;
            continue;
        }

        channel = rc_channel_new();

        channel->name = xml_get_prop(node, "name");
        channel->path = xml_get_prop(node, "path");

        tmp = xml_get_prop(node, "file_path");
        channel->file_path = rc_maybe_merge_paths(channel->path, tmp);
        g_free(tmp);
        
        tmp = xml_get_prop(node, "icon");
        channel->icon_file = rc_maybe_merge_paths(channel->path, tmp);
        g_free(tmp);

        channel->description = xml_get_prop(node, "description");

        channel->distro_target = NULL;

        tmp = xml_get_prop (node, "distro_target");
        if (tmp) {
            targets = g_strsplit (tmp, ":", 0);
            g_free (tmp);

            for (iter = targets; iter && *iter; iter++) {
                channel->distro_target =
                    g_slist_append (channel->distro_target, *iter);
            }

            g_free (targets);
        }

        tmp = xml_get_prop(node, "subs_url");
        channel->subs_file = rc_maybe_merge_paths(channel->path, tmp);
        g_free(tmp);

        tmp = xml_get_prop(node, "unsubs_url");
        channel->unsubs_file = rc_maybe_merge_paths(channel->path, tmp);
        g_free(tmp);

        tmp = xml_get_prop(node, "mirrored");
        if (tmp) {
            channel->mirrored = TRUE;
            g_free(tmp);
        }
        else {
            channel->mirrored = FALSE;
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
            
        tmp = xml_get_prop(node, "pkginfo_file");
        if (!tmp) {
            /* default */
            tmp = g_strdup("packageinfo.xml.gz");
            channel->pkginfo_compressed = TRUE;
        }
        channel->pkginfo_file = rc_maybe_merge_paths(channel->path, tmp);
        g_free(tmp);

        tmp = xml_get_prop(node, "pkgset_file");
        if (!tmp) {
            /* default */
            tmp = g_strdup("packageset.xml.gz");
            channel->pkgset_compressed = TRUE;
        }
        channel->pkgset_file = rc_maybe_merge_paths(channel->path, tmp);
        g_free(tmp);

        tmp = xml_get_prop(node, "id");
        channel->id = atoi(tmp);
        g_free(tmp);

        /* Tiers determine how channels are ordered in the client. */
        tmp = xml_get_prop(node, "tier");
        if (tmp) {
            channel->tier = atoi(tmp);
            g_free(tmp);
        }

        /* Priorities determine affinity amongst channels in dependency
           resolution. */
        tmp = xml_get_prop(node, "priority");
        if (tmp) {
            channel->priority = rc_channel_priority_parse(tmp);
            g_free(tmp);
        }

        tmp = xml_get_prop(node, "priority_when_current");
        if (tmp) {
            channel->priority_current = rc_channel_priority_parse(tmp);
            g_free(tmp);
        }

        tmp = xml_get_prop(node, "priority_when_unsubscribed");
        if (tmp) {
            channel->priority_unsubd = rc_channel_priority_parse(tmp);
            g_free(tmp);
        }

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

static void
rc_xml_node_process (xmlNode *node, RCChannel *channel)
{
    const xmlNode *iter;

    if (g_strcasecmp (node->name, "subchannel")) {
        return;
    }

    g_hash_table_freeze (channel->packages);
    g_hash_table_freeze (channel->dep_table);
    g_hash_table_freeze (channel->dep_name_table);

    iter = node->xmlChildrenNode;

    while (iter) {
        RCPackage *package;
        const RCPackageDepSList *prov_iter;

        package = rc_xml_node_to_package (iter, channel);

        if (package) {
            RCWorld *world = rc_get_world ();
            rc_world_add_package (world, package);

            for (prov_iter = package->provides; prov_iter;
                 prov_iter = prov_iter->next)
            {
                const RCPackageDep *dep = (RCPackageDep *)(prov_iter->data);
                
                rc_hash_table_slist_insert_unique (channel->dep_table,
                                                   (gpointer) &dep->spec, package, NULL);
                rc_hash_table_slist_insert_unique (channel->dep_name_table,
                                                   dep->spec.name, package, NULL);
            }

            g_hash_table_insert (channel->packages,
                                 (gpointer) package->spec.name,
                                 (gpointer) package);
        }

        iter = iter->next;
    }

    g_hash_table_thaw (channel->packages);
    g_hash_table_thaw (channel->dep_table);
    g_hash_table_thaw (channel->dep_name_table);
}

guint
rc_xml_node_to_channel (RCChannel *channel, xmlNode *node)
{
    xmlNode *iter;
    char *priority_str;

    if (g_strcasecmp (node->name, "channel")) {
        return (1);
    }

    iter = node->xmlChildrenNode;

    while (iter) {
        rc_xml_node_process (iter, channel);
        iter = iter->next;
    }

    return (0);
}
