/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set sw=4 ts=4 et: */
/* rc-packman-private.h
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

#ifndef _RC_PACKMAN_PRIVATE_H
#define _RC_PACKMAN_PRIVATE_H

#include <gtk/gtk.h>

typedef struct _RCPackmanPrivate RCPackmanPrivate;

#include "rc-package-spec.h"
#include "rc-packman.h"

#define RC_PACKMAN_CAP_NONE 				(0)
#define RC_PACKMAN_CAP_VIRTUAL_CONFLICTS	(1 << 0)
#define RC_PACKMAN_CAP_PROVIDE_ALL_VERSIONS	(1 << 1)
#define RC_PACKMAN_CAP_SELF_CONFLICT        (1 << 2)
#define RC_PACKMAN_CAP_LEGACY_EPOCH_HANDLING (1 << 3)

struct _RCPackmanPrivate {
    guint error;
    gchar *reason;

    /* The file extension of this package format, for ease in searching. */
    gchar *extension;

    gboolean busy;

    guint32 capabilities;
};

void rc_packman_set_file_extension (RCPackman *packman,
                                    const gchar *extension);

void rc_packman_clear_error (RCPackman *packman);

void rc_packman_set_error (RCPackman *packman, RCPackmanError error,
                           const gchar *format, ...);

gint rc_packman_generic_version_compare (
    RCPackageSpec *spec1, RCPackageSpec *spec2,
    int (*vercmp)(const char *, const char *));

#endif
