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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

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
   names, remove them from the system. */
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
        g_assert (pkg);

        filev[2 + i++] = g_strdup (pkg);
    }

    if (!(child = fork ())) {
        fclose (stdout);
        execv ("/usr/bin/dpkg", filev);
    } else {
        gint status;

        waitpid (child, &status, 0);

        g_strfreev (filev);

        if (WIFEXITED (status)) {
            return (WEXITSTATUS (status));
        } else {
            return (-1);
        }
    }

    g_assert_not_reached ();

    return (-1);
}

/* Install the packages listed in pkgs.  Because we don't have a convenient
   library with callbacks to use here, we've got to fork dpkg, once per
   package.  So, on the first pass through all of the packages, we unpack all
   of the packages, and at the end we configure them.  The only tricky part is
   keeping a list of all of the packages we've already unpacked, so that if we
   have to abort at any point, we can purge the unpacked but unconfigured
   packages. */

static void
rc_debman_install (RCPackman *p, GSList *pkgs)
{
    GSList *iter;
    GSList *installed_list = NULL;
    pid_t child;
    gboolean remove = FALSE;
    guint i = 0;

    /* If the list of packages is empty, let's spare ourselves the trouble of
       doing anything */
    if (!pkgs) {
        rc_packman_install_done (p, RC_PACKMAN_COMPLETE);
    }

    iter = pkgs;

    while (iter && !remove) {
        gchar *filename = (gchar *)(iter->data);

        /* Gotta give me a filename to install, otherwise purge the already
           unpacked packages and bail */
        if (!filename) {
            remove = TRUE;
            break;
        }

        /* Fork off dpkg to unpack the package */
        if (!(child = fork ())) {
            fclose (stdout);
            execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--unpack", filename,
                   NULL);
        } else {
            gint status;

            waitpid (child, &status, 0);

            /* Add the last package to the list of packages unpacked (even if
               we're not completely sure it was unpacked, since purging it will
               be harmless */
            installed_list = g_slist_append (installed_list, filename);

            rc_packman_package_installed (p, filename, ++i);

            /* Check the exit status of dpkg, and abort this install if things
               have started to go wrong */
            if (WIFEXITED (status) && WEXITSTATUS (status)) {
                remove = TRUE;
                break;
            }
        }

        iter = iter->next;
    }

    /* Check if we exited normally from the unpack loop.  If we did, configure
       the unpacked packages */
    if (!remove) {
        /* Once again, fork dpkg to do the dirty work */
        if (!(child = fork ())) {
            fclose (stdout);

            execl ("/usr/bin/dpkg", "/usr/bin/dpkg", "--configure",
                   "--pending", NULL);
        } else {
            gint status;

            waitpid (child, &status, 0);

            if (WIFEXITED (status) && !WEXITSTATUS (status)) {
                /* Looks like everything went ok, we're done */
                g_slist_foreach (installed_list, (GFunc) g_free, NULL);
                rc_packman_install_done (p, RC_PACKMAN_COMPLETE);
                return;
            }
        }
    }

    /* Whoops.  Looks like we were either unable to unpack all of the packages,
       or we were unable to complete the final configure.  Time to purge all of
       the packages we unpacked */
    for (iter = installed_list; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);
        /* A debian file has filename <name>_<epoch>:<version>-<release>..., so
           this should be a fairly safe way to get the actual package name.  Of
           course, if we ever don't name our packages the canonical way we're
           in trouble.  But then again, by the time we get here we're in
           trouble anyway, so I don't want to rely on any more fork'd dpkg
           methods */
        gchar **parts = g_strsplit (filename, "_", 1);
        gchar *pkg = g_strdup (strrchr (parts[0], '/') + 1);

        g_free (filename);
        filename = pkg;

        g_strfreev (parts);
    }

    if (!rc_debman_do_purge (installed_list)) {
        /* Looks like the purge went ok, so all we did was fail to install.
           Let the people know the truth and bow out gracefully. */
        rc_packman_install_done (p, RC_PACKMAN_FAIL);
    } else {
        /* Ok, if we got here, we're fucked.  Hard.  So grab the wife and kids
           and get the hell out of Dodge. */
        gchar *error = g_strdup ("The following packages were left in an inconsistant state:");

        for (iter = installed_list; iter; iter = iter->next) {
            gchar *tmp = g_strconcat (error, " ", (gchar *)(iter->data), NULL);
            g_free (error);
            error = tmp;
        }

        rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED, error);

        rc_packman_install_done (p, RC_PACKMAN_FAIL);
    }

    g_slist_foreach (installed_list, (GFunc) g_free, NULL);
} /* rc_debman_install */

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

    g_slist_free (names);

    if (!ret) {
        rc_packman_remove_done (p, RC_PACKMAN_COMPLETE);
    } else {
        rc_packman_remove_done (p, RC_PACKMAN_FAIL);
    }
} /* rc_debman_remove */

