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
#include "rc-common.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include <sys/wait.h>

#include "rc-util.h"
#include "rc-string.h"
#include "deps.h"

static void rc_debman_class_init (RCDebmanClass *klass);
static void rc_debman_init       (RCDebman *obj);

static GtkObjectClass *rc_debman_parent;

#define DEBMAN_DEFAULT_STATUS_FILE	"/var/lib/dpkg/status"

static char *status_file = NULL;
static char *rc_status_file = NULL;

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
} /* rc_debman_get_type */

static void
hash_destroy_pair (gchar *key, RCPackage *pkg)
{
    rc_package_free (pkg);
}

static void
hash_destroy (RCDebman *d)
{
    g_hash_table_foreach (d->pkg_hash, (GHFunc) hash_destroy_pair, NULL);
    g_hash_table_destroy (d->pkg_hash);
    d->pkg_hash = g_hash_table_new (g_str_hash, g_str_equal);
    d->hash_valid = FALSE;
}

/* Since I only want to scan through /var/lib/dpkg/status once, I need a
   shorthand way to match potentially many strings.  Returns the name of the
   package that matched, or NULL if none did. */
static gchar *
package_accept (gchar *line, RCPackageSList *pkgs)
{
    RCPackageSList *iter;

    for (iter = pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        if (!(strcmp (line + strlen ("Package: "), pkg->spec.name))) {
            return (pkg->spec.name);
        }
    }

    return (NULL);
}

/* Scan through the /var/lib/dpkg/status file, writing it out as
   /var/lib/dpkg/status.redcarpet, rewriting the Status: line of any package
   in remove_pkgs from "install ok installed" to "purge ok installed" to tell
   dpkg that we're going to eventually remove it.  This lets dpkg remove it
   itself if we are installing a package that conflicts with it.

   This system sucks hard. */
static gboolean
mark_purge (RCPackman *p, RCPackageSList *remove_pkgs)
{
    FILE *in_fp, *out_fp;
    gchar *line;
    RCLineBuf *lb;

    g_return_val_if_fail (g_slist_length (remove_pkgs) > 0, TRUE);

    if (!(in_fp = fopen (status_file, "r"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open /var/lib/dpkg/status for "
                              "reading");
        return (FALSE);
    }

    if (!(out_fp = fopen (rc_status_file, "w"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open /var/lib/dpkg/status.redcarpet "
                              "for writing");
        fclose (in_fp);
        return (FALSE);
    }

    lb = rc_line_buf_new (in_fp);
    while ((line = rc_line_buf_read (lb))) {
        gchar *package_name;

        fprintf (out_fp, "%s\n", line);

        if ((package_name = package_accept (line, remove_pkgs))) {
            gchar *status = rc_line_buf_read (lb);

            if (strcmp (status, "Status: install ok installed")) {
                gchar *error;

                error = g_strdup_printf ("Package %s is not installed "
                                         "correctly (\"%s\")", package_name,
                                         status);
                rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                                      error);
                g_free (error);

                fclose (in_fp);
                g_free (status);
                g_free (line);

                return (FALSE);
            }

            g_free (status);

            fprintf (out_fp, "Status: purge ok installed\n");
        }

        g_free (line);
    }
    rc_line_buf_destroy (lb);

    fclose (out_fp);
    fclose (in_fp);

    if (rename (rc_status_file, status_file)) {
        unlink (rc_status_file);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to rename "
                              "RC status file");
        return (FALSE);
    }

    return (TRUE);
}

typedef struct _InstallState InstallState;

struct _InstallState {
    guint seqno;
    guint total;
};

/* Lock the dpkg database.  Too bad this just doesn't work (and doesn't work
   in apt or dselect either).  Releasing the lock to call dpkg and trying to
   grab it afterwords is probably the most broken thing I've ever heard of. */
