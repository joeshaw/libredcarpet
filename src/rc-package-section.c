/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-section.c
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

#include <string.h>

#include "rc-package-section.h"
#include "rc-debug.h"

const gchar *
rc_package_section_to_string (RCPackageSection section)
{
    switch (section) {
    case RC_SECTION_OFFICE:
        return ("office");
    case RC_SECTION_IMAGING:
        return ("imaging");
    case RC_SECTION_PIM:
        return ("pim");
    case RC_SECTION_GAME:
        return ("game");
    case RC_SECTION_MISC:
        return ("misc");
    case RC_SECTION_MULTIMEDIA:
        return ("multimedia");
    case RC_SECTION_INTERNET:
        return ("internet");
    case RC_SECTION_UTIL:
        return ("util");
    case RC_SECTION_SYSTEM:
        return ("system");
    case RC_SECTION_DOC:
        return ("doc");
    case RC_SECTION_DEVEL:
        return ("devel");
    case RC_SECTION_DEVELUTIL:
        return ("develutil");
    case RC_SECTION_LIBRARY:
        return ("library");
    case RC_SECTION_XAPP:
        return ("xapp");
    default:
        rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid section number %d\n",
                  section);
        return ("misc");
    }
}

const gchar *
rc_package_section_to_user_string (RCPackageSection section)
{
    switch (section) {
    case RC_SECTION_OFFICE:
        return ("Productivity Applications");
    case RC_SECTION_IMAGING:
        return ("Imaging");
    case RC_SECTION_PIM:
        return ("Personal Information Management");
    case RC_SECTION_GAME:
        return ("Games");
    case RC_SECTION_MISC:
        return ("Miscellaneous");
    case RC_SECTION_MULTIMEDIA:
        return ("Multimedia");
    case RC_SECTION_INTERNET:
        return ("Internet Applications");
    case RC_SECTION_UTIL:
        return ("Utilities");
    case RC_SECTION_SYSTEM:
        return ("System Packages");
    case RC_SECTION_DOC:
        return ("Documentation");
    case RC_SECTION_DEVEL:
        return ("Development Packages");
    case RC_SECTION_DEVELUTIL:
        return ("Development Utilities");
    case RC_SECTION_LIBRARY:
        return ("Libraries");
    case RC_SECTION_XAPP:
        return ("X Applications");
    default:
        rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid section number %d\n",
                  section);
        return ("Miscellaneous");
    }
}

RCPackageSection
rc_string_to_package_section (const gchar *section)
{
    g_return_val_if_fail (section, RC_SECTION_MISC);

    switch (*section) {
    case 'd':
        if (!strcmp (section, "develutil")) {
            return (RC_SECTION_DEVELUTIL);
        }
        if (!strcmp (section, "devel")) {
            return (RC_SECTION_DEVEL);
        }
        if (!strcmp (section, "doc")) {
            return (RC_SECTION_DOC);
        }
        goto INVALID;
    case 'g':
        if (!strcmp (section, "game")) {
            return (RC_SECTION_GAME);
        }
        goto INVALID;
    case 'i':
        if (!strcmp (section, "imaging")) {
            return (RC_SECTION_IMAGING);
        }
        if (!strcmp (section, "internet")) {
            return (RC_SECTION_INTERNET);
        }
        goto INVALID;
    case 'l':
        if (!strcmp (section, "library")) {
            return (RC_SECTION_LIBRARY);
        }
        goto INVALID;
    case 'm':
        if (!strcmp (section, "misc")) {
            return (RC_SECTION_MISC);
        }
        if (!strcmp (section, "multimedia")) {
            return (RC_SECTION_MULTIMEDIA);
        }
        goto INVALID;
    case 'o':
        if (!strcmp (section, "office")) {
            return (RC_SECTION_OFFICE);
        }
        goto INVALID;
    case 'p':
        if (!strcmp (section, "pim")) {
            return (RC_SECTION_PIM);
        }
        goto INVALID;
    case 's':
        if (!strcmp (section, "system")) {
            return (RC_SECTION_SYSTEM);
        }
        goto INVALID;
    case 'u':
        if (!strcmp (section, "util")) {
            return (RC_SECTION_UTIL);
        }
        goto INVALID;
    case 'x':
        if (!strcmp (section, "xapp")) {
            return (RC_SECTION_XAPP);
        }
        goto INVALID;
    }

    return (RC_SECTION_MISC);

  INVALID:
    rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid section name %s\n", section);

    return (RC_SECTION_MISC);
}
