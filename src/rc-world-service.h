/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-service.h
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

#ifndef __RC_WORLD_SERVICE_H__
#define __RC_WORLD_SERVICE_H__

#include "rc-world-store.h"

#define RC_TYPE_WORLD_SERVICE            (rc_world_service_get_type ())
#define RC_WORLD_SERVICE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                          RC_TYPE_WORLD_SERVICE, RCWorldService))
#define RC_WORLD_SERVICE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                          RC_TYPE_WORLD_SERVICE, \
                                          RCWorldServiceClass))
#define RC_IS_WORLD_SERVICE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                          RC_TYPE_WORLD_SERVICE))
#define RC_IS_WORLD_SERVICE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                          RC_TYPE_WORLD_SERVICE))
#define RC_WORLD_SERVICE_GET_CLASS(obj)  (RC_WORLD_SERVICE_CLASS (G_OBJECT_GET_CLASS (obj)))

typedef struct _RCWorldService RCWorldService;
typedef struct _RCWorldServiceClass RCWorldServiceClass;

typedef gboolean (*RCWorldServiceAssembleFn) (RCWorldService  *worldserv,
                                              GError         **error);


struct _RCWorldService {
    RCWorldStore parent;

    char *url;
    char *name;
    char *unique_id;

    gboolean is_sticky;    /* if TRUE, can't be unmounted */
    gboolean is_invisible; /* ... to users */
    gboolean is_unsaved;   /* Never save into the services.xml file */
    gboolean is_singleton; /* only one such service at a time.  FIXME: broken */
};

struct _RCWorldServiceClass {
    RCWorldStoreClass parent_class;

    RCWorldServiceAssembleFn assemble_fn;
};

GType rc_world_service_get_type (void);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/* Instance methods */
char *rc_world_service_get_url    (RCWorldService *service);
void  rc_world_service_set_url    (RCWorldService *service, const char *url);
char *rc_world_service_get_name   (RCWorldService *service);
void  rc_world_service_set_name   (RCWorldService *service, const char *name);
char *rc_world_service_get_id     (RCWorldService *service);
void  rc_world_service_set_id     (RCWorldService *service, const char *id);
gboolean rc_world_service_get_is_sticky (RCWorldService *service);
void     rc_world_service_set_is_sticky (RCWorldService *service,
                                         gboolean sticky);
gboolean rc_world_service_get_is_invisible (RCWorldService *service);
void     rc_world_service_set_is_invisible (RCWorldService *service,
                                            gboolean invisible);
gboolean rc_world_service_get_is_singleton (RCWorldService *service);
void     rc_world_service_set_is_singleton (RCWorldService *service,
                                            gboolean singleton);
  
/* Static methods */
void  rc_world_service_register   (const char *scheme, GType world_type);
void  rc_world_service_unregister (const char *scheme);
GType rc_world_service_lookup     (const char *scheme);

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCWorld  *rc_world_service_mount (const char *url, GError **error);

/* only used for bindings */
void      rc_world_service_set_assemble_function (RCWorldService *service,
                                                  RCWorldServiceAssembleFn assemble_fn);

#endif /* __RC_WORLD_SERVICE_H__ */

