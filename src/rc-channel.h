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

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

struct _RCWorld;

typedef struct _RCChannel RCChannel;
typedef GSList RCChannelSList;
// typedef enum _RCChannelType RCChannelType;
typedef gboolean (*RCChannelFn) (RCChannel *, gpointer);
typedef gboolean (*RCChannelAndSubdFn) (RCChannel *, gboolean, gpointer);

#include "rc-package.h"

#define RC_CHANNEL_SYSTEM     ((RCChannel *) GINT_TO_POINTER (1))
#define RC_CHANNEL_ANY        ((RCChannel *) GINT_TO_POINTER (2))
#define RC_CHANNEL_NON_SYSTEM ((RCChannel *) GINT_TO_POINTER (3))

typedef enum {
    RC_CHANNEL_TYPE_UNKNOWN = -1,
    RC_CHANNEL_TYPE_HELIX,
    RC_CHANNEL_TYPE_DEBIAN,
    RC_CHANNEL_TYPE_APTRPM
} RCChannelType;


int rc_channel_priority_parse (const char *);

RCChannel *rc_channel_ref   (RCChannel *channel);
void       rc_channel_unref (RCChannel *channel);

/* Constructor / setters */

RCChannel *rc_channel_new (const char *id,
                           const char *name,
                           const char *alias,
                           const char *description);

void rc_channel_set_id_prefix (RCChannel  *channel,
                               const char *prefix);

void rc_channel_set_world (RCChannel *channel,
                           struct _RCWorld *world);

void rc_channel_set_legacy_id (RCChannel  *channel,
                               const char *legacy_id);

void rc_channel_set_type (RCChannel     *channel,
                          RCChannelType  type);

void rc_channel_set_priorities (RCChannel *channel,
                                gint subd_priority,
                                gint unsubd_priority);

void rc_channel_set_name (RCChannel  *channel,
                          const char *path);

void rc_channel_set_alias (RCChannel  *channel,
                           const char *path);

void rc_channel_set_path (RCChannel  *channel,
                          const char *path);

void rc_channel_set_file_path (RCChannel  *channel,
                               const char *file_path);

void rc_channel_set_icon_file (RCChannel  *channel,
                               const char *icon_file);

void rc_channel_set_pkginfo_file (RCChannel  *channel,
                                  const char *pkginfo_file);

void rc_channel_set_hidden (RCChannel *channel);

void rc_channel_set_system (RCChannel *channel);

void     rc_channel_make_immutable (RCChannel *channel);

gboolean rc_channel_is_immutable   (RCChannel *channel);


/* Accessors */

struct _RCWorld *rc_channel_get_world    (RCChannel *channel);

RCChannelType rc_channel_get_type        (RCChannel *channel);

const char   *rc_channel_get_id          (RCChannel *channel);

const char   *rc_channel_get_legacy_id   (RCChannel *channel);

const char   *rc_channel_get_name        (RCChannel *channel);

const char   *rc_channel_get_alias       (RCChannel *channel);

const char   *rc_channel_get_description (RCChannel *channel);

int           rc_channel_get_priority    (RCChannel *channel,
                                          gboolean   is_subscribed);

const char   *rc_channel_get_pkginfo_file (RCChannel *channel);

const char   *rc_channel_get_path         (RCChannel *channel);

const char   *rc_channel_get_file_path    (RCChannel *channel);

const char   *rc_channel_get_icon_file    (RCChannel *channel);

/* Distro target functions */

void     rc_channel_add_distro_target (RCChannel  *channel,
                                       const char *target);

gboolean rc_channel_has_distro_target (RCChannel  *channel,
                                       const char *target);

/* Subscription management */

gboolean rc_channel_is_subscribed    (RCChannel *channel);

void     rc_channel_set_subscription (RCChannel *channel,
                                      gboolean   subscribed);

/* Other */

gboolean rc_channel_is_system (RCChannel *);

gboolean rc_channel_is_hidden  (RCChannel *);

gboolean rc_channel_is_wildcard (RCChannel *a);
gboolean rc_channel_equal       (RCChannel *a, RCChannel *b);
gboolean rc_channel_equal_id    (RCChannel *a, const char *id);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RC_CHANNEL_H */
