/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
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

#include "rc-packman-private.h"
#include "rc-debman.h"
#include "rc-debman-private.h"
#include "rc-common.h"
#include "rc-util.h"
#include "rc-line-buf.h"
#include "deps.h"

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

static RCPackmanClass *rc_debman_parent;

#define DEBMAN_DEFAULT_STATUS_FILE	"/var/lib/dpkg/status"

static char *status_file = NULL;
static char *rc_status_file = NULL;

/* The evil evil evil evil global variable needed to do the get input hack */
gboolean child_wants_input = FALSE;

guint
rc_debman_get_type (void)
{
    static guint type = 0;

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

    return type;
}

/* Nothing but a quick hack for debugging purposes */

static void
dump_argv (gchar **argv)
{
    gchar **iter;

    fprintf (stderr, "DEBMAN:");

    for (iter = argv; *iter; iter++) {
        fprintf (stderr, " %s", *iter);
    }

    fprintf (stderr, "\n");
}

/*
  These functions are used to cleanly destroy the hash table in an RCDebman,
  either after an rc_packman_transact, when we need to invalidate the hash
  table, or at object destroy
*/

static void
hash_destroy_pair (gchar *key, RCPackage *package)
{
    rc_package_free (package);
}

static void
hash_destroy (RCDebman *debman)
{
    g_hash_table_freeze (debman->priv->pkg_hash);
    g_hash_table_foreach (debman->priv->pkg_hash,
                          (GHFunc) hash_destroy_pair, NULL);
    g_hash_table_destroy (debman->priv->pkg_hash);
    debman->priv->pkg_hash = g_hash_table_new (g_str_hash, g_str_equal);
    debman->priv->hash_valid = FALSE;
}

/*
  Lock the dpkg database.  Too bad this just doesn't work (and doesn't work
  in apt or dselect either).  Releasing the lock to call dpkg and trying to
  grab it afterwords is probably the most broken thing I've ever heard of.
*/

static gboolean
lock_database (RCDebman *debman)
{
    int fd;
    struct flock fl;

    if (getenv ("RC_ME_EVEN_HARDER") || getenv ("RC_DEBMAN_STATUS_FILE")) {
        return TRUE;
    }

    g_return_val_if_fail (debman->priv->lock_fd == -1, 0);

    fd = open ("/var/lib/dpkg/lock", O_RDWR | O_CREAT | O_TRUNC, 0640);

    if (fd < 0) {
        rc_packman_set_error (RC_PACKMAN (debman), RC_PACKMAN_ERROR_FATAL,
                              "Unable to lock /var/lib/dpkg/lock.  Perhaps "
                              "another process is accessing the dpkg "
                              "database?");
        return (FALSE);
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl (fd, F_SETLK, &fl) == -1) {
        if (errno != ENOLCK) {
            rc_packman_set_error (RC_PACKMAN (debman), RC_PACKMAN_ERROR_FATAL,
                                  "Unable to lock /var/lib/dpkg/status. "
                                  "Perhaps another process is accessing the "
                                  "dpkg database?");
            close (fd);
            return (FALSE);
        }
    }

    debman->priv->lock_fd = fd;

    return (TRUE);
}

static void
unlock_database (RCDebman *debman)
{
    if (getenv ("RC_ME_EVEN_HARDER") || getenv ("RC_DEBMAN_STATUS_FILE")) {
        return;
    }

    close (debman->priv->lock_fd);
    debman->priv->lock_fd = -1;
}

/*
  Since I only want to scan through /var/lib/dpkg/status once, I need a
  shorthand way to match potentially many strings.  Returns the name of the
  package that matched, or NULL if none did.
*/

static const gchar *
package_accept (gchar *line, RCPackageSList *packages)
{
    RCPackageSList *iter;
    gchar *name;

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
            return (package->spec.name);
        }
    }

    return (NULL);
}

/*
  The associated detritus and cruft related to mark_purge.  Given a list of
  packages, we need to scan through /var/lib/dpkg/status, and change the
  "install ok installed" string to "purge ok installed", to let dpkg know that
  we're going to be removing this package soon.
*/

typedef struct _DebmanMarkPurgeInfo DebmanMarkPurgeInfo;

struct _DebmanMarkPurgeInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    int out_fd;

    RCPackageSList *remove_pkgs;

    gboolean rewrite;
    gboolean error;
};

static void
mark_purge_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanMarkPurgeInfo *dmpi = (DebmanMarkPurgeInfo *)data;

    /* Is this a status line we want to fixicate? */
    if (dmpi->rewrite && !strncmp (line, "Status:", strlen ("Status:"))) {
        if (!rc_write (dmpi->out_fd, "Status: purge ok installed\n",
                       strlen ("Status: purge ok installed\n"))) {
            goto BROKEN;
        }

        dmpi->rewrite = FALSE;

        return;
    }

    if (dmpi->rewrite &&
        ((line && !line[0]) ||
         (!strncmp (line, "Package:", strlen ("Package:")))))
    {
        /* FIXME: somehow this is malformed */
        goto BROKEN;

        return;
    }

    if (!rc_write (dmpi->out_fd, line, strlen (line)) ||
        !rc_write (dmpi->out_fd, "\n", 1))
    {
        goto BROKEN;
    }

    if (package_accept (line, dmpi->remove_pkgs)) {
        dmpi->rewrite = TRUE;
    }

    return;

  BROKEN:
    dmpi->error = TRUE;
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dmpi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dmpi->read_done_id);
    g_main_quit (dmpi->loop);
}

static void
mark_purge_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                         gpointer data)
{
    DebmanMarkPurgeInfo *dmpi = (DebmanMarkPurgeInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (line_buf), dmpi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dmpi->read_done_id);

    g_main_quit (dmpi->loop);
}

