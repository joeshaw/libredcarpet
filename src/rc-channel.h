/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-channel.h
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

#ifndef _RC_CHANNEL_H
#define _RC_CHANNEL_H

#include <glib.h>
#include <libxml/tree.h>

typedef struct _RCChannel RCChannel;
typedef GSList RCChannelSList;
typedef gboolean (*RCChannelFn) (RCChannel *, gpointer);

#include "rc-package.h"

/*
  HELIX   packageinfo.xml
   DEBIAN  debian Packages.gz 
   REDHAT  up2date RDF [?]
*/
typedef enum {
    RC_CHANNEL_TYPE_HELIX,  
    RC_CHANNEL_TYPE_DEBIAN,
    RC_CHANNEL_TYPE_REDHAT,   
    RC_CHANNEL_TYPE_UNKNOWN,
    RC_CHANNEL_TYPE_LAST
} RCChannelType;

#define RC_CHANNEL_SYSTEM     (NULL)
#define RC_CHANNEL_ANY        ((RCChannel *) 0x1)
#define RC_CHANNEL_NON_SYSTEM ((RCChannel *) 0x2)

int rc_channel_priority_parse (const char *);

RCChannel *rc_channel_ref   (RCChannel *channel);
void       rc_channel_unref (RCChannel *channel);

/* Accessors */
 
const char   *rc_channel_get_id          (RCChannel *channel);

const char   *rc_channel_get_name        (RCChannel *channel);

const char   *rc_channel_get_alias       (RCChannel *channel);

const char   *rc_channel_get_description (RCChannel *channel);

int           rc_channel_get_priority    (RCChannel *channel,
                                          gboolean   is_subscribed);

RCChannelType rc_channel_get_type        (RCChannel *channel);


const char   *rc_channel_get_pkginfo_file       (RCChannel *channel);

gboolean      rc_channel_get_pkginfo_compressed (RCChannel *channel);

time_t        rc_channel_get_last_update        (RCChannel *channel);

const char   *rc_channel_get_path               (RCChannel *channel);

const char   *rc_channel_get_icon_file          (RCChannel *channel);


/* Subscription management */

gboolean rc_channel_subscribed       (RCChannel *channel);

void     rc_channel_set_subscription (RCChannel       *channel,
                                      gboolean         subscribed);

/* Iterators/Accessors for channel packages */

int rc_channel_foreach_package (RCChannel *channel,
                                RCPackageFn fn,
                                gpointer user_data);

int rc_channel_package_count (RCChannel *channel);


/* Other */

gboolean rc_channel_has_refresh_magic (RCChannel *);
gboolean rc_channel_use_refresh_magic (RCChannel *);

gboolean rc_channel_get_transient (RCChannel *);
gboolean rc_channel_get_silent    (RCChannel *);

const char *rc_channel_get_id_by_name (RCChannelSList *channels, char *name);

RCChannel *rc_channel_get_by_id (RCChannelSList *channels, const char *id);

RCChannel *rc_channel_get_by_name (RCChannelSList *channels, char *name);

gboolean rc_channel_is_wildcard (RCChannel *a);
gboolean rc_channel_equal       (RCChannel *a, RCChannel *b);
gboolean rc_channel_equal_id    (RCChannel *a, const char *id);

#endif /* _RC_CHANNEL_H */
