/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debman.c
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

#include <glib.h>

#include "rc-debug.h"
#include "rc-packman.h"
#include "rc-packman-private.h"
#include "rc-debman.h"
#include "rc-debman-private.h"
#include "rc-util.h"
#include "rc-line-buf.h"
#include "rc-verification.h"
#include "rc-verification-private.h"
#include "rc-debman-general.h"

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/poll.h>
#include <sys/stat.h>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <limits.h>
#ifndef fake_openpty
#include <pty.h>
#endif

#if FIX_THE_HACK
#include <gdk/gdkkeysyms.h>
#endif

#ifdef fake_openpty
#define openpty(a,b,c,d,e) /**/
#endif

static void rc_debman_class_init (RCDebmanClass *klass);
static void rc_debman_init       (RCDebman *obj);

static GObjectClass *parent_class;

#define DEBMAN_DEFAULT_STATUS_FILE "/var/lib/dpkg/status"

/* The Evil Hack */
gboolean child_wants_input = FALSE;

extern gchar *rc_libdir;

GType
rc_debman_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCDebmanClass),
            NULL, NULL,
            (GClassInitFunc) rc_debman_class_init,
            NULL, NULL,
            sizeof (RCDebman),
            0,
            (GInstanceInitFunc) rc_debman_init,
        };

        type = g_type_register_static (rc_packman_get_type (), 
                                       "RCDebman",
                                       &type_info,
                                       0);
    }

    return type;
} /* rc_debman_get_type */

/*
  Lame debugging thingy
*/

static void
dump_argv (int level, gchar **argv)
{
    gchar **iter;

    for (iter = argv; *iter; iter++) {
        rc_debug (level, " %s", *iter);
    }

    rc_debug (level, "\n");
} /* dump_argv */

static void
check_database (RCDebman *debman)
{
    struct stat buf;

    stat ("/var/lib/dpkg/status", &buf);

    if (buf.st_mtime != debman->priv->db_mtime) {
        g_signal_emit_by_name (RC_PACKMAN (debman), "database_changed");
        debman->priv->db_mtime = buf.st_mtime;
    }
}

static gboolean
database_check_func (RCDebman *debman)
{
    check_database (debman);

    return TRUE;
}

/*
  Go Wichert, go Wichert
*/

static void
i18n_fixer ()
{
    char *path;
    path = g_strdup_printf("PATH=%s:/sbin:/usr/sbin:/usr/local/sbin",
                           getenv("PATH"));

    putenv (path);
    putenv ("LANG");
    putenv ("LC_ALL");
}

/*
  These two functions are used to cleanly destroy the hash table in an
  RCDebman, either after an rc_packman_transact, when we need to
  invalidate the hash table, or at object destroy
*/

static void
hash_destroy_pair (gchar *key, RCPackage *package)
{
    rc_package_unref (package);
}

static void
hash_destroy (RCDebman *debman)
{
    g_hash_table_freeze (debman->priv->package_hash);
    g_hash_table_foreach (debman->priv->package_hash,
                          (GHFunc) hash_destroy_pair, NULL);
    g_hash_table_destroy (debman->priv->package_hash);

    debman->priv->package_hash = g_hash_table_new (g_str_hash, g_str_equal);
    debman->priv->hash_valid = FALSE;
}

/*
  Lock the dpkg database.  Too bad this just doesn't work (and doesn't
  work in apt or dselect either).  Releasing the lock to call dpkg and
  trying to grab it afterwords is probably the most broken thing I've
  ever heard of.
*/

static gboolean
lock_database (RCDebman *debman)
{
    int fd;
    struct flock fl;
    RCPackman *packman = RC_PACKMAN (debman);

    /* Developer hack */
    if (getenv ("RC_ME_EVEN_HARDER") || getenv ("RC_DEBMAN_STATUS_FILE")) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  __FUNCTION__ ": not checking lock file\n");

        return (TRUE);
    }

    /* Check for recursive lock */
    if (debman->priv->lock_fd != -1) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  __FUNCTION__ ": lock_fd is already %d, recursive lock?\n",
                  debman->priv->lock_fd);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "already holding lock");

        return (FALSE);
    }

    fd = open ("/var/lib/dpkg/lock", O_RDWR | O_CREAT | O_TRUNC, 0640);

    if (fd == -1) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  __FUNCTION__ ": couldn't open lock file\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't open lock file");

        return (FALSE);
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl (fd, F_SETLK, &fl) == -1) {
        if (errno != ENOLCK) {
            /* Note: in ignoring ENOLCK, I'm just doing what the
             * various Debian utilities do; I'm not convinced this is
             * very intelligent at all.  But when in Moronville, do as
             * the Morons do.  Or something. */

            /* Not bothering to even check this */
            rc_close (fd);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "couldn't acquire lock");

            rc_debug (RC_DEBUG_LEVEL_ERROR,
                      __FUNCTION__ ": failed to acquire lock file\n");

            return (FALSE);
        }
    }

    debman->priv->lock_fd = fd;

    rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": acquired lock file\n");

    g_source_remove (debman->priv->db_watcher_cb);

    return (TRUE);
}

static void
unlock_database (RCDebman *debman)
{
    if (getenv ("RC_ME_EVEN_HARDER") || getenv ("RC_DEBMAN_STATUS_FILE")) {
        return;
    }

    debman->priv->db_watcher_cb =
        g_timeout_add (5000, (GSourceFunc) database_check_func,
                       (gpointer) debman);

    if (!rc_close (debman->priv->lock_fd)) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  __FUNCTION__ ": close of lock_fd failed\n");
    }

    debman->priv->lock_fd = -1;

    rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": released lock file\n");
}

typedef struct _DebmanVerifyStatusInfo DebmanVerifyStatusInfo;

struct _DebmanVerifyStatusInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    int out_fd;

    gboolean error;
    gchar *error_line;
    guint error_line_number;
};

static char **
split_status (char *line)
{
    char **ret = g_new (char *, 4);
    char *head, *tail;
    int i;

    ret[0] = NULL;
    ret[1] = NULL;
    ret[2] = NULL;
    ret[3] = NULL;

    head = line;

    for (i = 0; i < 3; i++) {
        while (*head && isspace (*head)) {
            head++;
        }

        tail = head;

        while (*tail && !isspace (*tail)) {
            tail++;
        }

        ret[i] = g_strndup (head, tail - head);

        head = tail;
    }

    if (*head) {
        g_strfreev (ret);
        ret = NULL;
    }

    return ret;
}

static void
verify_status_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    char **status = NULL;
    DebmanVerifyStatusInfo *verify_status_info =
        (DebmanVerifyStatusInfo *)data;
    int out_fd = verify_status_info->out_fd; /* Just for sanity */
    char *ptr;

    if (g_strncasecmp (line, "status:", strlen ("status:"))) {
        /* This isn't a status line, so we don't need to do anything other than
           to write it to the new file */
        if (!rc_write (out_fd, line, strlen (line)) ||
            !rc_write (out_fd, "\n", 1))
        {
            goto BROKEN;
        }
        return;
    }

    ptr = line + strlen ("status:");
    while (*ptr && isspace (*ptr)) {
        ptr++;
    }

    status = split_status (ptr);

    if (!(status && status[0] && status[1] && status[2])) {
        /* There aren't exactly 3 tokens here */
        goto BROKEN;
    }

    /* we want the status to be ok */
    if (strcmp (status[1], "ok")) {
        goto BROKEN;
    }

    /* installed, not-installed, and config-files are ok, everything else I
       consider problematic */
    if (strcmp (status[2], "installed") &&
        strcmp (status[2], "not-installed") &&
        strcmp (status[2], "config-files"))
    {
        goto BROKEN;
    }

    /* If the package is installed, set the selection to install, and use
       whatever middle token the user had */
    if (!strcmp (status[2], "installed")) {
        if (!strcmp (status[0], "install") || !strcmp (status[0], "hold")) {
            if (!rc_write (out_fd, line, strlen (line)) ||
                !rc_write (out_fd, "\n", 1))
            {
                goto BROKEN;
            }
            goto END;
        }
        if (!rc_write (out_fd, "Status: install ",
                       strlen ("Status: install ")) ||
            !rc_write (out_fd, status[1], strlen (status[1])) ||
            !rc_write (out_fd, " installed\n", strlen (" installed\n")))
        {
            goto BROKEN;
        }
        goto END;
    }

    /* If the package is not-installed, just write the normal line if the
       selection was purge or deinstall, otherwise set the seletion to purge.
       Of course, use whatever middle token the user had. */
    if (!strcmp (status[2], "not-installed")) {
        if (!strcmp (status[0], "purge")) {
            if (!rc_write (out_fd, line, strlen (line)) ||
                !rc_write (out_fd, "\n", 1))
            {
                goto BROKEN;
            }
            goto END;
        }
        if (!strcmp (status[0], "deinstall")) {
            if (!rc_write (out_fd, line, strlen (line)) ||
                !rc_write (out_fd, "\n", 1))
            {
                goto BROKEN;
            }
            goto END;
        }
        if (!rc_write (out_fd, "Status: purge ", strlen ("Status: purge ")) ||
            !rc_write (out_fd, status[1], strlen (status[1])) ||
            !rc_write (out_fd, " not-installed\n",
                       strlen (" not-installed\n")))
        {
            goto BROKEN;
        }
        goto END;
    }

    /* If the package is config-files only, set the selection to deinstall,
       middle token as the user had it. */
    if (!strcmp (status[2], "config-files")) {
        if (!rc_write (out_fd, "Status: deinstall ",
                       strlen ("Status: deinstall ")) ||
            !rc_write (out_fd, status[1], strlen (status[1])) ||
            !rc_write (out_fd, " config-files\n", strlen (" config-files\n")))
        {
            goto BROKEN;
        }
        goto END;
    }

  BROKEN:
    verify_status_info->error = TRUE;
    g_signal_handler_disconnect (line_buf,
                                 verify_status_info->read_line_id);
    g_signal_handler_disconnect (line_buf,
                                 verify_status_info->read_done_id);
    g_main_quit (verify_status_info->loop);

  END:
    g_strfreev (status);
}