static gboolean
mark_purge (RCPackman *p, RCPackageSList *remove_pkgs)
{
    DebmanMarkPurgeInfo dmpi;
    GMainLoop *loop;
    int in_fd, out_fd;
    RCLineBuf *line_buf;

    g_return_val_if_fail (remove_pkgs, TRUE);

    if ((in_fd = open (status_file, O_RDONLY)) == -1) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open /var/lib/dpkg/status for "
                              "reading");
        return (FALSE);
    }

    if ((out_fd = creat (rc_status_file, 0644)) == -1) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open /var/lib/dpkg/status.redcarpet "
                              "for writing");
        close (in_fd);
        return (FALSE);
    }

    loop = g_main_new (FALSE);

    dmpi.rewrite = FALSE;
    dmpi.error = FALSE;
    dmpi.out_fd = out_fd;
    dmpi.remove_pkgs = remove_pkgs;
    dmpi.loop = loop;

    line_buf = rc_line_buf_new (in_fd);

    dmpi.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (mark_purge_read_line_cb),
                            (gpointer) &dmpi);
    dmpi.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (mark_purge_read_done_cb),
                            (gpointer) &dmpi);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    close (in_fd);
    close (out_fd);

    if (dmpi.error) {
        unlink (rc_status_file);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "The application was unable to mark the "
                              "selected packages for removal on your system.");
        return (FALSE);
    }

    if (rename (rc_status_file, status_file)) {
        unlink (rc_status_file);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to rename RC status file");
        return (FALSE);
    }

    return (TRUE);
}

/*
  This little guy gets remembered through the transaction, so that I can emit
  signals with the appropriate values when necessary
*/

typedef struct _DebmanInstallState DebmanInstallState;

struct _DebmanInstallState {
    guint seqno;
    guint total;
};

/*
  All of the crap to handle do_purge.  Basically just fork dpkg --purge
  --pending, and sniff the output for any interesting strings
*/

struct _DebmanDoPurgeInfo {
    GMainLoop *loop;

    guint read_line_id;
    guint read_done_id;

    RCPackman *packman;

    DebmanInstallState *dis;
};

typedef struct _DebmanDoPurgeInfo DebmanDoPurgeInfo;

static void
do_purge_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoPurgeInfo *ddpi = (DebmanDoPurgeInfo *)data;

    fprintf (stderr, "DEBMAN: %s\n", line);

    if (!strncmp (line, "Removing", strlen ("Removing"))) {
        gtk_signal_emit_by_name (GTK_OBJECT (ddpi->packman),
                                 "transaction_step", ++ddpi->dis->seqno,
                                 ddpi->dis->total);
    }
}

static void
do_purge_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                       gpointer data)
{
    DebmanDoPurgeInfo *ddpi = (DebmanDoPurgeInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (line_buf), ddpi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), ddpi->read_done_id);

    g_main_quit (ddpi->loop);
}

static gboolean
do_purge (RCPackman *packman, DebmanInstallState *dis)
{
    pid_t child;
    int status;
    int fds[2];
    RCLineBuf *line_buf;
    DebmanDoPurgeInfo ddpi;
    GMainLoop *loop;

    pipe (fds);
    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    signal (SIGCHLD, SIG_DFL);

    /* dpkg is going to need to grab this lock (sigh...) */
    unlock_database (RC_DEBMAN (packman));

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FAIL,
                              "Unable to fork dpkg to finish purging selected "
                              "packages.  Packages were marked for removal, "
                              "but were not removed.  To remedy this, run the "
                              "command 'dpkg --purge --pending' as root.");
        close (fds[0]);
        close (fds[1]);
        return (FALSE);
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL);

        fflush (stdout);
        close (fds[0]);
        dup2 (fds[1], STDOUT_FILENO);

        fprintf (stderr, "DEBMAN: /usr/bin/dpkg --purge --pending\n");

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--purge", "--pending", NULL);
        break;

    default:
        break;
    }

    close (fds[1]);

    loop = g_main_new (FALSE);

    ddpi.packman = packman;
    ddpi.loop = loop;
    ddpi.dis = dis;

    line_buf = rc_line_buf_new (fds[0]);

    ddpi.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (do_purge_read_line_cb),
                            (gpointer) &ddpi);
    ddpi.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (do_purge_read_done_cb),
                            (gpointer) &ddpi);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    close (fds[0]);

    waitpid (child, &status, 0);

    if (!(lock_database (RC_DEBMAN (packman)))) {
        /* FIXME: what to do here? */
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FAIL,
                              "Unable to run dpkg to finish purging selected "
                              "packages.  Packages were marked for removal, "
                              "but were not removed.  To remedy this, run the "
                              "command 'dpkg --purge --pending' as root.");
        return (FALSE);
    }

    return (TRUE);
}

/*
  All of the do_unpack flotsam.  Given a bunch of packages, calls dpkg
  --unpack on them in groups until they're all unpacked.  Once again, monitors
  dpkg output for useful strings.
*/

static GSList *
make_unpack_commands (GSList *commands, RCPackageSList *packages)
{
    GSList *package_iter;
    GSList *command_iter;

    GSList *ret = NULL;
    GArray *buf = NULL;
    guint buf_length = 0;

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

    return (ret);
}

typedef DebmanDoPurgeInfo DebmanDoUnpackInfo;

static void
do_unpack_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoUnpackInfo *ddui = (DebmanDoUnpackInfo *)data;

    fprintf (stderr, "DEBMAN: %s\n", line);

    if (!strncmp (line, "Unpacking", strlen ("Unpacking")) ||
        !strncmp (line, "Purging", strlen ("Purging")))
    {
        gtk_signal_emit_by_name (GTK_OBJECT (ddui->packman),
                                 "transaction_step", ++ddui->dis->seqno,
                                 ddui->dis->total);
    }
}

static void
do_unpack_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                        gpointer data)
{
    DebmanDoUnpackInfo *ddui = (DebmanDoUnpackInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (line_buf), ddui->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), ddui->read_done_id);

    g_main_quit (ddui->loop);
}

