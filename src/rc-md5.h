/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-md5.h
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
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

#ifndef _RC_MD5
#define _RC_MD5

#include <glib.h>

guint8 *rc_md5        (const gchar *filename);
gchar  *rc_md5_digest (const gchar *filename);

guint8 *rc_md5_from_string        (const gchar *str);
gchar  *rc_md5_digest_from_string (const gchar *str);

#endif /* RC_MD5 */
