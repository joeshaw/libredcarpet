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

static gchar *keyring = NULL;
static gchar *tmpdir = NULL;

void
rc_verification_set_keyring (const gchar *t_keyring)
{
    keyring = g_strdup (t_keyring);
}

void
rc_verification_cleanup ()
{
    if (tmpdir != NULL)
        rc_rmdir (tmpdir);
}

RCVerification *
rc_verification_new ()
{
    RCVerification *new;

    new = g_new0 (RCVerification, 1);

    return (new);
}

void
rc_verification_free (RCVerification *verification)
{
    g_free (verification->info);

    g_free (verification);
}

void
rc_verification_slist_free (RCVerificationSList *verification_list)
{
    g_slist_foreach (verification_list, (GFunc) rc_verification_free, NULL);

    g_slist_free (verification_list);
}

const char *
rc_verification_type_to_string (RCVerificationType type)
{
    char *type_str;

    switch (type) {
    case RC_VERIFICATION_TYPE_SANITY:
        type_str = "sanity";
        break;
    case RC_VERIFICATION_TYPE_SIZE:
        type_str = "size";
        break;
    case RC_VERIFICATION_TYPE_MD5:
        type_str = "MD5";
        break;
    case RC_VERIFICATION_TYPE_GPG:
        type_str = "GPG";
        break;
    default:
        type_str = "(invalid)";
        break;
    }

    return type_str;
}

static void
gpg_read_line_cb (RCLineBuf *line_buf, gchar *line, gpointer data)
{
    RCVerification *verification = data;
    const gchar *ptr;

    rc_debug (RC_DEBUG_LEVEL_DEBUG,
              G_GNUC_PRETTY_FUNCTION ": got \"%s\"\n", line);

    ptr = line + strlen ("[GNUPG:] ");

    if (!strncmp (ptr, "GOODSIG ", strlen ("GOODSIG "))) {
        rc_debug (RC_DEBUG_LEVEL_INFO,
                  G_GNUC_PRETTY_FUNCTION ": good GPG signature\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;

        ptr = strstr (ptr + strlen ("GOODSIG "), " ");

        if (ptr) {
            rc_debug (RC_DEBUG_LEVEL_INFO, G_GNUC_PRETTY_FUNCTION \
                      ": signer is \"%s\"\n", ptr + 1);
            verification->info = g_strdup (ptr + 1);
        }

        return;
    }

    if (!strncmp (ptr, "BADSIG ", strlen ("BADSIG "))) {
        rc_debug (RC_DEBUG_LEVEL_INFO,
                  G_GNUC_PRETTY_FUNCTION ": bad GPG signature\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;

        ptr = strstr (ptr + strlen ("BADSIG "), " ");

        if (ptr) {
            rc_debug (RC_DEBUG_LEVEL_INFO, G_GNUC_PRETTY_FUNCTION \
                      ": signer is \"%s\"\n", ptr + 1);
            verification->info = g_strdup (ptr + 1);
        }

        return;
    }

    if (!strncmp (ptr, "ERRSIG ", strlen ("ERRSIG "))) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, G_GNUC_PRETTY_FUNCTION \
                  ": error checking GPG signature\n");

        verification->status = RC_VERIFICATION_STATUS_UNDEF;

        return;
    }
}

static void
gpg_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status, gpointer data)
{
    GMainLoop *loop = data;

    g_main_quit (loop);
}

static void
child_setup_func (gpointer user_data)
{
    dup2 (STDOUT_FILENO, STDERR_FILENO);
}