static gboolean
do_unpack (RCPackman *packman, RCPackageSList *packages,
           DebmanInstallState *dis)
{
    GSList *argvl = NULL;
    GSList *args = NULL;

    GSList *iter;

    g_return_val_if_fail (g_slist_length (packages) > 0, TRUE);

    args = g_slist_append (args, g_strdup ("dpkg"));
    args = g_slist_append (args, g_strdup ("--unpack"));

    argvl = make_unpack_commands (args, packages);

    g_slist_foreach (args, (GFunc) g_free, NULL);
    g_slist_free (args);

    for (iter = argvl; iter; iter = iter->next) {
        gchar **argv = (gchar **)(iter->data);

        int fds[2];
        pid_t child;
        int status;
        RCLineBuf *line_buf;
        GMainLoop *loop;
        DebmanDoUnpackInfo ddui;

        pipe (fds);
        fcntl (fds[0], F_SETFL, O_NONBLOCK);
        fcntl (fds[1], F_SETFL, O_NONBLOCK);

        /* The locking in Debian sucks ass */
        unlock_database (RC_DEBMAN (packman));

        signal (SIGCHLD, SIG_DFL);

        child = fork ();

        switch (child) {
        case -1:
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "Unable to fork and exec dpkg to unpack "
                                  "selected packages.");
            close (fds[0]);
            close (fds[1]);

            g_slist_foreach (argvl, (GFunc) g_strfreev, NULL);
            g_slist_free (argvl);

            return (FALSE);
            break;

        case 0:
            signal (SIGPIPE, SIG_DFL); /* dpkg can bite me */

            close (fds[0]);

            fflush (stdout);
            dup2 (fds[1], STDOUT_FILENO);
            close (fds[1]);

            dump_argv (argv);

            fclose (stderr);

            execv ("/usr/bin/dpkg", argv);
            break;

        default:
            break;
        }

        close (fds[1]);

        loop = g_main_new (FALSE);

        ddui.packman = packman;
        ddui.loop = loop;
        ddui.dis = dis;

        line_buf = rc_line_buf_new (fds[0]);

        ddui.read_line_id =
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                                GTK_SIGNAL_FUNC (do_unpack_read_line_cb),
                                (gpointer) &ddui);
        ddui.read_done_id =
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                                GTK_SIGNAL_FUNC (do_unpack_read_done_cb),
                                (gpointer) &ddui);

        g_main_run (loop);
            
        gtk_object_unref (GTK_OBJECT (line_buf));

        g_main_destroy (loop);

        close (fds[0]);

        waitpid (child, &status, 0);

        if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
            gchar *error;
            RCPackageSList *iter;

            error = g_strdup ("dpkg failed to unpack all of the selected "
                              "packages.  The following packages may have "
                              "been left in an inconsistant state, and "
                              "should be checked by hand:");

            for (iter = packages; iter; iter = iter->next) {
                gchar *name =
                    ((RCPackage *)(iter->data))->spec.name;
                gchar *tmp = g_strconcat (error, " ", name, NULL);

                g_free (error);
                error = tmp;
            }

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FAIL, error);

            g_free (error);

            return (FALSE);
        }

        if (!(lock_database (RC_DEBMAN (packman)))) {
            /* FIXME: need to handle this */
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
    guint poll_write_id;

    RCPackman *pman;
    RCLineBuf *line_buf;
    DebmanInstallState *dis;
    GString *buf;
    int write_fd;
};

static void
do_configure_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanDoConfigureInfo *ddci = (DebmanDoConfigureInfo *)data;

    fprintf (stderr, "DEBMAN: %s\n", line);

    ddci->buf = g_string_append (ddci->buf, line);
    ddci->buf = g_string_append (ddci->buf, "\n");

    if (!strncmp (line, "Setting up ", strlen ("Setting up "))) {
        gtk_signal_emit_by_name (GTK_OBJECT (ddci->pman), "configure_step",
                                 ++ddci->dis->seqno, ddci->dis->total);
        g_string_free (ddci->buf, TRUE);
        ddci->buf = g_string_new (NULL);
    }
}

static void
do_configure_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                           gpointer data)
{
    DebmanDoConfigureInfo *ddci = (DebmanDoConfigureInfo *)data;

    gtk_signal_disconnect_by_func (GTK_OBJECT (line_buf),
                                   GTK_SIGNAL_FUNC (do_configure_read_line_cb),
                                   data);
    gtk_signal_disconnect_by_func (GTK_OBJECT (line_buf),
                                   GTK_SIGNAL_FUNC (do_configure_read_done_cb),
                                   data);

    g_main_quit (ddci->loop);
}

static void
debman_do_configure_sigusr2_cb (int signum)
{
    child_wants_input = TRUE;
    fflush (stdout);
}

static gboolean
key_press_cb (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
    printf ("key_press_cb\n");

    if (event->keyval == GDK_Return) {
        g_main_quit ((GMainLoop *)data);
        return (FALSE);
    }

    return (TRUE);
}

static gint
debman_do_configure_poll_write_cb (gpointer data)
{
    GMainLoop *loop;
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *label;
    GtkWidget *vbox;
    DebmanDoConfigureInfo *ddci = (DebmanDoConfigureInfo *)data;
    gchar *line;
    struct pollfd check;

    if (!child_wants_input) {
        return (TRUE);
    }

    check.fd = ddci->write_fd;
    check.events = POLLIN;

    if (poll (&check, 1, 0)) {
        return (TRUE);
    }

    loop = g_main_new (FALSE);

    window = gtk_window_new (GTK_WINDOW_DIALOG);

    entry = gtk_entry_new ();

    line = rc_line_buf_get_buf (ddci->line_buf);
    ddci->buf = g_string_append (ddci->buf, line);
    g_free (line);

    line = ddci->buf->str;
    line = g_strstrip (line);
    label = gtk_label_new (line);
    g_string_free (ddci->buf, TRUE);
    ddci->buf = g_string_new (NULL);

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

    line = gtk_editable_get_chars (GTK_EDITABLE (entry), 0, -1);

    gtk_widget_destroy (window);

    rc_write (ddci->write_fd, line, strlen (line));
    rc_write (ddci->write_fd, "\n", 1);

    g_free (line);

    child_wants_input = FALSE;

    return (TRUE);
}

static gboolean
do_configure (RCPackman *p, DebmanInstallState *dis)
{
    pid_t child;
    pid_t parent;
    int status;
    int rfds[2];
    int master, slave;
    RCLineBuf *line_buf;
    DebmanDoConfigureInfo ddci;
    GMainLoop *loop;
    gchar *evil_environment_line;

    pipe (rfds);
    fcntl (rfds[0], F_SETFL, O_NONBLOCK);
    fcntl (rfds[1], F_SETFL, O_NONBLOCK);

    openpty (&master, &slave, NULL, NULL, NULL);

    unlock_database (RC_DEBMAN (p));

    signal (SIGCHLD, SIG_DFL);
    signal (SIGUSR2, debman_do_configure_sigusr2_cb);

    parent = getpid ();

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to fork dpkg to configure unpacked "
                              "packages.  You will need to configure them by "
                              "hand by running the command `dpkg --configure "
                              "--pending` as root");
        close (rfds[0]);
        close (rfds[1]);

        close (master);
        close (slave);

        return (FALSE);
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL);

        fflush (stdout);
        close (rfds[0]);
        dup2 (rfds[1], STDOUT_FILENO);
        close (rfds[1]);

        close (master);
        dup2 (slave, STDIN_FILENO);
        close (slave);

        fprintf (stderr, "DEBMAN: /usr/bin/dpkg --configure --pending\n");

        evil_environment_line = g_strdup_printf ("RC_READ_NOTIFY_PID=%d",
                                                 parent);
        fprintf (stderr, "evil env: %s\n", evil_environment_line);
        putenv ("LD_PRELOAD=" SHAREDIR "/dpkg-helper.so");
        putenv (evil_environment_line);
        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--configure", "--pending",
               NULL);
        break;

    default:
        break;
    }

    close (rfds[1]);
    close (slave);

    loop = g_main_new (FALSE);

    ddci.pman = p;
    ddci.loop = loop;
    ddci.dis = dis;
    ddci.buf = g_string_new (NULL);
    ddci.write_fd = master;

    line_buf = rc_line_buf_new (rfds[0]);

    ddci.line_buf = line_buf;

    ddci.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (do_configure_read_line_cb),
                            (gpointer) &ddci);
    ddci.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (do_configure_read_done_cb),
                            (gpointer) &ddci);
    ddci.poll_write_id =
        gtk_timeout_add (100,
                         (GtkFunction) debman_do_configure_poll_write_cb,
                         (gpointer) &ddci);

    g_main_run (loop);

    gtk_timeout_remove (ddci.poll_write_id);

