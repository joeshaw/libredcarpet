/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-section.c
 *
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

#include "rc-package-section.h"
#include "rc-debug.h"

#include <string.h>

const gchar *
rc_package_section_to_string (RCPackageSection section)
{
    switch (section) {
    case SECTION_OFFICE:
        return ("SECTION_OFFICE");
    case SECTION_IMAGING:
        return ("SECTION_IMAGING");
    case SECTION_PIM:
        return ("SECTION_PIM");
    case SECTION_GAME:
        return ("SECTION_GAME");
    case SECTION_MISC:
        return ("SECTION_MISC");
    case SECTION_MULTIMEDIA:
        return ("SECTION_MULTIMEDIA");
    case SECTION_INTERNET:
        return ("SECTION_INTERNET");
    case SECTION_UTIL:
        return ("SECTION_UTIL");
    case SECTION_SYSTEM:
        return ("SECTION_SYSTEM");
    case SECTION_DOC:
        return ("SECTION_DOC");
    case SECTION_DEVEL:
        return ("SECTION_DEVEL");
    case SECTION_DEVELUTIL:
        return ("SECTION_DEVELUTIL");
    case SECTION_LIBRARY:
        return ("SECTION_LIBRARY");
    case SECTION_XAPP:
        return ("SECTION_XAPP");
    default:
        rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid section number %d\n",
                  section);
        return ("SECTION_MISC");
    }
}

RCPackageSection
rc_string_to_package_section (const gchar *section)
{
    const gchar *ptr;

    if (!section || strncmp (section, "SECTION_", strlen ("SECTION_"))) {
        goto INVALID;
    }

    ptr = section + strlen ("SECTION_");

    if (!*ptr) {
        goto INVALID;
    }

    switch (*ptr) {
    case 'D':
        if (!strcmp (ptr, "DEVELUTIL")) {
            return (SECTION_DEVELUTIL);
        }
        if (!strcmp (ptr, "DEVEL")) {
            return (SECTION_DEVEL);
        }
        if (!strcmp (ptr, "DOC")) {
            return (SECTION_DOC);
        }
        goto INVALID;
    case 'G':
        if (!strcmp (ptr, "GAME")) {
            return (SECTION_GAME);
        }
        goto INVALID;
    case 'I':
        if (!strcmp (ptr, "IMAGING")) {
            return (SECTION_IMAGING);
        }
        if (!strcmp (ptr, "INTERNET")) {
            return (SECTION_INTERNET);
        }
        goto INVALID;
    case 'L':
        if (!strcmp (ptr, "LIBRARY")) {
            return (SECTION_LIBRARY);
        }
        goto INVALID;
    case 'M':
        if (!strcmp (ptr, "MISC")) {
            return (SECTION_MISC);
        }
        if (!strcmp (ptr, "MULTIMEDIA")) {
            return (SECTION_MULTIMEDIA);
        }
        goto INVALID;
    case 'O':
        if (!strcmp (ptr, "OFFICE")) {
            return (SECTION_OFFICE);
        }
        goto INVALID;
    case 'P':
        if (!strcmp (ptr, "PIM")) {
            return (SECTION_PIM);
        }
        goto INVALID;
    case 'S':
        if (!strcmp (ptr, "SYSTEM")) {
            return (SECTION_SYSTEM);
        }
        goto INVALID;
    case 'U':
        if (!strcmp (ptr, "UTIL")) {
            return (SECTION_UTIL);
        }
        goto INVALID;
    case 'X':
        if (!strcmp (ptr, "XAPP")) {
            return (SECTION_XAPP);
        }
        goto INVALID;
    }

    return (SECTION_MISC);

  INVALID:
    rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid section name %s\n", section);

    return (SECTION_MISC);
}
