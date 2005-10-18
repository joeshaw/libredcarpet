/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * cur_to_xml.c
 *
 * Copyright (C) 2001-2002 Ximian, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (int argc, char **argv)
{
    RCPackman *packman;
    RCPackageSList *packages = NULL;
    RCPackageSList *iter;
    RCPackageUpdate *pkg_up;
    xmlDocPtr doc;
    xmlNode *channel_node;
    xmlNode *subchannel_node;
    gboolean dump_files = FALSE;

    if (argc == 2 && !strncmp ("--dump-files", argv[1], 12))
        dump_files = TRUE;

    g_type_init ();

    packman = rc_distman_new ();

    if (!packman)
        g_error ("Couldn't access the packaging system");

    if (rc_packman_get_error (packman)) {
        g_error ("Couldn't access the packaging system: %s",
                 rc_packman_get_reason (packman));
    }

    packages = rc_packman_query_all (packman);

    channel_node = xmlNewNode (NULL, "channel");
    subchannel_node = xmlNewNode (NULL, "subchannel");
    xmlAddChild (channel_node, subchannel_node);

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        xmlNode *package_node;

        pkg_up = rc_package_update_new ();

        rc_package_spec_copy ((RCPackageSpec *)pkg_up,
                              (RCPackageSpec *)package);

        pkg_up->package_url = g_strdup ("file://dummy.rpm");
        pkg_up->importance = RC_IMPORTANCE_SUGGESTED;
        pkg_up->description = g_strdup ("No information available.");
        pkg_up->package_size = 0;

        package->history = g_slist_append (package->history, pkg_up);

        package_node = rc_package_to_xml_node (package);

        if (dump_files) {
            RCPackageFileSList *files;
            xmlNode *files_node;

            files = rc_packman_file_list (packman, package);
            files_node = rc_package_file_list_to_xml_node (files);
            xmlAddChild (package_node, files_node);
        }

        xmlAddChild (subchannel_node, package_node);
    }

    doc = xmlNewDoc ("1.0");
    xmlDocSetRootElement (doc, channel_node);
    xmlDocDump (stdout, doc);

    exit (0);
}
    
