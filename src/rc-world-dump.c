/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-dump.c
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#include <config.h>
#include "rc-world-dump.h"

#include "rc-channel-private.h"

#include "rc-debug.h"
#include "rc-world-import.h"
#include "rc-xml.h"

static void
add_package_xml_cb (RCPackage *package,
                    gpointer   user_data)
{
    xmlNode *parent = user_data;
    xmlNode *node;

    node = rc_package_to_xml_node (package);
    xmlAddChild (parent, node);
}

typedef struct {
    RCWorld *world;
    xmlNode *parent;
} AddChannelClosure;

static void
add_channel_packages_cb (RCChannel *channel,
                         gpointer   user_data)
{
    AddChannelClosure *closure = user_data;
    xmlNode *node;

    node = rc_channel_to_xml_node (channel);
    rc_world_foreach_package (closure->world,
                              channel,
                              add_package_xml_cb,
                              node);

    xmlAddChild (closure->parent, node);
}

xmlNode *
rc_world_dump_to_xml (RCWorld *world,
                      xmlNode *extra_xml)
{
    xmlNode *parent;
    xmlNode *system_packages;
    AddChannelClosure closure;
   
    g_return_val_if_fail (world != NULL, NULL);

    parent = xmlNewNode (NULL, "world");
    
    if (extra_xml != NULL)
        xmlAddChild (parent, extra_xml);
    
    system_packages = xmlNewNode (NULL, "system_packages");
    xmlAddChild (parent, system_packages);

    rc_world_foreach_package (world,
                              RC_WORLD_SYSTEM_PACKAGES,
                              add_package_xml_cb,
                              system_packages);

    closure.world = world;
    closure.parent = parent;

    rc_world_foreach_channel (world,
                              add_channel_packages_cb,
                              &closure);

    return parent;
}

char *
rc_world_dump (RCWorld *world, xmlNode *extra_xml)
{
    xmlNode *dump;
    xmlDoc *doc;
    xmlChar *data;
    int data_size;

    g_return_val_if_fail (world != NULL, NULL);

    dump = rc_world_dump_to_xml (world, extra_xml);
    g_return_val_if_fail (dump != NULL, NULL);

    doc = xmlNewDoc ("1.0");
    xmlDocSetRootElement (doc, dump);
    xmlDocDumpMemory (doc, &data, &data_size);
    xmlFreeDoc (doc);

    return data;
}

void
rc_world_undump_from_xml (RCWorld *world,
                          xmlNode *dump_node)
{
    RCChannel *current_channel = NULL;
    xmlNode *channel_node;

    g_return_if_fail (world != NULL);
    g_return_if_fail (dump_node != NULL);

    if (g_strcasecmp (dump_node->name, "world")) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Unrecognized top-level node for undump: '%s'",
                  dump_node->name);
        return;
    }

    channel_node = dump_node->xmlChildrenNode;

    while (channel_node != NULL) {

        if (! g_strcasecmp (channel_node->name, "system_packages")) {

            rc_world_add_packages_from_xml (world, NULL, channel_node);
        
        } else if (! g_strcasecmp (channel_node->name, "channel")) {

            char *name;
            char *alias;
            char *id_str, *bid_str;
            guint32 id, bid;
            static guint32 dummy_id = 0xdeadbeef;
            char *subd_str;
            int subd;
            char *priority_str;
            char *priority_unsubd_str;
            char *priority_current_str;
            
            name = xml_get_prop (channel_node, "name");
            alias = xml_get_prop (channel_node, "alias");
            
            id_str = xml_get_prop (channel_node, "id");
            id = id_str ? atoi (id_str) : (dummy_id++);

            bid_str = xml_get_prop (channel_node, "bid");
            bid = bid_str ? atoi (bid_str) : (dummy_id++);
                        
            subd_str = xml_get_prop (channel_node, "subscribed");
            subd = subd_str ? atoi (subd_str) : 0;

            priority_str = xml_get_prop (channel_node, "priority_base");
            priority_unsubd_str = xml_get_prop (channel_node, "priority_unsubd");
            priority_current_str = xml_get_prop (channel_node, "priority_current");
            current_channel = rc_world_add_channel (world,
                                                    name,
                                                    alias ? alias : "foo",
                                                    id, bid,
                                                    RC_CHANNEL_TYPE_HELIX);

            if (current_channel) {

                current_channel->priority         = priority_str ? atoi (priority_str) : 0;
                current_channel->priority_unsubd  = priority_unsubd_str ? atoi (priority_unsubd_str) : 0;
                current_channel->priority_current = priority_current_str ? atoi (priority_current_str) : 0;

                rc_channel_set_subscription (current_channel, subd);
                rc_world_add_packages_from_xml (world, current_channel,
                                                channel_node);
            }

            g_free (name);
            g_free (alias);
            g_free (id_str);
            g_free (subd_str);

            g_free (priority_str);
            g_free (priority_unsubd_str);
            g_free (priority_current_str);
        }

        channel_node = channel_node->next;
    }
}

void
rc_world_undump (RCWorld    *world,
                 const char *buffer)
{
    xmlDoc *doc = NULL;
    xmlNode *root = NULL;
    int buffer_len;

    g_return_if_fail (world != NULL);
    g_return_if_fail (buffer != NULL);

    buffer_len = strlen (buffer);

    doc = xmlParseMemory (buffer, buffer_len);
    if (doc)
        root = xmlDocGetRootElement (doc);

    if (root)
        rc_world_undump_from_xml (world, root);
    else
        rc_debug (RC_DEBUG_LEVEL_WARNING, "Undump failed!");
}
