/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-distro.h
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef RC_DISTRO_H_
#define RC_DISTRO_H_

#ifndef RC_DISTRO_NO_GLIB
#include <glib.h>
#endif

typedef enum {
    RC_PKG_RPM,
    RC_PKG_DPKG,
    RC_PKG_UNSUPPORTED,
    RC_PKG_UNKNOWN
} RCPackageType;

typedef enum {
    RC_ARCH_UNKNOWN = 0,
    RC_ARCH_ANY,
    RC_ARCH_IA32,
    RC_ARCH_IA64,
    RC_ARCH_PPC,
    RC_ARCH_ALPHA,
    RC_ARCH_ARM,
    RC_ARCH_SPARC,
    RC_ARCH_SPARC64,
    RC_ARCH_MIPS,
    RC_ARCH_MIPS64,
} RCDistroArch;

typedef struct _RCDistroType {
    char *unique_name;
    char *pretend_name; /* i.e. connectiva-foo-i386 is the same as redhat-62-i386 */
    char *full_name;
    char *ver_string;
    RCPackageType pkgtype;
    RCDistroArch arch;
    char *extra_stuff;          /* language? */
#ifndef RC_DISTRO_NO_GLIB
    GHashTable *extra_hash;
#else
    void *extra_hash;
#endif
} RCDistroType;

RCDistroType *rc_figure_distro (void);
const char *rc_distro_option_lookup(RCDistroType *distro, const char *key);

#endif /* RC_DISTRO_H_ */
