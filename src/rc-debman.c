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
#include "rc-deps.h"
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
#include <pty.h>
#include <dirent.h>
#include <limits.h>

#include <gdk/gdkkeysyms.h>

static void rc_debman_class_init (RCDebmanClass *klass);
static void rc_debman_init       (RCDebman *obj);

static RCPackmanClass *parent_class;

#define DEBMAN_DEFAULT_STATUS_FILE "/var/lib/dpkg/status"

/* The Evil Hack */
gboolean child_wants_input = FALSE;

guint
rc_debman_get_type (void)
{
    static guint type = 0;

    RC_ENTRY;

    if (!type) {
        GtkTypeInfo type_info = {
            "RCDebman",
            sizeof (RCDebman),
            sizeof (RCDebmanClass),
            (GtkClassInitFunc) rc_debman_class_init,
            (GtkObjectInitFunc) rc_debman_init,
            (GtkArgSetFunc) NULL,
            (GtkArgGetFunc) NULL,
        };

        type = gtk_type_unique (rc_packman_get_type (), &type_info);
    }

    RC_EXIT;

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

/*
  These two functions are used to cleanly destroy the hash table in an
  RCDebman, either after an rc_packman_transact, when we need to
  invalidate the hash table, or at object destroy
*/

static void
hash_destroy_pair (gchar *key, RCPackage *package)
{
    rc_package_free (package);
}

static void
hash_destroy (RCDebman *debman)
{
    RC_ENTRY;

    g_hash_table_freeze (debman->priv->package_hash);
    g_hash_table_foreach (debman->priv->package_hash,
                          (GHFunc) hash_destroy_pair, NULL);
    g_hash_table_destroy (debman->priv->package_hash);

    debman->priv->package_hash = g_hash_table_new (g_str_hash, g_str_equal);
    debman->priv->hash_valid = FALSE;

    RC_EXIT;
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

    RC_ENTRY;

    /* Developer hack */
    if (getenv ("RC_ME_EVEN_HARDER") || getenv ("RC_DEBMAN_STATUS_FILE")) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  __FUNCTION__ ": not checking lock file\n");

        RC_EXIT;

        return (TRUE);
    }

    /* Check for recursive lock */
    if (debman->priv->lock_fd != -1) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  __FUNCTION__ ": lock_fd is already %d, recursive lock?\n",
                  debman->priv->lock_fd);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "already holding lock");

        RC_EXIT;

        return (FALSE);
    }

    fd = open ("/var/lib/dpkg/lock", O_RDWR | O_CREAT | O_TRUNC, 0640);

    if (fd == -1) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  __FUNCTION__ ": couldn't open lock file\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't open lock file");

        RC_EXIT;

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

            RC_EXIT;

            return (FALSE);
        }
    }

    debman->priv->lock_fd = fd;

    rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": acquired lock file\n");

    RC_EXIT;

    return (TRUE);
}

static void
unlock_database (RCDebman *debman)
{
    RC_ENTRY;

    if (getenv ("RC_ME_EVEN_HARDER") || getenv ("RC_DEBMAN_STATUS_FILE")) {
        return;
    }

    if (!rc_close (debman->priv->lock_fd)) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  __FUNCTION__ ": close of lock_fd failed\n");
    }

    debman->priv->lock_fd = -1;

    rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": released lock file\n");

    RC_EXIT;
}

/*
  The associated detritus and cruft related to mark_purge.  Given a
  list of packages, we need to scan through /var/lib/dpkg/status, and
  change the "install ok installed" string to "purge ok installed", to
  let dpkg know that we're going to be removing this package soon.
*/

/*
  Since I only want to scan through /var/lib/dpkg/status once, I need
  a shorthand way to match potentially many strings.  Returns the name
  of the package that matched, or NULL if none did.
*/

static const gchar *
package_accept (gchar *line, RCPackageSList *packages)
{
    RCPackageSList *iter;
    gchar *name;

    RC_ENTRY;

    if (strncmp (line, "Package:", strlen ("Package:")) != 0) {
        return NULL;
    }

    name = line + strlen ("Package:");
    while (*name == ' ') {
        name++;
    }

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        if (!(strcmp (name, package->spec.name))) {
            rc_debug (RC_DEBUG_LEVEL_DEBUG,
                      __FUNCTION__ ": found package %s\n", package->spec.name);

            RC_EXIT;

            return (package->spec.name);
        }
    }

    RC_EXIT;

    return (NULL);
}

typedef struct _DebmanMarkPurgeInfo DebmanMarkPurgeInfo;

struct _DebmanMarkPurgeInfo {
    GMainLoop *loop;

    RCDebman *debman;

    guint read_line_id;
    guint read_done_id;

    int out_fd;

    RCPackageSList *remove_packages;

    gboolean rewrite;
    gboolean error;
};

