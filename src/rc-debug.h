/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debug.h
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

#ifndef _RC_DEBUG_H
#define _RC_DEBUG_H

#include <glib.h>

#include <stdarg.h>
#include <stdio.h>

#ifdef EXTRA_DEBUGGING
#define RC_ENTRY fprintf (stderr, "==> %s (%s, line %d)\n", __FUNCTION__, \
                          __FILE__, __LINE__);
#define RC_EXIT  fprintf (stderr, "<== %s (%s, line %d)\n", __FUNCTION__, \
                          __FILE__, __LINE__);
#else
#define RC_ENTRY ;
#define RC_EXIT ;
#endif

typedef enum _RCDebugLevel RCDebugLevel;

enum _RCDebugLevel {
    RC_DEBUG_LEVEL_NONE     = 0,
    RC_DEBUG_LEVEL_ERROR    = 1,
    RC_DEBUG_LEVEL_CRITICAL = 2,
    RC_DEBUG_LEVEL_WARNING  = 3,
    RC_DEBUG_LEVEL_MESSAGE  = 4,
    RC_DEBUG_LEVEL_INFO     = 5,
    RC_DEBUG_LEVEL_DEBUG    = 6,
};

guint rc_debug_get_display_level (void);

void rc_debug_set_display_level (guint level);

guint rc_debug_get_log_level (void);

void rc_debug_set_log_level (guint level);

void rc_debug_set_log_file (FILE *file);

void rc_debug (RCDebugLevel level, gchar *format, ...);

#endif