static void
verify_status_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                            gpointer data)
{
    DebmanVerifyStatusInfo *verify_status_info =
        (DebmanVerifyStatusInfo *)data;

    g_signal_handler_disconnect (line_buf,
                                 verify_status_info->read_line_id);
    g_signal_handler_disconnect (line_buf,
                                 verify_status_info->read_done_id);

    g_main_quit (verify_status_info->loop);
}

static gboolean
verify_status (RCPackman *packman)
{
    DebmanVerifyStatusInfo verify_status_info;
    GMainLoop *loop;
    int in_fd, out_fd;
    RCLineBuf *line_buf;
    RCDebman *debman = RC_DEBMAN (packman);
    gboolean unlock_db = FALSE;

    if (debman->priv->lock_fd == -1) {
        if (!lock_database (debman))
            return FALSE;
        unlock_db = TRUE;
    }

    if ((in_fd = open (debman->priv->status_file, O_RDONLY)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't open %s for reading",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open %s for reading\n",
                  debman->priv->status_file);

        goto ERROR;
    }

    if ((out_fd = creat (debman->priv->rc_status_file, 0644)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't open %s for writing",
                              debman->priv->rc_status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open %s for writing\n",
                  debman->priv->rc_status_file);

        close (in_fd);

        goto ERROR;
    }

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, in_fd);

    verify_status_info.out_fd = out_fd;
    verify_status_info.error = FALSE;

    loop = g_main_new (FALSE);
    verify_status_info.loop = loop;

    verify_status_info.read_line_id =
        g_signal_connect (line_buf,
                          "read_line",
                          (GCallback) verify_status_read_line_cb,
                          &verify_status_info);
    
    verify_status_info.read_done_id =
        g_signal_connect (line_buf,
                          "read_done",
                          (GCallback) verify_status_read_done_cb,
                          &verify_status_info);

    g_main_run (loop);

    g_object_unref (line_buf);

    g_main_destroy (loop);

    close (in_fd);
    close (out_fd);

    if (verify_status_info.error) {
        unlink (debman->priv->rc_status_file);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "The %s file is malformed or contains errors",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": couldn't parse %s\n",
                  debman->priv->status_file);

        goto ERROR;
    }

    if (rename (debman->priv->rc_status_file, debman->priv->status_file)) {
        unlink (debman->priv->rc_status_file);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't rename %s to %s",
                              debman->priv->rc_status_file,
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": couldn't rename %s\n",
                  debman->priv->rc_status_file);

        goto ERROR;
    }

    return (TRUE);

  ERROR:
    if (unlock_db)
        unlock_database (debman);

    return FALSE;
}

/*
  The associated detritus and cruft related to mark_status.  Given a
  list of packages, we need to scan through /var/lib/dpkg/status, and
  change the "install ok installed" string to "purge ok installed", to
  let dpkg know that we're going to be removing this package soon.
*/

/*
  Since I only want to scan through /var/lib/dpkg/status once, I need
  a shorthand way to match potentially many strings.  Returns the
  package that matched, or NULL if none did.
*/

static RCPackage *
package_accept (gchar *line, RCPackageSList *packages)
{
    RCPackageSList *iter;
    gchar *name;

    if (strncmp (line, "Package:", strlen ("Package:")) != 0) {
        return NULL;
    }

    name = line + strlen ("Package:");
    while (isspace (*name)) {
        name++;
    }

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        if (!(strcmp (name, package->spec.name))) {
            rc_debug (RC_DEBUG_LEVEL_DEBUG,
                      __FUNCTION__ ": found package %s\n", package->spec.name);

            return (package);
        }
    }

    return (NULL);
}

typedef struct _DebmanMarkStatusInfo DebmanMarkStatusInfo;

struct _DebmanMarkStatusInfo {
    GMainLoop *loop;

    RCDebman *debman;

    guint read_line_id;
    guint read_done_id;

    int out_fd;

    RCPackageSList *install_packages;
    RCPackageSList *remove_packages;

    gboolean rewrite_status;
    gboolean rewrite_version;
    char *new_version;
    gboolean error;
};

static void
mark_status_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanMarkStatusInfo *mark_status_info = (DebmanMarkStatusInfo *)data;

    /* If rewrite_status is set (we encountered a package we're
     * removing) and this is a status line, write out our new status
     * line */
    if (mark_status_info->rewrite_status &&
        !strncasecmp (line, "Status:", strlen ("Status:")))
    {
        if (!rc_write (mark_status_info->out_fd, "Status: purge ok installed\n",
                       strlen ("Status: purge ok installed\n")))
        {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": failed to write new status line, aborting\n");

            rc_packman_set_error
                (RC_PACKMAN (mark_status_info->debman), RC_PACKMAN_ERROR_ABORT,
                 "write to %s failed",
                 mark_status_info->debman->priv->rc_status_file);

            goto BROKEN;
        }

        mark_status_info->rewrite_status = FALSE;

        return;
    }

#if 0
    /* If rewrite_version is set (we encountered a package we're
     * upgrading) and this is a version line, write out our new
     * version */
    if (mark_status_info->rewrite_version &&
        !strncasecmp (line, "Version:", strlen ("Version:")))
    {
        char *version_line;

        version_line = g_strdup_printf (
            "Version: %s\n", mark_status_info->new_version);
        g_free (mark_status_info->new_version);

        if (!rc_write (mark_status_info->out_fd, version_line,
                       strlen (version_line)))
        {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": failed to write new version line, aborting\n");

            rc_packman_set_error (
                RC_PACKMAN (mark_status_info->debman), RC_PACKMAN_ERROR_ABORT,
                "write to %s failed",
                mark_status_info->debman->priv->rc_status_file);

            goto BROKEN;
        }

        mark_status_info->rewrite_version = FALSE;

        return;
    }
#endif

    /* If the rewrite flag is still set, and we've gone through all
     * the fields and didn't encounter a status line (we got to a
     * blank line or another Package: line), something is really
     * wrong */
    if (mark_status_info->rewrite_status &&
        ((line && !line[0]) || (!strncasecmp (
                                    line, "Package:", strlen ("Package:")))))
    {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": package section had no Status/Version line, aborting\n");

        rc_packman_set_error (RC_PACKMAN (mark_status_info->debman),
                              RC_PACKMAN_ERROR_ABORT,
                              "%s appears to be malformed",
                              mark_status_info->debman->priv->status_file);

        goto BROKEN;
    }

    /* Nothing interesting is happening here, so just write out the
     * old data */
    if (!rc_write (mark_status_info->out_fd, line, strlen (line)) ||
        !rc_write (mark_status_info->out_fd, "\n", 1))
    {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to write old line, aborting\n");

        rc_packman_set_error (RC_PACKMAN (mark_status_info->debman),
                              RC_PACKMAN_ERROR_ABORT,
                              "write to %s failed",
                              mark_status_info->debman->priv->rc_status_file);

        goto BROKEN;
    }

    /* Check if this marks the beginning of a package whose status or
     * version we need to change */
    if (package_accept (line, mark_status_info->remove_packages)) {
        mark_status_info->rewrite_status = TRUE;
    }

#if 0
    if ((package = package_accept (line, mark_status_info->install_packages))) {
        char *version;

        version = rc_package_spec_version_to_str (
            (RCPackageSpec *)package);

        mark_status_info->rewrite_version = TRUE;
        mark_status_info->new_version = version;
    }
#endif

    return;

  BROKEN:
    mark_status_info->error = TRUE;

    g_signal_handler_disconnect (line_buf,
                                 mark_status_info->read_line_id);
    g_signal_handler_disconnect (line_buf,
                                 mark_status_info->read_done_id);

    g_main_quit (mark_status_info->loop);
}

static void
mark_status_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                         gpointer data)
{
    DebmanMarkStatusInfo *mark_status_info = (DebmanMarkStatusInfo *)data;

    g_signal_handler_disconnect (line_buf,
                                 mark_status_info->read_line_id);
    g_signal_handler_disconnect (line_buf,
                                 mark_status_info->read_done_id);

    g_main_quit (mark_status_info->loop);
}

static gboolean
mark_status (RCPackman *packman, RCPackageSList *install_packages,
            RCPackageSList *remove_packages)
{
    RCDebman *debman = RC_DEBMAN (packman);
    DebmanMarkStatusInfo mark_status_info;
    GMainLoop *loop;
    int in_fd, out_fd;
    RCLineBuf *line_buf;

//    g_return_val_if_fail (remove_packages, TRUE);

    if ((in_fd = open (debman->priv->status_file, O_RDONLY)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't open %s for reading",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open %s for reading\n",
                  debman->priv->status_file);

        goto FAILED;
    }

    if ((out_fd = creat (debman->priv->rc_status_file, 0644)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't open %s for writing",
                              debman->priv->rc_status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open %s for writing\n",
                  debman->priv->rc_status_file);

        close (in_fd);

        goto FAILED;
    }

    loop = g_main_new (FALSE);

    mark_status_info.rewrite_status = FALSE;
    mark_status_info.rewrite_version = FALSE;
    mark_status_info.error = FALSE;
    mark_status_info.out_fd = out_fd;
    mark_status_info.install_packages = install_packages;
    mark_status_info.remove_packages = remove_packages;
    mark_status_info.loop = loop;
    mark_status_info.debman = RC_DEBMAN (packman);

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, in_fd);

    mark_status_info.read_line_id =
        g_signal_connect (line_buf,
                          "read_line",
                          (GCallback) mark_status_read_line_cb,
                          &mark_status_info);
    mark_status_info.read_done_id =
        g_signal_connect (line_buf,
                          "read_done",
                          (GCallback) mark_status_read_done_cb,
                          &mark_status_info);

    g_main_run (loop);

    g_object_unref (line_buf);

    g_main_destroy (loop);

    close (in_fd);
    close (out_fd);

    if (mark_status_info.error) {
        unlink (debman->priv->rc_status_file);

        goto FAILED;
    }

    if (rename (debman->priv->rc_status_file, debman->priv->status_file)) {
        unlink (debman->priv->rc_status_file);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "rename of %s to %s failed",
                              debman->priv->rc_status_file,
                              debman->priv->status_file);

        goto FAILED;
    }

    return (TRUE);

  FAILED:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Unable to update status database");

    return (FALSE);
}

