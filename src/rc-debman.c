/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public License
 *    as published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *    GNU Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "rc-packman-private.h"

#include "rc-debman.h"

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/wait.h>

static void rc_debman_class_init (RCDebmanClass *klass);
static void rc_debman_init       (RCDebman *obj);

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

/* Helper function used by rc_debman_install during its "Oh fuck" disaster
   recovery phase, and also by rc_debman_remove.  Given a GSList * of package
   names, remove them from the system.

   Returns 0 on success, non-zero on failure.

   If it returns non-zero, we're really fucked.
*/
static gint
rc_debman_do_purge (GSList *pkgs)
{
    gchar **filev = g_new0 (gchar *, g_slist_length (pkgs) + 3);
    guint i = 0;
    pid_t child;
    GSList *iter;

    filev[0] = g_strdup ("/usr/bin/dpkg");
    filev[1] = g_strdup ("--purge");

    /* Assemble an array of strings of the package names */
    for (iter = pkgs; iter; iter = iter->next) {
        gchar *pkg = (gchar *)(iter->data);

        /* This would definitely be an issue */
        if (!pkg) {
            g_strfreev (filev);
            return (-1);
        }

        filev[2 + i++] = g_strdup (pkg);
    }

    if (!(child = fork ())) {
        /* Purge the packages */
        fclose (stdout);
        signal (SIGPIPE, SIG_DFL);
        execv ("/usr/bin/dpkg", filev);
    } else {
        gint status;

        /* While we're waiting for dpkg to return, keep everything as
           responsive as possible */
        while (!waitpid (child, &status, WNOHANG)) {
            while (gtk_events_pending ()) {
                gtk_main_iteration ();
            }
        }

        g_strfreev (filev);

        /* Just use the dpkg return value here, basically */
        if (WIFEXITED (status)) {
            return (WEXITSTATUS (status));
        } else {
            return (-1);
        }
    }

    g_assert_not_reached ();

    return (-1);
}

static void
rc_debman_install (RCPackman *p, GSList *pkgs)
{
    GSList *iter;
    pid_t child;
    int fds[2];
    gchar **argv;
    guint i;
    gchar *buf = NULL, *tmp = NULL, single[2];
    guint length, count = 0;
    gint status;

    length = g_slist_length (pkgs);

    argv = g_new0 (gchar *, length + 3);
    single[1] = '\0';

    argv[0] = g_strdup ("/usr/bin/dpkg");
    argv[1] = g_strdup ("--unpack");

    i = 2;

    for (iter = pkgs; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);

        argv[i++] = g_strdup (filename);
    }

    pipe (fds);
    fcntl (fds[0], F_SETFL, O_NONBLOCK);
    fcntl (fds[1], F_SETFL, O_NONBLOCK);

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to fork second process");
        g_strfreev (argv);
        close (fds[0]);
        close (fds[1]);
        return;
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL);

        fflush (stdout);
        close (fds[0]);
        dup2 (fds[1], 1);

        execv ("/usr/bin/dpkg", argv);
        break;

    default:
        close (fds[1]);
        while ((status = read (fds[0], single, 1))) {
            if (status == 1) {
                if (single[0] == '\n') {
                    if (!strncmp (buf, "Unpacking", strlen ("Unpacking"))) {
                        rc_packman_package_installed (p, "foo", ++count,
                                                      length);
                    }

                    g_free (buf);
                    buf = NULL;
                } else {
                    if (buf) {
                        tmp = g_strconcat (buf, single, NULL);
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

    close (fds[0]);
    g_free (buf);
    g_strfreev (argv);

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        gchar *error, *tmp;
        GSList *iter;

        error = g_strdup ("dpkg failed to install the selected packages.  The "
                          "following packages may have been left in an "
                          "inconsistant state, and should be checked by "
                          "hand:");

        for (iter = pkgs; iter; iter = iter->next) {
            tmp = g_strconcat (error, " ", (gchar *)(iter->data), NULL);
            g_free (error);
            error = tmp;
        }

        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL, error);

        g_free (error);

        return;
    }

    rc_packman_install_done (p);

    child = fork ();

    switch (child) {
    case -1:
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to fork process.  The packages have "
                              "been installed, but not configured.  You will "
                              "need to configure them by hand, by running the "
                              "command `dpkg --configure --pending` as root");
        return;
        break;

    case 0:
        signal (SIGPIPE, SIG_DFL);
        fclose (stdout);
        
        execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--configure", "--pending",
               NULL);
        break;

    default:
        while (!waitpid (child, &status, WNOHANG)) {
            while (gtk_events_pending ()) {
                gtk_main_iteration ();
            }
        }
        break;
    }

    if (!(WIFEXITED (status)) || WEXITSTATUS (status)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to run dpkg.  The packages have been "
                              "installed, but not configured.  You will need "
                              "to configure them by hand, by running the "
                              "command `dpkg --configure --pending` as root.");
    }

    rc_packman_configure_done (p);
}

