/* FIXME: put the GPL here */

#ifndef _RC_LINE_BUF_PRIVATE_H
#define _RC_LINE_BUF_PRIVATE_H

struct _RCLineBufPrivate {
    GIOChannel *channel;

    guint cb_id;

    GString *buf;
};

#endif /* _RC_LINE_BUF_PRIVATE_H */