/*
  This little guy gets remembered through the transaction, so that I
  can emit signals with the appropriate values when necessary
*/

typedef struct _DebmanInstallState DebmanInstallState;

struct _DebmanInstallState {
    guint seqno;
    guint total;
};

/*
  Data structures and evil functions for the gross "handle the
  stupid-ass interactive Debian package" hack
*/

typedef struct _DebmanHackInfo DebmanHackInfo;

struct _DebmanHackInfo {
    guint poll_write_id;

    int master;

    GString *buf;

    RCLineBuf *line_buf;
};

static void
debman_sigusr2_cb (int signum)
{
    child_wants_input = TRUE;

    fflush (stdout);
}

#if FIX_THE_HACK
static gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    if (event->keyval == GDK_Return) {
        g_main_quit ((GMainLoop *)data);

        return (FALSE);
    }

    return (TRUE);
}

static gboolean
delete_event_cb (GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return (TRUE);
}

static gint
debman_poll_write_cb (gpointer data)
{
    GMainLoop *loop;
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *vbox;
    DebmanHackInfo *hack_info = (DebmanHackInfo *)data;
    gchar *line;
    struct pollfd check;
    gchar *buf;
    GtkStyle *style;
    GdkFont *font;

    if (!child_wants_input) {
        return (TRUE);
    }

    check.fd = hack_info->master;
    check.events = POLLIN;

    if (poll (&check, 1, 0)) {
        return (TRUE);
    }

    loop = g_main_new (FALSE);

    window = gtk_window_new (GTK_WINDOW_DIALOG);

    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
                        GTK_SIGNAL_FUNC (delete_event_cb), NULL);

    entry = gtk_entry_new ();

    line = rc_line_buf_get_buf (hack_info->line_buf);
    hack_info->buf = g_string_append (hack_info->buf, line);
    g_free (line);

    line = hack_info->buf->str;
    line = g_strstrip (line);
    line = g_strdelimit (line, "\t", ' '); /* Tabs is bad, mmkay? */

    label = gtk_label_new (line);

    style = gtk_style_copy (gtk_widget_get_style (label));
    font = gdk_font_load ("fixed");
    style->font = font;
    gtk_widget_set_style (label, style);

    hack_info->buf = g_string_truncate (hack_info->buf, 0);

    gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);

    vbox = gtk_vbox_new (FALSE, 5);
    gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), entry, FALSE, FALSE, 0);

    gtk_signal_connect (GTK_OBJECT (entry), "key-press-event",
                        GTK_SIGNAL_FUNC (key_press_cb), (gpointer) loop);

    gtk_container_border_width (GTK_CONTAINER (window), 5);

    gtk_container_add (GTK_CONTAINER (window), vbox);

    gtk_widget_show_all (window);
    gtk_widget_grab_focus (entry);

    g_main_run (loop);

    g_free (rc_line_buf_get_buf (hack_info->line_buf));

    line = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

    gtk_widget_destroy (window);

    rc_write (hack_info->master, line, strlen (line));
    rc_write (hack_info->master, "\n", 1);

    /* This is an evil hack to get the shit we just typed before the
     * RCLineBuf can queue it all up.  Is there a better way I'm
     * missing? */
    buf = g_new0 (gchar, strlen (line) + 1);
    read (hack_info->master, buf, strlen (line) + 1);
    g_free (buf);

    g_free (line);

    child_wants_input = FALSE;

    return (TRUE);
}
#endif

/*
  All of the crap to handle do_purge.  Basically just fork dpkg
  --purge --pending, and sniff the output for any interesting strings
*/

struct _DebmanDoPurgeInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    RCPackman *packman;

    DebmanInstallState *install_state;

    DebmanHackInfo hack_info;
};

typedef struct _DebmanDoPurgeInfo DebmanDoPurgeInfo;

static void
do_purge_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoPurgeInfo *do_purge_info = (DebmanDoPurgeInfo *)data;

    rc_debug (RC_DEBUG_LEVEL_DEBUG, __FUNCTION__ ": got \"%s\"\n", line);

    do_purge_info->hack_info.buf =
        g_string_append (do_purge_info->hack_info.buf, line);
    do_purge_info->hack_info.buf =
        g_string_append (do_purge_info->hack_info.buf, "\n");

    if (!strncmp (line, "Removing", strlen ("Removing"))) {
        guint length = strlen ("Removing ");
        gchar *ptr;
        gchar *name;

        ptr = strchr (line + length, ' ');
        name = g_strndup (line + length, ptr - (line + length));

        g_signal_emit_by_name (do_purge_info->packman,
                               "transact_step",
                               ++do_purge_info->install_state->seqno,
                               RC_PACKMAN_STEP_REMOVE, name);

        g_free (name);

        do_purge_info->hack_info.buf =
            g_string_truncate (do_purge_info->hack_info.buf, 0);
    }

    if (!strncmp (line, "Purging", strlen ("Purging"))) {
        do_purge_info->hack_info.buf =
            g_string_truncate (do_purge_info->hack_info.buf, 0);
    }
}

static void
do_purge_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                       gpointer data)
{
    DebmanDoPurgeInfo *do_purge_info = (DebmanDoPurgeInfo *)data;

    g_signal_handler_disconnect (line_buf, do_purge_info->read_line_id);
    g_signal_handler_disconnect (line_buf, do_purge_info->read_done_id);

    g_main_quit (do_purge_info->loop);
}

static gboolean
do_purge (RCPackman *packman, DebmanInstallState *install_state)
{
    pid_t child;
    pid_t parent;
    int status;
    int master, slave;
    RCLineBuf *line_buf;
    DebmanDoPurgeInfo do_purge_info;
    GMainLoop *loop;

    if (!rc_file_exists ("/usr/bin/dpkg")) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": /usr/bin/dpkg does "
                  "not exist\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "/usr/bin/dpkg does not exist (suggest "
                              "'/usr/bin/dpkg --purge --pending' once dpkg "
                              "is installed)");

        return (FALSE);
    }

    openpty (&master, &slave, NULL, NULL, NULL);

    signal (SIGPIPE, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);
    signal (SIGUSR2, debman_sigusr2_cb);

    parent = getpid ();

    /* dpkg is going to need to grab this lock (sigh...) */
    unlock_database (RC_DEBMAN (packman));

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "fork failed");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": fork failed\n");

        close (master);
        close (slave);

        return (FALSE);

    case 0:
        close (master);

        fflush (stdin);
        dup2 (slave, STDIN_FILENO);

        fflush (stdout);
        dup2 (slave, STDOUT_FILENO);

        fflush (stderr);
        dup2 (slave, STDERR_FILENO);

        close (slave);

//        putenv ("LD_PRELOAD=" LIBDIR "/rc-dpkg-helper.so");
        putenv (g_strdup_printf ("LD_PRELOAD=%s/rc-dpkg-helper.so",
                                 rc_libdir));
        putenv (g_strdup_printf ("RC_READ_NOTIFY_PID=%d", parent));
        putenv ("PAGER=cat");

        i18n_fixer ();

        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                  ": /usr/bin/dpkg --purge --pending\n");

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--purge", "--pending",
               "--force-downgrade", "--force-configure-any",
               "--force-remove-essential", "--force-overwrite",
               "--force-overwrite-dir", NULL);

        exit (-1);

    default:
        break;
    }

    close (slave);

    loop = g_main_new (FALSE);

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, master);

    do_purge_info.packman = packman;
    do_purge_info.loop = loop;
    do_purge_info.install_state = install_state;

    do_purge_info.hack_info.buf = g_string_new (NULL);
    do_purge_info.hack_info.master = master;
    do_purge_info.hack_info.line_buf = line_buf;

    do_purge_info.read_line_id =
        g_signal_connect (line_buf, "read_line",
                          (GCallback) do_purge_read_line_cb,
                          &do_purge_info);
    do_purge_info.read_done_id =
        g_signal_connect (line_buf, "read_done",
                          (GCallback) do_purge_read_done_cb,
                          &do_purge_info);
#if FIX_THE_HACK
    do_purge_info.hack_info.poll_write_id =
        gtk_timeout_add (100,
                         (GtkFunction) debman_poll_write_cb,
                         (gpointer) &do_purge_info.hack_info);
#endif

    g_main_run (loop);

#if FIX_THE_HACK
    gtk_timeout_remove (do_purge_info.hack_info.poll_write_id);
#endif

    g_object_unref (line_buf);

    g_main_destroy (loop);

    g_string_free (do_purge_info.hack_info.buf, TRUE);

    close (master);

    waitpid (child, &status, 0);

    if (!(lock_database (RC_DEBMAN (packman)))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't reacquire lock file");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": lost database lock!\n");

        return (FALSE);
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dpkg exited abnormally");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": dpkg exited abnormally\n");

        return (FALSE);
    }

    return (TRUE);
}

