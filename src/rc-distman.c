/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-distman.c: glue between packman and distro
 *
 * Copyright 2000 Helix Code, Inc.
 *
 * Author: Jacob Berkman <jacob@Helixcode.com>
 *
 */

#include "config.h"

#include "rc-distman.h"

#include "rc-debman.h"

#ifdef HAVE_LIBRPM
#include "rc-rpmman.h"
#endif

RCPackman *
rc_distman_new (void)
{
    DistributionInfo distro;
	RCPackman *packman = NULL;

    rc_determine_distro (&distro);

    switch (distro.type) {
	case DISTRO_UNSUPPORTED:
	case DISTRO_UNKNOWN:
        break;
    case DISTRO_DEBIAN:
	case DISTRO_COREL:
        packman = RC_PACKMAN (rc_debman_new ());
        break;
    default:
#ifdef HAVE_LIBRPM
        packman = RC_PACKMAN (rc_rpmman_new ());
#else
        g_warning ("RPM support not enabled.");
#endif
        break;
    }

    return packman;
} /* rc_get_distro_packman */

