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
    RCPackman *packman;
    xmlDoc *doc;
    xmlNode *root;
    gboolean failed = FALSE;
    int i;
    

    g_type_init ();
    
    if (!rc_distro_parse_xml (NULL, 0))
        exit (-1);
    packman = rc_distman_new ();

    if (! packman)
        g_error ("Couldn't access the packaging system");

    if (rc_packman_get_error (packman)) {
        g_error ("Couldn't access the packaging system: %s",
                 rc_packman_get_reason (packman));
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
    
    if (failed)
        g_printerr ("XML generation cancelled.\n");
    else
        xmlDocDump (stdout, doc);

    return failed ? -1 : 0;
}

