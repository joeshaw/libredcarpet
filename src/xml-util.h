/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* xml-util.h
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

#ifndef __SOFTMGR_XML_UTIL_H
#define __SOFTMGR_XML_UTIL_H

#include <glib.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

/* The former will get either a property or a tag, whereas the latter will
   get only a property */

gchar *xml_get_value(const xmlNode *node, const gchar *name);
gboolean xml_get_gint32_value(const xmlNode *node, const gchar *name,
                              gint32 *value);
gint32 xml_get_gint32_value_default (const xmlNode *node, const gchar *name,
                                     const gint32 def);
gboolean xml_get_guint32_value(const xmlNode *node, const gchar *name,
                               guint32 *value);
guint32 xml_get_guint32_value_default (const xmlNode *node, const gchar *name,
                                       const guint32 def);

gchar *xml_get_prop(const xmlNode *node, const gchar *name);
guint32 xml_get_guint32_prop_default (const xmlNode *node, const gchar *name,
                                      const guint32 def);
gchar *xml_get_content (const xmlNode *);
guint32 xml_get_guint32_content_default (const xmlNode *node,
                                         const guint32 def);

#endif /* __SOFTMGR_XML_UTIL_H */
