/*
 * rc-package.c: Verification structures...
 *
 * Copyright (C) 2000 Helix Code, Inc.
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

#include "rc-verification.h"
#include "rc-util.h"
#include "rc-line-buf.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include "rc-md5.h"
#include <sys/stat.h>

#ifndef SHAREDIR
#define SHAREDIR "/home/itp/gnome/red-carpet/share/redcarpet/"
#endif

RCVerification *
rc_verification_new ()
{
    RCVerification *new;

    new = g_new0 (RCVerification, 1);

    return (new);
}

void
rc_verification_free (RCVerification *rcv)
{
    g_free (rcv->info);

    g_free (rcv);
}

void
rc_verification_slist_free (RCVerificationSList *rcvsl)
{
    g_slist_foreach (rcvsl, (GFunc) rc_verification_free, NULL);

    g_slist_free (rcvsl);
}

typedef struct _GPGInfo GPGInfo;

struct _GPGInfo {
    GMainLoop *loop;
    RCVerification *rcv;
    guint read_line_id;
    guint read_done_id;
};

static void
gpg_read_line_cb (RCLineBuf *rlb, gchar *line, gpointer data)
{
    GPGInfo *gpgi = (GPGInfo *)data;
    gchar *ptr = line + strlen ("[GNUPG:] ");

    if (!strncmp (ptr, "GOODSIG ", strlen ("GOODSIG "))) {
        gpgi->rcv = rc_verification_new ();
        gpgi->rcv->type = RC_VERIFICATION_TYPE_GPG;
        gpgi->rcv->status = RC_VERIFICATION_STATUS_PASS;
        ptr = strstr (ptr + strlen ("GOODSIG "), " ");
        if (ptr) {
            gpgi->rcv->info = g_strdup (ptr + 1);
        }
        return;
    }

    if (!strncmp (ptr, "BADSIG ", strlen ("BADSIG "))) {
        gpgi->rcv = rc_verification_new ();
        gpgi->rcv->type = RC_VERIFICATION_TYPE_GPG;
        gpgi->rcv->status = RC_VERIFICATION_STATUS_FAIL;
        ptr = strstr (ptr + strlen ("BADSIG "), " ");
        if (ptr) {
            gpgi->rcv->info = g_strdup (ptr + 1);
        }
        return;
    }

    if (!strncmp (ptr, "ERRSIG ", strlen ("ERRSIG "))) {
        gpgi->rcv = rc_verification_new ();
        gpgi->rcv->type = RC_VERIFICATION_TYPE_GPG;
        gpgi->rcv->status = RC_VERIFICATION_STATUS_UNDEF;
        return;
    }
}

static void
gpg_read_done_cb (RCLineBuf *rlb, RCLineBufStatus status, gpointer data)
{
    GPGInfo *gpgi = (GPGInfo *)data;

    gtk_signal_disconnect (GTK_OBJECT (rlb), gpgi->read_line_id);
    gtk_signal_disconnect (GTK_OBJECT (rlb), gpgi->read_done_id);

    g_main_quit (gpgi->loop);
}

RCVerification *
rc_verify_gpg (gchar *file, gchar *sig)
{
    const gchar *keyring = SHAREDIR "/red-carpet.gpg";
    pid_t child;
    int status;
    int fds[2];
    GPGInfo gpgi;
    RCLineBuf *rlb;
    GMainLoop *loop;

    if (pipe (fds)) {
        return (NULL);
    }

    child = fork ();

    switch (child) {
    case -1:
        close (fds[0]);
        close (fds[1]);
        return (NULL);
        break;

    case 0:
        fclose (stderr);
        close (fds[0]);
        fflush (stdout);
        dup2 (fds[1], 1);
        close (fds[1]);

        execlp ("gpg", "gpg", "--batch", "--quiet", "--no-secmem-warning",
                "--no-default-keyring", "--keyring", keyring, "--status-fd",
                "1", "--logger-fd", "2", "--verify", sig, file, NULL);
        return (NULL);
        break;

    default:
        break;
    }

    close (fds[1]);

    loop = g_main_new (FALSE);

    gpgi.loop = loop;

    rlb = rc_line_buf_new ();

    gpgi.read_line_id =
        gtk_signal_connect (GTK_OBJECT (rlb), "read_line",
                            GTK_SIGNAL_FUNC (gpg_read_line_cb),
                            (gpointer) &gpgi);
    gpgi.read_done_id =
        gtk_signal_connect (GTK_OBJECT (rlb), "read_done",
                            GTK_SIGNAL_FUNC (gpg_read_done_cb),
                            (gpointer) &gpgi);

    rc_line_buf_set_fd (rlb, fds[0]);

    g_main_run (loop);

    gtk_object_unref (GTK_OBJECT (rlb));

    g_main_destroy (loop);

    close (fds[0]);

    waitpid (child, &status, 0);

    if (!(WIFEXITED (status)) || (WEXITSTATUS (status) > 2)) {
        return (NULL);
    }

    return (gpgi.rcv);
}

RCVerification *
rc_verify_md5 (gchar *filename, guint8 *md5)
{
    guint8 *cmd5 = rc_md5 (filename);
    RCVerification *rcv = rc_verification_new ();

    rcv->type = RC_VERIFICATION_TYPE_MD5;

    if (!memcmp (md5, cmd5, 16)) {
        rcv->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rcv->status = RC_VERIFICATION_STATUS_FAIL;
    }

    g_free (cmd5);

    return (rcv);
}

RCVerification *
rc_verify_size (gchar *filename, guint32 size)
{
    struct stat buf;
    RCVerification *rcv = rc_verification_new ();

    rcv->type = RC_VERIFICATION_TYPE_SIZE;

    if (stat (filename, &buf) == -1) {
        rcv->status = RC_VERIFICATION_STATUS_UNDEF;
        return (rcv);
    }

    if (buf.st_size == size) {
        rcv->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rcv->status = RC_VERIFICATION_STATUS_FAIL;
    }

    return (rcv);
}