//        gtk_object_destroy (GTK_OBJECT (line_buf));
    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    close (rfds[0]);
    close (master);

    waitpid (child, &status, 0);

    signal (SIGUSR2, SIG_DFL);

    if (!(lock_database (RC_DEBMAN (p)))) {
        /* FIXME: need to handle this */
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "dpkg did not successfully configure the "
                              "unpacked packages.  You will need to configure "
                              "them by hand, by running the command `dpkg "
                              "--configure --pending` as root");
        return (FALSE);
    }

    return (TRUE);
}

/*
 * Combine the last four code blocks into a useful transaction.
 */

static void
rc_debman_transact (RCPackman *p, RCPackageSList *install_pkgs,
                    RCPackageSList *remove_pkgs)
{
    DebmanInstallState *dis = g_new0 (DebmanInstallState, 1);

    dis->total = g_slist_length (install_pkgs) +
        g_slist_length (remove_pkgs);

    if (remove_pkgs) {
        if (!(mark_purge (p, remove_pkgs))) {
            goto END;
        }
    }

    gtk_signal_emit_by_name (GTK_OBJECT (p), "transaction_step", 0,
                             dis->total);

    if (install_pkgs) {
        if (!(do_unpack (p, install_pkgs, dis))) {
            goto END;
        }
    }

    if (remove_pkgs && (dis->seqno < dis->total)) {
        if (!(do_purge (p, dis))) {
            goto END;
        }
    }

    gtk_signal_emit_by_name (GTK_OBJECT (p), "transaction_done");

    if (install_pkgs) {
        dis->total = g_slist_length (install_pkgs);
        dis->seqno = 0;

        gtk_signal_emit_by_name (GTK_OBJECT (p), "configure_step", 0,
                                 dis->total);

        if (!(do_configure (p, dis))) {
            goto END;
        }

        gtk_signal_emit_by_name (GTK_OBJECT (p), "configure_done");
    }

  END:
    g_free (dis);
    hash_destroy (RC_DEBMAN (p));
}

/*
 * Takes a string of format [<epoch>:]<version>[-<release>], and returns those
 * three elements, if they exist
 */

void
rc_debman_parse_version (gchar *input, guint32 *epoch, gchar **version,
                         gchar **release)
{
    gchar **t1, **t2;

    *epoch = 0;
    *version = NULL;
    *release = NULL;

    /* Check if there's an epoch set */
    t1 = g_strsplit (input, ":", 1);

    if (t1[1]) {
        /* There is an epoch */
        *epoch = strtoul (t1[0], NULL, 10);
        /* Split the version and release */
        t2 = g_strsplit (t1[1], "-", 1);
    } else {
        /* No epoch, split the version and release */
        t2 = g_strsplit (t1[0], "-", 1);
    }

    /* Version definitely gets set to something */
    *version = g_strdup (t2[0]);
    /* Set release (this may be dup'ing NULL) */
    *release = g_strdup (t2[1]);

    g_strfreev (t2);
    g_strfreev (t1);
}

/*
 * Given a line from a debian control file (or from /var/lib/dpkg/status), and
 * a pointer to a RCPackageDepSList, fill in the dependency information.  It's
 * ugly but memprof says it doesn't leak.
 */