/*
  All of the do_unpack flotsam.  Given a bunch of packages, calls dpkg
  --unpack on them in groups until they're all unpacked.  Once again,
  monitors dpkg output for useful strings.
*/

static GSList *
make_unpack_commands (gchar **command, RCPackageSList *packages)
{
    GSList *iter1, *iter2;
    gchar **iter3;

    GSList *lines = NULL;
    GSList *line = NULL;
    guint line_length = 0;

    glong arg_max;

    GSList *ret = NULL;

#if 0
    arg_max = sysconf (_SC_ARG_MAX);
    if (arg_max == -1) {
        arg_max = 16384;
    }
#else
    arg_max = 0x7FFFFFFF;
#endif

    iter1 = packages;

    while (iter1) {
        gchar *filename =
            ((RCPackage *)(iter1->data))->package_filename;
        /* Is it necessary to count the spaces? */
        guint filename_length = strlen (filename) + 1;

        if (!line) {
            for (iter3 = command; *iter3; iter3++) {
                line = g_slist_append (line, *iter3);
                line_length += strlen (*iter3) + 1;
            }

            /* We've counted one extra space for the first argument */
            line_length -= 1;
        }

        if (line_length + filename_length > arg_max) {
            lines = g_slist_append (lines, line);
            line = NULL;
        } else {
            line = g_slist_append (line, filename);
            line_length += filename_length;

            iter1 = iter1->next;
        }
    }

    /* Don't forget the last line may not have been full */
    if (line) {
        lines = g_slist_append (lines, line);
    }

    /* We now need to convert these to argv-style arrays that execv
     * can deal with */
    for (iter1 = lines; iter1; iter1 = iter1->next) {
        guint count = 0;
        gchar **argv;

        line = (GSList *)(iter1->data);

        argv = g_new0 (gchar *, g_slist_length (line) + 1);

        for (iter2 = line; iter2; iter2 = iter2->next) {
            argv[count++] = (gchar *)(iter2->data);
        }

        ret = g_slist_append (ret, argv);

        g_slist_free (line);
    }

    g_slist_free (lines);

    return (ret);
}

typedef DebmanDoPurgeInfo DebmanDoUnpackInfo;

static void
do_unpack_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoUnpackInfo *do_unpack_info = (DebmanDoUnpackInfo *)data;

    rc_debug (RC_DEBUG_LEVEL_DEBUG, __FUNCTION__ ": got \"%s\"\n", line);

    do_unpack_info->hack_info.buf =
        g_string_append (do_unpack_info->hack_info.buf, line);
    do_unpack_info->hack_info.buf =
        g_string_append (do_unpack_info->hack_info.buf, "\n");

    if (!strncmp (line, "Unpacking", strlen ("Unpacking")) ||
        !strncmp (line, "Purging", strlen ("Purging")))
    {
        guint length;
        RCPackmanStep step_type;
        gchar *ptr;
        gchar *name;

        if (line[0] == 'U') {
            length = strlen ("Unpacking ");
            if (!strncmp (line + length, "replacement ",
                          strlen ("replacement "))) {
                length += strlen ("replacement ");
            }
            step_type = RC_PACKMAN_STEP_INSTALL;
        } else {
            length = strlen ("Purging configuration files for ");
            step_type = RC_PACKMAN_STEP_REMOVE;
        }

        ptr = strchr (line + length, ' ');
        name = g_strndup (line + length, ptr - (line + length));

        g_signal_emit_by_name (do_unpack_info->packman,
                               "transact_step",
                               ++do_unpack_info->install_state->seqno,
                               step_type, name);

        g_free (name);

        do_unpack_info->hack_info.buf =
            g_string_truncate (do_unpack_info->hack_info.buf, 0);
    }

    if (!strncmp (line, "(Reading database ...",
                  strlen ("(Reading database ...")) ||
        !strncmp (line, "Preparing to replace ",
                  strlen ("Preparing to replace ")) ||
        !strncmp (line, "Selecting previously unselected ",
                  strlen ("Selecting previously unselected ")))
    {
        do_unpack_info->hack_info.buf =
            g_string_truncate (do_unpack_info->hack_info.buf, 0);
    }
}

static void
do_unpack_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanDoUnpackInfo *do_unpack_info = (DebmanDoUnpackInfo *)data;

    if (do_unpack_info->read_line_id) {
        g_signal_handler_disconnect (line_buf,
                                     do_unpack_info->read_line_id);
    }
    if (do_unpack_info->read_done_id) {
        g_signal_handler_disconnect (line_buf,
                                     do_unpack_info->read_done_id);
    }

    g_main_quit (do_unpack_info->loop);
}

static gboolean
do_unpack (RCPackman *packman, RCPackageSList *packages,
           DebmanInstallState *install_state, gboolean perform)
{
    GSList *argvl = NULL;
    GSList *iter;
    gchar *command[] = { "/usr/bin/dpkg", "--no-act",
                         "--auto-deconfigure", "--unpack",
                         "--force-downgrade", "--force-configure-any",
                         "--force-remove-essential", "--force-overwrite",
                         "--force-overwrite-dir", NULL };
    RCLineBuf *line_buf;
    GMainLoop *loop;
    DebmanDoUnpackInfo do_unpack_info;

    g_return_val_if_fail (g_slist_length (packages) > 0, TRUE);

    signal (SIGCHLD, SIG_DFL);
    signal (SIGPIPE, SIG_DFL);

    argvl = make_unpack_commands (command, packages);

    for (iter = argvl; iter; iter = iter->next) {
        gchar **argv = (gchar **)(iter->data);

        pid_t child;
        int status;
        int fds[2];

        if (pipe (fds)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "pipe failed");

            g_slist_foreach (argvl, (GFunc) g_free, NULL);
            g_slist_free (argvl);

            return (FALSE);
        }

        child = fork ();

        switch (child) {
        case -1:
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "fork failed");

            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__, ": fork failed\n");

            g_slist_foreach (argvl, (GFunc) g_free, NULL);
            g_slist_free (argvl);

            close (fds[0]);
            close (fds[1]);

            return (FALSE);

        case 0:
            close (fds[0]);

            rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ":");
            dump_argv (RC_DEBUG_LEVEL_INFO, argv);

            fflush (stdout);
            dup2 (fds[1], STDOUT_FILENO);

            fflush (stderr);
            dup2 (fds[1], STDERR_FILENO);

            i18n_fixer ();

            execv ("/usr/bin/dpkg", argv);
            break;

        default:
            break;
        }

        close (fds[1]);

        loop = g_main_new (FALSE);

        line_buf = rc_line_buf_new ();
        rc_line_buf_set_fd (line_buf, fds[0]);

        do_unpack_info.loop = loop;
        do_unpack_info.read_line_id = 0;

        do_unpack_info.read_done_id =
            g_signal_connect (line_buf, "read_done",
                              (GCallback) do_unpack_read_done_cb,
                              &do_unpack_info);

        g_main_run (loop);

        g_object_unref (line_buf);

        g_main_destroy (loop);

        close (fds[0]);

        waitpid (child, &status, 0);

        if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "dry run failed");

            g_slist_foreach (argvl, (GFunc) g_free, NULL);
            g_slist_free (argvl);

            return (FALSE);
        }
    }

    for (iter = argvl; iter; iter = iter->next) {
        gchar **argv = (gchar **)(iter->data);

        pid_t child;
        pid_t parent;
        int master, slave;
        int status;

        if (!getenv ("RC_JUST_KIDDING") && perform) {
            /* So this is a crufy hack, but I need something to replace
             * the --no-act with, and there doesn't seem to be a --yes-act
             * option.  Since --abort-after=50 is the default, this should
             * be harmless, no? */
            argv[1] = "--abort-after=50";
        }

        if (!rc_file_exists ("/usr/bin/dpkg")) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": /usr/bin/dpkg "
                      "does not exist\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "/usr/bin/dpkg does not exist (suggest "
                                  "'apt-get -f install')");

            return (FALSE);
        }

        openpty (&master, &slave, NULL, NULL, NULL);

        signal (SIGUSR2, debman_sigusr2_cb);

        parent = getpid ();

        /* The locking in Debian sucks ass */
        unlock_database (RC_DEBMAN (packman));

        child = fork ();

        switch (child) {
        case -1:
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "fork failed");

            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": fork failed\n");

            close (master);
            close (slave);

            g_slist_foreach (argvl, (GFunc) g_free, NULL);
            g_slist_free (argvl);

            return (FALSE);

        case 0:
            close (master);

            fflush (stdin);
            dup2 (slave, STDIN_FILENO);

            fflush (stdout);
            dup2 (slave, STDOUT_FILENO);

            fflush (stderr);
            dup2 (slave, STDERR_FILENO);

            close (slave);

//            putenv ("LD_PRELOAD=" LIBDIR "/rc-dpkg-helper.so");
            putenv (g_strdup_printf ("LD_PRELOAD=%s/rc-dpkg-helper.so",
                                     rc_libdir));
            putenv (g_strdup_printf ("RC_READ_NOTIFY_PID=%d", parent));
            putenv ("PAGER=cat");

            i18n_fixer ();

            rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ":");
            dump_argv (RC_DEBUG_LEVEL_INFO, argv);

            execv ("/usr/bin/dpkg", argv);
            break;

        default:
            break;
        }

        close (slave);

        loop = g_main_new (FALSE);

        line_buf = rc_line_buf_new ();
        rc_line_buf_set_fd (line_buf, master);

        do_unpack_info.packman = packman;
        do_unpack_info.loop = loop;
        do_unpack_info.install_state = install_state;

        do_unpack_info.hack_info.buf = g_string_new (NULL);
        do_unpack_info.hack_info.master = master;
        do_unpack_info.hack_info.line_buf = line_buf;

        do_unpack_info.read_line_id =
            g_signal_connect (line_buf, "read_line",
                              (GCallback) do_unpack_read_line_cb,
                              &do_unpack_info);
        do_unpack_info.read_done_id =
            g_signal_connect (line_buf, "read_done",
                              (GCallback) do_unpack_read_done_cb,
                              &do_unpack_info);