static void
mark_purge_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanMarkPurgeInfo *mark_purge_info = (DebmanMarkPurgeInfo *)data;

    RC_ENTRY;

    /* If rewrite is set (we encountered a package we want to rewrite)
     * and this is a status line, write out our new status line */
    if (mark_purge_info->rewrite &&
        !strncasecmp (line, "Status:", strlen ("Status:")))
    {
        if (!rc_write (mark_purge_info->out_fd, "Status: purge ok installed\n",
                       strlen ("Status: purge ok installed\n")))
        {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": failed to write new status line, aborting\n");

            rc_packman_set_error
                (RC_PACKMAN (mark_purge_info->debman), RC_PACKMAN_ERROR_ABORT,
                 "write to %s failed",
                 mark_purge_info->debman->priv->rc_status_file);

            goto BROKEN;
        }

        mark_purge_info->rewrite = FALSE;

        return;
    }

    /* If the rewrite flag is still set, and we've gone through all
     * the fields and didn't encounter a status line (we got to a
     * blank line or another Package: line), something is really
     * wrong */
    if (mark_purge_info->rewrite &&
        ((line && !line[0]) ||
         (!strncasecmp (line, "Package:", strlen ("Package:")))))
    {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": package section had no Status: line, aborting\n");

        rc_packman_set_error (RC_PACKMAN (mark_purge_info->debman),
                              RC_PACKMAN_ERROR_ABORT,
                              "%s appears to be malformed",
                              mark_purge_info->debman->priv->status_file);

        goto BROKEN;
    }

    /* Nothing interesting is happening here, so just write out the
     * old data */
    if (!rc_write (mark_purge_info->out_fd, line, strlen (line)) ||
        !rc_write (mark_purge_info->out_fd, "\n", 1))
    {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to write old line, aborting\n");

        rc_packman_set_error (RC_PACKMAN (mark_purge_info->debman),
                              RC_PACKMAN_ERROR_ABORT,
                              "write to %s failed",
                              mark_purge_info->debman->priv->rc_status_file);

        goto BROKEN;
    }

    /* Check if this marks the beginning of a package whose status we
     * need to change */
    if (package_accept (line, mark_purge_info->remove_packages)) {
        mark_purge_info->rewrite = TRUE;
    }

    RC_EXIT;

    return;

  BROKEN:
    mark_purge_info->error = TRUE;

    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           mark_purge_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           mark_purge_info->read_done_id);

    g_main_quit (mark_purge_info->loop);

    RC_EXIT;
}

static void
mark_purge_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                         gpointer data)
{
    DebmanMarkPurgeInfo *mark_purge_info = (DebmanMarkPurgeInfo *)data;

    RC_ENTRY;

    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           mark_purge_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           mark_purge_info->read_done_id);

    g_main_quit (mark_purge_info->loop);

    RC_EXIT;
}

static gboolean
mark_purge (RCPackman *packman, RCPackageSList *remove_packages)
{
    RCDebman *debman = RC_DEBMAN (packman);
    DebmanMarkPurgeInfo mark_purge_info;
    GMainLoop *loop;
    int in_fd, out_fd;
    RCLineBuf *line_buf;

    RC_ENTRY;

    g_return_val_if_fail (remove_packages, TRUE);

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

    mark_purge_info.rewrite = FALSE;
    mark_purge_info.error = FALSE;
    mark_purge_info.out_fd = out_fd;
    mark_purge_info.remove_packages = remove_packages;
    mark_purge_info.loop = loop;
    mark_purge_info.debman = RC_DEBMAN (packman);

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, in_fd);

    mark_purge_info.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (mark_purge_read_line_cb),
                            (gpointer) &mark_purge_info);
    mark_purge_info.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (mark_purge_read_done_cb),
                            (gpointer) &mark_purge_info);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    close (in_fd);
    close (out_fd);

    if (mark_purge_info.error) {
        unlink (debman->priv->rc_status_file);

        RC_EXIT;

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

    RC_EXIT;

    return (TRUE);

  FAILED:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Unable to mark packages for removal");

    RC_EXIT;

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
    RC_ENTRY;

    child_wants_input = TRUE;

    fflush (stdout);

    RC_EXIT;
}

static gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    RC_ENTRY;

    if (event->keyval == GDK_Return) {
        g_main_quit ((GMainLoop *)data);

        RC_EXIT;

        return (FALSE);
    }

    RC_EXIT;

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

    RC_ENTRY;

    if (!child_wants_input) {
        RC_EXIT;

        return (TRUE);
    }

    check.fd = hack_info->master;
    check.events = POLLIN;

    if (poll (&check, 1, 0)) {
        RC_EXIT;

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

    RC_EXIT;

    return (TRUE);
}

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
        gtk_signal_emit_by_name (GTK_OBJECT (do_purge_info->packman),
                                 "transaction_step",
                                 ++do_purge_info->install_state->seqno,
                                 do_purge_info->install_state->total);

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

    gtk_signal_disconnect (GTK_OBJECT (line_buf), do_purge_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), do_purge_info->read_done_id);

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

    RC_ENTRY;

    if (!rc_file_exists ("/usr/bin/dpkg")) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": /usr/bin/dpkg does "
                  "not exist\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "/usr/bin/dpkg does not exist (suggest "
                              "'/usr/bin/dpkg --purge --pending' once dpkg "
                              "is installed)");

        RC_EXIT;

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

        RC_EXIT;

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

        putenv ("LD_PRELOAD=" SHAREDIR "/dpkg-helper.so");
        putenv (g_strdup_printf ("RC_READ_NOTIFY_PID=%d", parent));
        putenv ("PAGER=cat");

        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                  ": /usr/bin/dpkg --purge --pending\n");

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--purge", "--pending", NULL);

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
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (do_purge_read_line_cb),
                            (gpointer) &do_purge_info);
    do_purge_info.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (do_purge_read_done_cb),
                            (gpointer) &do_purge_info);
    do_purge_info.hack_info.poll_write_id =
        gtk_timeout_add (100,
                         (GtkFunction) debman_poll_write_cb,
                         (gpointer) &do_purge_info.hack_info);

    g_main_run (loop);

    gtk_timeout_remove (do_purge_info.hack_info.poll_write_id);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    g_string_free (do_purge_info.hack_info.buf, TRUE);

    close (master);

    waitpid (child, &status, 0);

    if (!(lock_database (RC_DEBMAN (packman)))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't reacquire lock file");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": lost database lock!\n");

        RC_EXIT;

        return (FALSE);
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dpkg exited abnormally");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": dpkg exited abnormally\n");

        RC_EXIT;

        return (FALSE);
    }

    RC_EXIT;

    return (TRUE);
}

