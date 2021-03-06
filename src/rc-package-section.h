/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-section.h
 * Copyright (C) 2000-2002 Ximian, Inc.
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

#ifndef _RC_PACKAGE_SECTION_H
#define _RC_PACKAGE_SECTION_H

#include <glib.h>

typedef enum {
    RC_SECTION_OFFICE = 0,
    RC_SECTION_IMAGING,
    RC_SECTION_PIM,
    RC_SECTION_XAPP,
    RC_SECTION_GAME,
    RC_SECTION_MULTIMEDIA,
    RC_SECTION_INTERNET,
    RC_SECTION_UTIL,
    RC_SECTION_SYSTEM,
    RC_SECTION_DOC,
    RC_SECTION_LIBRARY,
    RC_SECTION_DEVEL,
    RC_SECTION_DEVELUTIL,
    RC_SECTION_MISC,
    RC_SECTION_LAST
} RCPackageSection;

const gchar *rc_package_section_to_string (RCPackageSection section);
const gchar *rc_package_section_to_user_string (RCPackageSection section);

RCPackageSection rc_string_to_package_section (const gchar *section);

#endif