#if FIX_THE_HACK
        do_unpack_info.hack_info.poll_write_id =
            gtk_timeout_add (100,
                             (GtkFunction) debman_poll_write_cb,
                             (gpointer) &do_unpack_info.hack_info);
#endif

        g_main_run (loop);

#if FIX_THE_HACK
        gtk_timeout_remove (do_unpack_info.hack_info.poll_write_id);
#endif
            
        g_object_unref (line_buf);

        g_main_destroy (loop);

        g_string_free (do_unpack_info.hack_info.buf, TRUE);

        close (master);

        waitpid (child, &status, 0);

        if (!(lock_database (RC_DEBMAN (packman)))) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "couldn't reacquire lock file");

            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": lost database lock!\n");

            g_slist_foreach (argvl, (GFunc) g_free, NULL);
            g_slist_free (argvl);

            return (FALSE);
        }

        if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "dpkg exited abnormally");

            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": dpkg exited abnormally\n");

            g_slist_foreach (argvl, (GFunc) g_free, NULL);
            g_slist_free (argvl);

            return (FALSE);
        }
    }

    g_slist_foreach (argvl, (GFunc) g_free, NULL);
    g_slist_free (argvl);

    return (TRUE);
}

/*
  The do_configure crowd.  Very similar to do_purge, just calls dpkg
  --configure --pending.  Also sniffs the out.
*/

typedef struct _DebmanDoConfigureInfo DebmanDoConfigureInfo;

struct _DebmanDoConfigureInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    RCPackman *packman;

    DebmanInstallState *install_state;

    DebmanHackInfo hack_info;
};

static void
do_configure_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoConfigureInfo *do_configure_info = (DebmanDoConfigureInfo *)data;

    rc_debug (RC_DEBUG_LEVEL_DEBUG, __FUNCTION__ ": got \"%s\"\n", line);

    do_configure_info->hack_info.buf =
        g_string_append (do_configure_info->hack_info.buf, line);
    do_configure_info->hack_info.buf =
        g_string_append (do_configure_info->hack_info.buf, "\n");

    if (!strncmp (line, "Setting up ", strlen ("Setting up "))) {
        guint length = strlen ("Setting up ");
        gchar *ptr;
        gchar *name;

        ptr = strchr (line + length, ' ');
        name = g_strndup (line + length, ptr - (line + length));

        g_signal_emit_by_name (do_configure_info->packman,
                               "transact_step",
                               ++do_configure_info->install_state->seqno,
                               RC_PACKMAN_STEP_CONFIGURE, name);

        g_free (name);

        do_configure_info->hack_info.buf =
            g_string_truncate (do_configure_info->hack_info.buf, 0);
    }
}

static void
do_configure_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                           gpointer data)
{
    DebmanDoConfigureInfo *do_configure_info = (DebmanDoConfigureInfo *)data;

    g_signal_handlers_disconnect_by_func (
        line_buf, (GCallback) do_configure_read_line_cb, data);
    g_signal_handlers_disconnect_by_func (
        line_buf, (GCallback) do_configure_read_done_cb, data);

    g_main_quit (do_configure_info->loop);
}

static gboolean
do_configure (RCPackman *packman, DebmanInstallState *install_state)
{
    pid_t child;
    pid_t parent;
    int status;
    int master, slave;
    RCLineBuf *line_buf;
    DebmanDoConfigureInfo do_configure_info;
    GMainLoop *loop;

    if (!rc_file_exists ("/usr/bin/dpkg")) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": /usr/bin/dpkg "
                  "does not exist\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "/usr/bin/dpkg does not exist (suggest "
                              "'dpkg --configure --pending')");

        return (FALSE);
    }

    openpty (&master, &slave, NULL, NULL, NULL);

    signal (SIGCHLD, SIG_DFL);
    signal (SIGPIPE, SIG_DFL);
    signal (SIGUSR2, debman_sigusr2_cb);

    parent = getpid ();

    unlock_database (RC_DEBMAN (packman));

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "fork failed");

        close (master);
        close (slave);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": fork failed\n");

        return (FALSE);

    case 0:
        close (master);

        fflush (stdin);
        dup2 (slave, STDIN_FILENO);

        fflush (stdout);
        dup2 (slave, STDOUT_FILENO);

        fflush (stderr);
        dup2 (slave, STDERR_FILENO);

        close (slave);

//        putenv ("LD_PRELOAD=" LIBDIR "/rc-dpkg-helper.so");
        putenv (g_strdup_printf ("LD_PRELOAD=%s/rc-dpkg-helper.so",
                                 rc_libdir));
        putenv (g_strdup_printf ("RC_READ_NOTIFY_PID=%d", parent));
        putenv ("PAGER=cat");

        i18n_fixer ();

        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                  ": /usr/bin/dpkg --configure --pending\n");

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--configure", "--pending",
               "--force-downgrade", "--force-configure-any",
               "--force-remove-essential", "--force-overwrite",
               "--force-overwrite-dir", NULL);

        exit (-1);

    default:
        break;
    }

    close (slave);

    loop = g_main_new (FALSE);

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, master);

    do_configure_info.packman = packman;
    do_configure_info.loop = loop;
    do_configure_info.install_state = install_state;

    do_configure_info.hack_info.buf = g_string_new (NULL);
    do_configure_info.hack_info.master = master;
    do_configure_info.hack_info.line_buf = line_buf;

    do_configure_info.read_line_id =
        g_signal_connect (line_buf, "read_line",
                          (GCallback) do_configure_read_line_cb,
                          &do_configure_info);
    do_configure_info.read_done_id =
        g_signal_connect (line_buf, "read_done",
                          (GCallback) do_configure_read_done_cb,
                          &do_configure_info);
#if FIX_THE_HACK
    do_configure_info.hack_info.poll_write_id =
        gtk_timeout_add (100,
                         (GtkFunction) debman_poll_write_cb,
                         (gpointer) &do_configure_info.hack_info);
#endif

    g_main_run (loop);

#if FIX_THE_HACK
    gtk_timeout_remove (do_configure_info.hack_info.poll_write_id);
#endif

    g_object_unref (line_buf);

    g_main_destroy (loop);

    g_string_free (do_configure_info.hack_info.buf, TRUE);

    close (master);

    waitpid (child, &status, 0);

    signal (SIGUSR2, SIG_DFL);

    if (!(lock_database (RC_DEBMAN (packman)))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't reacquire lock file");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": lost database lock!\n");

        return (FALSE);
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dpkg exited abnormally");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": dpkg exited abnormally\n");

        return (FALSE);
    }

    return (TRUE);
}

#if 0

typedef struct _Node Node;

struct _Node {
    /* The actual package represented in the node */
    RCPackage *package;

    /* The packages that pre-depend on me -- once this package is
     * installed, then these packages may be considered for
     * installation */
    Node **prereqs;
    int prereq_capacity;
    int prereq_count;

    /* How many things do I depend on?  (We don't need the actual
     * association here, just a count, so that we can quickly seed the
     * initial queue with unattached nodes) */
    int attachments;

    /* My eventual (lowest, where 0 is max height) depth in the
     * tree */
    int depth;
};

static Node *
node_new (RCPackage *package)
{
    Node *node;

    node = g_new (Node, 1);

    node->package = package;

    /* Begin with the capacity to store 5 prereq nodes (we can
     * expand this as necessary) */
    node->prereqs = g_new (Node *, 5);
    node->prereq_capacity = 5;
    node->prereq_count = 0;

    /* All nodes are initially unattached */
    node->attachments = 0;

    /* All nodes begin life at depth 0 */
    node->depth = 0;

    return node;
}

static void
node_free (Node *node)
{
    g_free (node->prereqs);
    g_free (node);
}

static void
node_add_prereq (Node *required_node, Node *prereq_node)
{
    /* The prereq_node is now attached to the dependend_node */
    prereq_node->attachments++;

    /* The required_node needs a pointer back to the prereq_node,
     * so make sure we have space */
    if (required_node->prereq_count == required_node->prereq_capacity) {
        required_node->prereq_capacity += 5;
        required_node->prereqs = g_renew (
            Node *, required_node->prereqs,
            required_node->prereq_capacity);
    }

    /* Add the back pointer and increment the prereq_count */
    required_node->prereqs[required_node->prereq_count++] =
        prereq_node;
}

static GHashTable *
construct_graph (RCPackageSList *packages)
{
    GHashTable *graph;
    GSList *iter;

    graph = g_hash_table_new (g_str_hash, g_str_equal);

    /* Initial population of the graph */
    for (iter = packages; iter; iter = iter->next) {
        Node *node;
        GSList *piter;

        /* Create a new node for this package */
        node = node_new ((RCPackage *)iter->data);

        /* Enter this node for each of the spec names that the
         * package provides */
        for (piter = node->package->provides; piter; piter = piter->next) {
            g_hash_table_insert (
                graph, ((RCPackageDep *)piter->data)->spec.name, node);
        }
    }

    return graph;
}

