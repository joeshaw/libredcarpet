/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-util.c
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

#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "rc-util.h"

gint
rc_mkdir(const char *dir, guint mode)
{
    char **subdirs;
    char *cd;
    char *cd_tmp;
    int current;
    int i;

    g_return_val_if_fail(dir, -1);
    g_return_val_if_fail(dir[0] == '/', -1);

    subdirs = g_strsplit(dir, "/", 0);

    for (current = 1; subdirs[current]; current++) {
	cd = g_strdup("/");
	for (i = 1; i <= current; i++) {
	    cd_tmp = g_strconcat(cd, "/", subdirs[i], NULL);
	    g_free(cd);
	    cd = cd_tmp;
	}
	
	if (!rc_file_exists(cd)) {
	    if (mkdir(cd, mode)) {
		g_free(cd);
		g_strfreev(subdirs);
		return -1;
	    }
	}

	g_free(cd);
    }

    g_strfreev(subdirs);
    return 0;
} /* rc_mkdir */

gint
rc_rmdir (gchar *dir)
{
    DIR *dp;
    struct dirent *entry;
    gint ret = 0;

    if (!(dp = opendir (dir))) {
        return (-1);
    }

    while ((entry = readdir (dp))) {
        if (strcmp (entry->d_name, ".") && strcmp (entry->d_name, "..")) {
            struct stat buf;

            gchar *filename = g_strconcat (dir, "/", entry->d_name, NULL);

            if (stat (filename, &buf)) {
                ret = -1;
            } else if (S_ISDIR (buf.st_mode)) {
                rc_rmdir (filename);
            } else if (S_ISREG (buf.st_mode)) {
                if (unlink (filename)) {
                    ret = -1;
                }
            }

            g_free (filename);
        }
    }

    closedir (dp);

    if (rmdir (dir)) {
        ret = -1;
    }

    return (ret);
} /* rc_rmdir */

gboolean 
rc_file_exists (const char *filename) 
{ 
    g_return_val_if_fail (filename != NULL, FALSE); 
 
    return (access (filename, F_OK) == 0); 
} 

gchar *
rc_is_program_in_path (const gchar *program)
{
    static gchar **paths = NULL;
    gchar **path_iter;
    gchar *file;
        
    if (!paths)
        paths = g_strsplit (g_getenv ("PATH"), ":", -1);

    for (path_iter = paths; *path_iter; path_iter++) {
        file = g_strconcat (*path_iter, "/", program, NULL);

        if (rc_file_exists (file)) {
            return (file);
        }

        g_free (file);
    }

    return (NULL);
}