static void
rc_debman_remove (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter;
    GSList *names = NULL;
    gint ret;

    for (iter = pkgs; iter; iter = iter->next) {
        names = g_slist_append (names, ((RCPackage *)(iter->data))->spec.name);
    }

    ret = rc_debman_do_purge (names);

    /* Only free the list, not the data */
    g_slist_free (names);

    if (!ret) {
        rc_packman_remove_done (p);
    } else {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL, "Package remove "
                              "failed.  Some or all packages may have been "
                              "left in an inconsistant state.");
        rc_packman_remove_done (p);
    }
} /* rc_debman_remove */

/* Takes a string of format [<epoch>:]<version>[-<release>], and returns those
   three elements, if they exist */
static void
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

/* Read a line from a file.  If the returned string is non-NULL, you must free
   it.  Return NULL only on EOF, and return "" for blank lines. */
static gchar *
readline (FILE *fp) {
    char buf[1024];
    gchar *ret;

    /* FIXME: do this right, instead of like this.  You know what I mean. */

    if (fgets (buf, 1024, fp)) {
        ret = g_strdup (buf);
        ret = g_strchomp (ret);
        return (ret);
    } else {
        return (NULL);
    }
}

/* Given a line from a debian control file (or from /var/lib/dpkg/status), and
   a pointer to a RCPackageDepSList, fill in the dependency information.  It's
   ugly but memprof says it doesn't leak. */
