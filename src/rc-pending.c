/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-pending.c
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

#include <config.h>
#include "rc-pending.h"

#include <math.h>
#include <stdlib.h>
#include "rc-marshal.h"
#include "rc-debug.h"

static GObjectClass *parent_class;

enum {
    UPDATE,
    COMPLETE,
    MESSAGE,
    LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

const char *
rc_pending_status_to_string (RCPendingStatus status)
{
    switch (status) {
    
    case RC_PENDING_STATUS_PRE_BEGIN:
        return "pre-begin";
        
    case RC_PENDING_STATUS_RUNNING:
        return "running";

    case RC_PENDING_STATUS_BLOCKING:
        return "blocking";

    case RC_PENDING_STATUS_ABORTED:
        return "aborted";

    case RC_PENDING_STATUS_FAILED:
        return "failed";

    case RC_PENDING_STATUS_FINISHED:
        return "finished";

    case RC_PENDING_STATUS_INVALID:
    default:
        return "invalid";
        
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_pending_finalize (GObject *obj)
{
    RCPending *pending = (RCPending *) obj;

    g_free (pending->description);
    g_free (pending->error_msg);

    if (parent_class->finalize)
        parent_class->finalize (obj);
}

static void
rc_pending_update_handler (RCPending *pending)
{
    rc_debug (RC_DEBUG_LEVEL_INFO,
              "id=%d '%s' %.1f%%/%ds remaining (%s)",
              pending->id, pending->description,
              pending->percent_complete,
              rc_pending_get_remaining_secs (pending),
              rc_pending_status_to_string (pending->status));
}

static void
rc_pending_complete_handler (RCPending *pending)
{
    rc_debug (RC_DEBUG_LEVEL_MESSAGE,
              "id=%d COMPLETE '%s' time=%ds (%s)",
              pending->id, pending->description,
              rc_pending_get_elapsed_secs (pending),
              rc_pending_status_to_string (pending->status));
}

static void
rc_pending_class_init (RCPendingClass *klass)
{
    GObjectClass *obj_class = (GObjectClass *) klass;

    parent_class = g_type_class_peek_parent (klass);

    obj_class->finalize = rc_pending_finalize;

    klass->update = rc_pending_update_handler;
    klass->complete = rc_pending_complete_handler;

    signals[UPDATE] =
        g_signal_new ("update",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (RCPendingClass, update),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[COMPLETE] =
        g_signal_new ("complete",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (RCPendingClass, complete),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);

    signals[MESSAGE] =
        g_signal_new ("message",
                      G_TYPE_FROM_CLASS (klass),
                      G_SIGNAL_RUN_FIRST,
                      G_STRUCT_OFFSET (RCPendingClass, message),
                      NULL, NULL,
                      rc_marshal_VOID__VOID,
                      G_TYPE_NONE, 0);
}

static void
rc_pending_init (RCPending *pending)
{
    pending->status         = RC_PENDING_STATUS_PRE_BEGIN;
    pending->completed_size = -1;
    pending->total_size     = -1;
}

GType
rc_pending_get_type (void)
{
    static GType type = 0;

    if (! type) {
        static GTypeInfo type_info = {
            sizeof (RCPendingClass),
            NULL, NULL,
            (GClassInitFunc) rc_pending_class_init,
            NULL, NULL,
            sizeof (RCPending),
            0,
            (GInstanceInitFunc) rc_pending_init
        };

        type = g_type_register_static (G_TYPE_OBJECT,
                                       "RCPending",
                                       &type_info,
                                       0);
    }

    return type;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static GHashTable *id_hash = NULL;

/* Wait three minutes before cleaning up any old pending items. */
#define CLEANUP_TIME (3*60)

/* Only do clean-up one per minute. */
#define CLEANUP_DELAY 60

static gboolean
pending_cleanup_cb (gpointer key,
                    gpointer value,
                    gpointer user_data)
{
    RCPending *pending = value;
    time_t *now = user_data;

    /* We will only clean up an RCPending if:
       + It is no longer active.
       + It became in active more that CLEANUP_TIME seconds ago.
       + It hasn't been polled in the last CLEANUP_TIME seconds.
       
       Hopefully this will be sufficient to keep RCPendings that
       are actually important from getting cleaned out from under us.
    */
    if (pending
        && ! rc_pending_is_active (pending)
        && difftime (*now, pending->last_time) > CLEANUP_TIME
        && (pending->poll_time == 0
            || difftime (*now, pending->poll_time) > CLEANUP_TIME)) {

        g_object_unref (pending);
        return TRUE;
    }

    return FALSE;
}

static void
rc_pending_cleanup (void)
{
    if (id_hash) {
        static time_t last_cleanup = (time_t)0;
        time_t now;

        if (getenv ("RC_NO_PENDING_CLEANUP"))
            return;
        
        time (&now);

        if (last_cleanup == (time_t)0
            || difftime (now, last_cleanup) > CLEANUP_DELAY) {

            g_hash_table_foreach_remove (id_hash,
                                         pending_cleanup_cb,
                                         &now);

            last_cleanup = now;
        }
    }
}


RCPending *
rc_pending_new (const char *description)
{
    static gint next_id = 1;

    RCPending *pending = g_object_new (RC_TYPE_PENDING, NULL);

    pending->description = g_strdup (description);
    pending->id = next_id;

    rc_pending_cleanup ();
    
    if (id_hash == NULL) {
        id_hash = g_hash_table_new (NULL, NULL);
    }

    g_hash_table_insert (id_hash,
                         GINT_TO_POINTER (next_id),
                         g_object_ref (pending));

    ++next_id;
    
    return pending;
}


RCPending *
rc_pending_lookup_by_id (gint id)
{
    RCPending *pending;

    if (id <= 0 || id_hash == NULL)
        return NULL;

    pending = g_hash_table_lookup (id_hash, GINT_TO_POINTER (id));

    if (pending) {
        g_return_val_if_fail (pending->id == id, NULL);
        
        time (&pending->poll_time);
    }

    /* We never have to worry about cleaning up the RCPending that
       we just looked up, since the poll time will be too recent. */
    rc_pending_cleanup ();

    return pending;
}

static void
get_all_ids_cb (gpointer key, gpointer val, gpointer user_data)
{
    GSList **id_list = user_data;
    RCPending *pending = val;
    RCPendingStatus status = rc_pending_get_status (pending);
    
    if (status == RC_PENDING_STATUS_PRE_BEGIN
        || status == RC_PENDING_STATUS_RUNNING
        || status == RC_PENDING_STATUS_BLOCKING) {

        time (&pending->poll_time);

        *id_list = g_slist_prepend (*id_list,
                                    GINT_TO_POINTER (rc_pending_get_id (pending)));
    }
}

GSList *
rc_pending_get_all_active_ids (void)
{
    GSList *id_list = NULL;

    rc_pending_cleanup ();

    g_hash_table_foreach (id_hash, get_all_ids_cb, &id_list);

    return id_list;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
rc_pending_timestamp (RCPending *pending)
{
    time (&pending->last_time);
}

void
rc_pending_begin (RCPending *pending)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (pending->status == RC_PENDING_STATUS_PRE_BEGIN);
    
    pending->status = RC_PENDING_STATUS_RUNNING;
    time (&pending->start_time);

    rc_pending_update (pending, 0);

    rc_debug (RC_DEBUG_LEVEL_MESSAGE,
              "id=%d BEGIN '%s' (%s)",
              pending->id, pending->description,
              rc_pending_status_to_string (pending->status));
}

void
rc_pending_update (RCPending *pending,
                    double      percent_complete)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (pending->status == RC_PENDING_STATUS_RUNNING);
    g_return_if_fail (0 <= percent_complete && percent_complete <= 100);

    rc_pending_timestamp (pending);

    pending->completed_size   = -1;
    pending->total_size       = -1;
    pending->percent_complete = percent_complete;

    g_signal_emit (pending, signals[UPDATE], 0);
}

void
rc_pending_update_by_size (RCPending *pending,
                            int         completed_size,
                            int         total_size)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (pending->status == RC_PENDING_STATUS_RUNNING);
    g_return_if_fail (0 <= completed_size && completed_size <= total_size);

    rc_pending_timestamp (pending);

    pending->completed_size   = completed_size;
    pending->total_size       = total_size;

    if (total_size > 0)
        pending->percent_complete = 100 * (completed_size / (double) total_size);
    else
        pending->percent_complete = 100; /* I guess 0/0 = 1 after all :-) */

    g_signal_emit (pending, signals[UPDATE], 0);
}

void
rc_pending_finished (RCPending *pending,
                      gint        retval)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (pending->status == RC_PENDING_STATUS_RUNNING);

    rc_pending_timestamp (pending);

    pending->status = RC_PENDING_STATUS_FINISHED;
    pending->retval = retval;

    g_signal_emit (pending, signals[COMPLETE], 0);
}

void
rc_pending_abort (RCPending *pending,
                   gint        retval)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (pending->status == RC_PENDING_STATUS_RUNNING);

    rc_pending_timestamp (pending);

    pending->status = RC_PENDING_STATUS_ABORTED;
    pending->retval = retval;

    g_signal_emit (pending, signals[COMPLETE], 0);
}

void
rc_pending_fail (RCPending *pending,
                  gint        retval,
                  const char *error_msg)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (pending->status == RC_PENDING_STATUS_RUNNING);

