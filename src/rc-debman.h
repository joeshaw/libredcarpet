/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _RC_DEBMAN_H
#define _RC_DEBMAN_H

#include <gtk/gtk.h>

#include "rc-packman.h"

#define GTK_TYPE_RC_DEBMAN        (rc_debman_get_type ())
#define RC_DEBMAN(obj)            (GTK_CHECK_CAST ((obj), \
                                   GTK_TYPE_RC_DEBMAN, RCDebman))
#define RC_DEBMAN_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), \
                                   GTK_TYPE_RC_DEBMAN, \
                                   RCDebmanClass))
#define IS_RC_DEBMAN(obj)         (GTK_CHECK_TYPE ((obj), \
                                   GTK_TYPE_RC_DEBMAN))
#define IS_RC_DEBMAN_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), \
                                   GTK_TYPE_RC_DEBMAN))

typedef struct _RCDebman      RCDebman;
typedef struct _RCDebmanClass RCDebmanClass;

struct _RCDebman {
    RCPackman parent;

    int lock_fd;
};

struct _RCDebmanClass {
    RCPackmanClass parent_class;
};

guint rc_debman_get_type (void);

RCDebman *rc_debman_new (void);

void rc_debman_parse_version (gchar *input, guint32 *epoch, gchar **version,
                              gchar **release);
RCPackageDepSList *rc_debman_fill_depends (gchar *input,
                                           RCPackageDepSList *list);

#endif /* _RC_DEBMAN_H */