/*
  All of the do_unpack flotsam.  Given a bunch of packages, calls dpkg
  --unpack on them in groups until they're all unpacked.  Once again,
  monitors dpkg output for useful strings.
*/

static GSList *
make_unpack_commands (GSList *commands, RCPackageSList *packages)
{
    GSList *package_iter;
    GSList *command_iter;

    GSList *ret = NULL;
    GArray *buf = NULL;
    guint buf_length = 0;

    RC_ENTRY;

    package_iter = packages;

    while (package_iter) {
        gchar *filename =
            g_strdup (((RCPackage *)(package_iter->data))->package_filename);
        guint filename_length = strlen (filename) + 1;

        if (!buf) {
            buf = g_array_new (TRUE, FALSE, sizeof (gchar *));
            buf_length = 0;

            for (command_iter = commands; command_iter;
                 command_iter = command_iter->next)
            {
                gchar *command = g_strdup ((gchar *)(command_iter->data));

                buf = g_array_append_val (buf, command);
                buf_length += strlen (command) + 1;
            }

            buf_length -= 1;
        }

        if (buf_length + filename_length > 4096) {
            ret = g_slist_append (ret, buf->data);

            g_array_free (buf, FALSE);
            buf = NULL;

            g_free (filename);
        } else {
            buf = g_array_append_val (buf, filename);
            buf_length += filename_length;

            package_iter = package_iter->next;
        }
    }

    if (buf) {
        ret = g_slist_append (ret, buf->data);

        g_array_free (buf, FALSE);
        buf = NULL;
    }

    RC_EXIT;

    return (ret);
}

typedef DebmanDoPurgeInfo DebmanDoUnpackInfo;

static void
do_unpack_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoUnpackInfo *do_unpack_info = (DebmanDoUnpackInfo *)data;

    RC_ENTRY;

    rc_debug (RC_DEBUG_LEVEL_DEBUG, __FUNCTION__ ": got \"%s\"\n", line);

    do_unpack_info->hack_info.buf =
        g_string_append (do_unpack_info->hack_info.buf, line);
    do_unpack_info->hack_info.buf =
        g_string_append (do_unpack_info->hack_info.buf, "\n");

    if (!strncmp (line, "Unpacking", strlen ("Unpacking")) ||
        !strncmp (line, "Purging", strlen ("Purging")))
    {
        gtk_signal_emit_by_name (GTK_OBJECT (do_unpack_info->packman),
                                 "transaction_step",
                                 ++do_unpack_info->install_state->seqno,
                                 do_unpack_info->install_state->total);

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

    RC_EXIT;
}

static void
do_unpack_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanDoUnpackInfo *do_unpack_info = (DebmanDoUnpackInfo *)data;

    RC_ENTRY;

    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           do_unpack_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           do_unpack_info->read_done_id);

    g_main_quit (do_unpack_info->loop);

    RC_EXIT;
}