RCPackageDepSList *
rc_debman_fill_depends (gchar *input, RCPackageDepSList *list)
{
    gchar **deps;
    guint i;

    /* All evidence indicates that the fields are comma-space separated, but if
       that ever turns out to be incorrect, we'll have to do this part more
       carefully. */
    /* Evidence has turned out incorrect; there can be arbitrary whitespace
       inserted anywhere within the depends fields -- or lack of said
       whitespace. Man, Debian is like a piece of bad fruit -- it looks great
       on the outside, but when you dig into it, you see the rot ;-) - vlad */
    /* Note that this can probably be made much prettier if we use the GNU
       regexp package; but I have no idea how much slower/faster that would
       be. - vlad */

    deps = g_strsplit (input, ",", 0);

    for (i = 0; deps[i]; i++) {
        gchar **elems;
        guint j;
        RCPackageDep *dep = NULL;
        gchar *curdep;

        curdep = g_strstrip (deps[i]);
        elems = g_strsplit (curdep, "|", 0);

        /* For the most part, there's only one element per dep, except for the
           rare case of OR'd deps. */
        for (j = 0; elems[j]; j++) {
            RCPackageDepItem *depi;
            gchar *curelem;
            gchar *s1, *s2;

            gchar *depname = NULL, *deprel = NULL, *depvers = NULL;

            curelem = g_strstrip (elems[j]);
            /* A space separates the name of the dep from the relationship and
               version. */
            /* No. "alsa-headers(<<0.5)" is perfectly valid. We have to parse
             * this by hand */

            /* First we grab the dep name */
            s1 = curelem;
            while (*s1 && !isspace(*s1) && *s1 != '(') s1++;
            /* s1 now points to the first character after the full name */
            depname = g_malloc (s1 - curelem + 1);
            strncpy (depname, curelem, s1 - curelem);
            depname[s1 - curelem] = '\0';

            if (*s1) {          /* we have a relation */
                /* Skip until the opening brace */
                while (*s1 && *s1 != '(') s1++;
                s1++; /* skip the brace */
                while (*s1 && isspace(*s1)) s1++; /* skip spaces before rel */
                s2 = s1;
                while (*s2 == '=' || *s2 == '>' || *s2 == '<') s2++;
                /* s2 now points to the first char after the relation */
                deprel = g_malloc (s2 - s1 + 1);
                strncpy (deprel, s1, s2 - s1);
                deprel[s2 - s1] = '\0';

                while (*s2 && isspace (*s2)) s2++;
                /* Now we have the version string start */
                s1 = s2;
                while (*s2 && !isspace (*s2) && *s2 != ')') s2++;
                depvers = g_malloc (s2 - s1 + 1);
                strncpy (depvers, s1, s2 - s1);
                depvers[s2 - s1] = '\0';

                /* We really shouldn't ignore the rest of the string, but we
                   do. */
                /* There shouldn't be anything after this, except a closing
                   paren */
            }

            if (!deprel) {
                /* There's no version in this dependency, just a name. */
                depi = rc_package_dep_item_new (depname, 0, NULL, NULL,
                                                RC_RELATION_ANY);
            } else {
                /* We've got to parse the rest of this mess. */
                guint relation = RC_RELATION_ANY;
                gint32 epoch;
                gchar *version, *release;

                /* The possible relations are "=", ">>", "<<", ">=", and "<=",
                   decide which apply */
                switch (deprel[0]) {
                case '=':
                    relation = RC_RELATION_EQUAL;
                    break;
                case '>':
                    relation = RC_RELATION_GREATER;
                    break;
                case '<':
                    relation = RC_RELATION_LESS;
                    break;
                }

                if (deprel[1] && (deprel[1] == '=')) {
                    relation |= RC_RELATION_EQUAL;
                }

                g_free (deprel);

                /* Break the single version string up */
                rc_debman_parse_version (depvers, &epoch, &version,
                                         &release);

                g_free (depvers);

                depi = rc_package_dep_item_new (depname, epoch, version,
                                                release, relation);
                g_free (version);
                g_free (release);
            }

            g_free (depname);
            dep = g_slist_append (dep, depi);
        }

        g_strfreev (elems);

        list = g_slist_append (list, dep);
    }

    g_strfreev (deps);

    return (list);
}

/*
 * The query_all associated cruft
 */

typedef struct _DebmanQueryInfo DebmanQueryInfo;

struct _DebmanQueryInfo {
    RCPackage *buf_pkg;
    GString *desc_buf;
    RCPackageSList *pkgs;
    gboolean error;
    GMainLoop *loop;
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
query_all_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanQueryInfo *dqi = (DebmanQueryInfo *)data;

    /* This is a blank line which terminates a package section in
       /var/lib/dpkg/status, so save out our current buffer */
    if (!line[0]) {
        if (dqi->desc_buf && dqi->desc_buf->len) {
            dqi->buf_pkg->description = get_description (dqi->desc_buf);
            dqi->desc_buf = NULL;
        }
        dqi->pkgs = g_slist_append (dqi->pkgs, dqi->buf_pkg);
        dqi->buf_pkg = NULL;
        return;
    }

    /* This is the beginning of a package section, so start up a new buffer
       and set the package name */
    if (!strncmp (line, "Package:", strlen ("Package:"))) {
        /* Better make sure this isn't a malformed /var/lib/dpkg/status file,
           so check to make sure we cleanly finished the last package */
        if (dqi->buf_pkg) {
            dqi->error = TRUE;
            /* FIXME: must do something smart here */
            return;
        }
        dqi->buf_pkg = rc_package_new ();
        dqi->buf_pkg->spec.name = g_strdup (line + strlen ("Package:"));
        dqi->buf_pkg->spec.name = g_strchug (dqi->buf_pkg->spec.name);
        return;
    }

    /* Check if this is a Status: line -- the only status we recognize is
       "install ok installed".  RCDebman should reset any selections at
       initialization, so that we don't have to worry about things like
       "purge ok installed". */
    if (!strcmp (line, "Status: install ok installed")) {
        dqi->buf_pkg->installed = TRUE;
        return;
    }

    /* We now also recognize "hold ok installed", and we set the hold bit
       appropriately.  Yay for us! */
    if (!strcmp (line, "Status: hold ok installed")) {
        dqi->buf_pkg->installed = TRUE;
        dqi->buf_pkg->hold = TRUE;
        return;
    }

    if (!strncmp (line, "Installed-Size:", strlen ("Installed-Size:"))) {
        dqi->buf_pkg->installed_size =
            strtoul (line + strlen ("Installed-Size:"), NULL, 10) * 1024;
        return;
    }

    if (!strncmp (line, "Description:", strlen ("Description:"))) {
        dqi->buf_pkg->summary = g_strdup (line + strlen ("Description:"));
        dqi->buf_pkg->summary = g_strchug (dqi->buf_pkg->summary);
        /* After Description: comes the long description */
        dqi->desc_buf = g_string_new (NULL);
        return;
    }

    if ((line[0] == ' ') && dqi->desc_buf) {
        if ((line[1] != '.') || line[2]) {
            dqi->desc_buf = g_string_append (dqi->desc_buf, line + 1);
        }
        dqi->desc_buf = g_string_append (dqi->desc_buf, "\n");
        return;
    }

    if (!strncmp (line, "Version:", strlen ("Version:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Version:")));
        rc_debman_parse_version (tmp, &dqi->buf_pkg->spec.epoch,
                                 &dqi->buf_pkg->spec.version,
                                 &dqi->buf_pkg->spec.release);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Section:", strlen ("Section:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Section:")));
        gchar *sec = strstr (tmp, "/");

        if (!sec) {
            sec = tmp;
        } else {
            sec++;
        }

