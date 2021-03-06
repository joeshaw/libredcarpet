/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-importance.h
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

#ifndef _RC_PACKAGE_IMPORTANCE_H
#define _RC_PACKAGE_IMPORTANCE_H

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef enum {
    RC_IMPORTANCE_INVALID = -1,

    RC_IMPORTANCE_NECESSARY,
    RC_IMPORTANCE_URGENT,
    RC_IMPORTANCE_SUGGESTED,
    RC_IMPORTANCE_FEATURE,
    RC_IMPORTANCE_MINOR,

    /* Not a real importance */
    RC_IMPORTANCE_LAST
} RCPackageImportance;

const gchar *rc_package_importance_to_string (RCPackageImportance importance);

RCPackageImportance rc_string_to_package_importance (const gchar *importance);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _RC_PACKAGE_IMPORTANCE_H */
