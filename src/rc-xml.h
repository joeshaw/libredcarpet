/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-xml.c: XML routines
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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

#ifndef _RC_XML_H
#define _RC_XML_H

#include "rc-channel.h"
#include "rc-package.h"
#include "xml-util.h"

/* SAX Parser */

int rc_xml_parse (const char *filename,
                  RCChannel *channel,
                  RCPackageFn callback,
                  gpointer user_data);

/* Old stuff */
RCPackage *rc_xml_node_to_package (const xmlNode *node,
                                   const RCChannel *channel);
RCPackageDep *rc_xml_node_to_package_dep (const xmlNode *node);
RCPackageUpdate *rc_xml_node_to_package_update (const xmlNode *node, 
						const RCPackage *package);

xmlNode *rc_channel_to_xml_node (RCChannel *channel);
xmlNode *rc_package_spec_to_xml_node (RCPackageSpec *spec);
xmlNode *rc_package_to_xml_node (RCPackage *package);
xmlNode *rc_package_dep_or_slist_to_xml_node (RCPackageDepSList *dep);
xmlNode *rc_package_dep_to_xml_node (RCPackageDep *dep_item);
xmlNode *rc_package_update_to_xml_node (RCPackageUpdate *update);
xmlNode *rc_package_file_list_to_xml_node (RCPackageFileSList *files);
#endif
