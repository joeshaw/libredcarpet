/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-info.h
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

#ifndef __RC_RESOLVER_INFO_H__
#define __RC_RESOLVER_INFO_H__

#include "rc-package.h"

typedef enum {
    RC_RESOLVER_INFO_TYPE_INVALID = 0,
    RC_RESOLVER_INFO_TYPE_NEEDED_BY,
    RC_RESOLVER_INFO_TYPE_CONFLICTS_WITH,
    RC_RESOLVER_INFO_TYPE_OBSOLETES,
    RC_RESOLVER_INFO_TYPE_DEPENDS_ON,
    RC_RESOLVER_INFO_TYPE_CHILD_OF,
    RC_RESOLVER_INFO_TYPE_MISC
} RCResolverInfoType;

#define RC_RESOLVER_INFO_PRIORITY_USER      500
#define RC_RESOLVER_INFO_PRIORITY_VERBOSE   100
#define RC_RESOLVER_INFO_PRIORITY_DEBUGGING   0

typedef struct _RCResolverInfo RCResolverInfo;

struct _RCResolverInfo {
    RCResolverInfoType type;
    RCPackage *package;
    int priority;

    GSList *package_list;
    char   *msg;
    char   *action;
    char   *trigger;

    guint is_error     : 1;
    guint is_important : 1;
};

typedef void (*RCResolverInfoFn) (RCResolverInfo *, gpointer);

const char        *rc_resolver_info_type_to_string   (RCResolverInfoType);
RCResolverInfoType rc_resolver_info_type_from_string (const char *);

RCResolverInfoType rc_resolver_info_type                (RCResolverInfo *);
gboolean           rc_resolver_info_merge               (RCResolverInfo *, RCResolverInfo *);
RCResolverInfo    *rc_resolver_info_copy                (RCResolverInfo *);
void               rc_resolver_info_free                (RCResolverInfo *);
char              *rc_resolver_info_to_string           (RCResolverInfo *);
char              *rc_resolver_info_packages_to_string  (RCResolverInfo *,
                                                         gboolean names_only);

gboolean           rc_resolver_info_is_about            (RCResolverInfo *, RCPackage *);
gboolean           rc_resolver_info_mentions            (RCResolverInfo *, RCPackage *);
void               rc_resolver_info_add_related_package (RCResolverInfo *, RCPackage *);

gboolean           rc_resolver_info_is_error         (RCResolverInfo *);
void               rc_resolver_info_flag_as_error    (RCResolverInfo *);

gboolean           rc_resolver_info_is_important      (RCResolverInfo *);
void               rc_resolver_info_flag_as_important (RCResolverInfo *);

RCResolverInfo    *rc_resolver_info_misc_new           (RCPackage *package, int priority,
                                                        char *msg);
void               rc_resolver_info_misc_add_action    (RCResolverInfo *info, char *action_msg);
void               rc_resolver_info_misc_add_trigger   (RCResolverInfo *info, char *trigger_msg);

RCResolverInfo    *rc_resolver_info_needed_by_new      (RCPackage *package);
void               rc_resolver_info_needed_add         (RCResolverInfo *, RCPackage *needed_by);
void               rc_resolver_info_needed_add_slist   (RCResolverInfo *, GSList *);

RCResolverInfo    *rc_resolver_info_conflicts_with_new (RCPackage *package, RCPackage *conflicts_with);

RCResolverInfo    *rc_resolver_info_obsoletes_new (RCPackage *package, RCPackage *obsoletes);

RCResolverInfo    *rc_resolver_info_depends_on_new (RCPackage *package, RCPackage *dependency);

RCResolverInfo    *rc_resolver_info_child_of_new (RCPackage *package, RCPackage *dependency);

#endif /* __RC_RESOLVER_INFO_H__ */

