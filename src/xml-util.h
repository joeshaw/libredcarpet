/*
 * xml-util.h: XML convenience routines
 *
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * Authors: Joe Shaw <joe@helixcode.com>
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

#ifndef __SOFTMGR_XML_UTIL_H
#define __SOFTMGR_XML_UTIL_H

#include <glib.h>

#include <parser.h>
#include <tree.h>

/* The former will get either a property or a tag, whereas the latter will
   get only a property */

gchar *xml_get_value(xmlNode *node, const gchar *name);
gchar *xml_get_prop(xmlNode *node, const gchar *name);
gboolean xml_get_gint32_value(xmlNode *node, const gchar *name, gint32 *value);
gint32 xml_get_gint32_value_default (xmlNode *node, const gchar *name, gint32 def);
gboolean xml_get_guint32_value(xmlNode *node, const gchar *name, guint32 *value);
guint32 xml_get_guint32_value_default (xmlNode *node, const gchar *name, guint32 def);

#endif /* __SOFTMGR_XML_UTIL_H */
