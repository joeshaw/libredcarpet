/*
 * rc-package-set.h: Tools for parsing and manipulating package
 * sets
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Authors: Vladimir Vukicevic <vladimir@helixcode.com>
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

#ifndef _RC_PACKAGE_SET_H
#define _RC_PACKAGE_SET_H

#include <glib.h>
#include <libredcarpet/rc-package.h>

typedef struct _RCPackageSet RCPackageSet;

struct _RCPackageSet {
    gchar *name;
    gchar *description;
    GSList *packages; /* of gchar *, string names of packages */
};

typedef GSList RCPackageSetSList;

RCPackageSet *rc_package_set_new (void);

void rc_package_set_free (RCPackageSet *set);

void rc_package_set_slist_free (RCPackageSetSList *set_list);

RCPackageSetSList *rc_package_set_parse (char *buf, int compressed_length);

#endif _RC_PACKAGE_SET_H