static void
process_predeps (GHashTable *graph, RCPackageSList *packages)
{
    GSList *iter;

    /* For every package, identify if any packages we pre-depend on
     * are also in the graph, and if so, add the reference from them
     * to us */
    for (iter = packages; iter; iter = iter->next) {
        Node *prereq_node;
        GSList *riter;

        prereq_node = g_hash_table_lookup (
            graph, ((RCPackage *)iter->data)->spec.name);

        /* This node had better be in the graph -- we just added it */
        g_assert (prereq_node != NULL);

        /* Walk our requirements looking for pre-deps also in the
         * graph */
        for (riter = prereq_node->package->requires; riter;
             riter = riter->next)
        {
            Node *required_node;

            /* Make sure this is a pre-dep and not just a normal
             * dependency */
            if (((RCPackageDep *)riter->data)->pre == 0) {
                continue;
            }

            required_node = g_hash_table_lookup (
                graph, ((RCPackageDep *)riter->data)->spec.name);

            /* It's ok if we don't find it, since not all of my
             * pre-deps are necessarily getting installed right now.
             * Some may already be installed */
            if (!required_node) {
                continue;
            }

            /* Make sure this isn't us!  (Some packages, oddly enough,
             * depend on themselves...) */
            if (required_node == prereq_node) {
                continue;
            }

            /* In an optimal case, we could check the system to make
             * sure we don't already have this pre-dep satisfied -- if
             * we did, we wouldn't need to pay attention to the
             * relationship.  That's harder, though, and not really
             * worth the trouble */

            node_add_prereq (required_node, prereq_node);
        }
    }
}

static RCPackageSList *
resolve_graph (GHashTable *graph, RCPackageSList *packages)
{
    GSList *queue = NULL;
    GSList *iter;

    /* Walk the nodes of the graph looking for unattached nodes
     * (attachments == 0) */
    for (iter = packages; iter; iter = iter->next) {
        Node *node;

        node = g_hash_table_lookup (
            graph, ((RCPackage *)iter->data)->spec.name);

        /* This must be found */
        g_assert (node != NULL);

        /* If this node is unattached, put it in our initial queue for
         * processing; if we've done everything right before now, the
         * order at this point shouldn't matter */
        if (node->attachments == 0) {
            queue = g_slist_prepend (queue, node);
        }
    }

    /* Now, process the queue, mutating it as we go.  Note that the
     * following walk will break if any node has a dependency on
     * itself, something we must be careful not to violate when adding
     * ordering criteria */

    for (iter = queue; iter; iter = iter->next) {
        Node *required_node;
        int i;

        required_node = (Node *)iter->data;

        /* Walk all of the prereq nodes */
        for (i = 0; i < required_node->prereq_count; i++) {
            Node *prereq_node;

            prereq_node = required_node->prereqs[i];

            /* First, ensure that this node hasn't been prematurely
             * handled already and isn't already scheduled to be
             * addressed */
            queue = g_slist_remove (queue, prereq_node);

            /* Now, set the depth of this node to be one below
             * (higher) than the depth of the node it depends on */
            prereq_node->depth = required_node->depth + 1;

            /* Finally, add the prereq_node to the queue of nodes
             * to be processed */
            queue = g_slist_append (queue, prereq_node);
        }
    }

    return queue;
}

static GSList *
order_packages (RCPackageSList *packages)
{
    GHashTable *graph;
    GSList *iter;
    GSList *ordered;
    GSList *ret = NULL;

    graph = construct_graph (packages);

    process_predeps (graph, packages);

    ordered = resolve_graph (graph, packages);

    g_hash_table_destroy (graph);

    iter = ordered;
    while (iter) {
        int depth;
        RCPackageSList *accum = NULL;

        depth = ((Node *)iter->data)->depth;
        accum = g_slist_append (accum, ((Node *)iter->data)->package);
        node_free ((Node *)iter->data);

        iter = iter->next;

        while (iter && ((Node *)iter->data)->depth == depth) {
            accum = g_slist_append (accum, ((Node *)iter->data)->package);
            node_free ((Node *)iter->data);
            iter = iter->next;
        }

        ret = g_slist_append (ret, accum);
    }

    g_slist_free (ordered);

    /* Debugging only */
    for (iter = ret; iter; iter = iter->next) {
        GSList *tmp;
        fprintf (stderr, "In a group:\n");
        for (tmp = (RCPackageSList *)iter->data; tmp; tmp = tmp->next) {
            fprintf (stderr, "  %s\n", ((RCPackage *)tmp->data)->spec.name);
        }
    }

    return ret;
}

#endif

/*
 * Combine the last four code blocks into a useful transaction.
 */

static void
rc_debman_transact (RCPackman *packman, RCPackageSList *install_packages,
                    RCPackageSList *remove_packages, gboolean perform)
{
    DebmanInstallState *install_state = g_new0 (DebmanInstallState, 1);
    gboolean unlock_db = FALSE;

//    order_packages (install_packages);

    if ((RC_DEBMAN (packman))->priv->lock_fd == -1) {
        if (!lock_database (RC_DEBMAN (packman)))
            goto END;
        unlock_db = TRUE;
    }

    install_state->total = (g_slist_length (install_packages) * 2) +
        g_slist_length (remove_packages);
    install_state->seqno = 0;

    g_signal_emit_by_name (packman, "transact_start",
                           install_state->total);

    rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
              ": about to update status file\n");

    if (!(mark_status (packman, install_packages, remove_packages))) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": update of status database failed\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "Unable to update status database");

        goto END;
    }

    if (install_packages) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": about to unpack\n");

        if (!(do_unpack (packman, install_packages, install_state, perform))) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": unpack failed\n");

            if (rc_packman_get_error (packman) == RC_PACKMAN_ERROR_FATAL) {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                      "Unable to unpack selected packages " \
                                      "(suggest 'apt-get -f install')");
            } else {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "Unable to unpack selected packages");
            }

            goto END;
        }
    }

    if (remove_packages && (install_state->seqno < install_state->total)) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": about to purge\n");

        if (!(do_purge (packman, install_state))) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": purge failed\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "Unable to remove selected packages " \
                                  "(suggest 'dpkg --purge --pending')");

            goto END;
        }
    }

    if (install_packages) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": about to configure\n");

        if (!(do_configure (packman, install_state))) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": configure failed\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "Unable to configure unpacked packages " \
                                  "(suggest 'dpkg --configure --pending')");

            goto END;
        }
    }

    g_signal_emit_by_name (packman, "transact_done");

  END:
    g_free (install_state);
    hash_destroy (RC_DEBMAN (packman));
    if (unlock_db)
        unlock_database (RC_DEBMAN (packman));
    /* If we screwed up the status file let's "fix" it */
    verify_status (packman);
}

/*
 * The query_all associated cruft
 */

typedef struct _DebmanQueryInfo DebmanQueryInfo;

struct _DebmanQueryInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    RCPackageSList *packages;

    RCPackage *package_buf;
    GString *desc_buf;
#if 0
    RCPackageDepSList *replaces;
#endif

    gboolean error;
};

static gchar *
get_description (GString *str)
{
    gchar *ret;

    str = g_string_truncate (str, str->len - 1);
    ret = str->str;
    g_string_free (str, FALSE);

    return (ret);
}

