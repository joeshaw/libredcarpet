/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
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

#include <unistd.h>
#include <dirent.h>
#include <stdlib.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "rc-util.h"
#include "rc-string.h"
#include "zlib.h"

//#include <libgnome/libgnome.h>

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

gboolean
rc_url_is_absolute (const char *url)
{
    if (g_strncasecmp (url, "http:", 5) == 0 ||
        g_strncasecmp (url, "ftp:", 4) == 0 ||
        g_strncasecmp (url, "file:", 5) == 0)
    {
        return TRUE;
    }

    return FALSE;
}

gchar *
rc_build_url (const gchar *method, const gchar *host, const gchar *path, const gchar *rest_url)
{
    if (rest_url && rc_url_is_absolute (rest_url)) {
        return g_strdup (rest_url);
    }

    if (rest_url && *rest_url == '/') {
        if (host) {
            return g_strdup_printf ("%s://%s%s", method ? method : "http", host, rest_url);
        } else {
            return g_strdup_printf ("%s:%s", method ? method : "http", rest_url);
        }
    }

    if (path && rc_url_is_absolute (path) && rest_url) {
        return g_strdup_printf ("%s/%s", path, rest_url);
    }

    if (host && path && rest_url) {
        return g_strdup_printf ("%s://%s%s%s/%s", method ? method : "http", host,
                                *path == '/' ? "" : "/", path, rest_url);
    } else if (host && path) {
        return g_strdup_printf ("%s://%s%s%s/", method ? method : "http", host,
                                *path == '/' ? "" : "/", path);
    } else if (path && rest_url) {
        return g_strdup_printf ("%s:%s%s/%s", method ? method : "http",
                                *path == '/' ? "" : "/", path, rest_url);
    } else if (host && rest_url) {
        return g_strdup_printf ("%s://%s/%s", method ? method : "http",
                                host, rest_url);
    }

    g_assert_not_reached ();
    return NULL;
}

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

    if (zret != Z_STREAM_END) {
        g_warning ("libz inflate failed! (%d)", zret);
    }

    inflateEnd (&zs);

    g_free (outbuf);

    g_byte_array_append (ba, "", 1);
    *out_ba = ba;
    return 0;
}
