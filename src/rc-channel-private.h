/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-channel-private.h
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Developed by Jon Trowbridge <trow@ximian.com>
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

struct _RCWorld;

struct _RCChannel {
    gint refs;

    struct _RCWorld *world;

    guint32 id;
    gchar *name;
    gchar *description;
    guint32 tier;
                           /* priority if channel is... */
    gint priority;         /* subscribed */
    gint priority_unsubd;  /* unsubscribed */
    gint priority_current; /* the current channel */

    gboolean mirrored;
    gboolean featured;

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

    RCPackageSetSList *package_sets;
};

RCChannel *rc_channel_new  (void);

void       rc_channel_free (RCChannel *channel);

#endif /* __RC_CHANNEL_PRIVATE_H__ */

