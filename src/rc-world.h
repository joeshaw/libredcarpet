/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world.h
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Developed by Jon Trowbridge <trow@ximian.com>
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef __RC_WORLD_H__
#define __RC_WORLD_H__

#include "rc-package.h"

typedef struct _RCWorld RCWorld;

struct _RCWorld {

	RCPackageHashTableByString *packages_by_name;
	RCPackageHashTableByString *provides_by_name;
    RCPackageHashTableByString *requires_by_name;

    int freeze_count;
};

#define RC_WORLD_SYSTEM_PACKAGES (NULL)
#define RC_WORLD_ANY_CHANNEL     ((RCChannel *) 0x1)
#define RC_WORLD_ANY_NON_SYSTEM  ((RCChannel *) 0x2)

RCWorld   *rc_get_world (void);

RCWorld   *rc_world_new  (void);
void       rc_world_free (RCWorld *);

void       rc_world_freeze (RCWorld *);
void       rc_world_thaw   (RCWorld *);
void       rc_world_add_package (RCWorld *, RCPackage *);
void       rc_world_add_packages_from_slist (RCWorld *, RCPackageSList *package_list);

void       rc_world_remove_package  (RCWorld *, RCPackage *);
void       rc_world_remove_packages (RCWorld *, RCChannel *);

RCPackage *rc_world_find_installed_version (RCWorld *, RCPackage *);

RCPackage *rc_world_get_package (RCWorld *, RCChannel *, const char *name);

int        rc_world_foreach_package (RCWorld *world, RCChannel *channel,
                                     RCPackageFn fn, gpointer user_data);

int        rc_world_foreach_package_by_name (RCWorld *world, const char *name,
                                             RCChannel *channel,
                                             RCPackageFn fn, gpointer user_data);

int        rc_world_foreach_upgrade         (RCWorld *world, RCPackage *package,
                                             RCChannel *channel,
                                             RCPackageFn fn, gpointer user_data);

RCPackage *rc_world_get_best_upgrade        (RCWorld *world, RCPackage *package);

int        rc_world_foreach_system_package_with_upgrade (RCWorld *world, 
                                                         RCPackagePairFn fn, gpointer user_data);

int        rc_world_foreach_providing_package (RCWorld *world, RCPackageDep *dep, 
                                               RCChannel *channel,
                                               RCPackageAndSpecFn fn, gpointer user_data);

gboolean   rc_world_check_providing_package   (RCWorld *world, RCPackageDep *dep, 
                                               RCChannel *channel, gboolean filter_dups_of_installed,
                                               RCPackageAndSpecCheckFn fn, gpointer user_data);

int        rc_world_foreach_requiring_package (RCWorld *world, RCPackageDep *dep,
                                               RCChannel *channel,
                                               RCPackageAndDepFn fn, gpointer user_data);

void       rc_world_spew (RCWorld *world, FILE *out);

#endif /* __RC_WORLD_H__ */

