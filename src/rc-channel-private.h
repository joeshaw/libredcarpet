/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-channel-private.h
 *
 * Copyright (C) 2002 Ximian, Inc.
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

#ifndef __RC_CHANNEL_PRIVATE_H__
#define __RC_CHANNEL_PRIVATE_H__

#include "rc-package.h"

struct _RCWorld;

struct _RCChannel {
    gint refs;

    struct _RCWorld *world;

    guint32 id;
    guint32 base_id;

    gchar *name;
    gchar *alias;
    gchar *description;
    guint32 tier;
                           /* priority if channel is... */
    gint priority;         /* subscribed */
    gint priority_unsubd;  /* unsubscribed */
    gint priority_current; /* the current channel */

    RCChannelType type;

    GSList *distro_target;

    gchar *path;
    gchar *file_path;

    gchar *icon_file;

    gchar *subs_file;
    gchar *unsubs_file;

    gchar *pkginfo_file;
    gboolean pkginfo_compressed;

    gchar *pkgset_file;
    gboolean pkgset_compressed;

    time_t last_update;

    void (*refresh_magic) (RCChannel *);

    gboolean mirrored   : 1;
    gboolean featured   : 1;
    gboolean subscribed : 1;
    gboolean transient  : 1;
    gboolean silent     : 1;
};

RCChannel *rc_channel_new  (void);

void       rc_channel_free (RCChannel *channel);

#endif /* __RC_CHANNEL_PRIVATE_H__ */

