#ifndef _RC_CHANNEL_H
#define _RC_CHANNEL_H

#include "rc-package.h"

typedef struct _RCSubchannel RCSubchannel;

struct _RCSubchannel {
    gchar *name;
    guint32 preference;

    RCPackageHashTableBySpec *packages;
};

RCSubchannel *rc_subchannel_new (void);

void rc_subchannel_free (RCSubchannel *rcs);

typedef GSList RCSubchannelSList;

void rc_subchannel_slist_free(RCSubchannelSList *rcsl);

typedef struct _RCChannel RCChannel;

struct _RCChannel {
    guint32 id;
    gchar *name;
    gchar *description;

    gchar *distribution;
    guint major, minor;

    gchar *path;

    gchar *subs_url;
    gchar *unsubs_url;

    /* for use as pixbufs in gui.h */
	char *icon_file;
    char *title_file;
    char *title_color;
    char *title_bg_image;

    time_t last_update;

    RCPackageHashTableBySpec *dep_table;
    RCSubchannelSList *subchannels;
};

RCChannel *rc_channel_new (void);

void rc_channel_free (RCChannel *rcc);

typedef GSList RCChannelSList;

void rc_channel_slist_free(RCChannelSList *rccl);

RCChannelSList *rc_channel_parse_xml(char *xmlbuf, int compressed_length);

int rc_channel_get_id_by_name(RCChannelSList *channels, char *name);

RCChannel *rc_channel_get_by_id(RCChannelSList *channels, int id);

RCChannel *rc_channel_get_by_name(RCChannelSList *channels, char *name);

#endif /* _RC_CHANNEL_H */
