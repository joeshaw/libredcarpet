/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-channel.h: RCChannel class routines and XML parsing.
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

#ifndef _RC_CHANNEL_H
#define _RC_CHANNEL_H

#include "rc-package.h"
#include "rc-package-set.h"

typedef struct _RCSubchannel RCSubchannel;

struct _RCSubchannel {
    gchar *name;
    guint32 preference;

    RCPackageHashTableByString *packages;
};

RCSubchannel *rc_subchannel_new (void);

void rc_subchannel_free (RCSubchannel *rcs);

typedef GSList RCSubchannelSList;

void rc_subchannel_slist_free(RCSubchannelSList *rcsl);

typedef enum _RCChannelType {
    RC_CHANNEL_TYPE_HELIX,      /* packageinfo.xml */
    RC_CHANNEL_TYPE_DEBIAN,     /* debian Packages.gz */
    RC_CHANNEL_TYPE_REDHAT,     /* redhat up2date RDF [?] */
    RC_CHANNEL_TYPE_UNKNOWN,
    RC_CHANNEL_TYPE_LAST
} RCChannelType;

typedef struct _RCChannel RCChannel;

struct _RCChannel {
    guint32 id;
    gchar *name;
    gchar *description;

    gboolean mirrored;
    gboolean available_select;

    RCChannelType type;

    gchar *distro_target;

    gchar *path;
    gchar *file_path;

    gchar *pkginfo_file;
    gboolean pkginfo_compressed;

    gchar *subs_url;
    gchar *unsubs_url;

    /* for use as pixbufs in gui.h */
    char *icon_file;
    char *title_file;

    time_t last_update;

    RCPackageHashTableBySpec *dep_table;
    RCSubchannelSList *subchannels;

    RCPackageSetSList *package_sets;
};

RCChannel *rc_channel_new (void);

void rc_channel_free (RCChannel *rcc);

typedef GSList RCChannelSList;

void rc_channel_slist_free(RCChannelSList *rccl);

RCChannelSList *rc_channel_parse_xml(char *xmlbuf, int compressed_length);

int rc_channel_get_id_by_name(RCChannelSList *channels, char *name);

RCChannel *rc_channel_get_by_id(RCChannelSList *channels, int id);

RCChannel *rc_channel_get_by_name(RCChannelSList *channels, char *name);

gint rc_channel_compare_func (gconstpointer a, gconstpointer b);

#endif /* _RC_CHANNEL_H */