static int
lock_database (RCDebman *p)
{
    int fd;
    struct flock fl;

    g_return_val_if_fail (p->lock_fd == -1, 0);

    fd = open ("/var/lib/dpkg/lock", O_RDWR | O_CREAT | O_TRUNC, 0640);

    if (fd < 0) {
        rc_packman_set_error (RC_PACKMAN (p), RC_PACKMAN_ERROR_FATAL,
                              "Unable to lock /var/lib/dpkg/lock.  Perhaps "
                              "another process is accessing the dpkg "
                              "database?");
        return (0);
    }

    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl (fd, F_SETLK, &fl) == -1) {
        if (errno != ENOLCK) {
            rc_packman_set_error (RC_PACKMAN (p), RC_PACKMAN_ERROR_FATAL,
                                  "Unable to lock /var/lib/dpkg/status. "
                                  "Perhaps another process is accessing the "
                                  "dpkg database?");
            close (fd);
            return (0);
        }
    }

    p->lock_fd = fd;

    return (1);
}

static void
unlock_database (RCDebman *p)
{
    close (p->lock_fd);
    p->lock_fd = -1;
}

/* basically fork dpkg --purge --pending.  Parses stdout to emit transaction
   steps as appropriate */
static gboolean
do_purge (RCPackman *p, InstallState *state)
{
    pid_t child;       /* The pid of the child process */
    int status;        /* The return value of dpkg */
    int fds[2];        /* The ends of the pipe */
    gchar *buf = NULL; /* The buffer of shit read from the pipe */
    gchar single[2];   /* A character read from the pipe */

    pipe (fds);
    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    unlock_database (RC_DEBMAN (p));

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
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
        dup2 (fds[1], 1);

        fclose (stderr);

#ifdef DEBUG
        fprintf (stderr, "rc-debman: /usr/bin/dpkg --purge --pending\n--- log ---\n");
#endif

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--purge", "--pending", NULL);
        break;

    default:
        close (fds[1]);
        single[1] = '\000';
        while ((status = read (fds[0], single, 1))) {
            if (status == 1) {
#ifdef DEBUG
                fputc (single[0], stderr);
#endif
                if (single[0] == '\n' && buf) {
                    if (!strncmp (buf, "Removing", strlen ("Removing"))) {
                        rc_packman_transaction_step (p, ++state->seqno,
                                                     state->total);
                    }

                    g_free (buf);
                    buf = NULL;
                } else {
                    if (buf) {
                        gchar *tmp = g_strconcat (buf, single, NULL);
                        g_free (buf);
                        buf = tmp;
                    } else {
                        buf = g_strdup (single);
                    }
                }
            }

            while (gtk_events_pending ()) {
                gtk_main_iteration ();
            }
        }
        waitpid (child, &status, WNOHANG);
        break;
    }

    if (!(lock_database (RC_DEBMAN (p)))) {
        /* FIXME: need to handle this */
    }

#ifdef DEBUG
    fprintf (stderr, "--- end ---\n");
#endif


    close (fds[0]);
    g_free (buf);

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to run dpkg to finish purging selected "
                              "packages.  Packages were marked for removal, "
                              "but were not removed.  To remedy this, run the "
                              "command 'dpkg --purge --pending' as root.");
        return (FALSE);
    }

    return (TRUE);
}

/* Fork dpkg to unpack the selected packages.  Parses the output to emit
   transaction steps (both for unpacked packages and for packages that dpkg
   finishes the purge on to put new ones in their place. */
