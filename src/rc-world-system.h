/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-system.h
 *
 * Copyright (C) 2003 Ximian, Inc.
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

#ifndef __RC_WORLD_SYSTEM_H__
#define __RC_WORLD_SYSTEM_H__

#include "rc-world-service.h"

#define RC_TYPE_WORLD_SYSTEM            (rc_world_system_get_type ())
#define RC_WORLD_SYSTEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                        RC_TYPE_WORLD_SYSTEM, RCWorldSystem))
#define RC_WORLD_SYSTEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                        RC_TYPE_WORLD_SYSTEM, \
                                        RCWorldSystemClass))
#define RC_IS_WORLD_SYSTEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                        RC_TYPE_WORLD_SYSTEM))
#define RC_IS_WORLD_SYSTEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                        RC_TYPE_WORLD_SYSTEM))
#define RC_WORLD_SYSTEM_GET_CLASS(obj)  (RC_WORLD_SYSTEM_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCWorldSystem      RCWorldSystem;
typedef struct _RCWorldSystemClass RCWorldSystemClass;

struct _RCWorldSystem {
    RCWorldService world;

    RCPackman *packman;

    RCChannel *system_channel;
    gboolean error_flag;

    guint database_changed_id;
};

struct _RCWorldSystemClass {
    RCWorldServiceClass parent_class;
};

GType rc_world_system_get_type (void);

RCWorld *rc_world_system_new (void);

void rc_world_system_register_service (void);

#endif /* __RC_WORLD_SYSTEM_H__ */

