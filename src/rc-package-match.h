/* This is -*- C -*- */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* $Id$ */

/*
 * rc-package-match.h
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

#ifndef __RC_PACKAGE_MATCH_H__
#define __RC_PACKAGE_MATCH_H__

#include "rc-package.h"
#include "rc-channel.h"

struct _RCWorld; /* forward declaration */

typedef struct _RCPackageMatch RCPackageMatch;

typedef gboolean (*RCPackageMatchFn) (struct _RCPackageMatch *, gpointer);

RCPackageMatch *rc_package_match_new (void);

void            rc_package_match_free (RCPackageMatch *package_match);


void            rc_package_match_set_channel (RCPackageMatch *match,
					      RCChannel      *channel);

void            rc_package_match_set_channel_id (RCPackageMatch *match,
						 const char     *cid);

const char     *rc_package_match_get_channel_id (RCPackageMatch *match);

void            rc_package_match_set_dep     (RCPackageMatch *match,
					      RCPackageDep   *dep);

RCPackageDep   *rc_package_match_get_dep     (RCPackageMatch *match);

void            rc_package_match_set_glob    (RCPackageMatch *match,
					      const char     *glob_str);

const char     *rc_package_match_get_glob    (RCPackageMatch *match);

void                rc_package_match_set_importance (RCPackageMatch *match,
						     RCPackageImportance importance,
						     gboolean match_gteq);

RCPackageImportance rc_package_match_get_importance (RCPackageMatch *match,
						     gboolean *match_gteq);

gboolean        rc_package_match_equal (RCPackageMatch *match1,
					RCPackageMatch *match2);

gboolean        rc_package_match_test (RCPackageMatch  *package_match,
				       RCPackage       *package,
				       struct _RCWorld *world);


/* Marshalling to/from strings */

xmlNode        *rc_package_match_to_xml_node   (RCPackageMatch *match);

RCPackageMatch *rc_package_match_from_xml_node (xmlNode         *node);

#endif /* __RC_PACKAGE_MATCH_H__ */

