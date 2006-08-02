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

typedef struct {
    RCPackman *packman;
    xmlNode *parent;
    gboolean dump_files;
} QueryAllInfo;

static gboolean
query_all_cb (RCPackage *package, gpointer user_data)
{
    RCPackageUpdate *pkg_up;
    xmlNode *package_node;
    QueryAllInfo *info = user_data;

    pkg_up = rc_package_update_new ();

    rc_package_spec_copy ((RCPackageSpec *)pkg_up,
                          (RCPackageSpec *)package);

    pkg_up->package_url = g_strdup ("file://dummy.rpm");
    pkg_up->importance = RC_IMPORTANCE_SUGGESTED;
    pkg_up->description = g_strdup ("No information available.");
    pkg_up->package_size = 0;

    package->history = g_slist_append (package->history, pkg_up);

    package_node = rc_package_to_xml_node (package);

    if (info->dump_files) {
        RCPackageFileSList *files;
        xmlNode *files_node;

        files = rc_packman_file_list (info->packman, package);
        files_node = rc_package_file_list_to_xml_node (files);
        xmlAddChild (package_node, files_node);
    }

    xmlAddChild (info->parent, package_node);

    return TRUE;
}

int
main (int argc, char **argv)
{
    xmlDocPtr doc;
    xmlNode *channel_node;
    xmlNode *subchannel_node;
    QueryAllInfo info;

    if (argc == 2 && !strncmp ("--dump-files", argv[1], 12))
        info.dump_files = TRUE;
    else
        info.dump_files = FALSE;

    g_type_init ();

    info.packman = rc_distman_new ();

    if (!info.packman)
        g_error ("Couldn't access the packaging system");

    if (rc_packman_get_error (info.packman)) {
        g_error ("Couldn't access the packaging system: %s",
                 rc_packman_get_reason (info.packman));
    }

    channel_node = xmlNewNode (NULL, "channel");
    subchannel_node = xmlNewNode (NULL, "subchannel");
    xmlAddChild (channel_node, subchannel_node);

    info.parent = subchannel_node;
    rc_packman_query_all (info.packman, query_all_cb, &info);

    doc = xmlNewDoc ("1.0");
    xmlDocSetRootElement (doc, channel_node);
    xmlDocDump (stdout, doc);

    exit (0);
}