    rc_pending_timestamp (pending);

    pending->status    = RC_PENDING_STATUS_FAILED;
    pending->retval    = retval;
    pending->error_msg = g_strdup (error_msg);

    g_signal_emit (pending, signals[COMPLETE], 0);
}

gboolean
rc_pending_is_active (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), FALSE);

    return pending->status != RC_PENDING_STATUS_FINISHED
        && pending->status != RC_PENDING_STATUS_ABORTED
        && pending->status != RC_PENDING_STATUS_FAILED;
}

const char *
rc_pending_get_description (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), NULL);

    return pending->description ? pending->description : "(no description)";
}

void
rc_pending_set_description (RCPending *pending,
                             const char *desc)
{
    g_return_if_fail (RC_IS_PENDING (pending));

    g_free (pending->description);
    pending->description = g_strdup (desc);
}

gint
rc_pending_get_id (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), -1);

    return pending->id;
}

RCPendingStatus
rc_pending_get_status (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), RC_PENDING_STATUS_INVALID);
    
    return pending->status;
}

double
rc_pending_get_percent_complete (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), -1);

    return pending->percent_complete;
}

int
rc_pending_get_completed_size (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), -1);
    
    return pending->completed_size;
}

int
rc_pending_get_total_size (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), -1);

    return pending->total_size;
}

