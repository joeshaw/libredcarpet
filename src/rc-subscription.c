/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-subscription.c
 *
 * Copyright (C) 2003 Ximian, Inc.
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

#include <config.h>
#include "rc-subscription.h"

#include <time.h>
#include "rc-debug.h"
#include "xml-util.h"

#define SUBSCRIPTION_PATH "/var/lib/rcd"
#define OLD_SUBSCRIPTION_PATH "/var/lib/redcarpet"
#define SUBSCRIPTION_NAME "/subscriptions.xml"

#define SUBSCRIPTION_FILE SUBSCRIPTION_PATH SUBSCRIPTION_NAME
#define OLD_SUBSCRIPTION_FILE OLD_SUBSCRIPTION_PATH SUBSCRIPTION_NAME

/* Old subscriptions expire in 60 days */
#define OLD_SUBSCRIPTION_EXPIRATION 60*24*60*60

static GList   *subscriptions = NULL;
static gboolean subscriptions_changed = FALSE;

typedef struct _RCSubscription RCSubscription;
struct _RCSubscription {
    char    *channel_id;
    time_t   last_seen;
    gboolean old; /* subscription imported from an old-style subs file */
};

static RCSubscription *
rc_subscription_new (const char *id)
{
    RCSubscription *sub;

    g_return_val_if_fail (id != NULL, NULL);

    sub = g_new0 (RCSubscription, 1);

    sub->channel_id = g_strdup (id);
    sub->last_seen  = time (NULL);
    sub->old        = FALSE;

    return sub;
}

static void
rc_subscription_free (RCSubscription *sub)
{
    if (sub != NULL) {
        g_free (sub->channel_id);
        g_free (sub);
    }
}

static gboolean
rc_subscription_match (RCSubscription *sub, RCChannel *channel)
{
    gboolean match;

    /* Paranoia is the programmer's friend. */
    g_return_val_if_fail (sub != NULL, FALSE);
    g_return_val_if_fail (sub->channel_id != NULL, FALSE);
    g_return_val_if_fail (channel != NULL, FALSE);
    g_return_val_if_fail (rc_channel_get_id (channel) != NULL, FALSE);

    /* If this is an old (i.e. imported from 1.x) subscription, we
       compare it against the channel id's tail. */
    if (sub->old) {
        const char *id = rc_channel_get_id (channel);
        int len1 = strlen (sub->channel_id);
        int len2 = strlen (id);

        if (len1 > len2)
            return FALSE;

        /* If the tails match, mutate the RCSubscription into a
           new-style subscription for that channel. */
        if (! strcmp (id + (len2 - len1), sub->channel_id)) {
            g_free (sub->channel_id);
            sub->channel_id = g_strdup (id);
            sub->old = FALSE;
            subscriptions_changed = TRUE;

            return TRUE;
        }

        return FALSE;
    }

    match = ! strcmp (sub->channel_id, rc_channel_get_id (channel));

    if (match) {
        time (& sub->last_seen);
    }

    return match;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_subscription_save (void)
{
    xmlDoc *doc;
    xmlNode *root;
    GList *iter;
    char buf[64];
    time_t now;
    int save_retval;

    if (! subscriptions_changed)
        return;

    if (! g_file_test (SUBSCRIPTION_PATH, G_FILE_TEST_EXISTS)) {
        if (rc_mkdir (SUBSCRIPTION_PATH, 0755)) {
            rc_debug (RC_DEBUG_LEVEL_WARNING,
                      "Unable to create directory '%s' "
                      "to save subscription data to.",
                      SUBSCRIPTION_PATH);
            rc_debug (RC_DEBUG_LEVEL_WARNING,
                      "Subscription will not be saved!");
            return;
        }
    }

    time (&now);

    root = xmlNewNode (NULL, "subscriptions");
    xmlNewProp (root, "version", "2.0");

    doc = xmlNewDoc ("1.0");
    xmlDocSetRootElement (doc, root);

    for (iter = subscriptions; iter != NULL; iter = iter->next) {
        RCSubscription *sub;
        xmlNode *sub_node;

        sub = iter->data;

        /* Drop "old" (i.e. imported from 1.x) subscriptions that
           we haven't seen for a while. */
        if (sub->old) {
            double elapsed = difftime (now, sub->last_seen);
            if (elapsed > OLD_SUBSCRIPTION_EXPIRATION)
                continue;
        }

        sub_node = xmlNewChild (root, NULL, "channel", NULL);

        xmlNewProp (sub_node, "id", sub->channel_id);

        g_snprintf (buf, sizeof (buf), "%ld", (long) sub->last_seen);
        xmlNewProp (sub_node, "last_seen", buf);

        if (sub->old)
            xmlNewProp (sub_node, "old", "1");
    }

    save_retval = xmlSaveFile (SUBSCRIPTION_FILE, doc);
    xmlFreeDoc (doc);

    if (save_retval > 0) {
        /* Writing out the subscription file succeeded. */
        subscriptions_changed = FALSE;
    } else {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Unable to save subscription data to '%s'",
                  SUBSCRIPTION_FILE);
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Subscription will not be saved!");
    }
}

