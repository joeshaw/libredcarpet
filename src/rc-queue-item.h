/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-queue-item.h
 *
 * Copyright (C) 2002 Ximian, Inc.
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

#ifndef __RC_QUEUE_ITEM_H__
#define __RC_QUEUE_ITEM_H__

#include "rc-package.h"
#include "rc-resolver-context.h"
#include "rc-resolver-info.h"

typedef enum {
    RC_QUEUE_ITEM_TYPE_UNKNOWN = 0,
    RC_QUEUE_ITEM_TYPE_INSTALL,
    RC_QUEUE_ITEM_TYPE_REQUIRE,
    RC_QUEUE_ITEM_TYPE_BRANCH,
    RC_QUEUE_ITEM_TYPE_GROUP,
    RC_QUEUE_ITEM_TYPE_CONFLICT,
    RC_QUEUE_ITEM_TYPE_UNINSTALL,
    RC_QUEUE_ITEM_TYPE_LAST
} RCQueueItemType;

typedef struct _RCQueueItem           RCQueueItem;
typedef struct _RCQueueItem_Install   RCQueueItem_Install;
typedef struct _RCQueueItem_Require   RCQueueItem_Require;
typedef struct _RCQueueItem_Branch    RCQueueItem_Branch;
typedef struct _RCQueueItem_Group     RCQueueItem_Group;
typedef struct _RCQueueItem_Conflict  RCQueueItem_Conflict;
typedef struct _RCQueueItem_Uninstall RCQueueItem_Uninstall;


struct _RCQueueItem {
    RCQueueItemType type;
    int priority;
    size_t size;
    GSList *pending_info;

    RCWorld *world;

    /* 
       is_redundant TRUE == can be dropped from any branch
    
       is_satisfied TRUE == can be dropped from any queue, and any
       branch containing it can also be dropped
    */
    gboolean (*is_redundant) (RCQueueItem *, RCResolverContext *);
    gboolean (*is_satisfied) (RCQueueItem *, RCResolverContext *);
    gboolean (*process)      (RCQueueItem *, RCResolverContext *, GSList **);
    void     (*destroy)      (RCQueueItem *);
    void     (*copy)         (const RCQueueItem *src, RCQueueItem *dest);
    int      (*cmp)          (const RCQueueItem *a, const RCQueueItem *b);
    char    *(*to_string)    (RCQueueItem *);
};

struct _RCQueueItem_Install {
    RCQueueItem parent;

    RCPackage *package;
    RCPackage *upgrades;
    GSList *deps_satisfied_by_this_install;
    GSList *needed_by;
    int channel_priority;
    int other_penalty;

    guint explicitly_requested : 1;
};

struct _RCQueueItem_Require {
    RCQueueItem parent;

    RCPackageDep *dep;
    RCPackage *requiring_package;
    RCPackage *upgraded_package;
    RCPackage *lost_package;
    guint remove_only : 1;
};

struct _RCQueueItem_Branch {
    RCQueueItem parent;

    char   *label;
    GSList *possible_items;
};

struct _RCQueueItem_Group {
    RCQueueItem parent;
    
    GSList *subitems;
};

struct _RCQueueItem_Conflict {
    RCQueueItem parent;

    RCPackageDep *dep;
    RCPackage *conflicting_package;

    guint actually_an_obsolete : 1;
};

struct _RCQueueItem_Uninstall {
    RCQueueItem parent;

    RCPackage *package;
    char *reason;
    RCPackageDep *dep_leading_to_uninstall;
    RCPackage *upgraded_to;

    guint explicitly_requested : 1;
    guint remove_only          : 1;
    guint due_to_obsolete      : 1;
};

RCQueueItemType rc_queue_item_type         (RCQueueItem *);
gboolean        rc_queue_item_is_redundant (RCQueueItem *, RCResolverContext *context);
gboolean        rc_queue_item_is_satisfied (RCQueueItem *, RCResolverContext *context);
gboolean        rc_queue_item_process      (RCQueueItem *, RCResolverContext *context, GSList **);
void            rc_queue_item_free         (RCQueueItem *);
RCQueueItem    *rc_queue_item_copy         (RCQueueItem *);
int             rc_queue_item_cmp          (const RCQueueItem *a, const RCQueueItem *b);
char           *rc_queue_item_to_string    (RCQueueItem *);
void            rc_queue_item_add_info     (RCQueueItem *, RCResolverInfo *);
void            rc_queue_item_log_info     (RCQueueItem *, RCResolverContext *);
RCWorld        *rc_queue_item_get_world    (RCQueueItem *);

RCQueueItem *rc_queue_item_new_install                  (RCWorld *, RCPackage *package);
void         rc_queue_item_install_set_upgrade_package  (RCQueueItem *item, RCPackage *upgrades);
void         rc_queue_item_install_add_dep              (RCQueueItem *item, RCPackageDep *dep);
void         rc_queue_item_install_add_needed_by        (RCQueueItem *item, RCPackage *package);
void         rc_queue_item_install_set_channel_priority (RCQueueItem *item, int);
int          rc_queue_item_install_get_channel_priority (RCQueueItem *item);
void         rc_queue_item_install_set_other_penalty    (RCQueueItem *item, int);
int          rc_queue_item_install_get_other_penalty    (RCQueueItem *item);
void         rc_queue_item_install_set_explicitly_requested (RCQueueItem *item);

RCQueueItem *rc_queue_item_new_require         (RCWorld *, RCPackageDep *dep);
void         rc_queue_item_require_add_package (RCQueueItem *item, RCPackage *package);
void         rc_queue_item_require_set_remove_only (RCQueueItem *item);

RCQueueItem *rc_queue_item_new_branch       (RCWorld *);
void         rc_queue_item_branch_set_label (RCQueueItem *branch, const char *str);
void         rc_queue_item_branch_add_item  (RCQueueItem *branch, RCQueueItem *subitem);
gboolean     rc_queue_item_branch_is_empty  (RCQueueItem *branch);
gboolean     rc_queue_item_branch_contains  (RCQueueItem *branch,
                                             RCQueueItem *possible_subbranch);

RCQueueItem *rc_queue_item_new_group       (RCWorld *);
void         rc_queue_item_group_add_item  (RCQueueItem *group, RCQueueItem *subitem);

RCQueueItem *rc_queue_item_new_conflict       (RCWorld *, RCPackageDep *dep, RCPackage *package);
 
RCQueueItem *rc_queue_item_new_uninstall             (RCWorld *, RCPackage *package, const char *reason);
void         rc_queue_item_uninstall_set_dep         (RCQueueItem *item, RCPackageDep *dep);
void         rc_queue_item_uninstall_set_remove_only (RCQueueItem *item);
void         rc_queue_item_uninstall_set_upgraded_to (RCQueueItem *item, RCPackage *packageo);
void         rc_queue_item_uninstall_set_explicitly_requested (RCQueueItem *item);






#endif /* __RC_QUEUE_ITEM_H__ */

