/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-file.c
 *
 * Copyright (C) 2003 Ximian, Inc.
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

#include <config.h>
#include "rc-package-file.h"

RCPackageFile *
rc_package_file_new (void)
{
    RCPackageFile *file;

    file = g_new0 (RCPackageFile, 1);

    return file;
}

void
rc_package_file_free (RCPackageFile *file)
{
    g_return_if_fail (file);

    g_free (file->filename);
    g_free (file->md5sum);
    g_free (file->link_target);

    g_free (file);
}

void
rc_package_file_slist_free (RCPackageFileSList *files)
{
    g_slist_foreach (files, (GFunc) rc_package_file_free, NULL);
    g_slist_free (files);
}