static gboolean
do_unpack (RCPackman *p, GSList *pkgs, InstallState *state)
{
    guint num_pkgs;    /* The number of packages to install */
    gchar **argv;      /* The command to exec */
    GSList *iter;      /* To iterate over the pkgs list */
    int fds[2];        /* both ends of the pipe */
    pid_t child;       /* The pid of the forked child */
    gchar single[2];   /* To read a character at a time from the pipe */
    int status;        /* The return value of the forked process */
    gchar *buf = NULL; /* The buffer of crap read from the pipe */
    guint i;

    num_pkgs = g_slist_length (pkgs);

    g_return_val_if_fail (num_pkgs > 0, TRUE);

    argv = g_new0 (gchar *, num_pkgs + 3);

#ifdef DEBUG
    fprintf (stderr, "rc-debman: /usr/bin/dpkg --unpack ");
#endif

    argv[0] = g_strdup ("/usr/bin/dpkg");
    argv[1] = g_strdup ("--unpack");

    i = 2;

    for (iter = pkgs; iter; iter = iter->next) {
        argv[i++] = g_strdup ((gchar *)(iter->data));
#ifdef DEBUG
        fprintf (stderr, "%s ", argv[i-1]);
#endif
    }

#ifdef DEBUG
    fprintf (stderr, "\n--- log ---\n");
#endif

    pipe (fds);
    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    unlock_database (RC_DEBMAN (p));

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to fork and exec dpkg to unpack "
                              "selected packages.");
        g_strfreev (argv);
        close (fds[0]);
        close (fds[1]);
        return (FALSE);
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL); /* All hail dpkg */

        fflush (stdout);   /* Flush stdout, and then set up stdout to write */
        close (fds[0]);    /* to one end of our pipe.  Then we do the nasty */
        dup2 (fds[1], 1);  /* parse the output hack.  Ick. */

        fclose (stderr);

        execv ("/usr/bin/dpkg", argv);
        break;

    default:
        close (fds[1]);
        single[1] = '\000';
        while ((status = read (fds[0], single, 1))) {
            if (status == 1) {
#ifdef DEBUG
                fputc (single[0], stderr);
#endif
                if (single[0] == '\n') {
                    if ((!strncmp (buf, "Unpacking", strlen ("Unpacking"))) ||
                        (!strncmp (buf, "Purging ", strlen ("Purging")))) {
                        rc_packman_transaction_step (p, ++state->seqno,
                                                     state->total);
                    }

                    g_free (buf);
                    buf = NULL;
                } else {
                    if (buf) {
                        gchar *tmp = g_strconcat (buf, single, NULL);
                        g_free (buf);
                        buf = tmp;
                    } else {
                        buf = g_strdup (single);
                    }
                }
            }

            while (gtk_events_pending ()) {
                gtk_main_iteration ();
            }
        }

        waitpid (child, &status, 0);
        break;
    }

    if (!(lock_database (RC_DEBMAN (p)))) {
        /* FIXME: need to handle this */
    }

#ifdef DEBUG
    fprintf (stderr, "--- end ---\n");
#endif

    close (fds[0]);
    g_free (buf);
    g_strfreev (argv);

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        gchar *error, *tmp;
        GSList *iter;

        error = g_strdup ("dpkg failed to unpack all of the selected "
                          "packages.  The following packages may have been "
                          "left in an inconsistant state, and should be "
                          "checked by hand:");

        for (iter = pkgs; iter; iter = iter->next) {
            tmp = g_strconcat (error, " ", (gchar *)(iter->data), NULL);
            g_free (error);
            error = tmp;
        }

        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL, error);

        g_free (error);

        return (FALSE);
    }

    return (TRUE);
}

/* Fork dpkg to configure the packages we've unpacked.  Emits configure step
   signals when appropriate. */
static gboolean
do_configure (RCPackman *p, InstallState *state)
{
    pid_t child;
    int status;
    int fds[2];
    gchar single[2];
    gchar *buf = NULL;

    pipe (fds);
    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    unlock_database (RC_DEBMAN (p));

#ifdef DEBUG
    fprintf (stderr, "rc-debman: /usr/bin/dpkg --configure --pending\n--- log ---\n");
#endif

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to fork dpkg to configure unpacked "
                              "packages.  You will need to configure them by "
                              "hand by running the command `dpkg --configure "
                              "--pending` as root");
        close (fds[0]);
        close (fds[1]);
        return (FALSE);
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL);

        fflush (stdout);
        close (fds[0]);
        dup2 (fds[1], 1);

        fclose (stderr);

        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--configure", "--pending",
               NULL);
        break;

    default:
        close (fds[1]);
        single[1] = '\000';
        while ((status = read (fds[0], single, 1))) {
            if (status == 1) {
#ifdef DEBUG
                fputc (single[0], stderr);
#endif
                if (single[0] == '\n') {
                    if (buf && (!strncmp (buf, "Setting up ",
                                   strlen ("Setting up ")))) {
                        rc_packman_configure_step (p, ++state->seqno,
                                                   state->total);
                    }

                    g_free (buf);
                    buf = NULL;
                } else {
                    if (buf) {
                        gchar *tmp = g_strconcat (buf, single, NULL);
                        g_free (buf);
                        buf = tmp;
                    } else {
                        buf = g_strdup (single);
                    }
                }
            }
            while (gtk_events_pending ()) {
                gtk_main_iteration ();
            }
        }

        waitpid (child, &status, 0);
        break;
    }

    if (!(lock_database (RC_DEBMAN (p)))) {
        /* FIXME: need to handle this */
    }