static void
query_all_read_line_cb (RCLineBuf *line_buf, gchar *status_line, gpointer data)
{
    DebmanQueryInfo *query_info = (DebmanQueryInfo *)data;
    char *ptr;
    char *line;
    guint epoch;

    line = alloca (strlen (status_line) + 1);
    strcpy (line, status_line);

    /* This is a blank line which terminates a package section in
       /var/lib/dpkg/status, so save out our current buffer */
    if (!line[0]) {
        if (query_info->package_buf) {
#if 0
            RCPackageDepSList *iter;
#endif

            if (query_info->desc_buf && query_info->desc_buf->len) {
                query_info->package_buf->description =
                    get_description (query_info->desc_buf);
                query_info->desc_buf = NULL;
            }

#if 0
            for (iter = query_info->replaces; iter; iter = iter->next) {
                RCPackageDep *dep = (RCPackageDep *)(iter->data);

                if (rc_package_dep_slist_has_dep (
                        query_info->package_buf->conflicts, dep))
                {
                    query_info->package_buf->provides = g_slist_prepend (
                        query_info->package_buf->provides, dep);
                } else {
                    rc_package_dep_free (dep);
                }
            }

            g_slist_free (query_info->replaces);
            query_info->replaces = NULL;
#endif

            query_info->packages = g_slist_prepend (query_info->packages,
                                                    query_info->package_buf);
            query_info->package_buf = NULL;

            return;
        }
    }

    ptr = line;
    if (*ptr != ' ') { /* These lines must begin with a space */
        while (*ptr != ':') {
            *ptr++ = tolower (*ptr);
        }
    }

    /* This is the beginning of a package section, so start up a new buffer
       and set the package name */
    if (!strncmp (line, "package:", strlen ("package:"))) {
        /* Better make sure this isn't a malformed /var/lib/dpkg/status file,
           so check to make sure we cleanly finished the last package */
        if (query_info->package_buf) {
            query_info->error = TRUE;

            /* FIXME: must do something smart here */
            return;
        }

        query_info->package_buf = rc_package_new ();
        /* All debian packages "have" epochs */
        query_info->package_buf->spec.has_epoch = 1;

        ptr = line + strlen ("package:");
        while (isspace (*ptr)) {
            ptr++;
        }
        query_info->package_buf->spec.name = g_strdup (ptr);

        return;
    }

    /* This isn't a blank line, and this isn't a package: line, so
     * we'd better have a package started by this point */
    if (!query_info->package_buf) {
        query_info->error = TRUE;
        return;
    }

    if (!strncmp (line, "status:", strlen ("status:"))) {
        char **status;

        status = split_status (line + strlen ("status:"));

        if (!(status && status[0] && status[1] && status[2])) {
            /* This should never have gotten through verify
             * status, and indicates something is seriously wrong */
            query_info->error = TRUE;
            goto DONE;
        }

        if (strcmp (status[0], "reinst-required") == 0) {
            /* Yet another condition that must never get through
             * verify status, as there's not much RC can do about
             * this.  Debian packaging blows. */
            query_info->error = TRUE;
            goto DONE;
        }

        if (strcmp (status[1], "ok") != 0) {
            /* Anything other than ok is a problem we don't know how
             * to deal with (no, I'm not interested in making RC yet
             * another tool to clean up the Debian mess) */
            query_info->error = TRUE;
            goto DONE;
        }

        if (strcmp (status[2], "installed") == 0) {
            query_info->package_buf->installed = TRUE;
            goto DONE;
        }

      DONE:
        g_strfreev (status);
        return;
    }

    if (!strncmp (line, "installed-size:", strlen ("installed-size:"))) {
        query_info->package_buf->installed_size =
            strtoul (line + strlen ("installed-size:"), NULL, 10) * 1024;

        return;
    }

    if (!strncmp (line, "description:", strlen ("description:"))) {
        ptr = line + strlen ("description:");

        while (*ptr && isspace (*ptr)) {
            ptr++;
        }

        query_info->package_buf->summary =
            g_strdup (ptr);

        /* After Description: comes the long description, so prepare
         * to suck that in */
        query_info->desc_buf = g_string_new (NULL);

        return;
    }

    if ((line[0] == ' ') && query_info->desc_buf) {
        if ((line[1] != '.') || line[2]) {
            query_info->desc_buf =
                g_string_append (query_info->desc_buf, line + 1);
        }

        query_info->desc_buf = g_string_append (query_info->desc_buf, "\n");

        return;
    }

    if (!strncmp (line, "version:", strlen ("version:"))) {
        ptr = line + strlen ("version:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }

        rc_debman_parse_version (ptr, &epoch,
                                 &query_info->package_buf->spec.version,
                                 &query_info->package_buf->spec.release);
        query_info->package_buf->spec.epoch = epoch;

        return;
    }

    if (!strncmp (line, "section:", strlen ("section:"))) {
        char *section;

        ptr = line + strlen ("section:");
        section = strchr (ptr, '/');

        if (!section) {
            section = ptr;
        }

        while (*section && isspace (*section)) {
            section++;
        }

        query_info->package_buf->section =
            rc_debman_section_to_package_section (section);

        return;
    }

    if (!strncmp (line, "depends:", strlen ("depends:"))) {
        GSList *list, *tmp;

        ptr = line + strlen ("depends:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }

        list = rc_debman_fill_depends (ptr);

        while ((tmp = g_slist_find_custom (
                    list, query_info->package_buf,
                    (GCompareFunc) rc_package_spec_compare_name)))
        {
            list = g_slist_remove_link (list, tmp);
            rc_package_dep_slist_free (tmp);
        }

        query_info->package_buf->requires = g_slist_concat (
            query_info->package_buf->requires, list);

        return;
    }

    if (!strncmp (line, "recommends:", strlen ("recommends:"))) {
        ptr = line + strlen ("recommends:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }

        query_info->package_buf->recommends = g_slist_concat (
            query_info->package_buf->recommends, rc_debman_fill_depends (ptr));

        return;
    }

    if (!strncmp (line, "suggests:", strlen ("suggests:"))) {
        ptr = line + strlen ("suggests:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }
        query_info->package_buf->suggests = g_slist_concat (
            query_info->package_buf->suggests, rc_debman_fill_depends (ptr));
        return;
    }

    if (!strncmp (line, "pre-depends:", strlen ("pre-depends:"))) {
        RCPackageDepSList *tmp = NULL, *iter;

        ptr = line + strlen ("pre-depends:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }

        tmp = rc_debman_fill_depends (ptr);

        for (iter = tmp; iter; iter = iter->next) {
            ((RCPackageDep *)(iter->data))->pre = 1;
        }

        query_info->package_buf->requires = g_slist_concat (
            query_info->package_buf->requires, tmp);

        return;
    }

    if (!strncmp (line, "conflicts:", strlen ("conflicts:"))) {
        ptr = line + strlen ("conflicts:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }
        query_info->package_buf->conflicts = g_slist_concat (
            query_info->package_buf->conflicts, rc_debman_fill_depends (ptr));
        return;
    }

    if (!strncmp (line, "provides:", strlen ("provides:"))) {
        ptr = line + strlen ("provides:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }
        query_info->package_buf->provides = g_slist_concat (
            query_info->package_buf->provides, rc_debman_fill_depends (ptr));
        return;
    }

#if 0
    if (!strncmp (line, "replaces:", strlen ("replaces:"))) {
        ptr = line + strlen ("replaces:");
        while (*ptr && isspace (*ptr)) {
            ptr++;
        }
        query_info->replaces = rc_debman_fill_depends (ptr);
        return;
    }
#endif
}

static void
query_all_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanQueryInfo *query_info = (DebmanQueryInfo *)data;

    g_signal_handler_disconnect (line_buf,
                                 query_info->read_line_id);
    g_signal_handler_disconnect (line_buf,
                                 query_info->read_done_id);

    query_info->error = (status == RC_LINE_BUF_ERROR);

    g_main_quit (query_info->loop);
}

static void
rc_debman_query_all_real (RCPackman *packman)
{
    int fd;
    RCLineBuf *line_buf;
    RCDebman *debman = RC_DEBMAN (packman);
    DebmanQueryInfo query_info;
    RCPackageSList *iter;
    GMainLoop *loop;

    if ((fd = open (debman->priv->status_file, O_RDONLY)) < 0) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't open %s for reading",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open /var/lib/dpkg/status\n");

        return;
    }

    loop = g_main_new (FALSE);

    query_info.packages = NULL;
    query_info.package_buf = NULL;
    query_info.desc_buf = NULL;
#if 0
    query_info.replaces = NULL;
#endif
    query_info.error = FALSE;
    query_info.loop = loop;

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, fd);

    query_info.read_line_id =
        g_signal_connect (line_buf, "read_line",
                          (GCallback) query_all_read_line_cb,
                          &query_info);
    query_info.read_done_id =
        g_signal_connect (line_buf, "read_done",
                          (GCallback) query_all_read_done_cb,
                          &query_info);

    g_main_run (loop);

    g_object_unref (line_buf);

    g_main_destroy (loop);

    for (iter = query_info.packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        if (package->installed) {
            package->provides =
                g_slist_prepend (package->provides,
                                 rc_package_dep_new_from_spec (
                                     &package->spec,
                                     RC_RELATION_EQUAL));
            g_hash_table_insert (debman->priv->package_hash,
                                 (gpointer) package->spec.name,
                                 (gpointer) package);
        } else {
            rc_package_unref (package);
        }
    }

    g_slist_free (query_info.packages);

    debman->priv->hash_valid = TRUE;

    close (fd);
}

static void
package_list_append (gchar *name, RCPackage *package,
                     RCPackageSList **package_list)
{
    *package_list = g_slist_prepend (*package_list, rc_package_copy (package));
}

static RCPackageSList *
rc_debman_query_all (RCPackman *packman)
{
    RCPackageSList *packages = NULL;

    if (!(RC_DEBMAN (packman)->priv->hash_valid)) {
        rc_debman_query_all_real (packman);
    }

    g_hash_table_foreach (RC_DEBMAN (packman)->priv->package_hash,
                          (GHFunc) package_list_append, &packages);

    return packages;
}

static RCPackageSList *
rc_debman_query (RCPackman *packman, const char *name)
{
    RCDebman *debman = RC_DEBMAN (packman);
    RCPackage *package;
    RCPackageSList *ret = NULL;

    if (!debman->priv->hash_valid) {
        rc_debman_query_all_real (packman);
    }

    package = g_hash_table_lookup (debman->priv->package_hash,
                                   (gconstpointer) name);

    if (package) {
        ret = g_slist_append (NULL, package);

        return ret;
    }

    return NULL;
}

static RCPackage *
rc_debman_query_file (RCPackman *packman, const gchar *filename)
{
    int fds[2];
    pid_t child;
    int status;
    RCLineBuf *line_buf;
    DebmanQueryInfo query_info;
    GMainLoop *loop;
    RCPackageDep *dep = NULL;

    if (pipe (fds)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't create pipe");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": pipe failed\n");

        return (FALSE);
    }

    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    signal (SIGPIPE, SIG_DFL);
    signal (SIGCHLD, SIG_DFL);

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "fork failed");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": fork failed\n");

        close (fds[0]);
        close (fds[1]);

        return (NULL);

    case 0:
        close (fds[0]);

        fflush (stdout);
        dup2 (fds[1], STDOUT_FILENO);
        close (fds[1]);

        i18n_fixer ();

        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                  ": /usr/bin/dpkg-deb -f %s\n", filename);

        execl ("/usr/bin/dpkg-deb", "/usr/bin/dpkg-deb", "-f", filename, NULL);

        exit (-1);

    default:
        break;
    }

    close (fds[1]);

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, fds[0]);

    query_info.package_buf = NULL;
    query_info.desc_buf = NULL;
    query_info.packages = NULL;
    query_info.error = FALSE;

    loop = g_main_new (FALSE);
    query_info.loop = loop;

    query_info.read_line_id =
        g_signal_connect (line_buf, "read_line",
                          (GCallback) query_all_read_line_cb,
                          &query_info);
    query_info.read_done_id =
        g_signal_connect (line_buf, "read_done",
                          (GCallback) query_all_read_done_cb,
                          &query_info);

    g_main_run (loop);

    g_object_unref (line_buf);

    g_main_destroy (loop);

    waitpid (child, &status, 0);

    close (fds[0]);

    if (!(WIFEXITED (status)) ||
        WEXITSTATUS (status) ||
        !query_info.package_buf)
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dpkg-deb exited abnormally");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": dpkg exited abnormally\n");

        return (NULL);
    }

    if (query_info.desc_buf) {
        query_info.package_buf->description =
            get_description (query_info.desc_buf);

        query_info.desc_buf = NULL;
    }

    /* Make the package provide itself (that's what we like to do,
           anyway) */
    dep = rc_package_dep_new_from_spec (
        (RCPackageSpec *)query_info.package_buf,
        RC_RELATION_EQUAL);

    query_info.package_buf->provides =
        g_slist_prepend (query_info.package_buf->provides, dep);

    return (query_info.package_buf);
}