gboolean
rc_url_is_absolute (const char *url)
{
    if (g_strncasecmp (url, "http:", 5) == 0 ||
        g_strncasecmp (url, "https:", 6) == 0 ||
        g_strncasecmp (url, "ftp:", 4) == 0 ||
        g_strncasecmp (url, "file:", 5) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

gchar *
rc_maybe_merge_paths(const char *parent_path, const char *child_path)
{
    /* Child path is NULL, so we return a dup of the parent path.
       Ex: rc_maybe_merge_paths("/foo", NULL) => "/foo" */
    if (!child_path)
        return g_strdup(parent_path);

    /* Child path is a fully qualified URL, so we return a dup of it.
       Ex: rc_maybe_merge_paths("/foo", "http://www.ximian.com") =>
       "http://www.ximian.com"

       OR

       Child path is an absolute path, so we just return a dup of it.
       Ex: rc_maybe_merge_paths("/foo", "/bar/baz") => "/bar/baz" */
    if (rc_url_is_absolute(child_path) || child_path[0] == '/')
        return g_strdup(child_path);

    /* Child path is a relative path, so we tack child path onto the end of
       parent path.
       Ex: rc_maybe_merge_paths("/foo", "bar/baz") => "/foo/bar/baz" */
    if (parent_path[strlen(parent_path) - 1] == '/')
        return g_strconcat(parent_path, child_path, NULL);
    else
        return g_strconcat(parent_path, "/", child_path, NULL);
} /* rc_maybe_merge_paths */

static void
hash_copy_helper (gpointer a, gpointer b, gpointer data)
{
    GHashTable *ht = (GHashTable *) data;

    g_hash_table_insert (ht, a, b);
}

GHashTable *
rc_hash_table_copy (GHashTable *ht, GHashFunc hfunc, GCompareFunc cfunc)
{
    GHashTable *nhash;

    nhash = g_hash_table_new (hfunc, cfunc);
    g_hash_table_foreach (ht, hash_copy_helper, nhash);

    return nhash;
}

void
rc_hash_table_slist_insert (GHashTable *ht, gpointer key, gpointer value)
{
    GSList *sl = NULL;
    gboolean found = FALSE;

    sl = g_hash_table_lookup (ht, key);
    if (sl) found = TRUE;
    sl = g_slist_append (sl, value);

    if (!found) {
        g_hash_table_insert (ht, key, sl);
    }
}

void
rc_hash_table_slist_insert_unique (GHashTable *ht, gpointer key, gpointer value,
                                   GCompareFunc eqfunc)
{
    GSList *sl = NULL;

    sl = g_hash_table_lookup (ht, key);
    if (sl) {
        if ((eqfunc && !g_slist_find_custom (sl, value, eqfunc)) ||
            (!eqfunc && !g_slist_find (sl, value)))
        {
            sl = g_slist_append (sl, value);
        }
    } else {
        sl = g_slist_append (sl, value);
        g_hash_table_insert (ht, key, sl);
    }
}

GSList *
rc_slist_unique (const GSList *sorted_list)
{
    GSList *out = NULL;
    const GSList *iter = sorted_list;
    gpointer last_thing = NULL;
    while (iter) {
        if (last_thing != iter->data) {
            out = g_slist_prepend (out, iter->data);
            last_thing = iter->data;
        }
        iter = iter->next;
    }

    return out;
}

/* Oh, how I wish I was using a real language and didn't have to do this tripe */
static void
hash_table_slist_free_helper (gpointer key, gpointer value, gpointer blah)
{
    g_slist_free ((GSList *) value);
}

void
rc_hash_table_slist_free (GHashTable *ht)
{
    g_hash_table_foreach (ht, hash_table_slist_free_helper, NULL);
}

/*
 * Magic gunzipping goodness
 */

/*
 * Count number of bytes to skip at start of buf
 */
static int gz_magic[2] = {0x1f, 0x8b};
/* gzip flag byte */
#define GZ_ASCII_FLAG   0x01 /* bit 0 set: file probably ascii text */
#define GZ_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define GZ_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define GZ_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define GZ_COMMENT      0x10 /* bit 4 set: file comment present */
#define GZ_RESERVED     0xE0 /* bits 5..7: reserved */

static int
count_gzip_header (guint8 *buf, guint32 input_length)
{
    int method, flags;
    guint8 *s = buf;
    guint32 left_len = input_length;

    if (left_len < 4) return -1;
    if (*s++ != gz_magic[0] || *s++ != gz_magic[1]) {
        return -2;
    }

    method = *s++;
    flags = *s++;
    left_len -= 4;

    if (method != Z_DEFLATED || (flags & GZ_RESERVED) != 0) {
        /* If it's not deflated, or the reserved isn't 0 */
        return -3;
    }

    /* Skip time, xflags, OS code */
    if (left_len < 6) return -4;
    s += 6;
    left_len -= 6;

    if (flags & GZ_EXTRA_FIELD) {
        unsigned int len;
        if (left_len < 2) return -5;
        len = (unsigned int)(*s++);
        len += ((unsigned int)(*s++)) << 8;
        if (left_len < len) return -6;
        s += len;
        left_len -= len;
    }

    /* Skip filename */
    if (flags & GZ_ORIG_NAME) {
        while (--left_len != 0 && *s++ != '\0') ;
        if (left_len == 0) return -7;
    }
    /* Skip comment */
    if (flags & GZ_COMMENT) {
        while (--left_len != 0 && *s++ != '\0') ;
        if (left_len == 0) return -7;
    }
    /* Skip CRC */
    if (flags & GZ_HEAD_CRC) {
        if (left_len < 2) return -7;
        s += 2;
        left_len -= 2;
    }

    return input_length - left_len;
}

gint
rc_uncompress_memory (guint8 *input_buffer, guint32 input_length,
                      GByteArray **out_ba)
{
    z_stream zs;
    gchar *outbuf = NULL;
    GByteArray *ba = NULL;
    guint32 data_len = 0;
    int zret;

    int gzip_hdr;

    g_return_val_if_fail (input_buffer, -1);
    g_return_val_if_fail (input_length, -2);

    ba = g_byte_array_new ();

    gzip_hdr = count_gzip_header (input_buffer, input_length);
    if (gzip_hdr < 0)
        return -1;

    zs.next_in = input_buffer + gzip_hdr;
    zs.avail_in = input_length - gzip_hdr;
    zs.zalloc = NULL;
    zs.zfree = NULL;
    zs.opaque = NULL;

    outbuf = g_malloc (10000);
    zs.next_out = outbuf;
    zs.avail_out = 10000;

    /* Negative inflateinit is magic to tell zlib that there is no
     * zlib header */
    inflateInit2 (&zs, -MAX_WBITS);

    while (1) {
        zret = inflate (&zs, Z_SYNC_FLUSH);
        if (zret != Z_OK && zret != Z_STREAM_END)
            break;

        g_byte_array_append (ba, outbuf, 10000 - zs.avail_out);
        data_len += 10000 - zs.avail_out;
        zs.next_out = outbuf;
        zs.avail_out = 10000;

        if (zret == Z_STREAM_END)
            break;
    }

    inflateEnd (&zs);
    g_free (outbuf);

    if (zret != Z_STREAM_END) {
        g_warning ("libz inflate failed! (%d)", zret);
        g_byte_array_free (ba, TRUE);
        ba = NULL;
    } else {
        g_byte_array_append (ba, "", 1);
        zret = 0;
    }

    *out_ba = ba;
    return zret;
}

gboolean
rc_write (int fd, const void *buf, size_t count)
{
    size_t bytes_remaining = count;
    const void *ptr = buf;

    while (bytes_remaining) {
        size_t bytes_written;

        bytes_written = write (fd, ptr, bytes_remaining);

        if (bytes_written == -1) {
            if (errno == EAGAIN || errno == EINTR) {
                continue;
            } else {
                break;
            }
        }

        bytes_remaining -= bytes_written;
        ptr += bytes_written;
    }

    if (bytes_remaining) {
        return (FALSE);
    }

    return (TRUE);
}

gboolean
rc_close (int fd)
{
    while (close (fd) == -1) {
        if (errno != EAGAIN && errno != EINTR) {
            return (FALSE);
        }
    }

    return (TRUE);
}

guint32
rc_string_to_guint32_with_default(const char *n, guint32 def)
{
    char *ret;
    guint32 z;

    z = strtoul(n, &ret, 10);
    if (*ret != '\0')
        return def;
    else
        return z;
} /* rc_string_to_guint32_with_default */
