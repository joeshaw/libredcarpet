/* rc-line-buf-private.h
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

#ifndef _RC_LINE_BUF_PRIVATE_H
#define _RC_LINE_BUF_PRIVATE_H

struct _RCLineBufPrivate {
    GIOChannel *channel;

    guint cb_id;

    GString *buf;
};

#endif /* _RC_LINE_BUF_PRIVATE_H */
