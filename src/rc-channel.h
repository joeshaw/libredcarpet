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

typedef void (*RCChannelFn) (RCChannel *, gpointer);

#include "rc-package.h"
#include "rc-package-set.h"

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

int rc_channel_priority_parse (const char *);

RCChannel *rc_channel_ref   (RCChannel *channel);
void       rc_channel_unref (RCChannel *channel);

/* Accessors */
 
guint32       rc_channel_get_id          (const RCChannel *channel);

const char   *rc_channel_get_name        (const RCChannel *channel);

const char   *rc_channel_get_alias       (const RCChannel *channel);

const char   *rc_channel_get_description (const RCChannel *channel);

int           rc_channel_get_priority    (const RCChannel *channel,
                                          gboolean         is_subscribed,
                                          gboolean         is_current);

RCChannelType rc_channel_get_type        (const RCChannel *channel);


const char   *rc_channel_get_pkginfo_file       (const RCChannel *channel);

gboolean      rc_channel_get_pkginfo_compressed (const RCChannel *channel);

time_t        rc_channel_get_last_update        (const RCChannel *channel);


/* Subscription management */

gboolean rc_channel_subscribed       (const RCChannel *channel);

void     rc_channel_set_subscription (RCChannel       *channel,
                                      gboolean         subscribed);

/* Iterators/Accessors for channel packages */

int rc_channel_foreach_package (const RCChannel *channel,
                                RCPackageFn fn,
                                gpointer user_data);

int rc_channel_package_count (const RCChannel *channel);


/* Other */

int rc_channel_get_id_by_name (RCChannelSList *channels, char *name);

RCChannel *rc_channel_get_by_id (RCChannelSList *channels, int id);

RCChannel *rc_channel_get_by_name (RCChannelSList *channels, char *name);

gint rc_channel_compare_func (gconstpointer a, gconstpointer b);

#endif /* _RC_CHANNEL_H */
