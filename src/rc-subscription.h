/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-subscription.h
 *
 * Copyright (C) 2003 Ximian, Inc
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

#ifndef __RC_SUBSCRIPTION_H__
#define __RC_SUBSCRIPTION_H__

#include "rc-channel.h"

/* Calling this function with a NULL channel will cause
   the subscriptions to be loaded from disk (if this hasn't
   happened already). */

void     rc_subscription_set_file   (const char *file);

gboolean rc_subscription_get_status (RCChannel *channel);

void     rc_subscription_set_status (RCChannel *channel,
                                     gboolean   channel_is_subscribed);



#endif /* __RC_SUBSCRIPTION_H__ */
