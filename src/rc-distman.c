/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-distman.c
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

#include "config.h"
#include "rc-distman.h"

#include <glib.h>
#include <stdlib.h>
#include "rc-distro.h"

#ifdef ENABLE_DPKG
#include "rc-debman.h"
#endif

#ifdef ENABLE_RPM
#include "rc-rpmman.h"
#endif

RCPackman *
rc_distman_new (void)
{
	RCPackman *packman = NULL;
    char *env;

    env = getenv("RC_PACKMAN_TYPE");
    if (env && g_strcasecmp(env, "dpkg") == 0) {
#ifdef ENABLE_DPKG
        packman = RC_PACKMAN(rc_debman_new());
#else
        g_warning ("DPKG support not enabled.");
#endif
    } else if (env && g_strcasecmp(env, "rpm") == 0) {
#ifdef ENABLE_RPM
        packman = RC_PACKMAN(rc_rpmman_new());
#else
        g_warning ("RPM support not enabled.");
#endif
    } else {
        switch (rc_distro_get_package_type ()) {
        case RC_DISTRO_PACKAGE_TYPE_UNKNOWN:
            break;
        case RC_DISTRO_PACKAGE_TYPE_DPKG:
#ifdef ENABLE_DPKG
            packman = RC_PACKMAN (rc_debman_new ());
#else
            g_warning ("DPKG support not enabled.");
#endif
            break;
        case RC_DISTRO_PACKAGE_TYPE_RPM:
#ifdef ENABLE_RPM
            packman = RC_PACKMAN (rc_rpmman_new ());
#else
            g_warning ("RPM support not enabled.");
#endif
            break;
        default:
            g_error ("Cannot determine correct distman for your distro (%s)",
                     rc_distro_get_name ());
            break;
        }
    }

    return packman;
} /* rc_get_distro_packman */