static gboolean
do_unpack (RCPackman *packman, RCPackageSList *packages,
           DebmanInstallState *install_state)
{
    GSList *argvl = NULL;
    GSList *args = NULL;

    GSList *iter;

    RC_ENTRY;

    g_return_val_if_fail (g_slist_length (packages) > 0, TRUE);

    args = g_slist_append (args, g_strdup ("dpkg"));
    args = g_slist_append (args, g_strdup ("--unpack"));

    argvl = make_unpack_commands (args, packages);

    g_slist_foreach (args, (GFunc) g_free, NULL);
    g_slist_free (args);

    for (iter = argvl; iter; iter = iter->next) {
        gchar **argv = (gchar **)(iter->data);

        pid_t child;
        pid_t parent;
        int master, slave;
        int status;
        RCLineBuf *line_buf;
        GMainLoop *loop;
        DebmanDoUnpackInfo do_unpack_info;

        if (!rc_file_exists ("/usr/bin/dpkg")) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": /usr/bin/dpkg "
                      "does not exist\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "/usr/bin/dpkg does not exist (suggest "
                                  "'apt-get -f install')");

            RC_EXIT;

            return (FALSE);
        }

        openpty (&master, &slave, NULL, NULL, NULL);

        signal (SIGCHLD, SIG_DFL);
        signal (SIGPIPE, SIG_DFL);
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

            g_slist_foreach (argvl, (GFunc) g_strfreev, NULL);
            g_slist_free (argvl);

            RC_EXIT;

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

            putenv ("LD_PRELOAD=" SHAREDIR "/dpkg-helper.so");
            putenv (g_strdup_printf ("RC_READ_NOTIFY_PID=%d", parent));
            putenv ("PAGER=cat");

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
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                                GTK_SIGNAL_FUNC (do_unpack_read_line_cb),
                                (gpointer) &do_unpack_info);
        do_unpack_info.read_done_id =
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                                GTK_SIGNAL_FUNC (do_unpack_read_done_cb),
                                (gpointer) &do_unpack_info);
        do_unpack_info.hack_info.poll_write_id =
            gtk_timeout_add (100,
                             (GtkFunction) debman_poll_write_cb,
                             (gpointer) &do_unpack_info.hack_info);

        g_main_run (loop);

        gtk_timeout_remove (do_unpack_info.hack_info.poll_write_id);
            
        gtk_object_unref (GTK_OBJECT (line_buf));

        g_main_destroy (loop);

        g_string_free (do_unpack_info.hack_info.buf, TRUE);

        close (master);

        waitpid (child, &status, 0);

        if (!(lock_database (RC_DEBMAN (packman)))) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "couldn't reacquire lock file");

            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": lost database lock!\n");

            g_slist_foreach (argvl, (GFunc) g_strfreev, NULL);
            g_slist_free (argvl);

            RC_EXIT;

            return (FALSE);
        }

        if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "dpkg exited abnormally");

            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": dpkg exited abnormally\n");

            g_slist_foreach (argvl, (GFunc) g_strfreev, NULL);
            g_slist_free (argvl);

            RC_EXIT;

            return (FALSE);
        }
    }

    g_slist_foreach (argvl, (GFunc) g_strfreev, NULL);
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

    RC_ENTRY;

    rc_debug (RC_DEBUG_LEVEL_DEBUG, __FUNCTION__ ": got \"%s\"\n", line);

    do_configure_info->hack_info.buf =
        g_string_append (do_configure_info->hack_info.buf, line);
    do_configure_info->hack_info.buf =
        g_string_append (do_configure_info->hack_info.buf, "\n");

    if (!strncmp (line, "Setting up ", strlen ("Setting up "))) {
        gtk_signal_emit_by_name (GTK_OBJECT (do_configure_info->packman),
                                 "configure_step",
                                 ++do_configure_info->install_state->seqno,
                                 do_configure_info->install_state->total);

        do_configure_info->hack_info.buf =
            g_string_truncate (do_configure_info->hack_info.buf, 0);
    }

    RC_EXIT;
}

static void
do_configure_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                           gpointer data)
{
    DebmanDoConfigureInfo *do_configure_info = (DebmanDoConfigureInfo *)data;

    RC_ENTRY;

    gtk_signal_disconnect_by_func (GTK_OBJECT (line_buf),
                                   GTK_SIGNAL_FUNC (do_configure_read_line_cb),
                                   data);
    gtk_signal_disconnect_by_func (GTK_OBJECT (line_buf),
                                   GTK_SIGNAL_FUNC (do_configure_read_done_cb),
                                   data);

    g_main_quit (do_configure_info->loop);

    RC_EXIT;
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

    RC_ENTRY;

    if (!rc_file_exists ("/usr/bin/dpkg")) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": /usr/bin/dpkg "
                  "does not exist\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "/usr/bin/dpkg does not exist (suggest "
                              "'dpkg --configure --pending')");

        RC_EXIT;

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

        RC_EXIT;

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

        putenv ("LD_PRELOAD=" SHAREDIR "/dpkg-helper.so");
        putenv (g_strdup_printf ("RC_READ_NOTIFY_PID=%d", parent));
        putenv ("PAGER=cat");

        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                  ": /usr/bin/dpkg --configure --pending\n");

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--configure", "--pending",
               NULL);

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
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (do_configure_read_line_cb),
                            (gpointer) &do_configure_info);
    do_configure_info.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (do_configure_read_done_cb),
                            (gpointer) &do_configure_info);
    do_configure_info.hack_info.poll_write_id =
        gtk_timeout_add (100,
                         (GtkFunction) debman_poll_write_cb,
                         (gpointer) &do_configure_info.hack_info);

    g_main_run (loop);

    gtk_timeout_remove (do_configure_info.hack_info.poll_write_id);

    gtk_object_unref (GTK_OBJECT (line_buf));

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

        RC_EXIT;

        return (FALSE);
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dpkg exited abnormally");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": dpkg exited abnormally\n");

        RC_EXIT;

        return (FALSE);
    }

    RC_EXIT;

    return (TRUE);
}

/*
 * Combine the last four code blocks into a useful transaction.
 */

static void
rc_debman_transact (RCPackman *packman, RCPackageSList *install_packages,
                    RCPackageSList *remove_packages)
{
    DebmanInstallState *install_state = g_new0 (DebmanInstallState, 1);

    RC_ENTRY;

    install_state->total = g_slist_length (install_packages) +
        g_slist_length (remove_packages);

    if (remove_packages) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                  ": about to mark for purge\n");

        if (!(mark_purge (packman, remove_packages))) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": mark for purge failed\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "Unable to mark packages for removal");

            RC_EXIT;

            goto END;
        }
    }

    gtk_signal_emit_by_name (GTK_OBJECT (packman), "transaction_step", 0,
                             install_state->total);

    if (install_packages) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": about to unpack\n");

        if (!(do_unpack (packman, install_packages, install_state))) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": unpack failed\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "Unable to unpack selected packages " \
                                  "(suggest 'dpkg --unpack --pending')");

            RC_EXIT;

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

            RC_EXIT;

            goto END;
        }
    }

    gtk_signal_emit_by_name (GTK_OBJECT (packman), "transaction_done");

    if (install_packages) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": about to configure\n");

        install_state->total = g_slist_length (install_packages);
        install_state->seqno = 0;

        gtk_signal_emit_by_name (GTK_OBJECT (packman), "configure_step", 0,
                                 install_state->total);

        if (!(do_configure (packman, install_state))) {
            rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                      ": configure failed\n");

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "Unable to configure unpacked packages " \
                                  "(suggest 'dpkg --configure --pending')");

            RC_EXIT;

            goto END;
        }

        gtk_signal_emit_by_name (GTK_OBJECT (packman), "configure_done");
    }

    RC_EXIT;

  END:
    g_free (install_state);
    hash_destroy (RC_DEBMAN (packman));
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

    gboolean error;
};