static RCVerificationSList *
rc_debman_verify (RCPackman *packman, RCPackage *package, guint32 type)
{
    RCVerificationSList *ret = NULL;
    RCPackageUpdate *update = NULL;

    g_assert (packman);
    g_assert (package);
    g_assert (package->package_filename);

    if (package->history) {
        update = rc_package_get_latest_update(package);
    } else {
        return (NULL);
    }

    if ((type & RC_VERIFICATION_TYPE_SIZE) && update &&
        package->package_filename && (update->package_size > 0))
    {
        RCVerification *verification;

        verification = rc_verify_size (package->package_filename,
                                       update->package_size);

        ret = g_slist_append (ret, verification);
    }

    if ((type & RC_VERIFICATION_TYPE_MD5) && update && update->md5sum) {
        RCVerification *verification;

        verification = rc_verify_md5_string (package->package_filename,
                                             update->md5sum);

        ret = g_slist_append (ret, verification);
    }

    if ((type & RC_VERIFICATION_TYPE_GPG) && package->signature_filename) {
        RCVerification *verification;

        verification = rc_verify_gpg (package->package_filename,
                                      package->signature_filename);

        ret = g_slist_append (ret, verification);
    }

    return (ret);
}

/* I stole this code from dpkg, and good GOD is it ugly.  I want to
 * vomit.  From now on, every day I shall fall to my knees in the
 * morning and thank the heavens that I don't work on dpkg. */

static int verrevcmp(const char *val, const char *ref) {
    int vc, rc;
    long vl, rl;
    const char *vp, *rp;

    if (!val) val= "";
    if (!ref) ref= "";
    for (;;) {
        vp= val;  while (*vp && !isdigit(*vp)) vp++;
        rp= ref;  while (*rp && !isdigit(*rp)) rp++;
        for (;;) {
            vc= val == vp ? 0 : *val++;
            rc= ref == rp ? 0 : *ref++;
            if (!rc && !vc) break;
            /* assumes ASCII character set */
            if (vc && !isalpha(vc)) vc += 256;
            if (rc && !isalpha(rc)) rc += 256;
            if (vc != rc) return vc - rc;
        }
        val= vp;
        ref= rp;
        vl=0;  if (isdigit(*vp)) vl= strtol(val,(char**)&val,10);
        rl=0;  if (isdigit(*rp)) rl= strtol(ref,(char**)&ref,10);
        if (vl != rl) return vl - rl;
        if (!*val && !*ref) return 0;
        if (!*val) return -1;
        if (!*ref) return +1;
    }
}

static gint
rc_debman_version_compare (RCPackman *packman,
                           RCPackageSpec *spec1,
                           RCPackageSpec *spec2)
{
    return (rc_packman_generic_version_compare (spec1, spec2, verrevcmp));
}

typedef struct _DebmanFindFileInfo DebmanFindFileInfo;

struct _DebmanFindFileInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    const gchar *target;

    gboolean accept;
};

static void
find_file_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanFindFileInfo *find_file_info = (DebmanFindFileInfo *)data;

    if (!strcmp (find_file_info->target, line)) {
        find_file_info->accept = TRUE;

        g_signal_handler_disconnect (line_buf,
                                     find_file_info->read_line_id);
        g_signal_handler_disconnect (line_buf,
                                     find_file_info->read_done_id);

        g_main_quit (find_file_info->loop);
    }
}

static void
find_file_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanFindFileInfo *find_file_info = (DebmanFindFileInfo *)data;

    g_signal_handler_disconnect (line_buf,
                                 find_file_info->read_line_id);
    g_signal_handler_disconnect (line_buf,
                                 find_file_info->read_done_id);

    g_main_quit (find_file_info->loop);
}

static RCPackage *
rc_debman_find_file (RCPackman *packman, const gchar *filename)
{
    DIR *info_dir;
    struct dirent *info_file;
    gchar realname[PATH_MAX];

    if (!g_path_is_absolute (filename)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": pathname is not absolute\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "pathname is not absolute");

        goto ERROR;
    }

    if (!realpath (filename, realname)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": realpath returned NULL\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "realpath returned NULL");

        goto ERROR;
    }

    if (!(info_dir = opendir ("/var/lib/dpkg/info"))) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": opendir /var/lib/dpkg/info failed\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to scan /var/lib/dpkg/info");

        goto ERROR;
    }

    while ((info_file = readdir (info_dir))) {
        int fd;
        RCLineBuf *line_buf;
        GMainLoop *loop;
        DebmanFindFileInfo find_file_info;
        guint length = strlen (info_file->d_name);
        gchar *fullname;

        if (strcmp (info_file->d_name + length - 5, ".list")) {
            continue;
        }

        fullname = g_strconcat ("/var/lib/dpkg/info/", info_file->d_name,
                                NULL);

        if ((fd = open (fullname, O_RDONLY)) == -1) {
            closedir (info_dir);

            g_free (fullname);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "failed to open %s", fullname);

            goto ERROR;
        }

        g_free (fullname);

        line_buf = rc_line_buf_new ();
        rc_line_buf_set_fd (line_buf, fd);

        find_file_info.target = realname;
        find_file_info.accept = FALSE;

        loop = g_main_new (FALSE);
        find_file_info.loop = loop;

        find_file_info.read_line_id =
            g_signal_connect (line_buf, "read_line",
                              (GCallback) find_file_read_line_cb,
                              &find_file_info);
        find_file_info.read_done_id =
            g_signal_connect (line_buf, "read_done",
                              (GCallback) find_file_read_done_cb,
                              &find_file_info);

        g_main_run (loop);

        g_object_unref (line_buf);

        g_main_destroy (loop);

        close (fd);

        if (find_file_info.accept) {
            RCPackageSList *packages;
            RCPackage *package;
            gchar *name;

            name = g_strndup (info_file->d_name, length - 5);
            closedir (info_dir);

            packages = rc_packman_query (packman, name);

            if (!packages)
                return NULL;
            else {
                package = packages->data;
                g_slist_free (packages);
                return package;
            }
        }
    }

    closedir (info_dir);

    return (NULL);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "find_file failed");

    return (NULL);
}

static gboolean
rc_debman_lock (RCPackman *packman)
{
    return (lock_database (RC_DEBMAN (packman)));
}

static void
rc_debman_unlock (RCPackman *packman)
{
    unlock_database (RC_DEBMAN (packman));
}

static void
rc_debman_finalize (GObject *obj)
{
    RCDebman *debman = RC_DEBMAN (obj);

    unlock_database (debman);

    hash_destroy (debman);

    g_free (debman->priv->rc_status_file);
    g_free (debman->priv);

    if (parent_class->finalize) {
        parent_class->finalize (obj);
    }
}

static void
rc_debman_class_init (RCDebmanClass *klass)
{
    GObjectClass *object_class =  (GObjectClass *) klass;
    RCPackmanClass *packman_class = (RCPackmanClass *) klass;

    object_class->finalize = rc_debman_finalize;

    parent_class = g_type_class_peek_parent (klass);

    packman_class->rc_packman_real_transact = rc_debman_transact;
    packman_class->rc_packman_real_query = rc_debman_query;
    packman_class->rc_packman_real_query_file = rc_debman_query_file;
    packman_class->rc_packman_real_query_all = rc_debman_query_all;
    packman_class->rc_packman_real_verify = rc_debman_verify;
    packman_class->rc_packman_real_find_file = rc_debman_find_file;
    packman_class->rc_packman_real_version_compare = rc_debman_version_compare;
    packman_class->rc_packman_real_lock = rc_debman_lock;
    packman_class->rc_packman_real_unlock = rc_debman_unlock;

    putenv ("DEBIAN_FRONTEND=noninteractive");

//    deps_conflicts_use_virtual_packages (FALSE);
//    g_warning ("vlad, fix deps_conflict_use_virtual_pakages");
}

static void
rc_debman_init (RCDebman *debman)
{
    RCPackman *packman = RC_PACKMAN (debman);

    debman->priv = g_new0 (RCDebmanPrivate, 1);

    debman->priv->lock_fd = -1;

    debman->priv->package_hash = g_hash_table_new (g_str_hash, g_str_equal);
    debman->priv->hash_valid = FALSE;

    if (getenv ("RC_DEBMAN_STATUS_FILE")) {
        debman->priv->status_file = getenv ("RC_DEBMAN_STATUS_FILE");
        debman->priv->rc_status_file =
            g_strconcat (debman->priv->status_file, ".redcarpet", NULL);
    } else {
        debman->priv->status_file = DEBMAN_DEFAULT_STATUS_FILE;
        debman->priv->rc_status_file =
            g_strdup (DEBMAN_DEFAULT_STATUS_FILE ".redcarpet");
    }

    rc_packman_set_file_extension(packman, "deb");

    rc_packman_set_capabilities(packman, RC_PACKMAN_CAP_VIRTUAL_CONFLICTS|RC_PACKMAN_CAP_SELF_CONFLICT);

    debman->priv->db_mtime = 0;
    check_database (debman);
    debman->priv->db_watcher_cb =
        g_timeout_add (5000, (GSourceFunc) database_check_func,
                       (gpointer) debman);

    if (geteuid ()) {
        /* We can't really verify the status file or lock the database */
        return;
    }

    if (!verify_status (packman)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "Couldn't parse %s",
                              debman->priv->status_file);
    }
}

RCDebman *
rc_debman_new (void)
{
    RCDebman *debman;

    debman = RC_DEBMAN (g_object_new (rc_debman_get_type (), NULL));

    return debman;
}
