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

#include "rc-world.h"
#include "rc-util.h"
#include "rc-debug.h"
#include "xml-util.h"

struct _RCChannel {
    gint refs;

    struct _RCWorld *world;

    RCChannelType type;

    gchar *id;

    gchar *name;
    gchar *alias;
    gchar *description;
                            /* priority if channel is... */
    gint priority;          /* subscribed */
    gint priority_unsubd;   /* unsubscribed */

    gchar *path;
    gchar *file_path;
    gchar *icon_file;
    gchar *pkginfo_file;

    GSList *distro_targets; /* List of targets (char *) for this channel */

    time_t last_update;

    gboolean system    : 1;
    gboolean hidden    : 1;
    gboolean immutable : 1;
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

#define DEFAULT_CHANNEL_PRIORITY 1600
#define UNSUBSCRIBED_CHANNEL_ADJUSTMENT(x) ((x)/2)
#define BAD_CHANNEL_PRIORITY     0 

typedef struct {
    const char *str;
    int priority;
} ChannelPriorityPair;

ChannelPriorityPair channel_priority_table[] = \
    { { "private",     6400 },
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
    /* it is safe to ref a wildcard channel pointer */
    if (channel != NULL && ! rc_channel_is_wildcard (channel)) {
        g_assert (channel->refs > 0);
        ++channel->refs;
    }
    
    return channel;
} /* rc_channel_ref */

void
rc_channel_unref (RCChannel *channel)
{
    /* it is safe to unref a wildcard channel pointer */
    if (channel != NULL && ! rc_channel_is_wildcard (channel)) {
        g_assert (channel->refs > 0);
        --channel->refs;

        if (channel->refs == 0) {
            g_free (channel->id);

            g_free (channel->name);
            g_free (channel->alias);
            g_free (channel->description);

            g_free (channel->path);
            g_free (channel->file_path);
            g_free (channel->pkginfo_file);
            g_free (channel->icon_file);

            g_slist_foreach (channel->distro_targets, (GFunc) g_free, NULL);
            g_slist_free (channel->distro_targets);
            
            g_free (channel);
        }
    }
} /* rc_channel_unref */

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCChannel *
rc_channel_new (const char *id,
                const char *name,
                const char *alias,
                const char *description)
{
    static int fake_id = 1;
    char fake_id_buffer[32];

    RCChannel *channel = g_new0 (RCChannel, 1);

    if (id == NULL) {
        g_snprintf (fake_id_buffer, 32, "fake-id-%d", fake_id);
        ++fake_id;
        id = fake_id_buffer;
    }

    if (name == NULL)
        name = "Unnamed Channel";

    if (alias == NULL)
        alias = name;

    if (description == NULL)
        description = "No description available.";

    channel->refs = 1;

    channel->type = RC_CHANNEL_TYPE_UNKNOWN;
    
    channel->priority = -1;
    channel->priority_unsubd = -1;

    channel->id          = g_strdup (id);
    channel->name        = g_strdup (name);
    channel->alias       = g_strdup (alias);
    channel->description = g_strdup (description);

    return channel;
}

void
rc_channel_set_id_prefix (RCChannel *channel,
                          const char *prefix)
{
    g_return_if_fail (channel != NULL);
    if (prefix && *prefix) {
        char *new_id = g_strconcat (prefix, "|", channel->id, NULL);
        g_free (channel->id);
        channel->id = new_id;
    }
}

void
rc_channel_set_world (RCChannel *channel,
                      RCWorld *world)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (world != NULL && RC_IS_WORLD (world));

    channel->world = world;
}

void
rc_channel_set_type (RCChannel *channel, RCChannelType type)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));

    channel->type = type;
}

void
rc_channel_set_priorities (RCChannel *channel,
                           gint subd_priority,
                           gint unsubd_priority)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (! rc_channel_is_immutable (channel));

    channel->priority = subd_priority;
    channel->priority_unsubd = unsubd_priority;
}

void
rc_channel_set_name (RCChannel *channel, const char *name)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));
    g_return_if_fail (name != NULL);

    if (channel->name)
        g_free (channel->name);

    channel->name = g_strdup (name);
}

void
rc_channel_set_alias (RCChannel *channel, const char *alias)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));
    g_return_if_fail (alias != NULL);

    if (channel->alias)
        g_free (channel->alias);

    channel->alias = g_strdup (alias);
}

void
rc_channel_set_path (RCChannel *channel, const char *path)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));
    g_return_if_fail (path != NULL);

    if (channel->path)
        g_free (channel->path);

    channel->path = g_strdup (path);
}

void
rc_channel_set_file_path (RCChannel *channel, const char *file_path)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));

    if (channel->file_path)
        g_free (channel->file_path);

    channel->file_path = g_strdup (file_path);
}

void
rc_channel_set_icon_file (RCChannel *channel, const char *icon_file)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));

    if (channel->icon_file)
        g_free (channel->icon_file);

    channel->icon_file = g_strdup (icon_file);
}

void
rc_channel_set_pkginfo_file (RCChannel *channel, const char *pkginfo_file)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));

    if (channel->pkginfo_file)
        g_free (channel->pkginfo_file);

    channel->pkginfo_file = g_strdup (pkginfo_file);
}

void
rc_channel_set_hidden (RCChannel *channel)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (!rc_channel_is_immutable (channel));

    channel->hidden = TRUE;
}

