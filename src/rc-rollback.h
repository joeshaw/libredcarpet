/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rollback.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef __RC_ROLLBACK_H__
#define __RC_ROLLBACK_H__

#include "rc-packman.h"

typedef struct _RCRollbackInfo       RCRollbackInfo;

typedef struct _RCRollbackAction     RCRollbackAction;
typedef GSList                       RCRollbackActionSList;

/*
 * Rollback info must be created before a transaction.  Once the transaction
 * has run, it has to be saved to disk or discarded.  Then it has to be
 * freed.
 */

RCRollbackInfo *rc_rollback_info_new  (RCPackman      *packman,
                                       RCPackageSList *install_packages,
                                       RCPackageSList *remove_packages);

void rc_rollback_info_free (RCRollbackInfo *rollback_info);

void rc_rollback_info_save (RCRollbackInfo *rollback_info);
void rc_rollback_info_discard (RCRollbackInfo *rollback_info);

void rc_rollback_action_free (RCRollbackAction *action);
void rc_rollback_action_slist_free (RCRollbackActionSList *action);

RCRollbackActionSList *rc_rollback_get_actions (time_t when);

void rc_rollback_action_slist_free (RCRollbackActionSList *actions);

gboolean rc_rollback_action_is_install (RCRollbackAction *action);
RCPackage *rc_rollback_action_get_package (RCRollbackAction *action);
RCPackageUpdate *rc_rollback_action_get_package_update (RCRollbackAction *action);

void rc_rollback_restore_files (RCRollbackActionSList *actions);

#endif /* __RC_ROLLBACK_H__ */