static gchar *
get_description (GString *str)
{
    gchar *ret;

    RC_ENTRY;

    str = g_string_truncate (str, str->len - 1);
    ret = str->str;
    g_string_free (str, FALSE);

    RC_EXIT;

    return (ret);
}

static void
query_all_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanQueryInfo *query_info = (DebmanQueryInfo *)data;

    RC_ENTRY;

    /* This is a blank line which terminates a package section in
       /var/lib/dpkg/status, so save out our current buffer */
    if (!line[0]) {
        if (query_info->desc_buf && query_info->desc_buf->len) {
            query_info->package_buf->description =
                get_description (query_info->desc_buf);
            query_info->desc_buf = NULL;
        }

        query_info->packages = g_slist_append (query_info->packages,
                                           query_info->package_buf);
        query_info->package_buf = NULL;

        RC_EXIT;

        return;
    }

    /* This is the beginning of a package section, so start up a new buffer
       and set the package name */
    if (!strncmp (line, "Package:", strlen ("Package:"))) {
        /* Better make sure this isn't a malformed /var/lib/dpkg/status file,
           so check to make sure we cleanly finished the last package */
        if (query_info->package_buf) {
            query_info->error = TRUE;

            RC_EXIT;

            /* FIXME: must do something smart here */
            return;
        }

        query_info->package_buf = rc_package_new ();
        query_info->package_buf->spec.name =
            g_strdup (line + strlen ("Package:"));
        query_info->package_buf->spec.name =
            g_strchug (query_info->package_buf->spec.name);

        RC_EXIT;

        return;
    }

    /* Check if this is a Status: line -- the only status we recognize is
       "install ok installed".  RCDebman should reset any selections at
       initialization, so that we don't have to worry about things like
       "purge ok installed". */
    if (!strcmp (line, "Status: install ok installed")) {
        query_info->package_buf->installed = TRUE;

        RC_EXIT;

        return;
    }

    /* We now also recognize "hold ok installed", and we set the hold bit
       appropriately.  Yay for us! */
    if (!strcmp (line, "Status: hold ok installed")) {
        query_info->package_buf->installed = TRUE;
        query_info->package_buf->hold = TRUE;

        RC_EXIT;

        return;
    }

    if (!strncmp (line, "Installed-Size:", strlen ("Installed-Size:"))) {
        query_info->package_buf->installed_size =
            strtoul (line + strlen ("Installed-Size:"), NULL, 10) * 1024;

        RC_EXIT;

        return;
    }

    if (!strncmp (line, "Description:", strlen ("Description:"))) {
        query_info->package_buf->summary =
            g_strdup (line + strlen ("Description:"));
        query_info->package_buf->summary =
            g_strchug (query_info->package_buf->summary);

        /* After Description: comes the long description, so prepare
         * to suck that in */
        query_info->desc_buf = g_string_new (NULL);

        RC_EXIT;

        return;
    }

    if ((line[0] == ' ') && query_info->desc_buf) {
        if ((line[1] != '.') || line[2]) {
            query_info->desc_buf =
                g_string_append (query_info->desc_buf, line + 1);
        }

        query_info->desc_buf = g_string_append (query_info->desc_buf, "\n");

        RC_EXIT;

        return;
    }

    if (!strncmp (line, "Version:", strlen ("Version:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Version:")));

        rc_debman_parse_version (tmp, &query_info->package_buf->spec.epoch,
                                 &query_info->package_buf->spec.version,
                                 &query_info->package_buf->spec.release);

        g_free (tmp);

        RC_EXIT;

        return;
    }

    if (!strncmp (line, "Section:", strlen ("Section:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Section:")));
        gchar *section = strstr (tmp, "/");

        if (!section) {
            section = tmp;
        } else {
            while (*section == ' ') {
                section++;
            }
        }

        query_info->package_buf->section =
            rc_debman_section_to_package_section (tmp);

        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Depends:", strlen ("Depends:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Depends:")));
        query_info->package_buf->requires =
            rc_debman_fill_depends (tmp, query_info->package_buf->requires);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Recommends:", strlen ("Recommends:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Recommends:")));
        query_info->package_buf->recommends =
            rc_debman_fill_depends (tmp, query_info->package_buf->recommends);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Suggests:", strlen ("Suggests:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Suggests:")));
        query_info->package_buf->suggests =
            rc_debman_fill_depends (tmp, query_info->package_buf->suggests);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Pre-Depends:", strlen ("Pre-Depends:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Pre-Depends:")));
        query_info->package_buf->requires =
            rc_debman_fill_depends (tmp, query_info->package_buf->requires);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Conflicts:", strlen ("Conflicts:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Conflicts:")));
        query_info->package_buf->conflicts =
            rc_debman_fill_depends (tmp, query_info->package_buf->conflicts);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Provides:", strlen ("Provides:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Provides:")));
        query_info->package_buf->provides =
            rc_debman_fill_depends (tmp, query_info->package_buf->provides);
        g_free (tmp);
        return;
    }
}

