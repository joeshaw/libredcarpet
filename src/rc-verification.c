/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-verification.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "rc-debug.h"
#include "rc-verification.h"
#include "rc-verification-private.h"
#include "rc-util.h"
#include "rc-line-buf.h"
#include "rc-md5.h"

gchar *keyring = SHAREDIR "/red-carpet.gpg";

void
rc_verification_set_keyring (const gchar *t_keyring)
{
    keyring = g_strdup (t_keyring);
}

RCVerification *
rc_verification_new ()
{
    RCVerification *new;

    new = g_new0 (RCVerification, 1);

    return (new);
} /* rc_verification_new */

void
rc_verification_free (RCVerification *verification)
{
    g_free (verification->info);

    g_free (verification);
} /* rc_verification_free */

void
rc_verification_slist_free (RCVerificationSList *verification_list)
{
    g_slist_foreach (verification_list, (GFunc) rc_verification_free, NULL);

    g_slist_free (verification_list);
} /* rc_verification_slist_free */

typedef struct _GPGInfo GPGInfo;

struct _GPGInfo {
    GMainLoop *loop;

    RCVerification *verification;

    guint read_line_id;
    guint read_done_id;
};

static void
gpg_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    GPGInfo *gpg_info = (GPGInfo *)data;
    const gchar *ptr;

    rc_debug (RC_DEBUG_LEVEL_DEBUG, __FUNCTION__ ": got \"%s\"\n", line);

    ptr = line + strlen ("[GNUPG:] ");

    if (!strncmp (ptr, "GOODSIG ", strlen ("GOODSIG "))) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": good GPG signature\n");

        gpg_info->verification->status = RC_VERIFICATION_STATUS_PASS;

        ptr = strstr (ptr + strlen ("GOODSIG "), " ");

        if (ptr) {
            rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                      ": signer is \"%s\"\n", ptr + 1);
            gpg_info->verification->info = g_strdup (ptr + 1);
        }

        return;
    }

    if (!strncmp (ptr, "BADSIG ", strlen ("BADSIG "))) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": bad GPG signature\n");

        gpg_info->verification->status = RC_VERIFICATION_STATUS_FAIL;

        ptr = strstr (ptr + strlen ("BADSIG "), " ");

        if (ptr) {
            rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ \
                      ": signer is \"%s\"\n", ptr + 1);
            gpg_info->verification->info = g_strdup (ptr + 1);
        }

        return;
    }

    if (!strncmp (ptr, "ERRSIG ", strlen ("ERRSIG "))) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, __FUNCTION__ \
                  ": error checking GPG signature\n");

        gpg_info->verification->status = RC_VERIFICATION_STATUS_UNDEF;

        return;
    }
} /* gpg_read_line_cb */

static void
gpg_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status, gpointer data)
{
    GPGInfo *gpg_info = (GPGInfo *)data;

    g_signal_handler_disconnect (line_buf, gpg_info->read_line_id);
    g_signal_handler_disconnect (line_buf, gpg_info->read_done_id);

    g_main_quit (gpg_info->loop);
} /* gpg_read_done_cb */

