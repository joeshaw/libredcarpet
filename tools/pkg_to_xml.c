/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * pkg_to_xml.c
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#include "libredcarpet.h"

int main (int argc, char *argv[])
{
    char *distro_xml;
    RCPackman *packman;
    xmlDoc *doc = NULL;
    xmlNode *root;
    gboolean failed = FALSE;
    int i;
    
    g_type_init ();

    distro_xml = getenv ("RC_DISTRIBUTIONS_FILE");

    if (distro_xml) {
        RCBuffer *buf;

        if (!g_file_test (distro_xml, G_FILE_TEST_EXISTS)) {
            g_printerr ("Distributions file '%s' not found.\n", distro_xml);
            failed = TRUE;
            goto out;
        }

        buf = rc_buffer_map_file (distro_xml);
        if (!buf) {
            g_printerr ("Unable to open distributions file '%s'.\n",
                        distro_xml);
            failed = TRUE;
            goto out;
        }

        /* Try once compressed, once uncompressed */
        if (!rc_distro_parse_xml (buf->data, buf->size) &&
            !rc_distro_parse_xml (buf->data, 0)) {
            g_printerr ("Unable to parse distributions file '%s'.\n",
                        distro_xml);
            rc_buffer_unmap_file (buf);
            failed = TRUE;
            goto out;
        }

        rc_buffer_unmap_file (buf);
    }
    else {
        if (!rc_distro_parse_xml (NULL, 0)) {
            g_printerr ("Unable to parse internal distribution info\n");
            failed = TRUE;
            goto out;
        }
    }

    packman = rc_distman_new ();

    if (! packman) {
        g_printerr ("Couldn't access the packaging system.\n");
        failed = TRUE;
        goto out;
    }

    if (rc_packman_get_error (packman)) {
        g_printerr ("Couldn't access the packaging system: %s\n",
                    rc_packman_get_reason (packman));
        failed = TRUE;
        goto out;
    }

    doc = xmlNewDoc ("1.0");
    root = xmlNewNode (NULL, "packages");
    xmlDocSetRootElement (doc, root);

    for (i = 1; i < argc; ++i) {
        RCPackage *pkg;
        xmlNode *node = NULL;

        if (! g_file_test (argv[i], G_FILE_TEST_EXISTS)) {

            g_printerr ("File '%s' not found.\n", argv[i]);
            failed = TRUE;

        } else if (! g_file_test (argv[i], G_FILE_TEST_IS_REGULAR)) {

            g_printerr ("'%s' is not a regular file.\n", argv[i]);
            failed = TRUE;

        } else {
                
            pkg = rc_packman_query_file (packman, argv[i]);
            
            if (! pkg) {

                g_printerr ("File '%s' does not appear to be a valid package.\n", argv[i]);
                failed = TRUE;

            } else {

                node = rc_package_to_xml_node (pkg);

                if (! node) {

                    g_printerr ("XMLification of %s failed.\n",
                                rc_package_spec_to_str_static (&pkg->spec));
                    failed = TRUE;

                } else {

                    xmlAddChild (root, node);

                }
            }
        }
    }

out:
    if (failed)
        g_printerr ("XML generation cancelled.\n");
    else
        xmlDocDump (stdout, doc);

    return failed ? -1 : 0;
}

