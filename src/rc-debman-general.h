/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debman-general.h
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

#ifndef _RC_DEBMAN_GENERAL_H
#define _RC_DEBMAN_GENERAL_H

#include <glib.h>

#include "rc-package.h"
#include "rc-package-dep.h"

void rc_debman_parse_version (gchar *input, guint32 *epoch, gchar **version,
                              gchar **release);
RCPackageDepSList *rc_debman_fill_depends (gchar *input, gboolean pre);

RCPackageSection rc_debman_section_to_package_section (const gchar *section);

#endif /* _RC_DEBMAN_GENERAL_H */