static RCPackageDepSList *
rc_debman_fill_depends (gchar *input, RCPackageDepSList *list)
{
    gchar **deps;
    guint i;

    /* All evidence indicates that the fields are comma-space separated, but if
       that ever turns out to be incorrect, we'll have to do this part more
       carefully. */
    deps = g_strsplit (input, ", ", 0);

    for (i = 0; deps[i]; i++) {
        gchar **elems = g_strsplit (deps[i], " | ", 0);
        guint j;
        RCPackageDep *dep = NULL;

        /* For the most part, there's only one element per dep, except for the
           rare case of OR'd deps. */
        for (j = 0; elems[j]; j++) {
            RCPackageDepItem *depi;
            gchar **parts;

            /* A space separates the name of the dep from the relationship and
               version. */
            parts = g_strsplit (elems[j], " ", 1);

            if (!parts[1]) {
                /* There's no version in this dependency, just a name. */
                depi = rc_package_dep_item_new (parts[0], 0, NULL, NULL,
                                                RC_RELATION_ANY);
            } else {
                /* We've got to parse the rest of this mess. */
                gchar *relstring, *verstring;
                guint relation = RC_RELATION_ANY;
                guint32 epoch;
                gchar *version, *release;

                if (parts[1][1] == '=') {
                    relstring = g_strndup (parts[1] + 1, 1);
                    verstring = g_strndup (parts[1] + 2,
                                           strlen (parts[1] + 2) - 1);
                } else {
                    relstring = g_strndup (parts[1] + 1, 2);
                    verstring = g_strndup (parts[1] + 3,
                                           strlen (parts[1] + 3) - 1);
                }

                verstring = g_strstrip (verstring);

                /* The possible relations are "=", ">>", "<<", ">=", and "<=",
                   decide which apply */
                switch (relstring[0]) {
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

                if (relstring[1] && (relstring[1] == '=')) {
                    relation |= RC_RELATION_EQUAL;
                }

                g_free (relstring);

                /* Break the single version string up */
                rc_debman_parse_version (verstring, &epoch, &version,
                                         &release);

                g_free (verstring);

                depi = rc_package_dep_item_new (parts[0], epoch, version,
                                                release, relation);

                g_free (version);
                g_free (release);
            }

            g_strfreev (parts);

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
rc_debman_query_helper (FILE *fp, RCPackage *pkg)
{
    gchar *buf;
    RCPackageDepItem *item;
    RCPackageDep *dep = NULL;
    gboolean desc = FALSE;

    while ((buf = readline (fp)) && buf[0]) {
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
        } else if (!strncmp (buf, "Depends: ", strlen ("Depends: "))) {
            pkg->requires = rc_debman_fill_depends (buf + strlen ("Depends: "),
                                                    pkg->requires);
            /*
        } else if (!strncmp (buf, "Recommends: ", strlen ("Recommends: "))) {
            pkg->requires = rc_debman_fill_depends (buf +
                                                    strlen ("Recommends: "),
                                                    pkg->requires);
            */
            /*
        } else if (!strncmp (buf, "Suggests: ", strlen ("Suggests: "))) {
            pkg->requires = rc_debman_fill_depends (buf +
                                                    strlen ("Suggests: "),
                                                    pkg->requires); */
        } else if (!strncmp (buf, "Pre-Depends: ", strlen ("Pre-Depends: "))) {
            pkg->requires = rc_debman_fill_depends (buf +
                                                    strlen ("Pre-Depends: "),
                                                    pkg->requires);
        } else if (!strncmp (buf, "Conflicts: ", strlen ("Conflicts: "))) {
            pkg->conflicts = rc_debman_fill_depends (buf +
                                                     strlen ("Conflicts: "),
                                                     pkg->conflicts);
        } else if (!strncmp (buf, "Replaces: ", strlen ("Replaces: "))) {
            pkg->conflicts = rc_debman_fill_depends (buf +
                                                     strlen ("Replaces: "),
                                                     pkg->conflicts);
        } else if (!strncmp (buf, "Provides: ", strlen ("Provides: "))) {
            pkg->provides = rc_debman_fill_depends (buf +
                                                    strlen ("Provides: "),
                                                    pkg->provides);
        }

        g_free (buf);
    }

    g_free (buf);

    /* Make sure to provide myself, for the dep code! */
    item = rc_package_dep_item_new (pkg->spec.name, pkg->spec.epoch,
                                    pkg->spec.version, pkg->spec.release,
                                    RC_RELATION_EQUAL);
    dep = g_slist_append (NULL, item);
    pkg->provides = g_slist_append (pkg->provides, dep);
}

static RCPackage *
rc_debman_query (RCPackman *p, RCPackage *pkg)
{
    FILE *fp;
    gchar *buf;
    gchar *target;

    if (!(fp = fopen ("/var/lib/dpkg/status", "r"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_ABORT,
                              "Unable to open /var/lib/dpkg/status");
        return (pkg);
    }

    pkg->spec.installed = FALSE;

    g_assert (pkg);
    g_assert (pkg->spec.name);

    target = g_strconcat ("Package: ", pkg->spec.name, NULL);

    while ((buf = readline (fp))) {
        if (!strcmp (buf, target)) {
            g_free (buf);
            rc_debman_query_helper (fp, pkg);
            break;
        }

        g_free (buf);
    }

    g_free (target);

    fclose (fp);
    
    return (pkg);
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

    path = rc_mktmpdir (NULL);

    if (!(child = fork())) {
        fclose (stdout);
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

    buf = readline (fp);

    pkg->spec.name = g_strdup (buf + strlen ("Package: "));

    g_free (buf);

    rc_debman_query_helper (fp, pkg);

    rc_rmdir (path);

    g_free (path);

    return (pkg);
}

static RCPackageSList *
rc_debman_query_all (RCPackman *p)
{
    FILE *fp;
    gchar *buf;
    RCPackageSList *pkgs = NULL;

    if (!(fp = fopen ("/var/lib/dpkg/status", "r"))) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open /var/lib/dpkg/status");
        return (NULL);
    }

    while ((buf = readline (fp))) {
        RCPackage *pkg = rc_package_new ();

        if (strncmp (buf, "Package: ", strlen ("Package: "))) {
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Malformed /var/lib/dpkg/status file");
            g_free (buf);
            rc_package_free (pkg);
            rc_package_slist_free (pkgs);
            return (NULL);
        }

        pkg->spec.name = g_strdup (buf + strlen ("Package: "));

        g_free (buf);

        rc_debman_query_helper (fp, pkg);

        if (pkg->spec.installed) {
            pkgs = g_slist_append (pkgs, pkg);
        } else {
            rc_package_free (pkg);
        }
    }

    return (pkgs);
}

static void
rc_debman_class_init (RCDebmanClass *klass)
{
    RCPackmanClass *rcpc = (RCPackmanClass *) klass;

    rcpc->rc_packman_real_install = rc_debman_install;
    rcpc->rc_packman_real_remove = rc_debman_remove;
    rcpc->rc_packman_real_query = rc_debman_query;
    rcpc->rc_packman_real_query_file = rc_debman_query_file;
    rcpc->rc_packman_real_query_all = rc_debman_query_all;

    putenv ("DEBIAN_FRONTEND=noninteractive");
} /* rc_debman_class_init */

static void
rc_debman_init (RCDebman *obj)
{
    RCPackman *p = RC_PACKMAN (obj);

    p->pre_config = FALSE;
    p->pkg_progress = FALSE;
    p->post_config = TRUE;
} /* rc_debman_init */

RCDebman *
rc_debman_new (void)
{
    RCDebman *new =
        RC_DEBMAN (gtk_type_new (rc_debman_get_type ()));

    return new;
} /* rc_debman_new */
