/* rc-debman-general.h
 * Copyright (C) 2000, 2001 Helix Code Inc.
 * Copyright (C) 2001 Ximian Inc.
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

#ifndef _RC_DEBMAN_GENERAL_H
#define _RC_DEBMAN_GENERAL_H

#include <libredcarpet/rc-package-dep.h>

void rc_debman_parse_version (gchar *input, guint32 *epoch, gchar **version,
                              gchar **release);
RCPackageDepSList *rc_debman_fill_depends (gchar *input,
                                           RCPackageDepSList *list);


#endif /* _RC_DEBMAN_GENERAL_H */
