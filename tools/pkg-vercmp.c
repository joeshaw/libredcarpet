/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * pkg-vercmp.c
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

#include <config.h>

#include "libredcarpet.h"

int main (int argc, char *argv[])
{
    RCPackman *packman = NULL;
    gboolean has_epoch;
    guint32 epoch;
    char *version;
    char *release;
    RCPackageSpec *spec1, *spec2;
    int rc;

    if (argc < 3)
        g_error ("Usage: %s <version> <version>", argv[0]);

    g_type_init ();

    packman = rc_distman_new ();

    if (!packman)
        g_error ("Couldn't access the packaging system");

    if (rc_packman_get_error (packman)) {
        g_error ("Couldn't access the packaging system: %s",
                 rc_packman_get_reason (packman));
    }

    if (!rc_packman_parse_version (packman, argv[1], &has_epoch, &epoch,
                                   &version, &release)) {
        g_error ("Couldn't parse version string: %s", argv[1]);
    }

    spec1 = g_new0 (RCPackageSpec, 1);
    rc_package_spec_init (spec1, "package", has_epoch,
                          epoch, version, release, RC_ARCH_UNKNOWN);

    if (!rc_packman_parse_version (packman, argv[2], &has_epoch, &epoch,
                                   &version, &release)) {
        g_error ("Couldn't parse version string: %s", argv[2]);
    }

    spec2 = g_new0 (RCPackageSpec, 2);
    rc_package_spec_init (spec2, "package", has_epoch,
                          epoch, version, release, RC_ARCH_UNKNOWN);

    rc = rc_packman_version_compare (packman, spec1, spec2);
    if (rc > 0)
        printf ("gt\n");
    else if (rc < 0)
        printf ("lt\n");
    else
        printf ("eq\n");

    return 0;
}