RCVerification *
rc_verify_gpg (gchar *file, gchar *sig)
{
//    const gchar *keyring = SHAREDIR "/red-carpet.gpg";
    pid_t child;
    int status;
    int fds[2];
    GPGInfo gpg_info;
    RCLineBuf *line_buf;
    GMainLoop *loop;
    gchar *gpg_command;
    RCVerification *verification;
    gchar *gpgdir;

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_GPG;
    verification->status = RC_VERIFICATION_STATUS_UNDEF;

    gpg_command = rc_is_program_in_path ("gpg");

    if (!gpg_command) {
        verification->status = RC_VERIFICATION_STATUS_UNDEF;
        verification->info =
            g_strdup ("gpg does not appear to be on your PATH");

        return (verification);
    }

    /* There's a chance that the ~root/.gnupg directory doesn't exist
     * yet, and stupidly enough, gnupg creates this directory the
     * first time it is run but doesn't then do what it was called to
     * do.  This causes spurious signature failures on first time
     * runs.  To fix it, if the directory is missing we'll run a
     * harmless command, gpg --list-keys, to ensure that the directory
     * exists.  We use --list-keys instead of just gpg by itself in
     * the off chance that we're wrong, as gpg by itself just blocks
     * waiting for input in normal operation. */

    gpgdir = g_strconcat (g_get_home_dir (), "/.gnupg", NULL);

    if (!rc_file_exists (gpgdir)) {
        gchar *command;
        int result;

        command = g_strconcat (gpg_command, " --list-keys", NULL);
        result = system (command);
        g_free (command);

        if (!rc_file_exists (gpgdir)) {
            verification->status = RC_VERIFICATION_STATUS_UNDEF;
            verification->info = g_strdup (
                "gpg was unable to create ~/.gnupg");

            g_free (gpgdir);

            return (verification);
        }

        g_free (gpgdir);
    }

    if (pipe (fds)) {
        verification->status = RC_VERIFICATION_STATUS_UNDEF;
        verification->info =
            g_strdup ("unable to create a pipe to communicate with gpg");

        return (verification);
    }

    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    signal (SIGCHLD, SIG_DFL);

    child = fork ();

    switch (child) {
    case -1:
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ ": fork() failed\n");

        close (fds[0]);
        close (fds[1]);

        verification->status = RC_VERIFICATION_STATUS_UNDEF;
        verification->info =
            g_strdup ("unable to exec gpg to verify signature");

        return (verification);

    case 0:
        close (fds[0]);

        fflush (stdout);
        dup2 (fds[1], STDOUT_FILENO);

        close (fds[1]);

        /* I wish I could add --no-auto-key-retrieve, but it's not in
         * gpg 1.0.1, and is in 1.0.4.  Maybe we can require a
         * specific version of gpg... */
        execl (gpg_command, gpg_command, "--batch", "--quiet",
               "--no-secmem-warning", "--no-default-keyring",
               "--keyring", keyring, "--status-fd",
               "1", "--logger-fd", "1", "--verify", sig, file, NULL);

        _exit (-1);

    default:
        break;
    }

    close (fds[1]);

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, fds[0]);

    loop = g_main_new (FALSE);

    gpg_info.loop = loop;
    gpg_info.verification = verification;

    gpg_info.read_line_id =
        g_signal_connect (line_buf,
                          "read_line",
                          (GCallback) gpg_read_line_cb,
                          &gpg_info);
    
    gpg_info.read_done_id =
        g_signal_connect (line_buf,
                          "read_done",
                          (GCallback) gpg_read_done_cb,
                          &gpg_info);

    g_main_run (loop);

    g_object_unref (line_buf);

    g_main_destroy (loop);

    close (fds[0]);

    waitpid (child, &status, 0);

    if (!(WIFEXITED (status)) || (WEXITSTATUS (status) > 2)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR, __FUNCTION__ \
                  ": gpg exited abnormally\n");

        verification->status = RC_VERIFICATION_STATUS_UNDEF;
        verification->info = g_strdup ("gpg returned an unknown error code");

        return (verification);
    }

    return (gpg_info.verification);
} /* rc_verify_gpg */

RCVerification *
rc_verify_md5 (gchar *filename, guint8 *md5)
{
    guint8 *cmd5;
    RCVerification *verification;

    cmd5 = rc_md5 (filename);

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_MD5;

    if (!memcmp (md5, cmd5, 16)) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": good md5\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": bad md5\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;
    }

    g_free (cmd5);

    return (verification);
} /* rc_verify_md5 */

RCVerification *
rc_verify_md5_string (gchar *filename, gchar *md5)
{
    gchar *cmd5;
    RCVerification *verification;

    cmd5 = rc_md5_string (filename);

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_MD5;

    if (!strcmp (md5, cmd5)) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": good md5\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": bad md5\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;
    }

    g_free (cmd5);

    return (verification);
} /* rc_verify_md5_string */

RCVerification *
rc_verify_size (gchar *filename, guint32 size)
{
    struct stat buf;
    RCVerification *verification;

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_SIZE;

    if (stat (filename, &buf) == -1) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, __FUNCTION__ \
                  ": couldn't stat file\n");

        verification->status = RC_VERIFICATION_STATUS_UNDEF;

        return (verification);
    }

    if (buf.st_size == size) {
        rc_debug (RC_DEBUG_LEVEL_INFO, __FUNCTION__ ": good size check\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rc_debug (RC_DEBUG_LEVEL_WARNING, __FUNCTION__ ": bad size check\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;
    }

    return (verification);
} /* rc_verify_size */
