/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-distman.c: glue between packman and distro
 *
 * Copyright 2000 Helix Code, Inc.
 *
 * Author: Jacob Berkman <jacob@Helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"
#include <stdlib.h>

#include "rc-distro.h"

#include "rc-distman.h"

#include "rc-debman.h"

#ifdef HAVE_LIBRPM
#include "rc-rpmman.h"
#endif

RCPackman *
rc_distman_new (void)
{
    RCDistroType *dtype;
	RCPackman *packman = NULL;
    char *env;

    dtype = rc_figure_distro ();

    env = getenv("RC_PACKMAN_TYPE");
    if (env && g_strcasecmp(env, "dpkg") == 0)
        packman = RC_PACKMAN(rc_debman_new());
    else if (env && g_strcasecmp(env, "rpm") == 0) {
#ifdef HAVE_LIBRPM
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
                packman = RC_PACKMAN (rc_debman_new ());
                break;
            case RC_PKG_RPM:
#ifdef HAVE_LIBRPM
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

