/* rc-verification-private.h
 * Copyright (C) 2000 Helix Code, Inc.
 * Author: Ian Peters <itp@helixcode.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef _RC_VERIFICATION_PRIVATE_H
#define _RC_VERIFICATION_PRIVATE_H

#include <glib.h>

#include "rc-verification.h"

RCVerification *rc_verify_gpg (gchar *filename, gchar *signature);

RCVerification *rc_verify_md5 (gchar *filename, guint8 *md5);

RCVerification *rc_verify_md5_string (gchar *filename, gchar *md5);

RCVerification *rc_verify_size (gchar *filename, guint32 size);

#endif /* _RC_VERIFICATION_PRIVATE_H */