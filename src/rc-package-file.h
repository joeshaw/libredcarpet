/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-file.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#ifndef _RC_PACKAGE_FILE_H
#define _RC_PACKAGE_FILE_H

#include <glib.h>

typedef struct {
    char *filename;

    gsize size;

    char *md5sum;

    guint uid;
    guint gid;

    guint16 mode;
    gint32 mtime;
    gboolean ghost; /* Ghost file */
} RCPackageFile;

typedef GSList RCPackageFileSList;

RCPackageFile *rc_package_file_new (void);

#endif /* _RC_PACKAGE_FILE_H */
