/* rc-debug.h
 * Copyright (C) 2000-2002 Ximian, Inc.
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

typedef enum {
    RC_DEBUG_LEVEL_ALWAYS   = -1,
    RC_DEBUG_LEVEL_NONE     = 0,
    RC_DEBUG_LEVEL_ERROR    = 1,
    RC_DEBUG_LEVEL_CRITICAL = 2,
    RC_DEBUG_LEVEL_WARNING  = 3,
    RC_DEBUG_LEVEL_MESSAGE  = 4,
    RC_DEBUG_LEVEL_INFO     = 5,
    RC_DEBUG_LEVEL_DEBUG    = 6,
} RCDebugLevel;

typedef void (*RCDebugFn) (const char *message,
                           RCDebugLevel level,
                           gpointer user_data);

guint
rc_debug_add_handler (RCDebugFn fn,
                      RCDebugLevel level,
                      gpointer user_data);

void
rc_debug_remove_handler (guint id);

void
rc_debug_full (RCDebugLevel  level,
               const char   *format,
               ...);

#ifdef RC_DEBUG_VERBOSE

const char *
rc_debug_helper (const char *format,
                 ...);

#define rc_debug(level, format...) \
        rc_debug_full(level, "%s (%s, %s:%d)", rc_debug_helper (format), \
                      __FUNCTION__, __FILE__, __LINE__)

#else

#define rc_debug rc_debug_full

#endif

#endif
