/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-xml.c: XML routines
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
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

#ifndef _RC_XML_H
#define _RC_XML_H

#include "rc-channel.h"
#include "rc-package.h"
#include "xml-util.h"

/* SAX Parser */
typedef struct _RCPackageSAXContext      RCPackageSAXContext;

RCPackageSAXContext *rc_package_sax_context_new (RCChannel *channel);
void rc_package_sax_context_parse_chunk (RCPackageSAXContext *ctx,
                                         char *xmlbuf,
                                         int size);
RCPackageSList *rc_package_sax_context_done (RCPackageSAXContext *ctx);

/* Old stuff */
RCPackage *rc_xml_node_to_package (const xmlNode *node,
                                   const RCChannel *channel);
RCPackageDep *rc_xml_node_to_package_dep (const xmlNode *node);
RCPackageUpdate *rc_xml_node_to_package_update (const xmlNode *node, 
						const RCPackage *package);

xmlNode *rc_package_to_xml_node (RCPackage *package);
xmlNode *rc_package_dep_or_slist_to_xml_node (RCPackageDepSList *dep);
xmlNode *rc_package_dep_to_xml_node (RCPackageDep *dep_item);
xmlNode *rc_package_update_to_xml_node (RCPackageUpdate *update);

#endif