static void
query_all_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanQueryInfo *query_info = (DebmanQueryInfo *)data;

    RC_ENTRY;

    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           query_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           query_info->read_done_id);

    query_info->error = (status == RC_LINE_BUF_ERROR);

    g_main_quit (query_info->loop);

    RC_EXIT;
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

    RC_ENTRY;

    if ((fd = open (debman->priv->status_file, O_RDONLY)) < 0) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't open %s for reading",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open /var/lib/dpkg/status\n");

        RC_EXIT;

        return;
    }

    loop = g_main_new (FALSE);

    query_info.packages = NULL;
    query_info.package_buf = NULL;
    query_info.desc_buf = NULL;
    query_info.error = FALSE;
    query_info.loop = loop;

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, fd);

    query_info.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (query_all_read_line_cb),
                            (gpointer) &query_info);
    query_info.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (query_all_read_done_cb),
                            (gpointer) &query_info);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    for (iter = query_info.packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        if (package->installed) {
            package->provides =
                g_slist_append (package->provides,
                                rc_package_dep_new_with_item (
                                    rc_package_dep_item_new_from_spec (
                                        &package->spec,
                                        RC_RELATION_EQUAL)));
            g_hash_table_insert (debman->priv->package_hash,
                                 (gpointer) package->spec.name,
                                 (gpointer) package);
        } else {
            rc_package_free (package);
        }
    }

    g_slist_free (query_info.packages);

    debman->priv->hash_valid = TRUE;

    close (fd);

    RC_EXIT;
}

static void
package_list_append (gchar *name, RCPackage *package,
                     RCPackageSList **package_list)
{
    RC_ENTRY;

    *package_list = g_slist_append (*package_list, rc_package_copy (package));

    RC_EXIT;
}

static RCPackageSList *
rc_debman_query_all (RCPackman *packman)
{
    RCPackageSList *packages = NULL;

    RC_ENTRY;

    if (!(RC_DEBMAN (packman)->priv->hash_valid)) {
        rc_debman_query_all_real (packman);
    }

    g_hash_table_foreach (RC_DEBMAN (packman)->priv->package_hash,
                          (GHFunc) package_list_append, &packages);

    RC_EXIT;

    return packages;
}

static RCPackage *
rc_debman_query (RCPackman *packman, RCPackage *package)
{
    RCDebman *debman = RC_DEBMAN (packman);
    RCPackage *lpackage;

    RC_ENTRY;

    g_assert (package);
    g_assert (package->spec.name);

    if (!debman->priv->hash_valid) {
        rc_debman_query_all_real (packman);
    }

    lpackage = g_hash_table_lookup (debman->priv->package_hash,
                                    (gconstpointer) package->spec.name);

    if (lpackage &&
        (!package->spec.version ||
         !strcmp (lpackage->spec.version, package->spec.version)) &&
        (!package->spec.release ||
         !strcmp (lpackage->spec.release, package->spec.release)))
    {
        rc_package_free (package);

        RC_EXIT;

        return (rc_package_copy (lpackage));
    } else {
        RC_EXIT;

        return (package);
    }
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
    RCPackageDepItem *dep_item;
    RCPackageDep *dep = NULL;

    RC_ENTRY;

    if (pipe (fds)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't create pipe");

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": pipe failed\n");

        RC_EXIT;

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

        RC_EXIT;

        return (NULL);

    case 0:
        close (fds[0]);

        fflush (stdout);
        dup2 (fds[1], STDOUT_FILENO);
        close (fds[1]);

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
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (query_all_read_line_cb),
                            (gpointer) &query_info);
    query_info.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (query_all_read_done_cb),
                            (gpointer) &query_info);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

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

        RC_EXIT;

        return (NULL);
    }

    if (query_info.desc_buf) {
        query_info.package_buf->description =
            get_description (query_info.desc_buf);

        query_info.desc_buf = NULL;
    }

    /* Make the package provide itself (that's what we like to do,
           anyway) */
    dep_item = rc_package_dep_item_new_from_spec (
        (RCPackageSpec *)query_info.package_buf,
        RC_RELATION_EQUAL);

    dep = g_slist_append (dep, dep_item);

    query_info.package_buf->provides =
        g_slist_append (query_info.package_buf->provides, dep);

    RC_EXIT;

    return (query_info.package_buf);
}

typedef struct _DebmanVerifyStatusInfo DebmanVerifyStatusInfo;

struct _DebmanVerifyStatusInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    int out_fd;

    gboolean error;
};

