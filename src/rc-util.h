/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-util.h
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#ifndef _RC_UTIL_H
#define _RC_UTIL_H

#include <glib.h>
#include <time.h>
#include <stdio.h>

/* Function to safely delete a directory recursively (only deletes regular
   files), should be safe from symlink attacks.

   Returns 0 on success, -1 on error
*/
gint rc_rmdir (gchar *dir);

/* Create the directory specified and all of the child directories. Whee. */
gint rc_mkdir(const char *dir, guint mode);

gboolean rc_file_exists (const char *filename);

gchar *rc_is_program_in_path (const gchar *program);

/* Check if the URL is relative or absolute */
gboolean rc_url_is_absolute (const char *url);

/* Build a URL given the info, checking if rest_url is absolute */
gchar *rc_build_url (const gchar *method, const gchar *host,
                     const gchar *path, const gchar *rest_url);

/* ... */
GHashTable *
rc_hash_table_copy (GHashTable *ht, GHashFunc hfunc, GCompareFunc cfunc);

/* assume key hashes to slist, append value to end of slist */
void rc_hash_table_slist_insert (GHashTable *ht, gpointer key, gpointer value);
/* assume key hashes to slist, append value to end of slist if it isn't present */
void rc_hash_table_slist_insert_unique (GHashTable *ht, gpointer key, gpointer value,
                                        GCompareFunc eqfunc);
void rc_hash_table_slist_free (GHashTable *ht);

/* uncompress memory */
gint rc_uncompress_memory (guint8 *input_buffer, guint32 input_length,
                           GByteArray **out_ba);

/* Safely write a buffer to a fd (handle all those pesky EINTR issues) */
gboolean rc_write (int fd, const void *buf, size_t count);

/* Close a file descriptor (didn't know it was this hard, did you?) */
gboolean rc_close (int fd);

#endif
