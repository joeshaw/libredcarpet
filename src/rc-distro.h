/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-distro.h
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
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

#ifndef RC_DISTRO_H_
#define RC_DISTRO_H_

#include <glib.h>

#include <time.h>

#include "rc-arch.h"

typedef struct _RCDistro RCDistro;

typedef enum {
    RC_DISTRO_PACKAGE_TYPE_UNKNOWN,
    RC_DISTRO_PACKAGE_TYPE_RPM,
    RC_DISTRO_PACKAGE_TYPE_DPKG,
} RCDistroPackageType;

typedef enum {
    RC_DISTRO_STATUS_UNSUPPORTED,
    RC_DISTRO_STATUS_PRESUPPORTED,
    RC_DISTRO_STATUS_SUPPORTED,
    RC_DISTRO_STATUS_DEPRECATED,
    RC_DISTRO_STATUS_RETIRED,
} RCDistroStatus;

RCDistro *rc_distro_parse_xml (const char *xml_buf,
                                          guint compressed_length);

RCDistro *rc_distro_get_current (void);

void rc_distro_free (RCDistro *);

const char          *rc_distro_get_name         (RCDistro *);
const char          *rc_distro_get_version      (RCDistro *);
RCArch               rc_distro_get_arch         (RCDistro *);
RCDistroPackageType  rc_distro_get_package_type (RCDistro *);
const char          *rc_distro_get_target       (RCDistro *);
const char          *rc_distro_get_role         (RCDistro *);
RCDistroStatus       rc_distro_get_status       (RCDistro *);
time_t               rc_distro_get_death_date   (RCDistro *);

#endif
