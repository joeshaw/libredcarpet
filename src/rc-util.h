/* -*- Mode: C; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *             Joe Shaw <joe@helixcode.com>
 *             Vladimir Vukicevic <vladimir@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <glib.h>
#include <time.h>
#include <stdio.h>

/* Function to safely delete a directory recursively (only deletes regular
   files), should be safe from symlink attacks.

   Returns 0 on success, -1 on error
*/
gint rc_rmdir (gchar *dir);

/* Create a temporary directory, in REDCARPET_TMPDIR, then TMPDIR, then /tmp.
   If prefix is NULL, the directory name begins with redcarpet.

   Returns: the name of a newly created temporary directory, which must be
   freed when you are done with it.  Remove the directory when you want with
   rc_rmdir()
*/
gchar *rc_mktmpdir (const gchar *prefix);


/* Create the directory specified and all of the child directories. Whee. */
gint rc_mkdir(const char *dir, guint mode);

gboolean rc_file_exists (const char *filename);

/* Check if the URL is relative or absolute */
gboolean rc_url_is_absolute (const char *url);

/* Build a URL given the info, checking if rest_url is absolute */
gchar *rc_build_url (const gchar *method, const gchar *host,
                     const gchar *path, const gchar *rest_url);



/* uncompress memory */
gint rc_uncompress_memory (guint8 *input_buffer, guint32 input_length,
                           GByteArray **out_ba);


typedef struct _RCLineBuf RCLineBuf;
struct _RCLineBuf {
    FILE *fp;
    int fp_length;
    gchar *save_buf;
    size_t save_buf_len;
    gchar *save_buf_base;
    size_t save_buf_base_len;

    int eof;
};

RCLineBuf *rc_line_buf_new (FILE *fp);
gchar *rc_line_buf_read (RCLineBuf *lb);
void rc_line_buf_destroy (RCLineBuf *lb);

#endif
