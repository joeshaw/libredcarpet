/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debman-private.h
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

#ifndef _RC_DEBMAN_PRIVATE_H
#define _RC_DEBMAN_PRIVATE_H

#include <glib.h>

typedef struct _RCDebmanPrivate RCDebmanPrivate;

struct _RCDebmanPrivate {
    int lock_fd;

    GHashTable *package_hash;
    gboolean hash_valid;

    const gchar *status_file;
    gchar *rc_status_file;

    time_t db_mtime;
    guint db_watcher_cb;
};

#endif /* _RC_DEBMAN_PRIVATE_H */