        if (!g_strcasecmp (sec, "admin")) {
            dqi->buf_pkg->section = SECTION_SYSTEM;
        } else if (!g_strcasecmp (sec, "base")) {
            dqi->buf_pkg->section = SECTION_SYSTEM;
        } else if (!g_strcasecmp (sec, "comm")) {
            dqi->buf_pkg->section = SECTION_INTERNET;
        } else if (!g_strcasecmp (sec, "devel")) {
            dqi->buf_pkg->section = SECTION_DEVEL;
        } else if (!g_strcasecmp (sec, "doc")) {
            dqi->buf_pkg->section = SECTION_DOC;
        } else if (!g_strcasecmp (sec, "editors")) {
            dqi->buf_pkg->section = SECTION_UTIL;
        } else if (!g_strcasecmp (sec, "electronics")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "games")) {
            dqi->buf_pkg->section = SECTION_GAME;
        } else if (!g_strcasecmp (sec, "graphics")) {
            dqi->buf_pkg->section = SECTION_IMAGING;
        } else if (!g_strcasecmp (sec, "hamradio")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "interpreters")) {
            dqi->buf_pkg->section = SECTION_DEVELUTIL;
        } else if (!g_strcasecmp (sec, "libs")) {
            dqi->buf_pkg->section = SECTION_LIBRARY;
        } else if (!g_strcasecmp (sec, "mail")) {
            dqi->buf_pkg->section = SECTION_PIM;
        } else if (!g_strcasecmp (sec, "math")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "misc")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "net")) {
            dqi->buf_pkg->section = SECTION_INTERNET;
        } else if (!g_strcasecmp (sec, "news")) {
            dqi->buf_pkg->section = SECTION_INTERNET;
        } else if (!g_strcasecmp (sec, "oldlibs")) {
            dqi->buf_pkg->section = SECTION_LIBRARY;
        } else if (!g_strcasecmp (sec, "otherosfs")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "science")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "shells")) {
            dqi->buf_pkg->section = SECTION_SYSTEM;
        } else if (!g_strcasecmp (sec, "sound")) {
            dqi->buf_pkg->section = SECTION_MULTIMEDIA;
        } else if (!g_strcasecmp (sec, "tex")) {
            dqi->buf_pkg->section = SECTION_MISC;
        } else if (!g_strcasecmp (sec, "text")) {
            dqi->buf_pkg->section = SECTION_UTIL;
        } else if (!g_strcasecmp (sec, "utils")) {
            dqi->buf_pkg->section = SECTION_UTIL;
        } else if (!g_strcasecmp (sec, "web")) {
            dqi->buf_pkg->section = SECTION_INTERNET;
        } else if (!g_strcasecmp (sec, "x11")) {
            dqi->buf_pkg->section = SECTION_XAPP;
        } else {
            dqi->buf_pkg->section = SECTION_MISC;
        }

        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Depends:", strlen ("Depends:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Depends:")));
        dqi->buf_pkg->requires =
            rc_debman_fill_depends (tmp, dqi->buf_pkg->requires);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Recommends:", strlen ("Recommends:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Recommends:")));
        dqi->buf_pkg->recommends =
            rc_debman_fill_depends (tmp, dqi->buf_pkg->recommends);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Suggests:", strlen ("Suggests:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Suggests:")));
        dqi->buf_pkg->suggests =
            rc_debman_fill_depends (tmp, dqi->buf_pkg->suggests);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Pre-Depends:", strlen ("Pre-Depends:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Pre-Depends:")));
        dqi->buf_pkg->requires =
            rc_debman_fill_depends (tmp, dqi->buf_pkg->requires);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Conflicts:", strlen ("Conflicts:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Conflicts:")));
        dqi->buf_pkg->conflicts =
            rc_debman_fill_depends (tmp, dqi->buf_pkg->conflicts);
        g_free (tmp);
        return;
    }

    if (!strncmp (line, "Provides:", strlen ("Provides:"))) {
        gchar *tmp = g_strchug (g_strdup (line + strlen ("Provides:")));
        dqi->buf_pkg->provides =
            rc_debman_fill_depends (tmp, dqi->buf_pkg->provides);
        g_free (tmp);
        return;
    }
}

static void
query_all_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status, gpointer data)
{
    DebmanQueryInfo *dqi = (DebmanQueryInfo *)data;

    gtk_signal_disconnect_by_func (GTK_OBJECT (line_buf),
                                   GTK_SIGNAL_FUNC (query_all_read_line_cb),
                                   data);
    gtk_signal_disconnect_by_func (GTK_OBJECT (line_buf),
                                   GTK_SIGNAL_FUNC (query_all_read_done_cb),
                                   data);

    dqi->error = (status == RC_LINE_BUF_ERROR);

    g_main_quit (dqi->loop);
}

static void
rc_debman_query_all_real (RCPackman *p)
{
    int fd;
    RCLineBuf *line_buf;
    RCDebman *d = RC_DEBMAN (p);
    DebmanQueryInfo dqi;
    RCPackageSList *iter;
    GMainLoop *loop;

    if ((fd = open (status_file, O_RDONLY)) < 0) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open /var/lib/dpkg/status");
        return;
    }

    loop = g_main_new (FALSE);

    dqi.pkgs = NULL;
    dqi.buf_pkg = NULL;
    dqi.desc_buf = NULL;
    dqi.error = FALSE;
    dqi.loop = loop;

    line_buf = rc_line_buf_new (fd);

    gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                        GTK_SIGNAL_FUNC (query_all_read_line_cb),
                        (gpointer) &dqi);
    gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                        GTK_SIGNAL_FUNC (query_all_read_done_cb),
                        (gpointer) &dqi);

    g_main_run (loop);

//    gtk_object_destroy (GTK_OBJECT (line_buf));
    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    for (iter = dqi.pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        if (pkg->installed) {
            pkg->provides =
                g_slist_append (pkg->provides,
                                rc_package_dep_new_with_item (
                                    rc_package_dep_item_new_from_spec (
                                        &pkg->spec,
                                        RC_RELATION_EQUAL)));
            g_hash_table_insert (d->priv->pkg_hash, (gpointer) pkg->spec.name,
                                 (gpointer) pkg);
        } else {
            rc_package_free (pkg);
        }
    }

    g_slist_free (dqi.pkgs);

    d->priv->hash_valid = TRUE;

    close (fd);
}

static void
pkg_list_append (gchar *name, RCPackage *pkg, RCPackageSList **pkg_list)
{
    *pkg_list = g_slist_append (*pkg_list, rc_package_copy (pkg));
}

static RCPackageSList *
rc_debman_query_all (RCPackman *p)
{
    RCPackageSList *pkgs = NULL;

    if (!(RC_DEBMAN (p)->priv->hash_valid)) {
        rc_debman_query_all_real (p);
    }

    g_hash_table_foreach (RC_DEBMAN (p)->priv->pkg_hash,
                          (GHFunc) pkg_list_append, &pkgs);

    return pkgs;
}

