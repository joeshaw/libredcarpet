/*
 * xml-util.h: XML convenience routines
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Authors: Joe Shaw <joe@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef __SOFTMGR_XML_UTIL_H
#define __SOFTMGR_XML_UTIL_H

#include <parser.h>
#include <tree.h>

/* The former will get either a property or a tag, whereas the latter will
   get only a property */

char *xml_get_value(xmlNode *node, const char *name);
char *xml_get_prop(xmlNode *node, const char *name);

#endif /* __SOFTMGR_XML_UTIL_H */