static void
verify_status_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    gchar **tokens = NULL;
    gchar *tmp = NULL;
    DebmanVerifyStatusInfo *verify_status_info =
        (DebmanVerifyStatusInfo *)data;
    int out_fd = verify_status_info->out_fd; /* Just for sanity */

    RC_ENTRY;

    if (strncmp (line, "Status:", strlen ("Status:"))) {
        /* This isn't a status line, so we don't need to do anything other than
           to write it to the new file */
        if (!rc_write (out_fd, line, strlen (line)) ||
            !rc_write (out_fd, "\n", 1))
        {
            goto BROKEN;
        }
        RC_EXIT;
        return;
    }

    tmp = g_strdup (line + strlen ("Status:"));
    tmp = g_strchug (tmp);

    tokens = g_strsplit (tmp, " ", 3);

    if (tokens[3] || !tokens[2] || !tokens[1] || !tokens[0]) {
        /* There aren't exactly 3 tokens here */
        goto BROKEN;
    }

    /* we want the status to be ok */
    if (strcmp (tokens[1], "ok")) {
        goto BROKEN;
    }

    /* installed, not-installed, and config-files are ok, everything else I
       consider problematic */
    if (strcmp (tokens[2], "installed") &&
        strcmp (tokens[2], "not-installed") &&
        strcmp (tokens[2], "config-files"))
    {
        goto BROKEN;
    }

    /* If the package is installed, set the selection to install, and use
       whatever middle token the user had */
    if (!strcmp (tokens[2], "installed")) {
        if (!strcmp (tokens[0], "install") || !strcmp (tokens[0], "hold")) {
            if (!rc_write (out_fd, line, strlen (line)) ||
                !rc_write (out_fd, "\n", 1))
            {
                goto BROKEN;
            }
            goto END;
        }
        if (!rc_write (out_fd, "Status: install ",
                       strlen ("Status: install ")) ||
            !rc_write (out_fd, tokens[1], strlen (tokens[1])) ||
            !rc_write (out_fd, " installed\n", strlen (" installed\n")))
        {
            goto BROKEN;
        }
        goto END;
    }

    /* If the package is not-installed, just write the normal line if the
       selection was purge or deinstall, otherwise set the seletion to purge.
       Of course, use whatever middle token the user had. */
    if (!strcmp (tokens[2], "not-installed")) {
        if (!strcmp (tokens[0], "purge")) {
            if (!rc_write (out_fd, line, strlen (line)) ||
                !rc_write (out_fd, "\n", 1))
            {
                goto BROKEN;
            }
            goto END;
        }
        if (!strcmp (tokens[0], "deinstall")) {
            if (!rc_write (out_fd, line, strlen (line)) ||
                !rc_write (out_fd, "\n", 1))
            {
                goto BROKEN;
            }
            goto END;
        }
        if (!rc_write (out_fd, "Status: purge ", strlen ("Status: purge ")) ||
            !rc_write (out_fd, tokens[1], strlen (tokens[1])) ||
            !rc_write (out_fd, " not-installed\n",
                       strlen (" not-installed\n")))
        {
            goto BROKEN;
        }
        goto END;
    }

    /* If the package is config-files only, set the selection to deinstall,
       middle token as the user had it. */
    if (!strcmp (tokens[2], "config-files")) {
        if (!rc_write (out_fd, "Status: deinstall ",
                       strlen ("Status: deinstall ")) ||
            !rc_write (out_fd, tokens[1], strlen (tokens[1])) ||
            !rc_write (out_fd, " config-files\n", strlen (" config-files\n")))
        {
            goto BROKEN;
        }
        goto END;
    }

  BROKEN:
    verify_status_info->error = TRUE;
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           verify_status_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           verify_status_info->read_done_id);
    g_main_quit (verify_status_info->loop);

  END:
    g_strfreev (tokens);
    g_free (tmp);

    RC_EXIT;
}

static void
verify_status_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                            gpointer data)
{
    DebmanVerifyStatusInfo *verify_status_info =
        (DebmanVerifyStatusInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           verify_status_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
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

    RC_ENTRY;

    if ((in_fd = open (debman->priv->status_file, O_RDONLY)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't open %s for reading",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open %s for reading\n",
                  debman->priv->status_file);

        RC_EXIT;

        return (FALSE);
    }

    if ((out_fd = creat (debman->priv->rc_status_file, 0644)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't open %s for writing",
                              debman->priv->rc_status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": failed to open %s for writing\n",
                  debman->priv->rc_status_file);

        close (in_fd);

        RC_EXIT;

        return (FALSE);
    }

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, in_fd);

    verify_status_info.out_fd = out_fd;
    verify_status_info.error = FALSE;

    loop = g_main_new (FALSE);
    verify_status_info.loop = loop;

    verify_status_info.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (verify_status_read_line_cb),
                            (gpointer) &verify_status_info);
    verify_status_info.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (verify_status_read_done_cb),
                            (gpointer) &verify_status_info);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    close (in_fd);
    close (out_fd);

    if (verify_status_info.error) {
        unlink ("/var/lib/dpkg/status.redcarpet");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "The %s file is malformed or contains errors",
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": couldn't rename /var/lib/dpkg/status.redcarpet\n");

        RC_EXIT;

        return (FALSE);
    }

    if (rename ("/var/lib/dpkg/status.redcarpet", "/var/lib/dpkg/status")) {
        unlink ("/var/lib/dpkg/status.redcarpet");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "couldn't rename %s to %s",
                              debman->priv->rc_status_file,
                              debman->priv->status_file);

        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": couldn't rename /var/lib/dpkg/status.redcarpet\n");

        RC_EXIT;

        return (FALSE);
    }

    RC_EXIT;

    return (TRUE);
}

