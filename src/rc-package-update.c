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

RCPackageUpdate *
rc_package_update_copy (RCPackageUpdate *src)
{
    RCPackageUpdate *dest = rc_package_update_new ();

    rc_package_spec_copy (&dest->spec, &src->spec);

    dest->package_url    = g_strdup (src->package_url);
    dest->package_size   = src->package_size;
    dest->installed_size = src->installed_size;
    dest->signature_url  = g_strdup (src->signature_url);
    dest->signature_size = src->signature_size;
    dest->md5sum         = g_strdup (src->md5sum);
    dest->importance     = src->importance;
    dest->description    = g_strdup (src->description);
    dest->hid            = src->hid;
    dest->license        = g_strdup (src->license);

    return dest;
}

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
rc_package_update_slist_copy (RCPackageUpdateSList *old_slist)
{
    RCPackageUpdateSList *iter;
    RCPackageUpdateSList *new_slist = NULL;
    
    for (iter = old_slist; iter != NULL; iter = iter->next) {
        new_slist = g_slist_prepend (new_slist,
                                     rc_package_update_copy (iter->data));
    }
    
    new_slist = g_slist_reverse (new_slist);

    return new_slist;
}

RCPackageUpdateSList *
rc_package_update_slist_sort (RCPackageUpdateSList *old_slist)
{
    RCPackageUpdateSList *new_slist = NULL;

    new_slist =
        g_slist_sort (old_slist, (GCompareFunc) rc_package_spec_compare_name);

    return (new_slist);
}
