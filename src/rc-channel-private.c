/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-channel-private.c
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

#include "rc-channel.h"
#include "rc-channel-private.h"

RCChannel *
rc_channel_new (void)
{
    RCChannel *channel;

    channel = g_new0 (RCChannel, 1);

    channel->refs = 1;
    
    channel->type = RC_CHANNEL_TYPE_HELIX; /* default */

    channel->priority = -1;
    channel->priority_unsubd = -1;
    channel->priority_current = -1;

    return (channel);
} /* rc_channel_new */
