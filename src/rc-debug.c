/* rc-debug.c
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

#include <glib.h>

#include "rc-debug.h"

#include <stdarg.h>
#include <stdio.h>

typedef struct _RCDebugHandler RCDebugHandler;

struct _RCDebugHandler {
    RCDebugFn    fn;
    RCDebugLevel level;
    gpointer     user_data;
    guint        id;
};

static GSList *handlers = NULL;

guint
rc_debug_add_handler (RCDebugFn    fn,
                      RCDebugLevel level,
                      gpointer     user_data)
{
    RCDebugHandler *handler;

    g_assert (fn);

    handler = g_new0 (RCDebugHandler, 1);

    handler->fn = fn;
    handler->level = level;
    handler->user_data = user_data;

    if (handlers)
        handler->id = ((RCDebugHandler *) handlers->data)->id + 1;
    else
        handler->id = 1;

    handlers = g_slist_prepend (handlers, handler);

    return handler->id;
}

void
rc_debug_remove_handler (guint id)
{
    GSList *iter;

    iter = handlers;
    while (iter) {
        RCDebugHandler *handler = (RCDebugHandler *)iter->data;

        if (handler->id == id) {
            handlers = g_slist_remove_link (handlers, iter);
            g_free (handler);
            return;
        }

        iter = iter->next;
    }

    rc_debug (RC_DEBUG_LEVEL_WARNING, "Couldn't find debug handler %d", id);
}

const char *
rc_debug_helper (const char *format,
                 ...)
{
    va_list args;
    static char *str = NULL;

    if (str)
        g_free (str);

    va_start (args, format);
    str = g_strdup_vprintf (format, args);
    va_end (args);

    return str;
}

void
rc_debug_full (RCDebugLevel  level,
               const char   *format,
               ...)
{
    va_list args;
    GSList *iter;
    char *str;

    va_start (args, format);
    str = g_strdup_vprintf (format, args);
    va_end (args);

    iter = handlers;
    while (iter) {
        RCDebugHandler *handler = (RCDebugHandler *)iter->data;

        if ((handler->level == RC_DEBUG_LEVEL_ALWAYS) ||
            (level <= handler->level))
            handler->fn (str, level, handler->user_data);

        iter = iter->next;
    }

    g_free (str);
}
