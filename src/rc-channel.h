#ifndef _RC_CHANNEL_H
#define _RC_CHANNEL_H

#include "rc-package.h"

typedef struct _RCSubchannel RCSubchannel;

struct _RCSubchannel {
    gchar *name;
    guint32 preference;

    RCPackageHashTableByString *packages;
};

RCSubchannel *rc_subchannel_new (void);

void rc_subchannel_free (RCSubchannel *rcs);

typedef GSList RCSubchannelSList;

void rc_subchannel_slist_free(RCSubchannelSList *rcsl);

typedef enum _RCChannelType {
    RC_CHANNEL_TYPE_HELIX,      /* packageinfo.xml */
    RC_CHANNEL_TYPE_DEBIAN,     /* debian Packages.gz */
    RC_CHANNEL_TYPE_REDHAT,     /* redhat up2date RDF */
    RC_CHANNEL_TYPE_UNKNOWN,
    RC_CHANNEL_TYPE_LAST
} RCChannelType;

typedef struct _RCChannel RCChannel;

struct _RCChannel {
    guint32 id;
    gchar *name;
    gchar *description;

    gboolean mirrored;

    RCChannelType type;

    gchar *distribution;
    guint major, minor;

    gchar *path;
    gchar *file_path;

    gchar *pkginfo_file;
    gboolean pkginfo_compressed;

    gchar *subs_url;
    gchar *unsubs_url;

    /* for use as pixbufs in gui.h */
    char *icon_file;
    char *title_file;

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

gint rc_channel_compare_func (gconstpointer a, gconstpointer b);

#endif /* _RC_CHANNEL_H */
