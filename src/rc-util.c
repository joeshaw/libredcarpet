/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-util.c
 * Copyright (C) 2000-2003 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#include <config.h>

#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <zlib.h>
#include <string.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "rc-util.h"
#include "rc-debug.h"

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

char *
rc_mkdtemp (char *template)
{
#ifdef HAVE_MKDTEMP
    return mkdtemp (template);
#else
    unsigned int len;
    char *replace;
    struct timeval tv;

    len = strlen (template);

    if (len < 6) {
        errno = EINVAL;
        return NULL;
    }

    replace = template + (len - 6);

    if (strcmp (replace, "XXXXXX")) {
        errno = EINVAL;
        return NULL;
    }

    gettimeofday (&tv, NULL);
    srand ((unsigned int)tv.tv_usec);

    do {
        int i;
        for (i = 0; i < 6; i++) {
            int value = rand () % 62;
            if (value < 10)
                replace[i] = value + 48;
            else if (value < 36)
                replace[i] = value + 55;
            else
                replace[i] = value + 61;
        }
        errno = 0;
    } while (mkdir (template, 0700) && errno == EEXIST);

    if (errno)
        return NULL;

    return template;
#endif
}

gint
rc_rmdir (const char *dir)
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

            if (lstat (filename, &buf)) {
                ret = -1;
            } else if (S_ISDIR (buf.st_mode)) {
                rc_rmdir (filename);
            } else if (S_ISREG (buf.st_mode)) {
                if (unlink (filename)) {
                    ret = -1;
                }
            }
#ifdef S_ISSOCK
            else if (S_ISSOCK (buf.st_mode)) {
                if (unlink (filename)) {
                    ret = -1;
                }
            }
#endif
#ifdef S_ISLNK
            else if (S_ISLNK (buf.st_mode)) {
                if (unlink (filename)) {
                    ret = -1;
                }
            }
#endif
		

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

