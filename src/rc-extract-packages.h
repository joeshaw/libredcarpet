/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-extract-packages.h
 *
 * Copyright (C) 2003 Ximian, Inc.
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

#ifndef __RC_EXTRACT_PACKAGES_H__
#define __RC_EXTRACT_PACKAGES_H__

#include "rc-channel.h"
#include "rc-package.h"
#include "rc-package-match.h"

gint rc_extract_packages_from_xml_node (xmlNode *node,
                                        RCChannel *channel,
                                        RCPackageFn callback,
                                        gpointer user_data);


gint rc_extract_packages_from_helix_buffer (const guint8 *data, int len,
                                            RCChannel *channel,
                                            RCPackageFn callback,
                                            gpointer user_data);

gint rc_extract_packages_from_helix_file   (const char *filename,
                                            RCChannel *channel,
                                            RCPackageFn callback,
                                            gpointer user_data);


gint rc_extract_packages_from_debian_buffer (const guint8 *data, int len,
                                             RCChannel *channel,
                                             RCPackageFn callback,
                                             gpointer user_data);

gint rc_extract_packages_from_debian_file   (const char *filename,
                                             RCChannel *channel,
                                             RCPackageFn callback,
                                             gpointer user_data);

RCPackage * rc_extract_yum_package          (const guint8 *data, int len,
                                             RCPackman *packman,
                                             char *url);

gint rc_extract_packages_from_aptrpm_buffer (const guint8 *data, int len,
                                             RCPackman *packman,
                                             RCChannel *channel,
                                             RCPackageFn callback,
                                             gpointer user_data);

gint rc_extract_packages_from_aptrpm_file   (const char *filename,
                                             RCPackman *packman,
                                             RCChannel *channel,
                                             RCPackageFn callback,
                                             gpointer user_data);

gint rc_extract_packages_from_undump_buffer (const guint8 *data, int len,
                                             RCChannelAndSubdFn channel_callback,
                                             RCPackageFn package_callback,
                                             RCPackageMatchFn lock_callback,
                                             gpointer user_data);

gint rc_extract_packages_from_undump_file   (const char *filename,
                                             RCChannelAndSubdFn channel_callback,
                                             RCPackageFn package_callback,
                                             RCPackageMatchFn lock_callback,
                                             gpointer user_data);

gint rc_extract_packages_from_directory (const char *path,
                                         RCChannel *channel,
                                         RCPackman *packman,
                                         gboolean recursive,
                                         RCPackageFn callback,
                                         gpointer user_data);


#endif /* __RC_EXTRACT_PACKAGES_H__ */

