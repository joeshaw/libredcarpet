/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-import.h
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

#ifndef __RC_WORLD_IMPORT_H__
#define __RC_WORLD_IMPORT_H__

#include <rc-world.h>

void  rc_world_add_channels_from_xml (RCWorld *world,
                                      xmlNode *node);

void  rc_world_add_packages_from_slist (RCWorld *world,
                                        RCPackageSList *package_list);

guint rc_world_add_packages_from_xml (RCWorld *world,
                                      RCChannel *channel,
                                      xmlNode *node);

RCChannel *rc_world_add_channel_from_buffer (RCWorld *world,
                                             const char *channel_name,
                                             const char *alias,
                                             guint32 channel_id,
                                             RCChannelType type,
                                             gchar *buf,
                                             gint compressed_length);

gboolean rc_world_add_packages_from_buffer (RCWorld *world,
                                            RCChannel *channel,
                                            gchar *buf,
                                            gint compressed_length);

#endif /* __RC_WORLD_IMPORT_H__ */

