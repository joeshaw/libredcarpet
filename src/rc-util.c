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


gchar *
rc_mktmpdir (const gchar *prefix) {
    gchar *path;
    gchar *tmpdir;

    tmpdir = getenv ("REDCARPET_TMPDIR");
    if (!tmpdir) {
        tmpdir = getenv ("TMPDIR");
    }
    if (tmpdir) {
        tmpdir = g_strdup (tmpdir);
    } else {
        tmpdir = g_strdup ("/tmp");
    }

    if (prefix) {
        path = g_strconcat (tmpdir, "/", prefix, "-XXXXXX", NULL);
    } else {
        path = g_strconcat (tmpdir, "/redcarpet-XXXXXX", NULL);
    }
    g_free (tmpdir);

    path = mktemp (path);

    if (mkdir (path, 0700)) {
        g_free (path);
        return (NULL);
    }

    return (path);
}

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

RCLineBuf *
rc_line_buf_new (FILE *fp)
{
    RCLineBuf *lb = g_new0 (RCLineBuf, 1);
    long cpos, clen;

    cpos = ftell (fp);
    fseek (fp, -1, SEEK_END);
    clen = ftell (fp);
    fseek (fp, cpos, SEEK_SET);

    lb->fp = fp;
    lb->fp_length = clen - cpos;

    lb->save_buf_base = g_malloc (100);
    lb->save_buf_base_len = 100;
    lb->save_buf = lb->save_buf_base;
    lb->save_buf_len = 0;

    lb->eof = 0;

    return lb;
}

gchar *
rc_line_buf_read (RCLineBuf *lb)
{
    int i;
    RCString *rcs;
    gchar *out;
    size_t really_read;

    if (lb->eof && lb->save_buf_len == 0) {
        return NULL;
    }

    /* First check if there's a \n in the buffer */
    rcs = rc_string_new (100);

    /* Ok, at this point there is a chunk of string in the buffer,
     * but no complete line
     */

    /* Copy the buffer to the out */
    do {
        /* Is there a \n in the stream? */
        for (i = 0; i <= lb->save_buf_len; i++) {
            if (lb->save_buf[i] == '\n') {
                int slen = i;
                rc_string_append_slen (rcs, lb->save_buf, slen);
                lb->save_buf += slen + 1;
                lb->save_buf_len -= slen + 1;
                out = rc_string_get_chars (rcs);
                rc_string_free (rcs);
                
                return out;
            }
        }

        if (lb->eof) {
            /* We've already hit an eof, and didn't find a \n, so the buffer
             * must not end with one. We return what we have.
             */
            
            rc_string_append_slen (rcs, lb->save_buf, lb->save_buf_len);
            lb->save_buf_len = 0;
            out = rc_string_get_chars (rcs);
            rc_string_free (rcs);

            return out;
        }

        rc_string_append_slen (rcs, lb->save_buf, lb->save_buf_len);
        lb->save_buf = lb->save_buf_base;
        lb->save_buf_len = 0;

        /* Read another chunk */
        really_read = fread (lb->save_buf_base, 1, lb->save_buf_base_len, lb->fp);
        if (really_read < lb->save_buf_base_len && feof (lb->fp)) {
            lb->eof = 1;
        }
        lb->save_buf_len = really_read;

    } while (1);
}

void
rc_line_buf_destroy (RCLineBuf *lb)
{
    g_free (lb->save_buf_base);
    g_free (lb);
}
