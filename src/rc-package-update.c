/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-update.c
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
#include <stdlib.h>

#include <libxml/tree.h>

#include "rc-package-update.h"
#include "xml-util.h"

RCPackageUpdate *
rc_package_update_new ()
{
    RCPackageUpdate *update = g_new0 (RCPackageUpdate, 1);

    return (update);
} /* rc_package_update_new */

void
rc_package_update_free (RCPackageUpdate *update)
{
    g_return_if_fail (update);

    rc_package_spec_free_members(&update->spec);

    g_free (update->package_url);

    g_free (update->signature_url);

    g_free (update->md5sum);

    g_free (update->description);

    g_free (update->license);

    g_free (update);
} /* rc_package_update_free */

void
rc_package_update_slist_free (RCPackageUpdateSList *update_slist)
{
    g_slist_foreach (update_slist, (GFunc) rc_package_update_free, NULL);

    g_slist_free (update_slist);
} /* rc_package_update_slist_free */

RCPackageUpdateSList *
rc_package_update_slist_sort (RCPackageUpdateSList *old_slist)
{
    RCPackageUpdateSList *new_slist = NULL;

    new_slist =
        g_slist_sort (old_slist, (GCompareFunc) rc_package_spec_compare_name);

    return (new_slist);
}
