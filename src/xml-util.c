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

#include <stdlib.h>
#include <gnome-xml/xmlmemory.h>

#include "softmgr.h"
#include "xml-util.h"

gchar *
xml_get_value(xmlNode *node, const gchar *name)
{
    gchar *ret;
    xmlChar *xml_s;
    xmlNode *child;

    xml_s = xmlGetProp(node, name);
    if (xml_s) {
        ret = g_strdup (xml_s);
        xmlFree (xml_s);
        return ret;
    }


    child = node->childs;
    while (child) {
        if (g_strcasecmp(child->name, name) == 0) {
            xml_s = xmlNodeGetContent(child);
            if (xml_s) {
                ret = g_strdup (xml_s);
                xmlFree (xml_s);
                return ret;
            }
            child = child->next;
	}
    }

    return NULL;
} /* xml_get_value */

gchar *
xml_get_prop(xmlNode *node, const gchar *name)
{
    xmlChar *ret;
    gchar *gs;

    ret = xmlGetProp(node, name);
    if (ret) {
        gs = g_strdup (ret);
        xmlFree (ret);
        return gs;
    } else {
        return NULL;
    }
} /* xml_get_prop */

gint32 xml_get_gint32_value_default (xmlNode *node, const gchar *name)
{
    gint32 z;
    if (xml_get_gint32_value (node, name, &z))
        return z;
    else
        return 0;
}
	       
gboolean
xml_get_gint32_value(xmlNode *node, const gchar *name, gint32 *value)
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

guint32 xml_get_guint32_value_default (xmlNode *node, const gchar *name)
{
    guint32 z;
    if (xml_get_guint32_value (node, name, &z))
        return z;
    else
        return 0;
}

gboolean
xml_get_guint32_value(xmlNode *node, const gchar *name, guint32 *value)
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
