/*
 * xml-util.c: Convenience functions for XML stuff
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

#include "softmgr.h"
#include "xml-util.h"

char *
xml_get_value(xmlNode *node, const char *name)
{
	char *ret;
	xmlNode *child;

	ret = xmlGetProp(node, name);
	if (ret)
		return ret;

	child = node->childs;
	while (child) {
		if (g_strcasecmp(child->name, name) == 0) {
			ret = xmlNodeGetContent(child);
			if (ret)
				return ret;
		}
		child = child->next;
	}

	return NULL;
} /* xml_get_value */

char *
xml_get_prop(xmlNode *node, const char *name)
{
	char *ret;

	ret = xmlGetProp(node, name);
	if (ret)
		return ret;
	else
		return NULL;
} /* xml_get_prop */
	       