static void
rc_subscription_load_old_subscriptions (void)
{
    static gboolean tried_to_do_this_already = FALSE;
    xmlDoc *doc;
    xmlNode *node;

    if (tried_to_do_this_already)
        return;
    tried_to_do_this_already = TRUE;

    if (! g_file_test (OLD_SUBSCRIPTION_FILE, G_FILE_TEST_EXISTS)) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Can't find rcd 1.x subscription file '%s'",
                  OLD_SUBSCRIPTION_FILE);
        return;
    }

    doc = xmlParseFile (OLD_SUBSCRIPTION_FILE);
    if (doc == NULL) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "Can't parse rcd 1.x subscription file '%s'",
                  OLD_SUBSCRIPTION_FILE);
        return;
    }

    node = xmlDocGetRootElement (doc);

    if (! xml_node_match (node, "subscriptions")) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "rcd 1.x subscription file '%s' is malformed",
                  OLD_SUBSCRIPTION_FILE);
        return;
    }

    rc_debug (RC_DEBUG_LEVEL_INFO, "Importing rcd 1.x subscriptions.");
    
    node = node->xmlChildrenNode;

    while (node != NULL) {

        if (xml_node_match (node, "channel")) {
            RCSubscription *sub;
            char *id_str;

            id_str = xml_get_prop (node, "channel_id");
            if (id_str && *id_str) {

                sub = rc_subscription_new (id_str);
                sub->old = TRUE;

                subscriptions = g_list_prepend (subscriptions, sub);
            }
        }

        node = node->next;
    }

    /* If we've imported old subscriptions, we need to write them
       out immediately into the new subscriptions file. */

    subscriptions_changed = TRUE;
    rc_subscription_save ();
}

static void
rc_subscription_load (void)
{
    xmlDoc *doc;
    xmlNode *node;

    if (! g_file_test (SUBSCRIPTION_FILE, G_FILE_TEST_EXISTS)) {
        rc_subscription_load_old_subscriptions ();
        return;
    }

    doc = xmlParseFile (SUBSCRIPTION_FILE);
    if (doc == NULL) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "Can't parse subscription file '%s'",
                  SUBSCRIPTION_FILE);
        return;
    }

    node = xmlDocGetRootElement (doc);

    if (! xml_node_match (node, "subscriptions")) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "Subscription file '%s' is malformed",
                  SUBSCRIPTION_FILE);
        return;
    }

    node = node->xmlChildrenNode;

    while (node != NULL) {

        if (xml_node_match (node, "channel")) {
            RCSubscription *sub;
            char *id_str, *last_seen_str;

            id_str = xml_get_prop (node, "id");
            last_seen_str = xml_get_prop (node, "last_seen");

            if (id_str && *id_str) {
                sub = rc_subscription_new (id_str);
             
                if (last_seen_str)
                    sub->last_seen = (time_t) atol (last_seen_str);
                else
                    sub->last_seen = time (NULL);

                sub->old       = xml_get_guint32_prop_default (node, "old", 0);

                subscriptions = g_list_prepend (subscriptions, sub);
            }

            g_free (id_str);
            g_free (last_seen_str);

        }

        node = node->next;
    }

    xmlFreeDoc (doc);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_subscription_get_status (RCChannel *channel)
{
    GList *iter;

    if (subscriptions == NULL)
        rc_subscription_load ();

    if (channel == NULL)
        return FALSE;

    for (iter = subscriptions; iter != NULL; iter = iter->next) {
        RCSubscription *sub = iter->data;
        if (rc_subscription_match (sub, channel))
            return TRUE;
    }

    rc_subscription_save ();

    return FALSE;
}

void
rc_subscription_set_status (RCChannel *channel,
                            gboolean   subscribe_to_channel)
{
    gboolean currently_subscribed;
    GList *iter;

    g_return_if_fail (channel != NULL);

    currently_subscribed = rc_subscription_get_status (channel);

    if (currently_subscribed && !subscribe_to_channel) {

        /* Unsubscribe to the channel */
        for (iter = subscriptions; iter != NULL; iter = iter->next) {
            RCSubscription *sub = iter->data;
            if (rc_subscription_match (sub, channel)) {
                subscriptions = g_list_delete_link (subscriptions, iter);
                rc_subscription_free (sub);
                subscriptions_changed = TRUE;
                break;
            }
        }

    } else if (!currently_subscribed && subscribe_to_channel) {

        /* Subscribe to the channel */
        RCSubscription *sub;
        sub = rc_subscription_new (rc_channel_get_id (channel));
        subscriptions = g_list_prepend (subscriptions, sub);
        subscriptions_changed = TRUE;
    }

    rc_subscription_save ();
}