static RCVerificationSList *
rc_debman_verify (RCPackman *packman, RCPackage *package)
{
    RCVerificationSList *ret = NULL;
    RCPackageUpdate *update = (RCPackageUpdate *)package->history->data;

    RC_ENTRY;

    g_assert (packman);
    g_assert (package);
    g_assert (package->package_filename);

    if (update->md5sum) {
        RCVerification *verification;

        verification = rc_verify_md5_string (package->package_filename,
                                             update->md5sum);

        ret = g_slist_append (ret, verification);
    }

    if (package->signature_filename) {
        RCVerification *verification;

        verification = rc_verify_gpg (package->package_filename,
                                      package->signature_filename);

        ret = g_slist_append (ret, verification);
    }

    RC_EXIT;

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

    RC_ENTRY;

    if (!strcmp (find_file_info->target, line)) {
        find_file_info->accept = TRUE;

        gtk_signal_disconnect (GTK_OBJECT (line_buf),
                               find_file_info->read_line_id);
        gtk_signal_disconnect (GTK_OBJECT (line_buf),
                               find_file_info->read_done_id);

        g_main_quit (find_file_info->loop);
    }

    RC_EXIT;
}

static void
find_file_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanFindFileInfo *find_file_info = (DebmanFindFileInfo *)data;

    RC_ENTRY;

    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           find_file_info->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf),
                           find_file_info->read_done_id);

    g_main_quit (find_file_info->loop);

    RC_EXIT;
}

static RCPackage *
rc_debman_find_file (RCPackman *packman, const gchar *filename)
{
    DIR *info_dir;
    struct dirent *info_file;
    gchar realname[PATH_MAX];

    RC_ENTRY;

    if (!g_path_is_absolute (filename)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": pathname is not absolute\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "pathname is not absolute");

        RC_EXIT;

        goto ERROR;
    }

    if (!realpath (filename, realname)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": realpath returned NULL\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "realpath returned NULL");

        RC_EXIT;

        goto ERROR;
    }

    if (!(info_dir = opendir ("/var/lib/dpkg/info"))) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": opendir /var/lib/dpkg/info failed\n");

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to scan /var/lib/dpkg/info");

        RC_EXIT;

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

            RC_EXIT;

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
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                                GTK_SIGNAL_FUNC (find_file_read_line_cb),
                                (gpointer) &find_file_info);
        find_file_info.read_done_id =
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                                GTK_SIGNAL_FUNC (find_file_read_done_cb),
                                (gpointer) &find_file_info);

        g_main_run (loop);

        gtk_object_unref (GTK_OBJECT (line_buf));

        g_main_destroy (loop);

        close (fd);

        if (find_file_info.accept) {
            RCPackage *package;

            package = rc_package_new ();

            package->spec.name = g_strndup (info_file->d_name, length - 5);

            closedir (info_dir);

            package = rc_packman_query (packman, package);

            if (!package->installed) {
                rc_package_free (package);

                RC_EXIT;

                return (NULL);
            } else {
                RC_EXIT;

                return (package);
            }
        }
    }

    closedir (info_dir);

    RC_EXIT;

    return (NULL);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "find_file failed");

    return (NULL);
}

static void
rc_debman_destroy (GtkObject *obj)
{
    RCDebman *debman = RC_DEBMAN (obj);

    RC_ENTRY;

    unlock_database (debman);

    hash_destroy (debman);

    g_free (debman->priv);

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (obj);

    RC_EXIT;
}

static void
rc_debman_class_init (RCDebmanClass *klass)
{
    GtkObjectClass *object_class =  (GtkObjectClass *) klass;
    RCPackmanClass *packman_class = (RCPackmanClass *) klass;

    RC_ENTRY;

    object_class->destroy = rc_debman_destroy;

    parent_class = gtk_type_class (rc_packman_get_type ());

    packman_class->rc_packman_real_transact = rc_debman_transact;
    packman_class->rc_packman_real_query = rc_debman_query;
    packman_class->rc_packman_real_query_file = rc_debman_query_file;
    packman_class->rc_packman_real_query_all = rc_debman_query_all;
    packman_class->rc_packman_real_verify = rc_debman_verify;
    packman_class->rc_packman_real_find_file = rc_debman_find_file;
    packman_class->rc_packman_real_version_compare = rc_debman_version_compare;

    putenv ("DEBIAN_FRONTEND=noninteractive");

    deps_conflicts_use_virtual_packages (FALSE);

    RC_EXIT;
}

static void
rc_debman_init (RCDebman *debman)
{
    RCPackman *packman = RC_PACKMAN (debman);

    packman->priv->features =
        RC_PACKMAN_FEATURE_POST_CONFIG;

    debman->priv = g_new0 (RCDebmanPrivate, 1);

    debman->priv->lock_fd = -1;

    debman->priv->package_hash = g_hash_table_new (g_str_hash, g_str_equal);
    debman->priv->hash_valid = FALSE;

    if (getenv ("RC_DEBMAN_STATUS_FILE")) {
        debman->priv->status_file = getenv ("RC_DEBMAN_STATUS_FILE");
        debman->priv->rc_status_file =
            g_strconcat (debman->priv->status_file, ".redcarpet", NULL);
    } else {
        debman->priv->status_file = g_strdup (DEBMAN_DEFAULT_STATUS_FILE);
        debman->priv->rc_status_file =
            g_strdup (DEBMAN_DEFAULT_STATUS_FILE ".redcarpet");
    }

    if (geteuid ()) {
        /* We can't really verify the status file or lock the database */
        RC_EXIT;

        return;
    }

    if (!lock_database (debman)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "Unable to lock database");
    } else {
        if (!verify_status (packman)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "Couldn't parse %s",
                                  debman->priv->status_file);
        }
    }

    RC_EXIT;
}

RCDebman *
rc_debman_new (void)
{
    RCDebman *debman;

    RC_ENTRY;

    debman = RC_DEBMAN (gtk_type_new (rc_debman_get_type ()));

    RC_EXIT;

    return debman;
}