#ifdef DEBUG
    fprintf (stderr, "--- end ---\n");
#endif

    close (fds[0]);
    g_free (buf);

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

static void
rc_debman_transact (RCPackman *p, GSList *install_pkgs,
                    RCPackageSList *remove_pkgs)
{
    InstallState *state = g_new0 (InstallState, 1);

    state->total = g_slist_length (install_pkgs) +
        g_slist_length (remove_pkgs);

    /* Mark the packages which are eventually to be removed in
       /var/lib/dpkg/status.  We do this so that dpkg will feel free to replace
       them as necessary, without fucking up the dependencies.  We'll just
       --purge --pending at the end to make sure everything gets cleaned up. */
    if (remove_pkgs) {
        if (!(mark_purge (p, remove_pkgs))) {
            goto END;
        }
    }

    /* Now let's unpack the packages which are to be installed.  This stage may
       result in some of the packages marked for purging being removed, so we
       catch that, too, and generate a transaction step signal. */
    if (install_pkgs) {
        if (!(do_unpack (p, install_pkgs, state))) {
            goto END;
        }
    }

    /* And now, of course, clean out any old packages which didn't get purged
       by dpkg in the course of installing new things */
    if (remove_pkgs && (state->seqno < state->total)) {
        if (!(do_purge (p, state))) {
            goto END;
        }
    }

    state->total = g_slist_length (install_pkgs);
    state->seqno = 0;

    /* We need to --configure the unpacked packages here */
    if (install_pkgs) {
        if (!(do_configure (p, state))) {
            goto END;
        }
    }

  END:
    g_free (state);
    hash_destroy (RC_DEBMAN (p));
}

/* Takes a string of format [<epoch>:]<version>[-<release>], and returns those
   three elements, if they exist */
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

/* Given a line from a debian control file (or from /var/lib/dpkg/status), and
   a pointer to a RCPackageDepSList, fill in the dependency information.  It's
   ugly but memprof says it doesn't leak. */
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

/* Given a file pointer to the beginning of a block of package information,
   fill in the data in the RCPackage fields. */