time_t
rc_pending_get_start_time (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), (time_t) 0);

    return pending->start_time;
}

time_t
rc_pending_get_last_time (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), (time_t) 0);

    return pending->last_time;
}

gint
rc_pending_get_elapsed_secs (RCPending *pending)
{
    time_t t;

    g_return_val_if_fail (RC_IS_PENDING (pending), -1);

    if (pending->start_time == (time_t) 0)
        return -1;

    if (pending->status == RC_PENDING_STATUS_RUNNING)
        time (&t);
    else
        t = pending->last_time;

    return (gint)(t - pending->start_time);
}

gint
rc_pending_get_expected_secs (RCPending *pending)
{
    double t;

    g_return_val_if_fail (RC_IS_PENDING (pending), -1);

    if (pending->start_time == (time_t) 0
        || pending->last_time == (time_t) 0
        || pending->start_time == pending->last_time
        || pending->percent_complete <= 1e-8)
        return -1;

    t = (pending->last_time - pending->start_time) / (pending->percent_complete / 100);
    return (gint) rint (t);
}

gint
rc_pending_get_remaining_secs (RCPending *pending)
{
    gint elapsed, expected;

    g_return_val_if_fail (RC_IS_PENDING (pending), -1);

    elapsed = rc_pending_get_elapsed_secs (pending);
    if (elapsed < 0)
        return -1;

    expected = rc_pending_get_expected_secs (pending);
    if (expected < 0)
        return -1;

    return elapsed <= expected ? expected - elapsed : 0;
}

const char *
rc_pending_get_error_msg (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), NULL);

    return pending->error_msg;
} /* rc_pending_get_error_msg */

void
rc_pending_add_message (RCPending *pending, const char *message)
{
    g_return_if_fail (RC_IS_PENDING (pending));
    g_return_if_fail (message);

    pending->messages = g_slist_append (pending->messages, g_strdup (message));

    g_signal_emit (pending, signals[MESSAGE], 0);
} /* rc_pending_add_message */

GSList *
rc_pending_get_messages (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), NULL);

    return pending->messages;
} /* rc_pending_get_messages */

const char *
rc_pending_get_latest_message (RCPending *pending)
{
    g_return_val_if_fail (RC_IS_PENDING (pending), NULL);

    if (!pending->messages)
        return NULL;

    return (const char *) g_slist_last (pending->messages)->data;
} /* rc_pending_get_latest_message */
