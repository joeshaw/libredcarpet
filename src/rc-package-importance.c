/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-importance.c
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

#include <string.h>

#include "rc-package-importance.h"
#include "rc-debug.h"

const gchar *
rc_package_importance_to_string (RCPackageImportance importance)
{
    switch (importance) {
    case RC_IMPORTANCE_INVALID:
        return ("invalid");
    case RC_IMPORTANCE_NECESSARY:
        return ("necessary");
    case RC_IMPORTANCE_URGENT:
        return ("urgent");
    case RC_IMPORTANCE_SUGGESTED:
        return ("suggested");
    case RC_IMPORTANCE_FEATURE:
        return ("feature");
    case RC_IMPORTANCE_MINOR:
        return ("minor");
    case RC_IMPORTANCE_NEW:
        return ("new");
    case RC_IMPORTANCE_MAX:
        return ("max");
    default:
        rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid section number %s\n",
                  importance);
        return ("invalid");
    }
}

RCPackageImportance
rc_string_to_package_importance (const gchar *importance)
{
    if (!importance) {
        goto INVALID;
    }

    switch (*importance) {
    case 'f':
        if (!strcmp (importance, "feature")) {
            return (RC_IMPORTANCE_FEATURE);
        }
        goto INVALID;
    case 'm':
        if (!strcmp (importance, "max")) {
            return (RC_IMPORTANCE_MAX);
        }
        if (!strcmp (importance, "minor")) {
            return (RC_IMPORTANCE_MINOR);
        }
        goto INVALID;
    case 'n':
        if (!strcmp (importance, "necessary")) {
            return (RC_IMPORTANCE_NECESSARY);
        }
        if (!strcmp (importance, "new")) {
            return (RC_IMPORTANCE_NEW);
        }
        goto INVALID;
    case 's':
        if (!strcmp (importance, "suggested")) {
            return (RC_IMPORTANCE_SUGGESTED);
        }
        goto INVALID;
    case 'u':
        if (!strcmp (importance, "urgent")) {
            return (RC_IMPORTANCE_URGENT);
        }
        goto INVALID;
    }

  INVALID:
    rc_debug (RC_DEBUG_LEVEL_WARNING, "invalid importance %s\n", importance);

    return (RC_IMPORTANCE_INVALID);
}
