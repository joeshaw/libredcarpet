/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public License
 *    as published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *    GNU Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _RC_PACKMAN_PRIVATE_H
#define _RC_PACKMAN_PRIVATE_H

#include "rc-packman.h"

struct _RCPackmanPrivate {
    guint error;
    gchar *reason;

    gboolean busy;

    RCPackmanFeatures features;
};

void rc_packman_clear_error (RCPackman *packman);

void rc_packman_set_error (RCPackman *packman, RCPackmanError error,
                           const gchar *format, ...);

gint rc_packman_generic_version_compare (
    RCPackageSpec *spec1, RCPackageSpec *spec2,
    int (*vercmp)(const char *, const char *));
                                         

#endif