static RCPackage *
rc_debman_query (RCPackman *p, RCPackage *pkg)
{
    RCDebman *d = RC_DEBMAN (p);
    RCPackage *lpkg;

    g_assert (pkg);
    g_assert (pkg->spec.name);

    if (!d->priv->hash_valid) {
        rc_debman_query_all_real (p);
    }

    lpkg = g_hash_table_lookup (d->priv->pkg_hash,
                                (gconstpointer) pkg->spec.name);

    if (lpkg &&
        (!pkg->spec.version ||
         !strcmp (lpkg->spec.version, pkg->spec.version)) &&
        (!pkg->spec.release ||
         !strcmp (lpkg->spec.release, pkg->spec.release))) {
        rc_package_free (pkg);
        return (rc_package_copy (lpkg));
    } else {
        return (pkg);
    }
}

static RCPackage *
rc_debman_query_file (RCPackman *p, const gchar *filename)
{
    int rfds[2];
    pid_t child;
    int status;
    RCLineBuf *line_buf;
    DebmanQueryInfo dqi;
    GMainLoop *loop;
    RCPackageDepItem *dep_item;
    RCPackageDep *dep = NULL;

    pipe (rfds);
    fcntl (rfds[0], F_SETFL, O_NONBLOCK);
    fcntl (rfds[1], F_SETFL, O_NONBLOCK);

    signal (SIGCHLD, SIG_DFL);

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to fork and exec dpkg-deb to query "
                              "file");
        close (rfds[0]);
        close (rfds[1]);
        return (NULL);
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL);

        close (rfds[0]);

        fflush (stdout);
        dup2 (rfds[1], STDOUT_FILENO);
        close (rfds[1]);

        fclose (stderr);

        execl ("/usr/bin/dpkg-deb", "/usr/bin/dpkg-deb", "-f", filename, NULL);
        break;

    default:
        break;
    }

    close (rfds[1]);

    loop = g_main_new (FALSE);

    dqi.buf_pkg = NULL;
    dqi.desc_buf = NULL;
    dqi.pkgs = NULL;
    dqi.error = FALSE;
    dqi.loop = loop;

    line_buf = rc_line_buf_new (rfds[0]);

    gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                        GTK_SIGNAL_FUNC (query_all_read_line_cb),
                        (gpointer) &dqi);
    gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                        GTK_SIGNAL_FUNC (query_all_read_done_cb),
                        (gpointer) &dqi);

    g_main_run (loop);

//        gtk_object_destroy (GTK_OBJECT (line_buf));
    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    waitpid (child, &status, 0);

    close (rfds[0]);

    if (!(WIFEXITED (status)) ||
        WEXITSTATUS (status) ||
        !dqi.buf_pkg)
    {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "There was an error running the dpkg-deb "
                              "command to query the file.");
        return (NULL);
    }

    if (dqi.desc_buf) {
        dqi.buf_pkg->description = get_description (dqi.desc_buf);
        dqi.desc_buf = NULL;
    }

    /* Make the package provide itself (that's what we like to do,
           anyway) */
    dep_item =
        rc_package_dep_item_new_from_spec ((RCPackageSpec *)dqi.buf_pkg,
                                           RC_RELATION_EQUAL);
    dep = g_slist_append (dep, dep_item);
    dqi.buf_pkg->provides = g_slist_append (dqi.buf_pkg->provides, dep);

    return (dqi.buf_pkg);
}

typedef struct _DebmanVerifyStatusInfo DebmanVerifyStatusInfo;

struct _DebmanVerifyStatusInfo {
    int out_fd;
    gboolean error;
    GMainLoop *loop;
    guint read_line_id;
    guint read_done_id;
};

static void
verify_status_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    gchar **tokens = NULL;
    gchar *tmp = NULL;
    DebmanVerifyStatusInfo *dvsi = (DebmanVerifyStatusInfo *)data;
    int out_fd = dvsi->out_fd; /* Just for sanity */

    if (strncmp (line, "Status:", strlen ("Status:"))) {
        /* This isn't a status line, so we don't need to do anything other than
           to write it to the new file */
        if (!rc_write (out_fd, line, strlen (line)) ||
            !rc_write (out_fd, "\n", 1))
        {
            goto BROKEN;
        }
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
    dvsi->error = TRUE;
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dvsi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dvsi->read_done_id);
    g_main_quit (dvsi->loop);

  END:
    g_strfreev (tokens);
    g_free (tmp);
}

static void
verify_status_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                            gpointer data)
{
    DebmanVerifyStatusInfo *dvsi = (DebmanVerifyStatusInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (line_buf), dvsi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dvsi->read_done_id);

    g_main_quit (dvsi->loop);
}

static gboolean
verify_status (RCPackman *p)
{
    DebmanVerifyStatusInfo dvsi;
    GMainLoop *loop;
    int in_fd, out_fd;
    RCLineBuf *line_buf;

    rc_packman_set_error (p, RC_PACKMAN_ERROR_NONE, NULL);

    if ((in_fd = open ("/var/lib/dpkg/status", O_RDONLY)) == -1) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FATAL,
                              "Unable to open /var/lib/dpkg/status for "
                              "reading");
        return (FALSE);
    }

    if ((out_fd = creat ("/var/lib/dpkg/status.redcarpet", 644)) == -1) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FATAL,
                              "Unable to open /var/lib/dpkg/status.redcarpet "
                              "for writing");
        close (in_fd);
        return (FALSE);
    }

    loop = g_main_new (FALSE);

    dvsi.out_fd = out_fd;
    dvsi.loop = loop;
    dvsi.error = FALSE;

    line_buf = rc_line_buf_new (in_fd);

    dvsi.read_line_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                            GTK_SIGNAL_FUNC (verify_status_read_line_cb),
                            (gpointer) &dvsi);
    dvsi.read_done_id =
        gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                            GTK_SIGNAL_FUNC (verify_status_read_done_cb),
                            (gpointer) &dvsi);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (line_buf));

    g_main_destroy (loop);

    close (in_fd);
    close (out_fd);

    if (dvsi.error) {
        unlink ("/var/lib/dpkg/status.redcarpet");
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FATAL,
                              "The /var/lib/dpkg/status file appears to have "
                              "errors or be malformed.  Until these errors "
                              "are corrected, it is not safe for the "
                              "application to perform package management "
                              "operations on your system.  Please check this "
                              "file by hand, or use one of the Debian "
                              "programs, such as dpkg or apt-get, to correct "
                              "it.");
        return (FALSE);
    }

    if (rename ("/var/lib/dpkg/status.redcarpet", "/var/lib/dpkg/status")) {
        unlink ("/var/lib/dpkg/status.redcarpet");
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FATAL,
                              "Unable to rename RC status file");
        return (FALSE);
    }

    return (TRUE);
}