RCVerification *
rc_verify_gpg (gchar *file, gchar *sig)
{
    RCVerification *verification;
    gchar **env;
    static gchar *gpg_command = NULL;

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_GPG;
    verification->status = RC_VERIFICATION_STATUS_UNDEF;

    /* Our calling application must provide us with a gpg keyring
     * before we can verify any signatures */
    if (!keyring) {
        verification->info = g_strdup ("no gpg keyring was provided");
        return verification;
    }

    /* If we haven't found gpg in the path yet, we'd better look for
     * it */
    if (!gpg_command) {
        gpg_command = g_find_program_in_path ("gpg");

        /* Without gpg, we can't verify these signatures at all */
        if (!gpg_command) {
            verification->info =
                g_strdup ("gpg does not appear to be in your PATH");
            return verification;
        }
    }

    if (!tmpdir || !g_file_test (tmpdir, G_FILE_TEST_IS_DIR)) {
        tmpdir = rc_mkdtemp (g_strdup ("/tmp/.gpg-home-XXXXXX"));

        if (!tmpdir) {
            verification->status = RC_VERIFICATION_STATUS_UNDEF;
            verification->info =
                g_strdup ("unable to create temporary gpg homedir");

            return verification;
        }
    }

    env = g_new0 (char *, 3);
    env[0] = g_strconcat ("HOME=", tmpdir, NULL);
    env[1] = g_strconcat ("GNUPGHOME=", tmpdir, NULL);
    
    { /* I suppose this could be its own function, but... */
        RCLineBuf *line_buf;
        GMainLoop *loop;
        GError *error;
        gint stdout_fd;
        char *verify_argv[] =
            { gpg_command, "--batch", "--quiet", "--no-secmem-warning",
              "--no-default-keyring", "--keyring", keyring,
              "--status-fd", "1", "--logger-fd", "1", "--verify",
              sig, file, NULL };

        error = NULL;
        if (!g_spawn_async_with_pipes (
                NULL, verify_argv, env, 0, child_setup_func,
                NULL, NULL, NULL, &stdout_fd, NULL, &error))
        {
            rc_debug (RC_DEBUG_LEVEL_ERROR,
                      "g_spawn failed: %s", error->message);
            verification->status = RC_VERIFICATION_STATUS_UNDEF;
            verification->info = g_strconcat (
                "unable to verify signature: ", error->message, NULL);

            g_error_free (error);
            g_strfreev (env);

            return verification;
        }

        line_buf = rc_line_buf_new ();
        rc_line_buf_set_fd (line_buf, stdout_fd);

        loop = g_main_new (FALSE);

        g_signal_connect (line_buf, "read_line", (GCallback) gpg_read_line_cb,
                          verification);
        g_signal_connect (line_buf, "read_done", (GCallback) gpg_read_done_cb,
                          loop);

        g_main_run (loop);

        g_object_unref (line_buf);
        g_main_destroy (loop);

        g_strfreev (env);

        return verification;
    }
}

RCVerification *
rc_verify_md5 (gchar *filename, guint8 *md5)
{
    guint8 *cmd5;
    RCVerification *verification;

    cmd5 = rc_md5 (filename);

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_MD5;

    if (!memcmp (md5, cmd5, 16)) {
        rc_debug (RC_DEBUG_LEVEL_INFO, G_GNUC_PRETTY_FUNCTION ": good md5\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rc_debug (RC_DEBUG_LEVEL_INFO, G_GNUC_PRETTY_FUNCTION ": bad md5\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;
    }

    g_free (cmd5);

    return (verification);
}

RCVerification *
rc_verify_md5_string (gchar *filename, gchar *md5)
{
    gchar *cmd5;
    RCVerification *verification;

    cmd5 = rc_md5_digest (filename);

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_MD5;

    if (!strcmp (md5, cmd5)) {
        rc_debug (RC_DEBUG_LEVEL_INFO, G_GNUC_PRETTY_FUNCTION ": good md5\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rc_debug (RC_DEBUG_LEVEL_INFO, G_GNUC_PRETTY_FUNCTION ": bad md5\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;
    }

    g_free (cmd5);

    return (verification);
}

RCVerification *
rc_verify_size (gchar *filename, guint32 size)
{
    struct stat buf;
    RCVerification *verification;

    verification = rc_verification_new ();

    verification->type = RC_VERIFICATION_TYPE_SIZE;

    if (stat (filename, &buf) == -1) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, G_GNUC_PRETTY_FUNCTION \
                  ": couldn't stat file\n");

        verification->status = RC_VERIFICATION_STATUS_UNDEF;

        return (verification);
    }

    if (buf.st_size == size) {
        rc_debug (RC_DEBUG_LEVEL_INFO,
                  G_GNUC_PRETTY_FUNCTION ": good size check\n");

        verification->status = RC_VERIFICATION_STATUS_PASS;
    } else {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  G_GNUC_PRETTY_FUNCTION ": bad size check\n");

        verification->status = RC_VERIFICATION_STATUS_FAIL;
    }

    return (verification);
}
