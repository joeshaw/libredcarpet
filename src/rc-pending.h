/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-pending.h
 *
 * Copyright (C) 2002-2003 Ximian, Inc.
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

#ifndef __RC_PENDING_H__
#define __RC_PENDING_H__

#include <time.h>
#include <glib-object.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

typedef struct _RCPending       RCPending;
typedef struct _RCPendingClass  RCPendingClass;

typedef enum {
    RC_PENDING_STATUS_INVALID = 0,
    RC_PENDING_STATUS_PRE_BEGIN,
    RC_PENDING_STATUS_RUNNING,
    RC_PENDING_STATUS_BLOCKING,
    RC_PENDING_STATUS_ABORTED,
    RC_PENDING_STATUS_FAILED,
    RC_PENDING_STATUS_FINISHED
} RCPendingStatus;

const char *rc_pending_status_to_string (RCPendingStatus status);

#define RC_INVALID_PENDING_ID 0


struct _RCPending {
    GObject parent;

    char *description;
    gint id;

    RCPendingStatus status;

    double percent_complete;

    int completed_size;
    int total_size;

    time_t start_time;
    time_t last_time;
    time_t poll_time;

    gint retval;
    char *error_msg;

    GSList *messages;
};

struct _RCPendingClass {
    GObjectClass parent_class;

    void (*update) (RCPending *);
    void (*complete) (RCPending *);
    void (*message) (RCPending *);
};

#define RC_TYPE_PENDING            (rc_pending_get_type ())
#define RC_PENDING(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                    RC_TYPE_PENDING, RCPending))
#define RC_PENDING_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                    RC_TYPE_PENDING, RCPendingClass))
#define RC_IS_PENDING(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                    RC_TYPE_PENDING))
#define RC_IS_PENDING_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                    RC_TYPE_PENDING))

GType rc_pending_get_type (void);

RCPending *rc_pending_new                (const char *description);
RCPending *rc_pending_lookup_by_id       (gint id);
GSList    *rc_pending_get_all_active_ids (void);

void rc_pending_begin    (RCPending *);

void rc_pending_update         (RCPending *, double percent_complete);
void rc_pending_update_by_size (RCPending *, int size, int total_size);

void rc_pending_finished (RCPending *, gint retval);
void rc_pending_abort    (RCPending *, gint retval);
void rc_pending_fail     (RCPending *, gint retval, const char *error_msg);

gboolean rc_pending_is_active (RCPending *);

const char     *rc_pending_get_description      (RCPending *);
void            rc_pending_set_description      (RCPending *, const char *desc);
gint            rc_pending_get_id               (RCPending *);
RCPendingStatus rc_pending_get_status           (RCPending *);
double          rc_pending_get_percent_complete (RCPending *);
int             rc_pending_get_completed_size   (RCPending *);
int             rc_pending_get_total_size       (RCPending *);
time_t          rc_pending_get_start_time       (RCPending *);
time_t          rc_pending_get_last_time        (RCPending *);

gint            rc_pending_get_elapsed_secs     (RCPending *);
gint            rc_pending_get_expected_secs    (RCPending *);
gint            rc_pending_get_remaining_secs   (RCPending *);
const char     *rc_pending_get_error_msg        (RCPending *);

void            rc_pending_add_message        (RCPending *,
                                                 const char *message);
GSList         *rc_pending_get_messages       (RCPending *);
const char     *rc_pending_get_latest_message (RCPending *);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __RC_PENDING_H__ */



