/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rpmman.h
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

#ifndef _RC_RPMMAN_H
#define _RC_RPMMAN_H

#include <gtk/gtk.h>

#include <rpm/rpmlib.h>

#include "rc-packman.h"

#define GTK_TYPE_RC_RPMMAN        (rc_rpmman_get_type ())
#define RC_RPMMAN(obj)            (GTK_CHECK_CAST ((obj), \
                                   GTK_TYPE_RC_RPMMAN, RCRpmman))
#define RC_RPMMAN_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                   GTK_TYPE_RC_RPMMAN, \
                                   RCRpmmanClass))
#define IS_RC_RPMMAN(obj)         (GTK_CHECK_TYPE ((obj), \
                                   GTK_TYPE_RC_RPMMAN))
#define IS_RC_RPMMAN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
                                   GTK_TYPE_RC_RPMMAN))

typedef struct _RCRpmman      RCRpmman;
typedef struct _RCRpmmanClass RCRpmmanClass;

struct _RCRpmman {
    RCPackman parent;

    rpmdb read_db;

    gchar *rpmroot;
};

struct _RCRpmmanClass {
    RCPackmanClass parent_class;
};

guint rc_rpmman_get_type (void);

RCRpmman *rc_rpmman_new (void);

#endif /* _RC_RPMMAN_H */