gint
rc_cp (const char *fromfile, const char *tofile)
{
    int fdin = -1, fdout = -1;
    char *src = NULL, *dest = NULL;
    int ret = -1;
    int mode;
    struct stat st;

    fdin = open (fromfile, O_RDONLY);
    
    if (fdin < 0)
        goto out;

    if (fstat (fdin, &st) < 0)
        goto out;

    mode = st.st_mode & (S_IRWXU | S_IRWXG | S_IRWXO);
    fdout = open (tofile, O_RDWR | O_CREAT | O_TRUNC, mode);

    if (fdout < 0)
        goto out;

    /* Zero-byte file */
    if (!st.st_size) {
        ret = 0;
        goto out;
    }

    if (lseek (fdout, st.st_size - 1, SEEK_SET) < 0)
        goto out;

    if (rc_write (fdout, "\0", 1) < 0)
        goto out;

    src = mmap (NULL, st.st_size, PROT_READ, MAP_SHARED, fdin, 0);

    if (src == MAP_FAILED)
        goto out;

    dest = mmap (NULL, st.st_size, PROT_READ | PROT_WRITE,
                 MAP_SHARED, fdout, 0);

    if (dest == MAP_FAILED)
        goto out;

    memcpy (dest, src, st.st_size);

    ret = 0;

out:
    if (fdin >= 0)
        rc_close (fdin);

    if (fdout >= 0)
        rc_close (fdout);

    if (src != NULL)
        munmap (src, st.st_size);

    if (dest != NULL)
        munmap (dest, st.st_size);

    return ret;
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

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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
count_gzip_header (const guint8 *buf, guint32 input_length)
{
    int method, flags;
    const guint8 *s = buf;
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
rc_uncompress_memory (const guint8 *input_buffer, guint32 input_length,
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
    g_return_val_if_fail (out_ba, -3);

    ba = g_byte_array_new ();

    gzip_hdr = count_gzip_header (input_buffer, input_length);
    if (gzip_hdr < 0)
        return -1;

    zs.next_in = (guint8 *) input_buffer + gzip_hdr;
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

gint
rc_compress_memory (const guint8 *input_buffer, guint input_length,
                    GByteArray **out_ba)
{
    z_stream zs;
    gchar *outbuf = NULL;
    GByteArray *ba = NULL;
    guint32 data_len = 0;
    int zret;

    g_return_val_if_fail (input_buffer, -1);
    g_return_val_if_fail (input_length, -2);
    g_return_val_if_fail (out_ba, -3);

    ba = g_byte_array_new ();

    zs.next_in = (guint8 *) input_buffer;
    zs.avail_in = input_length;
    zs.zalloc = NULL;
    zs.zfree = NULL;
    zs.opaque = NULL;

    outbuf = g_malloc (10000);
    zs.next_out = outbuf;
    zs.avail_out = 10000;

    deflateInit (&zs, Z_DEFAULT_COMPRESSION);

    while (1) {
        if (zs.avail_in)
            zret = deflate (&zs, Z_SYNC_FLUSH);
        else
            zret = deflate (&zs, Z_FINISH);
            
        if (zret != Z_OK && zret != Z_STREAM_END)
            break;

        g_byte_array_append (ba, outbuf, 10000 - zs.avail_out);
        data_len += 10000 - zs.avail_out;
        zs.next_out = outbuf;
        zs.avail_out = 10000;

        if (zret == Z_STREAM_END)
            break;
    }

    deflateEnd (&zs);
    g_free (outbuf);

    if (zret != Z_STREAM_END) {
        g_warning ("libz deflate failed! (%d)", zret);
        g_byte_array_free (ba, TRUE);
        ba = NULL;
    } else {
        zret = 0;
    }

    *out_ba = ba;
    return zret;
} /* rc_compress_memory */

gboolean
rc_memory_looks_gzipped (const guint8 *buffer)
{
    g_return_val_if_fail (buffer != NULL, FALSE);

    /* This is from RFC 1952 */

    return buffer[0] == gz_magic[0]  /* ID1 */
        && buffer[1] == gz_magic[1]; /* ID2 */
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

/* 
 * This just allows reading from the buffer for now.  It could be extended to
 * do writing if necessary.
 */
RCBuffer *
rc_buffer_map_file(const char *filename)
{
    struct stat s;
    int fd;
    gpointer data;
    RCBuffer *buf = NULL;

    g_return_val_if_fail(filename, NULL);

    if (stat(filename, &s) < 0)
        return NULL;

    fd = open(filename, O_RDONLY);

    if (fd < 0)
        return NULL;

    data = mmap(NULL, s.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    close (fd);

    if (data == MAP_FAILED)
        return NULL;

    /* Transparently uncompress */
    if (s.st_size > 3 && rc_memory_looks_gzipped (data)) {
        GByteArray *byte_array = NULL;

        if (rc_uncompress_memory (data, s.st_size, &byte_array)) {
            rc_debug (RC_DEBUG_LEVEL_WARNING,
                      "Uncompression of '%s' failed", filename);
        } else {
            buf = g_new (RCBuffer, 1);
            buf->data       = byte_array->data;
            buf->size       = byte_array->len;
            buf->is_mmapped = FALSE;
        }

        munmap (data, s.st_size);

        if (byte_array)
            g_byte_array_free (byte_array, FALSE);

    } else {
        buf = g_new(RCBuffer, 1);
        buf->data       = data;
        buf->size       = s.st_size;
        buf->is_mmapped = TRUE;
    }

    return buf;
} /* rc_buffer_map_file */

void
rc_buffer_unmap_file(RCBuffer *buf)
{
    g_return_if_fail (buf);

    if (buf->is_mmapped)
        munmap (buf->data, buf->size);
    else
        g_free (buf->data);

    g_free (buf);
} /* rc_buffer_unmap_file */

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

xmlDoc *
rc_parse_xml_from_buffer (const guint8 *input_buffer,
                          guint32       input_length)
{
    xmlDoc *doc = NULL;

    g_return_val_if_fail (input_buffer != NULL, NULL);

    if (input_length > 3 && rc_memory_looks_gzipped (input_buffer)) { 
        GByteArray *buf;       
        if (rc_uncompress_memory (input_buffer, input_length, &buf))
            return NULL;
        doc = xmlParseMemory (buf->data, buf->len - 1);
        g_byte_array_free (buf, TRUE);
    } else {
        doc = xmlParseMemory (input_buffer, input_length);
    }

    return doc;
}

xmlDoc *
rc_parse_xml_from_file (const char *filename)
{
    RCBuffer *buf;
    xmlDoc *doc = NULL;

    g_return_val_if_fail (filename && *filename, NULL);

    buf = rc_buffer_map_file (filename);
    if (buf) {
        doc = xmlParseMemory (buf->data, buf->size);
        rc_buffer_unmap_file (buf);
    }

    return doc;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */
    
guint
rc_str_case_hash (gconstpointer key)
{
    const char *p = key;
    guint h = g_ascii_toupper (*p);

    if (h) {
        for (p += 1; *p != '\0'; p++)
            h = (h << 5) - h + g_ascii_toupper (*p);
    }

    return h;
}

gboolean
rc_str_case_equal (gconstpointer v1, gconstpointer v2)
{
    const char *string1 = v1;
    const char *string2 = v2;

    return g_ascii_strcasecmp (string1, string2) == 0;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
hash_values_to_list_cb (gpointer key, gpointer value, gpointer user_data)
{
    GSList **list = user_data;
 
    *list = g_slist_prepend (*list, value);
}
 
GSList *
rc_hash_values_to_list (GHashTable *hash_table)
{
    GSList *list = NULL;
 
    g_hash_table_foreach (hash_table, hash_values_to_list_cb, &list);
 
    return list;
}
 
static void
hash_keys_to_list_cb (gpointer key, gpointer value, gpointer user_data)
{
    GSList **list = user_data;
 
    *list = g_slist_prepend (*list, key);
}
 
GSList *
rc_hash_keys_to_list (GHashTable *hash_table)
{
    GSList *list = NULL;
 
    g_hash_table_foreach (hash_table, hash_keys_to_list_cb, &list);
 
    return list;
}
