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

#include <stdlib.h>

#include "rc-distro.h"
#include "rc-distman.h"

#ifdef WITH_DPKG
#include "rc-debman.h"
#endif

#if (defined(WITH_RPM3) || defined(WITH_RPM4)) && defined(HAVE_LIBRPM)
#include "rc-rpmman.h"
#endif

RCPackman *
rc_distman_new (void)
{
    RCDistroType *dtype;
	RCPackman *packman = NULL;
    char *env;

    dtype = rc_figure_distro ();

    if (!dtype) {
        g_error ("I can't figure out what distribution you're on!\n");
    }

    env = getenv("RC_PACKMAN_TYPE");
    if (env && g_strcasecmp(env, "dpkg") == 0) {
#ifdef WITH_DPKG
        packman = RC_PACKMAN(rc_debman_new());
#else
        g_warning ("DPKG support not enabled.");
#endif
    } else if (env && g_strcasecmp(env, "rpm") == 0) {
#if (defined(WITH_RPM3) || defined(WITH_RPM4)) && defined(HAVE_LIBRPM)
        packman = RC_PACKMAN(rc_rpmman_new());
#else
        g_warning ("RPM support not enabled.");
#endif
    } else {
        switch (dtype->pkgtype) {
            case RC_PKG_UNSUPPORTED:
            case RC_PKG_UNKNOWN:
                break;
            case RC_PKG_DPKG:
#ifdef WITH_DPKG
                packman = RC_PACKMAN (rc_debman_new ());
#else
                g_warning ("DPKG support not enabled.");
#endif
                break;
            case RC_PKG_RPM:
#if (defined(WITH_RPM3) || defined(WITH_RPM4)) && defined(HAVE_LIBRPM)
                packman = RC_PACKMAN (rc_rpmman_new ());
#else
                g_warning ("RPM support not enabled.");
#endif
                break;
            default:
                g_error ("Cannot determine correct distman for your distro (%s)", dtype->unique_name);
                break;
        }
    }

    return packman;
} /* rc_get_distro_packman */