static void
rc_debman_query_helper (RCLineBuf *lb, RCPackage **old_pkg)
{
    gchar *buf;
    RCPackageDepItem *item;
    RCPackageDep *dep = NULL;
    gboolean desc = FALSE;
    RCPackage *pkg = rc_package_new ();

    while ((buf = rc_line_buf_read (lb)) && buf[0]) {
        if (!strncmp (buf, "Status: install ok installed",
                      strlen ("Status: install ok installed"))) {
            pkg->spec.installed = TRUE;
        } else if (!strncmp (buf, "Installed-Size: ",
                             strlen ("Installed-Size: "))) {
            pkg->spec.installed_size = strtoul (buf +
                                                strlen ("Installed-Size: "),
                                                NULL, 10) * 1024;
        } else if (!strncmp (buf, "Description: ", strlen ("Description: "))) {
            pkg->summary = g_strdup (buf + strlen ("Description: "));
            desc = TRUE;
        } else if (!strncmp (buf, " ", strlen (" ")) && desc) {
            gchar *buf2;

            if (!strcmp (buf, " .")) {
                buf2 = g_strdup ("\n");
            } else {
                buf2 = g_strdup (buf);
            }

            if (pkg->description) {
                gchar *tmp = g_strconcat (pkg->description,
                                          buf2 + strlen (" "), "\n", NULL);
                g_free (pkg->description);
                pkg->description = tmp;
            } else {
                pkg->description = g_strconcat (buf2 + strlen (" "), "\n",
                                                NULL);
            }

            g_free (buf2);
        } else if (!strncmp (buf, "Version: ", strlen ("Version: "))) {
            rc_debman_parse_version (buf + strlen ("Version: "),
                                     &pkg->spec.epoch, &pkg->spec.version,
                                     &pkg->spec.release);
        } else if (!strncmp (buf, "Section: ", strlen ("Section: "))) {
            gchar **tokens = g_strsplit (buf + strlen ("Section: "), "/", -1);
            gint i = 0;
            gchar *sec;
            while (tokens[i+1] != NULL) i++;
            sec = g_strchomp (tokens[i]);
            if (!g_strcasecmp (sec, "admin")) {
                pkg->spec.section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (sec, "base")) {
                pkg->spec.section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (sec, "comm")) {
                pkg->spec.section = SECTION_INTERNET;
            } else if (!g_strcasecmp (sec, "devel")) {
                pkg->spec.section = SECTION_DEVEL;
            } else if (!g_strcasecmp (sec, "doc")) {
                pkg->spec.section = SECTION_DOC;
            } else if (!g_strcasecmp (sec, "editors")) {
                pkg->spec.section = SECTION_UTIL;
            } else if (!g_strcasecmp (sec, "electronics")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "games")) {
                pkg->spec.section = SECTION_GAME;
            } else if (!g_strcasecmp (sec, "graphics")) {
                pkg->spec.section = SECTION_IMAGING;
            } else if (!g_strcasecmp (sec, "hamradio")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "interpreters")) {
                pkg->spec.section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (sec, "libs")) {
                pkg->spec.section = SECTION_LIBRARY;
            } else if (!g_strcasecmp (sec, "mail")) {
                pkg->spec.section = SECTION_PIM;
            } else if (!g_strcasecmp (sec, "math")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "misc")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "net")) {
                pkg->spec.section = SECTION_INTERNET;
            } else if (!g_strcasecmp (sec, "news")) {
                pkg->spec.section = SECTION_INTERNET;
            } else if (!g_strcasecmp (sec, "oldlibs")) {
                pkg->spec.section = SECTION_LIBRARY;
            } else if (!g_strcasecmp (sec, "otherosfs")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "science")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "shells")) {
                pkg->spec.section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (sec, "sound")) {
                pkg->spec.section = SECTION_MULTIMEDIA;
            } else if (!g_strcasecmp (sec, "tex")) {
                pkg->spec.section = SECTION_MISC;
            } else if (!g_strcasecmp (sec, "text")) {
                pkg->spec.section = SECTION_UTIL;
            } else if (!g_strcasecmp (sec, "utils")) {
                pkg->spec.section = SECTION_UTIL;
            } else if (!g_strcasecmp (sec, "web")) {
                pkg->spec.section = SECTION_INTERNET;
            } else if (!g_strcasecmp (sec, "x11")) {
                pkg->spec.section = SECTION_XAPP;
            } else {
                pkg->spec.section = SECTION_MISC;
            }
            g_strfreev (tokens);
        } else if (!strncmp (buf, "Depends: ", strlen ("Depends: "))) {
            pkg->requires = rc_debman_fill_depends (buf + strlen ("Depends: "),
                                                    pkg->requires);
        } else if (!strncmp (buf, "Recommends: ", strlen ("Recommends: "))) {
            pkg->recommends = rc_debman_fill_depends (buf +
                                                    strlen ("Recommends: "),
                                                    pkg->recommends);
        } else if (!strncmp (buf, "Suggests: ", strlen ("Suggests: "))) {
            pkg->suggests = rc_debman_fill_depends (buf +
                                                    strlen ("Suggests: "),
                                                    pkg->suggests);
        } else if (!strncmp (buf, "Pre-Depends: ", strlen ("Pre-Depends: "))) {
            pkg->requires = rc_debman_fill_depends (buf +
                                                    strlen ("Pre-Depends: "),
                                                    pkg->requires);
        } else if (!strncmp (buf, "Conflicts: ", strlen ("Conflicts: "))) {
            pkg->conflicts = rc_debman_fill_depends (buf +
                                                     strlen ("Conflicts: "),
                                                     pkg->conflicts);
            /*
        } else if (!strncmp (buf, "Replaces: ", strlen ("Replaces: "))) {
            pkg->conflicts = rc_debman_fill_depends (buf +
                                                     strlen ("Replaces: "),
                                                     pkg->conflicts);
            */
        } else if (!strncmp (buf, "Provides: ", strlen ("Provides: "))) {
            pkg->provides = rc_debman_fill_depends (buf +
                                                    strlen ("Provides: "),
                                                    pkg->provides);
        }

        g_free (buf);
    }

    g_free (buf);

    /* Trim the trailing \n in the description */
    if (pkg->description) {
        guint length = strlen (pkg->description);
        if (length > 1) {
            pkg->description[length - 2] = '\0';
        }
    }

    if (!(strcmp ((*old_pkg)->spec.name, "gaim"))) {
        sleep (1);
    }

    /* Check and see if this is what I was looking for */
    if ((!((*old_pkg)->spec.version) ||
         !strcmp ((*old_pkg)->spec.version, pkg->spec.version)) &&
        (!((*old_pkg)->spec.release) ||
         !strcmp ((*old_pkg)->spec.release, pkg->spec.release))) {

        pkg->spec.name = g_strdup ((*old_pkg)->spec.name);
        /* Make sure to provide myself, for the dep code! */
        item = rc_package_dep_item_new (pkg->spec.name, pkg->spec.epoch,
                                        pkg->spec.version, pkg->spec.release,
                                        RC_RELATION_EQUAL);
        dep = g_slist_append (NULL, item);
        pkg->provides = g_slist_append (pkg->provides, dep);

        rc_package_free (*old_pkg);
        *old_pkg = pkg;
    } else {
        rc_package_free (pkg);
    }
}

