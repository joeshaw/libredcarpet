#include "rc-channel.h"
#include "rc-common.h"

#include <stdlib.h>
#include <unistd.h>
#include "xml-util.h"

RCSubchannel *
rc_subchannel_new (void)
{
    RCSubchannel *rcs = g_new0 (RCSubchannel, 1);

    return (rcs);
} /* rc_subchannel_new */

void
rc_subchannel_free (RCSubchannel *rcs)
{
    g_free (rcs->name);

    /* I'm not freeing the hashtable because Vlad asked me not too...
       /me is skeptical */

    g_free (rcs);
} /* rc_subchannel_free */

void
rc_subchannel_slist_free(RCSubchannelSList *rcsl)
{
    g_slist_foreach(rcsl, (GFunc) rc_subchannel_free, NULL);

    g_slist_free(rcsl);
} /* rc_subchannel_slist_free */

RCChannel *
rc_channel_new (void)
{
    RCChannel *rcc = g_new0 (RCChannel, 1);

    return (rcc);
} /* rc_channel_new */

void
rc_channel_free (RCChannel *rcc)
{
    g_free (rcc->name);
    g_free (rcc->description);
    g_free (rcc->distribution);
    g_free (rcc->path);
    g_free (rcc->subs_url);
    g_free (rcc->unsubs_url);
    g_free (rcc->icon_file);
    g_free (rcc->title_file);
    g_free (rcc->title_color);
    g_free (rcc);
} /* rc_channel_free */

void
rc_channel_slist_free(RCChannelSList *rccl)
{
    g_slist_foreach(rccl, (GFunc) rc_channel_free, NULL);
    
    g_slist_free(rccl);
} /* rc_channel_slist_free */

RCChannelSList *
rc_channel_parse_xml(char *xmlbuf, int compressed_length)
{
    RCChannelSList *cl = NULL;
    xmlDoc *doc;
    xmlNode *root, *node;

    RC_ENTRY;

    /* We first write this to a file, because xmlbuf contains
     * gzip-compressed data (and gnome-transfer's gzip handling
     * is broken), because then we can use gnome-xml's transparent
     * gzip handling
     */

    if (compressed_length) {
        gchar *gz_tmp_name = g_strdup ("/tmp/rc-xml.XXXXXX");
        int gz_fd;
        gz_fd = mkstemp (gz_tmp_name);
        write (gz_fd, xmlbuf, compressed_length);
        /* gz_fd is purposely kept open so that no intruding file
         * can be stuck instead of our gz
         */
        doc = xmlParseFile (gz_tmp_name);
        close (gz_fd);
        unlink (gz_tmp_name);
        g_free (gz_tmp_name);
    } else {
        doc = xmlParseMemory(xmlbuf, strlen(xmlbuf));
    }
    
    if (!doc) {
        g_warning("Unable to parse channel list.");
        RC_EXIT;
        return NULL;
    }

    root = doc->root;
    if (!root) {
        /* Uh. Bad. */
        g_warning("channels.xml document has no root");
        xmlFreeDoc(doc);
        RC_EXIT;
        return NULL;
    }

    node = root->childs;
    while (node) {
        char *tmp;
        RCChannel *channel;

	channel = rc_channel_new();

        channel->name = xml_get_prop(node, "name");
        channel->path = xml_get_prop(node, "path");
	channel->icon_file = xml_get_prop(node, "icon");
	channel->title_file = xml_get_prop(node, "title");
	channel->title_color = xml_get_prop(node, "bgcolor");
	channel->title_bg_image = xml_get_prop(node, "bgimage");
	channel->description = xml_get_prop(node, "description");
        channel->distribution = xml_get_prop(node, "distribution");
        tmp = xml_get_prop (node, "major");
        channel->major = atoi (tmp);
        g_free (tmp);
        tmp = xml_get_prop (node, "minor");
        channel->minor = atoi (tmp);
        g_free (tmp);
	channel->subs_url = xml_get_prop(node, "subs_url");
        channel->unsubs_url = xml_get_prop(node, "unsubs_url");
        tmp = xml_get_prop(node, "id");
	channel->id = atoi(tmp);
        g_free(tmp);
	tmp = xml_get_prop(node, "last_update");
	if (tmp)
	    channel->last_update = atol(tmp);
	g_free(tmp);

        cl = g_slist_append(cl, channel);

        node = node->next;
    }

    xmlFreeDoc(doc);

    RC_EXIT;

    return cl;
} /* rc_channel_parse_xml */
        
int
rc_channel_get_id_by_name(RCChannelSList *channels, char *name)
{
    RCChannelSList *i;

    RC_ENTRY;
      
    i = channels;
    while (i) {
        RCChannel *c = i->data;

        if (g_strcasecmp(c->name, name) == 0) {
            RC_EXIT;
            return c->id;
        }

        i = i->next;
    }

    RC_EXIT;
    return -1;
} /* rc_channel_get_id_by_name */

RCChannel *
rc_channel_get_by_id(RCChannelSList *channels, int id)
{
    RCChannelSList *i;
    
    RC_ENTRY;
    
    i = channels;
    while (i) {
        RCChannel *c = i->data;

        if (c->id == id) {
            RC_EXIT;
            return c;
        }

        i = i->next;
    }

    RC_EXIT;
    return NULL;
} /* rc_channel_get_by_id */

RCChannel *
rc_channel_get_by_name(RCChannelSList *channels, char *name)
{
    int id;
    RCChannel *c;
    
    RC_ENTRY;
    
    id = rc_channel_get_id_by_name(channels, name);
    c = rc_channel_get_by_id(channels, id);

    RC_EXIT;
    return c;
} /* rc_channel_get_by_name */
