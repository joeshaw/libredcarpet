/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-distman.c
 *
 * Copyright (C) 2000-2003 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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

#include "rc-debug.h"

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
    char *preferred;

    preferred = getenv ("RC_PACKMAN_TYPE");

    if (!preferred) {
#ifdef DEFAULT_BACKEND
        preferred = DEFAULT_BACKEND;
#else
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "No packaging systems available");
        return NULL;
#endif
    }

    if (g_strcasecmp (preferred, "dpkg") == 0) {
#ifdef ENABLE_DPKG
        packman = RC_PACKMAN (rc_debman_new ());
#else
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "dpkg support not enabled.");
#endif
    } else if (g_strcasecmp (preferred, "rpm") == 0) {
#ifdef ENABLE_RPM
        packman = RC_PACKMAN (rc_rpmman_new ());
#else
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "RPM support not enabled.");
#endif
    } else {
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "Invalid packaging backend: %s",
                  preferred);
    }

    return packman;
}

