/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* xml-util.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#include <stdlib.h>

#include "xml-util.h"

gchar *
xml_get_value(const xmlNode *node, const gchar *name)
{
    gchar *ret;
    xmlChar *xml_s;
    xmlNode *child;

    xml_s = xmlGetProp((xmlNode *)node, name);
    if (xml_s) {
        ret = g_strdup (xml_s);
        xmlFree (xml_s);
        return ret;
    }

    child = node->xmlChildrenNode;

    while (child) {
        if (g_strcasecmp(child->name, name) == 0) {
            xml_s = xmlNodeGetContent(child);
            if (xml_s) {
                ret = g_strdup (xml_s);
                xmlFree (xml_s);
                return ret;
            }
	}
	child = child->next;
    }

    return NULL;
} /* xml_get_value */

gint32
xml_get_gint32_value_default (const xmlNode *node, const gchar *name,
                              const gint32 def)
{
    gint32 z;
    if (xml_get_gint32_value (node, name, &z))
        return z;
    else
        return def;
}
	       
gboolean
xml_get_gint32_value(const xmlNode *node, const gchar *name, gint32 *value)
{
    gchar *strval, *ret;
    gint32 z;

    strval = xml_get_value(node, name);
    if (!strval) {
        return FALSE;
    }

    z = strtol (strval, &ret, 10);
    if (*ret != '\0') {
        g_free (strval);
        return FALSE;
    }

    g_free (strval);
    *value = z;
    return TRUE;
}

guint32
xml_get_guint32_value_default (const xmlNode *node, const gchar *name,
                               const guint32 def)
{
    guint32 z;
    if (xml_get_guint32_value (node, name, &z))
        return z;
    else
        return def;
}

gboolean
xml_get_guint32_value (const xmlNode *node, const gchar *name, guint32 *value)
{
    gchar *strval, *ret;
    gint32 z;

    strval = xml_get_value(node, name);
    if (!strval) {
        return FALSE;
    }

    z = strtoul (strval, &ret, 10);
    if (*ret != '\0') {
        g_free (strval);
        return FALSE;
    }

    g_free (strval);
    *value = z;
    return TRUE;
}

gchar *
xml_get_prop (const xmlNode *node, const gchar *name)
{
    xmlChar *ret;
    gchar *gs;

    ret = xmlGetProp((xmlNode *)node, name);
    if (ret) {
        gs = g_strdup (ret);
        xmlFree (ret);
        return gs;
    } else {
        return NULL;
    }
} /* xml_get_prop */

guint32
xml_get_guint32_prop_default (const xmlNode *node, const gchar *name,
                              const guint32 def)
{
    xmlChar *buf;
    guint32 ret;

    buf = xmlGetProp ((xmlNode *)node, name);

    if (buf) {
        ret = strtol (buf, NULL, 10);
        xmlFree (buf);
        return (ret);
    } else {
        return (def);
    }
} /* xml_get_guint32_prop_default */

gchar *
xml_get_content (const xmlNode *node)
{
    xmlChar *buf;
    gchar *ret;

    buf = xmlNodeGetContent ((xmlNode *)node);

    ret = g_strdup (buf);

    xmlFree (buf);

    return (ret);
}

guint32
xml_get_guint32_content_default (const xmlNode *node, const guint32 def)
{
    xmlChar *buf;
    guint32 ret;

    buf = xmlNodeGetContent ((xmlNode *)node);

    if (buf) {
        ret = strtol (buf, NULL, 10);
        xmlFree (buf);
        return (ret);
    } else {
        return (def);
    }
}

xmlNode *
xml_get_node (const xmlNode *node, const char *name)
{
    xmlNode *iter;

    for (iter = node->xmlChildrenNode; iter; iter = iter->next) {
        if (g_strcasecmp (iter->name, name) == 0)
            return iter;
    }
}