void
rc_channel_set_system (RCChannel *channel)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (! rc_channel_is_immutable (channel));

    channel->system = TRUE;
}

void
rc_channel_make_immutable (RCChannel *channel)
{
    g_return_if_fail (channel != NULL);

    channel->immutable = TRUE;
}

gboolean
rc_channel_is_immutable (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->immutable;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

struct _RCWorld *
rc_channel_get_world (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->world;
}

RCChannelType
rc_channel_get_type (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, RC_CHANNEL_TYPE_UNKNOWN);

    return channel->type;
}

const char *
rc_channel_get_id (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);
    g_return_val_if_fail (rc_channel_is_wildcard (channel) == FALSE, NULL);
    
    return channel->id;
}

const char *
rc_channel_get_name (RCChannel *channel)
{
    if (channel == RC_CHANNEL_ANY)
        return "[Any]";
    if (channel == RC_CHANNEL_SYSTEM)
        return "[System]";
    if (channel == RC_CHANNEL_NON_SYSTEM)
        return "[NonSystem]";

    return channel->name ? channel->name : "Unnamed Channel";
}

const char *
rc_channel_get_alias (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->alias;
}

const char *
rc_channel_get_description (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);

    return channel->description ?
        channel->description : "No Description Available";
}

int
rc_channel_get_priority (RCChannel *channel,
                         gboolean is_subscribed)
{
    int priority;

    g_return_val_if_fail (channel != NULL, BAD_CHANNEL_PRIORITY);

    priority = channel->priority;
    if (priority <= 0)
        priority = DEFAULT_CHANNEL_PRIORITY;

    if (!is_subscribed) {
        if (channel->priority_unsubd > 0) {
            priority = channel->priority_unsubd;
        } else {
            priority = UNSUBSCRIBED_CHANNEL_ADJUSTMENT (priority);
        }
    }

    return priority;
}

const char *
rc_channel_get_path (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, NULL);
    return channel->path;
}

const char *
rc_channel_get_file_path (RCChannel *channel)
{
    static char *file_path = NULL;

    g_return_val_if_fail (channel != NULL, NULL);

    g_free (file_path);

    file_path = rc_maybe_merge_paths (channel->path, channel->file_path);

    return file_path;
}

const char *
rc_channel_get_icon_file (RCChannel *channel)
{
    static char *icon_file = NULL;

    g_return_val_if_fail (channel != NULL, NULL);

    g_free (icon_file);

    icon_file = rc_maybe_merge_paths (channel->path, channel->icon_file);

    return icon_file;
}

const char *
rc_channel_get_pkginfo_file (RCChannel *channel)
{
    static char *pkginfo_file = NULL;

    g_return_val_if_fail (channel != NULL, NULL);

    g_free (pkginfo_file);

    pkginfo_file = rc_maybe_merge_paths (channel->path, channel->pkginfo_file);

    return pkginfo_file;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_channel_add_distro_target (RCChannel *channel, const char *target)
{
    g_return_if_fail (channel != NULL);
    g_return_if_fail (target != NULL);

    channel->distro_targets = g_slist_prepend (channel->distro_targets,
                                               g_strdup (target));
}

gboolean
rc_channel_has_distro_target (RCChannel *channel, const char *target)
{
    GSList *iter;

    g_return_val_if_fail (channel != NULL, FALSE);
    g_return_val_if_fail (target != NULL, FALSE);

    for (iter = channel->distro_targets; iter != NULL; iter = iter->next) {
        if (g_strcasecmp ((char *) iter->data, target) == 0)
            return TRUE;
    }

    return FALSE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_channel_is_subscribed (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return rc_world_is_subscribed (channel->world, (RCChannel *) channel);
}

void
rc_channel_set_subscription (RCChannel *channel,
                             gboolean   subscribed)
{
    g_return_if_fail (channel != NULL);

    rc_world_set_subscription (channel->world, channel, subscribed);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_channel_is_system (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);
    
    return channel->system;
}

gboolean
rc_channel_is_hidden (RCChannel *channel)
{
    g_return_val_if_fail (channel != NULL, FALSE);

    return channel->hidden;
}

gboolean
rc_channel_is_wildcard (RCChannel *a)
{
    return a == RC_CHANNEL_SYSTEM 
        || a == RC_CHANNEL_NON_SYSTEM
        || a == RC_CHANNEL_ANY;
}

gboolean
rc_channel_equal (RCChannel *a, RCChannel *b)
{
    if (a == RC_CHANNEL_ANY || b == RC_CHANNEL_ANY)
        return TRUE;

    if (rc_channel_is_wildcard (a) && rc_channel_is_wildcard (b))
        return a == b;

    /* So at this point we know that between a and b there is
       at most one wildcard. */

    if (a == RC_CHANNEL_SYSTEM)
        return rc_channel_is_system (b);
    else if (a == RC_CHANNEL_NON_SYSTEM)
        return ! rc_channel_is_system (b);

    if (b == RC_CHANNEL_SYSTEM)
        return rc_channel_is_system (a);
    else if (b == RC_CHANNEL_NON_SYSTEM)
        return ! rc_channel_is_system (a);
    
    return rc_channel_equal_id (a, rc_channel_get_id (b));
}

gboolean
rc_channel_equal_id (RCChannel *a, const char *id)
{
    return ! strcmp (rc_channel_get_id (a), id);
}

