/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-package-set.c
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

#include <glib.h>
#include <string.h>

#include <libredcarpet/rc-package-set.h>
#include <libredcarpet/rc-util.h>
#include <libredcarpet/xml-util.h>

RCPackageSet *
rc_package_set_new ()
{
    RCPackageSet *set;

    set = g_new0 (RCPackageSet, 1);

    return (set);
}

void
rc_package_set_free (RCPackageSet *set)
{
    g_free (set->name);

    g_free (set->description);

    g_slist_foreach (set->packages, (GFunc)g_free, NULL);

    g_free (set);
}

void
rc_package_set_slist_free (RCPackageSetSList *set_list)
{
    g_slist_foreach (set_list, (GFunc)rc_package_set_free, NULL);

    g_slist_free (set_list);
}

RCPackageSetSList *
rc_package_set_parse (char *buf,
                      int compressed_length)
{
    xmlDoc *doc;
    xmlNode *root, *node;
    RCPackageSetSList *psetl = NULL;

    g_return_val_if_fail (buf, NULL);

    if (compressed_length) {
        GByteArray *ba;

        if (rc_uncompress_memory (buf, compressed_length, &ba)) {
            g_warning ("Uncompression failed!");
            return NULL;
        }

        doc = xmlParseMemory(ba->data, ba->len - 1);
        g_byte_array_free (ba, TRUE);
    }
    else {
	doc = xmlParseMemory(buf, strlen(buf));
    }

#if LIBXML_VERSION  < 20000
    root = doc->root;
#else
    root = xmlDocGetRootElement (doc);
#endif
    if (!doc) {
        g_warning ("Unable to parse package set list.");
        return NULL;
    }

#if LIBXML_VERSION  < 20000
    node = root->childs;
#else
    node = root->children;
#endif
    while (node) {
        RCPackageSet *rps;
        xmlNode *xmn;

        if (strcasecmp (node->name, "set") != 0) {
            g_warning ("Ran into something other than a set in packageset (%s)", node->name);
            node = node->next;
            continue;
        }

        rps = g_new0 (RCPackageSet, 1);
        rps->name = xml_get_value (node, "name");
        rps->description = xml_get_value (node, "description");

#if LIBXML_VERSION < 20000
        xmn = node->childs;
#else
        xmn = node->children;
#endif
        while (xmn) {
            if (strcasecmp (xmn->name, "include") == 0) {
                gchar *temp;

                if ((temp = xml_get_prop (xmn, "set")) != NULL) {
                    /* include the packages from the other set */
                    /* temp has name of other set */
                    RCPackageSetSList *others;
                    others = psetl;
                    while (others) {
                        RCPackageSet *ps = (RCPackageSet *) others->data;
                        if (strcmp (temp, ps->name) == 0) {
                            GSList *otherlist;
                            otherlist = ps->packages;
                            while (otherlist) {
                                gchar *op = (gchar *) otherlist->data;
                                rps->packages = g_slist_append (rps->packages,
                                                                g_strdup (op));
                                otherlist = otherlist->next;
                            }
                            break;
                        }
                        others = others->next;
                    }
                    if (others == NULL) {
                        g_warning ("packageset '%s' referenced non-existant set '%s'",
                                   rps->name, temp);
                    }
                    g_free (temp);
                } else if ((temp = xml_get_prop (xmn, "package")) != NULL) {
                    rps->packages = g_slist_append (rps->packages, temp);
                }
            }
            xmn = xmn->next;
        }

        psetl = g_slist_append (psetl, rps);

        node = node->next;
    }

    xmlFreeDoc (doc);

    return psetl;
}