static RCVerificationSList *
rc_debman_verify (RCPackman *p, RCPackage *pkg)
{
    RCVerificationSList *ret = NULL;
    RCPackageUpdate *update = (RCPackageUpdate *)pkg->history->data;

    g_assert (p);
    g_assert (pkg);
    g_assert (pkg->package_filename);

    if (update->md5sum) {
        RCVerification *verification;

        verification = rc_verify_md5_string (pkg->package_filename,
                                             update->md5sum);

        ret = g_slist_append (ret, verification);
    }

    if (pkg->signature_filename) {
        RCVerification *verification;

        verification = rc_verify_gpg (pkg->package_filename,
                                      pkg->signature_filename);

        ret = g_slist_append (ret, verification);
    }

    return (ret);
}

typedef struct _DebmanFindFileInfo DebmanFindFileInfo;

struct _DebmanFindFileInfo {
    guint read_line_id;
    guint read_done_id;
    GMainLoop *loop;
    const gchar *target;
    gboolean accept;
};

static void
find_file_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    DebmanFindFileInfo *dffi = (DebmanFindFileInfo *)data;

    if (!strcmp (dffi->target, line)) {
        dffi->accept = TRUE;
        gtk_signal_disconnect (GTK_OBJECT (line_buf), dffi->read_line_id);
        gtk_signal_disconnect (GTK_OBJECT (line_buf), dffi->read_done_id);
        g_main_quit (dffi->loop);
    }
}

static void
find_file_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status, gpointer data)
{
    DebmanFindFileInfo *dffi = (DebmanFindFileInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (line_buf), dffi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (line_buf), dffi->read_done_id);
    g_main_quit (dffi->loop);
}

static RCPackage *
rc_debman_find_file (RCPackman *p, const gchar *filename)
{
    DIR *info_dir;
    struct dirent *info_file;
    gchar realname[PATH_MAX];

    if (!g_path_is_absolute (filename)) {
        return (NULL);
    }

    if (!realpath (filename, realname)) {
        return (NULL);
    }

    if (!(info_dir = opendir ("/var/lib/dpkg/info"))) {
        return (NULL);
    }

    while ((info_file = readdir (info_dir))) {
        int fd;
        RCLineBuf *line_buf;
        GMainLoop *loop;
        DebmanFindFileInfo dffi;
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
            return (NULL);
        }

        g_free (fullname);

        loop = g_main_new (FALSE);

        dffi.target = realname;
        dffi.loop = loop;
        dffi.accept = FALSE;

        line_buf = rc_line_buf_new (fd);

        dffi.read_line_id =
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_line",
                                GTK_SIGNAL_FUNC (find_file_read_line_cb),
                                (gpointer) &dffi);
        dffi.read_done_id =
            gtk_signal_connect (GTK_OBJECT (line_buf), "read_done",
                                GTK_SIGNAL_FUNC (find_file_read_done_cb),
                                (gpointer) &dffi);

        g_main_run (loop);

        gtk_object_unref (GTK_OBJECT (line_buf));

        g_main_destroy (loop);

        close (fd);

        if (dffi.accept) {
            RCPackage *package;

            package = rc_package_new ();

            package->spec.name = g_strndup (info_file->d_name, length - 5);

            closedir (info_dir);

            package = rc_packman_query (p, package);

            if (!package->installed) {
                rc_package_free (package);

                return (NULL);
            } else {
                return (package);
            }
        }
    }

    closedir (info_dir);

    return (NULL);
}

static void
rc_debman_destroy (GtkObject *obj)
{
    RCDebman *d = RC_DEBMAN (obj);

    unlock_database (d);

    hash_destroy (d);

    g_free (d->priv);

    if (GTK_OBJECT_CLASS (rc_debman_parent)->destroy)
        (* GTK_OBJECT_CLASS (rc_debman_parent)->destroy) (obj);
}

static void
rc_debman_class_init (RCDebmanClass *klass)
{
    GtkObjectClass *object_class =  (GtkObjectClass *) klass;
    RCPackmanClass *packman_class = (RCPackmanClass *) klass;

    object_class->destroy = rc_debman_destroy;

    rc_debman_parent = gtk_type_class (rc_packman_get_type ());

    packman_class->rc_packman_real_transact = rc_debman_transact;
    packman_class->rc_packman_real_query = rc_debman_query;
    packman_class->rc_packman_real_query_file = rc_debman_query_file;
    packman_class->rc_packman_real_query_all = rc_debman_query_all;
    packman_class->rc_packman_real_verify = rc_debman_verify;
    packman_class->rc_packman_real_find_file = rc_debman_find_file;

    putenv ("DEBIAN_FRONTEND=noninteractive");

    deps_conflicts_use_virtual_packages (FALSE);
}

static void
rc_debman_init (RCDebman *obj)
{
    RCPackman *p = RC_PACKMAN (obj);

    p->priv->features =
        RC_PACKMAN_FEATURE_POST_CONFIG;

    obj->priv = g_new0 (RCDebmanPrivate, 1);

    obj->priv->lock_fd = -1;

    obj->priv->pkg_hash = g_hash_table_new (g_str_hash, g_str_equal);
    obj->priv->hash_valid = FALSE;

    if (geteuid ()) {
        /* We can't really verify the status file or lock the database */
        return;
    }

    if (!lock_database (obj)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "The dpkg database (/var/lib/dpkg/status) is "
                              "currently locked by another process.  Without "
                              "this lock, this application cannot safely "
                              "operate on your system, and will exit.  "
                              "Please end any other program accessing the "
                              "database (such as dpkg, dselect, or apt), and "
                              "try again.");
    } else {
        verify_status (p);
    }
}

RCDebman *
rc_debman_new (void)
{
    RCDebman *new =
        RC_DEBMAN (gtk_type_new (rc_debman_get_type ()));

    if (status_file == NULL) {
        if (getenv ("RC_DEBMAN_STATUS_FILE")) {
            status_file = getenv ("RC_DEBMAN_STATUS_FILE");
            rc_status_file = g_strconcat (status_file, ".redcarpet", NULL);
        } else {
            status_file = DEBMAN_DEFAULT_STATUS_FILE;
            rc_status_file = DEBMAN_DEFAULT_STATUS_FILE ".redcarpet";
        }
    }

    return new;
}