static void
rc_debman_query_all_real (RCPackman *p)
{
    FILE *fp;
    gchar *buf;
    RCLineBuf *lb;
    RCDebman *d = RC_DEBMAN (p);

    if (!(fp = fopen ("/var/lib/dpkg/status", "r"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open /var/lib/dpkg/status");
        return;
    }

    lb = rc_line_buf_new (fp);

    while ((buf = rc_line_buf_read (lb))) {
        RCPackage *pkg = rc_package_new ();

        if (strncmp (buf, "Package: ", strlen ("Package: "))) {
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Malformed /var/lib/dpkg/status file");
            g_free (buf);
            rc_package_free (pkg);
            return;
        }

        pkg->spec.name = g_strdup (buf + strlen ("Package: "));

        g_free (buf);

        rc_debman_query_helper (lb, &pkg);

        if (pkg->spec.installed) {
            g_hash_table_insert (d->pkg_hash,
                                 (gpointer) pkg->spec.name,
                                 (gpointer) pkg);
        } else {
            rc_package_free (pkg);
        }
    }

    d->hash_valid = TRUE;

    rc_line_buf_destroy (lb);
    fclose (fp);
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

    if (!(RC_DEBMAN (p)->hash_valid)) {
        rc_debman_query_all_real (p);
    }

    g_hash_table_foreach (RC_DEBMAN (p)->pkg_hash, (GHFunc) pkg_list_append,
                          &pkgs);

    return pkgs;
}

#if 0
static RCPackage *
rc_debman_query_real (RCPackman *p, RCPackage *pkg)
{
    FILE *fp;
    gchar *buf;
    gchar *target;
    RCLineBuf *lb;

    if (!(fp = fopen (status_file, "r"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open /var/lib/dpkg/status");
        return (pkg);
    }

    pkg->spec.installed = FALSE;

    g_assert (pkg);
    g_assert (pkg->spec.name);

    target = g_strconcat ("Package: ", pkg->spec.name, NULL);

    lb = rc_line_buf_new (fp);
    while ((buf = rc_line_buf_read (lb))) {
        if (!strcmp (buf, target)) {
            g_free (buf);
            rc_debman_query_helper (lb, &pkg);
            break;
        }

        g_free (buf);
    }
    rc_line_buf_destroy (lb);

    g_free (target);
    fclose (fp);
    
    return (pkg);
}
#endif

static RCPackage *
rc_debman_query (RCPackman *p, RCPackage *pkg)
{
    RCDebman *d = RC_DEBMAN (p);
    RCPackage *lpkg;

    g_assert (pkg);
    g_assert (pkg->spec.name);

    if (!d->hash_valid) {
#if 0
        return (rc_debman_query_real (p, pkg));
#endif
        rc_debman_query_all_real (p);
    }

    lpkg = g_hash_table_lookup (d->pkg_hash, (gconstpointer) pkg->spec.name);

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
rc_debman_query_file (RCPackman *p, gchar *filename)
{
    gchar *file;
    pid_t child;
    FILE *fp;
    RCPackage *pkg = rc_package_new ();
    gchar *buf;
    gchar *path;
    RCLineBuf *lb;

    path = rc_mktmpdir (NULL);

    if (!(child = fork())) {
        fclose (stdout);
        fclose (stderr);
        signal (SIGPIPE, SIG_DFL);
        execl ("/usr/bin/dpkg-deb", "/usr/bin/dpkg-deb", "--control",
               filename, path, NULL);
    } else {
        /* FIXME: do some error checking */

        while (!waitpid (child, NULL, WNOHANG)) {
            while (gtk_events_pending ()) {
                gtk_main_iteration ();
            }
        }
    }

    file = g_strconcat (path, "/control", NULL);

    if (!(fp = fopen (file, "r"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open package control file");
        rc_rmdir (path);
        g_free (path);
        g_free (file);
        return (NULL);
    }

    
    g_free (file);

    lb = rc_line_buf_new (fp);
    buf = rc_line_buf_read (lb);

    pkg->spec.name = g_strdup (buf + strlen ("Package: "));

    g_free (buf);

    rc_debman_query_helper (lb, &pkg);
    rc_line_buf_destroy (lb);
    fclose (fp);

    rc_rmdir (path);

    g_free (path);

    return (pkg);
}

static void
rc_debman_destroy (GtkObject *obj)
{
    RCDebman *d = RC_DEBMAN (obj);

    unlock_database (d);

    hash_destroy (d);

    if (GTK_OBJECT_CLASS (rc_debman_parent)->destroy)
        (* GTK_OBJECT_CLASS (rc_debman_parent)->destroy) (obj);
}

static void
rc_debman_class_init (RCDebmanClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    RCPackmanClass *rcpc = (RCPackmanClass *) klass;

    object_class->destroy = rc_debman_destroy;

    rc_debman_parent = gtk_type_class (rc_packman_get_type ());

    rcpc->rc_packman_real_transact = rc_debman_transact;
    rcpc->rc_packman_real_query = rc_debman_query;
    rcpc->rc_packman_real_query_file = rc_debman_query_file;
    rcpc->rc_packman_real_query_all = rc_debman_query_all;

    putenv ("DEBIAN_FRONTEND=noninteractive");

    deps_conflicts_use_virtual_packages (FALSE);
} /* rc_debman_class_init */

static void
rc_debman_init (RCDebman *obj)
{
    RCPackman *p = RC_PACKMAN (obj);

    p->pre_config = FALSE;
    p->pkg_progress = FALSE;
    p->post_config = TRUE;

    obj->lock_fd = -1;

    lock_database (obj);

    obj->pkg_hash = g_hash_table_new (g_str_hash, g_str_equal);
    obj->hash_valid = FALSE;
} /* rc_debman_init */

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
} /* rc_debman_new */