static void
rc_debman_parse_version (gchar *input, guint32 *epoch, gchar **version,
                         gchar **release)
{
    gchar **t1, **t2;

    *epoch = 0;
    *version = NULL;
    *release = NULL;

    t1 = g_strsplit (input, ":", 1);

    if (t1[1]) {
        *epoch = strtoul (t1[0], NULL, 10);
        t2 = g_strsplit (t1[1], "-", 1);
    } else {
        t2 = g_strsplit (t1[0], "-", 1);
    }

    *version = g_strdup (t2[0]);
    *release = g_strdup (t2[1]);

    g_strfreev (t2);
    g_strfreev (t1);
}

static gchar *
readline (FILE *fp) {
    gchar *buf = g_new0 (gchar, 1024);

    /* FIXME: do this right, instead of like this.  You know what I mean. */
    fgets (buf, 1024, fp);

    buf = g_strchomp (buf);

    if (buf && !buf[0]) {
        g_free (buf);
        buf = NULL;
    }

    return (buf);
}

/* FIXME: check this function most carefully for memory leaks when you are
   awake next */

static RCPackageDepSList *
rc_debman_fill_depends (gchar *input, RCPackageDepSList *list)
{
    gchar **deps;
    guint i;

    deps = g_strsplit (input, ", ", 0);

    for (i = 0; deps[i]; i++) {
        gchar **elems = g_strsplit (deps[i], " | ", 0);
        guint j;
        RCPackageDep *dep = NULL;

        for (j = 0; elems[j]; j++) {
            RCPackageDepItem *depi;
            gchar **parts;

            parts = g_strsplit (elems[j], " ", 1);

            if (!parts[1]) {
                depi = rc_package_dep_item_new (parts[0], 0, NULL, NULL,
                                                RC_RELATION_ANY);
            } else {
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


static void
rc_debman_query_helper (FILE *fp, RCPackage *pkg, gboolean do_depends)
{
    gchar *buf;

    RCPackageDepItem *item;
    RCPackageDep *dep = NULL;

    while ((buf = readline (fp))) {
        if (!strncmp (buf, "Status: install", strlen ("Status: install"))) {
            pkg->spec.installed = TRUE;
        } else if (!strncmp (buf, "Installed-Size: ",
                             strlen ("Installed-Size: "))) {
            pkg->spec.installed_size = strtoul (buf + strlen ("Installed-Size: "),
                                                NULL, 10) * 1024;
        } else if (!strncmp (buf, "Version: ", strlen ("Version: "))) {
            rc_debman_parse_version (buf + strlen ("Version: "),
                                     &pkg->spec.epoch, &pkg->spec.version,
                                     &pkg->spec.release);
        } else if (do_depends && !strncmp (buf, "Depends: ",
                                           strlen ("Depends: "))) {
            /* Do the dependencies */
            pkg->requires = rc_debman_fill_depends (buf + strlen ("Depends: "),
                                                    pkg->requires);
        } else if (do_depends && !strncmp (buf, "Recommends: ",
                                           strlen ("Recommends: "))) {
            pkg->requires = rc_debman_fill_depends (buf +
                                                    strlen ("Recommends: "),
                                                    pkg->requires);
//        } else if (do_depends && !strncmp (buf, "Suggests: ",
//                                           strlen ("Suggests: "))) {
//            pkg->requires = rc_debman_fill_depends (buf +
//                                                    strlen ("Suggests: "),
//                                                    pkg->requires);
        } else if (do_depends && !strncmp (buf, "Pre-Depends: ",
                                           strlen ("Pre-Depends: "))) {
            pkg->requires = rc_debman_fill_depends (buf +
                                                    strlen ("Pre-Depends: "),
                                                    pkg->requires);
        } else if (do_depends && !strncmp (buf, "Conflicts: ",
                                           strlen ("Conflicts: "))) {
            pkg->conflicts = rc_debman_fill_depends (buf +
                                                     strlen ("Conflicts: "),
                                                     pkg->conflicts);
        } else if (do_depends && !strncmp (buf, "Replaces: ",
                                           strlen ("Replaces: "))) {
            pkg->conflicts = rc_debman_fill_depends (buf +
                                                     strlen ("Replaces: "),
                                                     pkg->conflicts);
        } else if (do_depends && !strncmp (buf, "Provides: ",
                                           strlen ("Provides: "))) {
            pkg->provides = rc_debman_fill_depends (buf +
                                                    strlen ("Provides: "),
                                                    pkg->provides);
        }

        g_free (buf);
    }

    if (do_depends) {
        item = rc_package_dep_item_new (pkg->spec.name, pkg->spec.epoch,
                                        pkg->spec.version, pkg->spec.release,
                                        RC_RELATION_EQUAL);
        dep = g_slist_append (NULL, item);
        pkg->provides = g_slist_append (pkg->provides, dep);
    }
}

static RCPackage *
rc_debman_query (RCPackman *p, RCPackage *pkg)
{
    FILE *fp = fopen ("/var/lib/dpkg/status", "r");
    gchar *buf;
    gchar *target;

    pkg->spec.installed = FALSE;

    g_assert (pkg);
    g_assert (pkg->spec.name);

    target = g_strconcat ("Package: ", pkg->spec.name, NULL);

    while ((buf = readline (fp))) {
        if (!strcmp (buf, target)) {
            rc_debman_query_helper (fp, pkg, FALSE);
            break;
        }

        g_free (buf);
    }

    g_free (target);

    fclose (fp);
    
    return (pkg);
}

static RCPackageSList *
rc_debman_depends (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter;

    for (iter = pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);
    
        FILE *fp = fopen ("/var/lib/dpkg/status", "r");
        gchar *buf;
        gchar *target;

        pkg->spec.installed = FALSE;

        g_assert (pkg);
        g_assert (pkg->spec.name);

        target = g_strconcat ("Package: ", pkg->spec.name, NULL);

        while ((buf = readline (fp))) {
            if (!strcmp (buf, target)) {
                g_free (buf);
                rc_debman_query_helper (fp, pkg, TRUE);
                break;
            }

            g_free (buf);
        }

        g_free (target);

        fclose (fp);
    }
    
    return (pkgs);
}

static RCPackage *
rc_debman_file_helper (RCPackman *p, gchar *filename, gboolean do_depends)
{
    gchar *tmpdir;
    gchar *path;
    gchar *file;
    pid_t child;
    FILE *fp;
    RCPackage *pkg = rc_package_new ();
    gchar *buf;
    DIR *dp;
    struct dirent *entry;

    tmpdir = getenv ("TMPDIR");
    if (!tmpdir) {
        tmpdir = g_strdup ("/tmp");
    }

    path = g_strconcat (tmpdir, "/rc-debman-XXXXXX", NULL);
    g_free (tmpdir);

    path = mktemp (path);

    /* FIXME: error checking */
    mkdir (path, 0755);

    if (!(child = fork())) {
        fclose (stdout);
        execl ("/usr/bin/dpkg-deb", "/usr/bin/dpkg-deb", "--control",
               filename, path, NULL);
    } else {
        /* FIXME: do some error checking */

        waitpid (child, NULL, 0);
    }

    file = g_strconcat (path, "/control", NULL);

    fp = fopen (file, "r");

    buf = readline (fp);

    pkg->spec.name = g_strdup (buf + strlen ("Package: "));

    g_free (buf);

    rc_debman_query_helper (fp, pkg, do_depends);

    dp = opendir (path);

    while ((entry = readdir (dp))) {
        if (strcmp (entry->d_name, ".") && strcmp (entry->d_name, "..")) {
            gchar *filename = g_strconcat (path, "/", entry->d_name, NULL);

            unlink (filename);

            g_free (filename);
        }
    }

    closedir (dp);

    rmdir (path);

    g_free (file);
    g_free (path);

    return (pkg);
}

static RCPackage *
rc_debman_query_file (RCPackman *p, gchar *filename)
{
    return (rc_debman_file_helper (p, filename, FALSE));
}

static RCPackageSList *
rc_debman_depends_files (RCPackman *p, GSList *filenames)
{
    GSList *iter;
    RCPackageSList *list = NULL;

    for (iter = filenames; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);

        list = g_slist_append (list,
                               rc_debman_file_helper (p, filename, TRUE));
    }

    return (list);
}

static RCPackageSList *
rc_debman_query_all (RCPackman *p)
{
    FILE *fp;
    gchar *buf;
    RCPackageSList *pkgs = NULL;

    fp = fopen ("/var/lib/dpkg/status", "r");

    while ((buf = readline (fp))) {
        RCPackage *pkg = rc_package_new ();

        if (strncmp (buf, "Package: ", strlen ("Package: "))) {
            rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                                  "Malformed /var/lib/dpkg/status file");
            g_free (buf);
            rc_package_free (pkg);
            rc_package_slist_free (pkgs);
            return (NULL);
        }

        pkg->spec.name = g_strdup (buf + strlen ("Package: "));

        g_free (buf);

        rc_debman_query_helper (fp, pkg, FALSE);

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
    rcpc->rc_packman_real_depends = rc_debman_depends;
    rcpc->rc_packman_real_depends_files = rc_debman_depends_files;
} /* rc_debman_class_init */

static void
rc_debman_init (RCDebman *obj)
{
} /* rc_debman_init */

RCDebman *
rc_debman_new (void)
{
    RCDebman *new =
        RC_DEBMAN (gtk_type_new (rc_debman_get_type ()));

    return new;
} /* rc_debman_new */
