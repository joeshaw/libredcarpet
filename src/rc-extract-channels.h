/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-extract-channels.h
 *
 * Copyright (C) 2002-2003 Ximian, Inc.
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

#ifndef __RC_EXTRACT_CHANNELS_H__
#define __RC_EXTRACT_CHANNELS_H__

#include "rc-channel.h"

gint rc_extract_channels_from_helix_buffer (const guint8 *data, int len,
                                            RCChannelFn callback,
                                            gpointer user_data);

gint rc_extract_channels_from_helix_file (const char *filename,
                                          RCChannelFn callback,
                                          gpointer user_data);

#endif /* __RC_EXTRACT_CHANNELS_H__ */

