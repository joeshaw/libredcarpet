/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-channel.c
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

RCChannel *
rc_channel_ref (RCChannel *channel)
{
    if (channel != NULL) {
        g_assert (channel->refs > 0);
        ++channel->refs;
    }
    
    return channel;
} /* rc_channel_ref */

void
rc_channel_unref (RCChannel *channel)
{
    if (channel != NULL) {
        g_assert (channel->refs > 0);
        --channel->refs;

        if (channel->refs == 0) {
            g_free (channel->name);
            g_free (channel->alias);
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
            
            g_free (channel);
        }
    }
} /* rc_channel_unref */

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

guint32
rc_channel_get_id (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, 0);
    
    return channel->id;
}

guint32
rc_channel_get_base_id (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, 0);

    return channel->base_id;
}

const char *
rc_channel_get_name (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->name ? channel->name : "Unnamed Channel";
}

const char *
rc_channel_get_alias (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->alias;
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

const char *
rc_channel_get_pkginfo_file (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->pkginfo_file;
}

gboolean
rc_channel_get_pkginfo_compressed (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->pkginfo_compressed;
}

time_t
rc_channel_get_last_update (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, (time_t) 0);
    return channel->last_update;
}

const char *
rc_channel_get_path (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);
    return channel->path;
}

const char *
rc_channel_get_subs_file (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);
    return channel->subs_file;
}

const char *
rc_channel_get_unsubs_file (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);
    return channel->unsubs_file;
}

const char *
rc_channel_get_icon_file (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);
    return channel->icon_file;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_channel_subscribed (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->subscribed;
}

void
rc_channel_set_subscription (RCChannel *channel,
                             gboolean   subscribed)
{
    if ((channel->subscribed ^ subscribed)
        && channel->world != NULL
        && ! rc_channel_get_silent (channel))
        rc_world_touch_subscription_sequence_number (channel->world);
    channel->subscribed = subscribed;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

int
rc_channel_foreach_package (const RCChannel *channel,
                            RCPackageFn fn,
                            gpointer user_data)
{
    g_return_val_if_fail (channel != NULL, -1);

    if (channel->world == NULL) {
        g_warning ("rc_channel_foreach_package called on an unattached channel!");
        return -1;
    }

    return rc_world_foreach_package (channel->world, 
                                     (RCChannel *) channel,
                                     fn, user_data);
}

int
rc_channel_package_count (const RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, -1);

    if (channel->world == NULL) {
        g_warning ("rc_channel_package_count called on an unattached channel!");
        return -1;
    }

    return rc_channel_foreach_package (channel, NULL, NULL);
}
                          

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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

gboolean
rc_channel_has_refresh_magic (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->refresh_magic != NULL;
}

gboolean
rc_channel_use_refresh_magic (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    if (channel->refresh_magic) {
        channel->refresh_magic (channel);
        return TRUE;
    }

    return FALSE;
}

gboolean
rc_channel_get_transient (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->transient;
}

gboolean
rc_channel_get_silent (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->silent;
}

int
rc_channel_get_id_by_name (RCChannelSList *channels, char *name)
{
    RCChannelSList *iter;
      
    iter = channels;

    while (iter) {
        RCChannel *channel = iter->data;

        if (g_strcasecmp (channel->name, name) == 0)
            return channel->id;

        iter = iter->next;
    }

    return (-1);
} /* rc_channel_get_id_by_name */

RCChannel *
rc_channel_get_by_id (RCChannelSList *channels, int id)
{
    RCChannelSList *iter;
    
    iter = channels;

    while (iter) {
        RCChannel *channel = iter->data;

        if (channel->id == id)
            return (channel);

        iter = iter->next;
    }

    return (NULL);
} /* rc_channel_get_by_id */

RCChannel *
rc_channel_get_by_name(RCChannelSList *channels, char *name)
{
    int id;
    RCChannel *channel;
    
    id = rc_channel_get_id_by_name(channels, name);
    channel = rc_channel_get_by_id(channels, id);

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

