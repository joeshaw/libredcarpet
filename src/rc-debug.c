/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debug.c
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

#include <glib.h>

#include "rc-debug.h"

#include <stdarg.h>
#include <stdio.h>

static RCDebugLevel display_level = RC_DEBUG_LEVEL_WARNING;
static RCDebugLevel log_level     = RC_DEBUG_LEVEL_DEBUG;

static FILE *log_file = NULL;

guint
rc_debug_get_display_level ()
{
    return (display_level);
}

void
rc_debug_set_display_level (guint level)
{
    display_level = level;
}

guint
rc_debug_get_log_level ()
{
    return (log_level);
}

void
rc_debug_set_log_level (guint level)
{
    log_level = level;
}

void
rc_debug_set_log_file (FILE *file)
{
    log_file = file;
}

void
rc_debug (RCDebugLevel level, gchar *format, ...)
{
    va_list args;

    va_start (args, format);

    if (level <= display_level) {
        vfprintf (stderr, format, args);
    }

    if (level <= log_level && log_file) {
        vfprintf (log_file, format, args);
    }

    va_end (args);
}
