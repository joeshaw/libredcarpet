/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-subscriptions.c
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Developed by Jon Trowbridge <trow@ximian.com>
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include "rc-world-subscriptions.h"

#include "rc-debug.h"

static void
clear_sub_cb (RCChannel *channel,
              gpointer   user_data)
{
    rc_channel_set_subscription (channel, FALSE);
}

gboolean
rc_world_import_subscriptions_from_xml (RCWorld *world,
                                        xmlNode *node)
{
    g_return_val_if_fail (world != NULL, FALSE);
    g_return_val_if_fail (node != NULL, FALSE);

    if (strcmp (node->name, "subscriptions"))
        return FALSE;

    /* Clear out our subscriptions */
    rc_world_foreach_channel (world,
                              clear_sub_cb,
                              NULL);

    node = node->xmlChildrenNode;
    while (node) {
        if (! strcmp (node->name, "channel")) {
            xmlChar *id_str = xmlGetProp (node, "channel_id");
            char *endptr;
            long id;

            id = strtol (id_str, &endptr, 10);
            if (endptr && *endptr == '\0' && id > 0) {

                RCChannel *channel;
                channel = rc_world_get_channel_by_id (world, (guint32)id);
                
                if (channel == NULL) {
                    rc_debug (RC_DEBUG_LEVEL_WARNING,
                              "Unknown channel id '%s'",
                              id_str);
                } else {
                    rc_channel_set_subscription (channel, TRUE);
                }

            } else {
                rc_debug (RC_DEBUG_LEVEL_WARNING,
                          "Invalid subscribed channel id '%s'",
                          id_str);
            }

            xmlFree (id_str);
        }

        node = node->next;
    }

    return TRUE;
}

void
to_xml_cb (RCChannel *channel,
           gpointer   user_data)
{
    char buf[16];
    xmlNode *root = user_data;
    xmlNode *node;

    if (! rc_channel_subscribed (channel))
        return;
    
    node = xmlNewChild (root, NULL, "channel", NULL);

    g_snprintf (buf, 16, "%d", rc_channel_get_id (channel));
    xmlNewProp (node, "channel_id", buf);

    /* For backwards compatibility */
    xmlNewProp (node, "last_updated", "0");
}

xmlNode *
rc_world_export_subcriptions_to_xml (RCWorld *world)
{
    xmlNode *root;

    root = xmlNewNode (NULL, "subscriptions");
    rc_world_foreach_channel (world,
                              to_xml_cb,
                              root);

    return root;
}
    
