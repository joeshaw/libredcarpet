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

#include <config.h>
#include "rc-channel.h"

#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <glib.h>

#include <libxml/tree.h>

#include "rc-channel-private.h"
#include "rc-world.h"
#include "rc-util.h"
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
            c++;
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

guint32
rc_channel_get_id (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, 0);
    
    return channel->id;
}

const char *
rc_channel_get_name (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->name ? channel->name : "Unnamed Channel";
}

const char *
rc_channel_get_description (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->description ?
        channel->description : "No Description Available";
}

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

RCChannelType
rc_channel_get_type (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, RC_CHANNEL_TYPE_UNKNOWN);

    return channel->type;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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

