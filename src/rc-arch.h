/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-arch.h
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

#ifndef RC_ARCH_H_
#define RC_ARCH_H_

#include <glib.h>

typedef enum {
    RC_ARCH_UNKNOWN = -1,
    RC_ARCH_NOARCH = 0,
    RC_ARCH_I386,
    RC_ARCH_I486,
    RC_ARCH_I586,
    RC_ARCH_I686,
    RC_ARCH_ATHLON,
    RC_ARCH_PPC,
    RC_ARCH_PPC64,
    RC_ARCH_S390,
    RC_ARCH_S390X,
    RC_ARCH_SPARC,
    RC_ARCH_SPARC64,
} RCArch;

RCArch
rc_arch_from_string (const char *arch_name);

const char *
rc_arch_to_string (RCArch arch);

RCArch
rc_arch_get_system_arch (void);

GSList *
rc_arch_get_compat_list (RCArch arch);

gint
rc_arch_get_compat_score (GSList *compat_arch_list, RCArch arch);

#endif
