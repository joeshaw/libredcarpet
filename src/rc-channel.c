/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-channel.c: RCChannel class routines and XML parsing.
 *
 * Copyright (C) 2000 Helix Code, Inc.
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

#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

#include "xml-util.h"

#include "rc-channel.h"
#include "rc-common.h"
#include "rc-util.h"
#include "pkginfo.h"

RCSubchannel *
rc_subchannel_new (void)
{
    RCSubchannel *rcs = g_new0 (RCSubchannel, 1);

    return (rcs);
} /* rc_subchannel_new */

void
rc_subchannel_free (RCSubchannel *rcs)
{
    g_free (rcs->name);

    /* I'm not freeing the hashtable because Vlad asked me not too...
       /me is skeptical */

    g_free (rcs);
} /* rc_subchannel_free */

void
rc_subchannel_slist_free(RCSubchannelSList *rcsl)
{
    g_slist_foreach(rcsl, (GFunc) rc_subchannel_free, NULL);

    g_slist_free(rcsl);
} /* rc_subchannel_slist_free */

RCChannel *
rc_channel_new (void)
{
    RCChannel *rcc = g_new0 (RCChannel, 1);
    rcc->type = RC_CHANNEL_TYPE_HELIX; /* default */
    return (rcc);
} /* rc_channel_new */

void
rc_channel_free (RCChannel *rcc)
{
    g_free (rcc->name);
    g_free (rcc->description);
    g_free (rcc->distro_target);
    g_free (rcc->path);
    g_free (rcc->subs_url);
    g_free (rcc->unsubs_url);
    g_free (rcc->icon_file);
    g_free (rcc->title_file);
    g_free (rcc);
} /* rc_channel_free */

void
rc_channel_slist_free(RCChannelSList *rccl)
{
    g_slist_foreach(rccl, (GFunc) rc_channel_free, NULL);
    
    g_slist_free(rccl);
} /* rc_channel_slist_free */

RCChannelSList *
rc_channel_parse_xml(char *xmlbuf, int compressed_length)
{
    RCChannelSList *cl = NULL;
    xmlDoc *doc;
    xmlNode *root, *node;

    RC_ENTRY;

    if (compressed_length) {
        GByteArray *ba;

        if (rc_uncompress_memory (xmlbuf, compressed_length, &ba)) {
            g_warning ("Uncompression failed!");
            return NULL;
        }

        doc = xmlParseMemory(ba->data, ba->len - 1);
        g_byte_array_free (ba, TRUE);
    } else {
        doc = xmlParseMemory(xmlbuf, strlen(xmlbuf));
    }

    if (!doc) {
        g_warning("Unable to parse channel list.");
        RC_EXIT;
        return NULL;
    }

#if LIBXML_VERSION  < 20000
    root = doc->root;
#else
    root = xmlDocGetRootElement (doc);
#endif
    if (!root) {
        /* Uh. Bad. */
        g_warning("channels.xml document has no root");
        xmlFreeDoc(doc);
        RC_EXIT;
        return NULL;
    }

#if LIBXML_VERSION  < 20000
    node = root->childs;
#else
    node = root->children;
#endif
    while (node) {
        char *tmp;
        RCChannel *channel;

        channel = rc_channel_new();

        channel->name = xml_get_prop(node, "name");
        channel->path = xml_get_prop(node, "path");
        channel->file_path = xml_get_prop(node, "file_path");
        channel->icon_file = xml_get_prop(node, "icon");
        channel->title_file = xml_get_prop(node, "title");
        channel->description = xml_get_prop(node, "description");
        channel->distro_target = xml_get_prop(node, "distro_target");
        channel->pkginfo_file = xml_get_prop(node, "pkginfo_file");
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

        if (channel->file_path == NULL) {
            channel->file_path = g_strdup (channel->path);
        }

        cl = g_slist_append(cl, channel);

        node = node->next;
    }

    xmlFreeDoc(doc);

    RC_EXIT;

    return cl;
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
rc_channel_get_id_by_name(RCChannelSList *channels, char *name)
{
    RCChannelSList *i;

    RC_ENTRY;
      
    i = channels;
    while (i) {
        RCChannel *c = i->data;

        if (g_strcasecmp(c->name, name) == 0) {
            RC_EXIT;
            return c->id;
        }

        i = i->next;
    }

    RC_EXIT;
    return -1;
} /* rc_channel_get_id_by_name */

RCChannel *
rc_channel_get_by_id(RCChannelSList *channels, int id)
{
    RCChannelSList *i;
    
    RC_ENTRY;
    
    i = channels;
    while (i) {
        RCChannel *c = i->data;

        if (c->id == id) {
            RC_EXIT;
            return c;
        }

        i = i->next;
    }

    RC_EXIT;
    return NULL;
} /* rc_channel_get_by_id */

RCChannel *
rc_channel_get_by_name(RCChannelSList *channels, char *name)
{
    int id;
    RCChannel *c;
    
    RC_ENTRY;
    
    id = rc_channel_get_id_by_name(channels, name);
    c = rc_channel_get_by_id(channels, id);

    RC_EXIT;
    return c;
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

RCPackage *
rc_find_best_package (RCPackageDepItem *pdep, RCChannelSList *chs, gint user_pref)
{
    RCPackage *ret = NULL;

    while (chs) {
        RCChannel *ch = (RCChannel *) chs->data;
        RCPackage *found;
        found = pkginfo_find_package_with_constraint (ch,
                                                      pdep->spec.name, user_pref,
                                                      pdep);
#if 0
        if (!found) {
            found = g_hash_table_lookup (ch->dep_table,
                                         &pdep->spec);
        }
#endif
        if (found) {
            if (ret) {
                if (rc_package_spec_compare (&found->spec, &ret->spec) > 0) {
                    ret = found;
                }
            } else {
                ret = found;
            }
        }
        chs = chs->next;
    }

    return ret;
}
