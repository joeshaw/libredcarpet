/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-pretty-name.h
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <gnome-xml/xmlmemory.h>

#include "xml-util.h"
#include "rc-pretty-name.h"
#include "rc-util.h"

static GHashTable *rc_pretty_name_hash = NULL;


const gchar *
rc_pretty_name_lookup (const gchar *package)
{
    gchar *out_name;

    g_return_val_if_fail(package, NULL);

    if (!rc_pretty_name_hash) {
        /* This isn't necessarily an error. If the server couldn't find the
           file and the user chose to continue without it, then this wouldn't
           be set. */
        return package;
    }

    out_name = (gchar *) g_hash_table_lookup (rc_pretty_name_hash, package);
    if (out_name)
        return (const char *) out_name;
    else
        return package;
}

void
rc_pretty_name_parse_xml (gchar *tbuf, gint compressed_length)
{
    gchar *buf;
    GByteArray *byte_array = NULL;
    xmlDocPtr xml_doc;
    xmlNodePtr xml_iter;

    if (!rc_pretty_name_hash) {
        rc_pretty_name_hash = g_hash_table_new (g_str_hash, g_str_equal);
    }

    if (compressed_length) {
        if (rc_uncompress_memory (tbuf, compressed_length, &byte_array)) {
            g_warning ("Uncompression failed");
            return;
        }

        buf = byte_array->data;
    } else {
        buf = tbuf;
    }

    xml_doc = xmlParseMemory (buf, strlen (buf));

    xml_iter = xml_doc->xmlRootNode;
    
    while (xml_iter) {
        xmlNodePtr item_iter;

        if (g_strcasecmp (xml_iter->name, "prettynames")) {
            xml_iter = xml_iter->next;
            continue;
        }

        item_iter = xml_iter->xmlChildrenNode;

        while (item_iter) {
            xmlNodePtr attr_iter;
            gchar *pretty_name = NULL;
            GSList *package_names_list = NULL;

            if (g_strcasecmp (item_iter->name, "item")) {
                item_iter = item_iter->next;
                continue;
            }

            attr_iter = item_iter->xmlChildrenNode;

            while (attr_iter) {
                xmlChar *xml_s;
                xml_s = xmlNodeGetContent (attr_iter);

                if (!g_strcasecmp (attr_iter->name, "name")) {
                    pretty_name = g_strdup (xml_s);
                } else if (!g_strcasecmp (attr_iter->name, "package")) {
                    package_names_list = g_slist_append (package_names_list, g_strdup (xml_s));
                }

                xmlFree (xml_s);
                attr_iter = attr_iter->next;
            }

            if (pretty_name && package_names_list) {
                GSList *packiter = package_names_list;
                while (packiter) {
                    gchar *pnm = (gchar *) packiter->data;
                    g_hash_table_insert (rc_pretty_name_hash, pnm, pretty_name);
                    packiter = packiter->next;
                }
                g_slist_free (package_names_list);
            }

            item_iter = item_iter->next;
        }

        xml_iter = xml_iter->next;
    }


    xmlFreeDoc (xml_doc);

    if (byte_array) {
        g_byte_array_free (byte_array, TRUE);
    }
}
