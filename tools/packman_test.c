#include <gtk/gtk.h>

#include <string.h>
#include <stdio.h>

#include <rc-packman.h>
#include <rc-rpmman.h>

void pkg_installed_cb (RCPackman *, gchar *, gpointer);
void install_done_cb (RCPackman *, RCPackmanOperationStatus);
void remove_done_cb (RCPackman *, RCPackmanOperationStatus);

void
pkg_installed_cb (RCPackman *hp, gchar *file, gpointer data)
{
    printf ("Installed %s\n", file);
}

void
install_done_cb (RCPackman *hp, RCPackmanOperationStatus status)
{
    switch (status) {
    case RC_PACKMAN_COMPLETE:
	printf ("Installation complete.\n");
	break;
    case RC_PACKMAN_FAIL:
	printf ("Installation failed: %s\n", rc_packman_get_reason (hp));
	break;
    case RC_PACKMAN_ABORT:
	printf ("Installation aborted: %s\n", rc_packman_get_reason (hp));
	break;
    }
}

void
remove_done_cb (RCPackman *hp, RCPackmanOperationStatus status)
{
    switch (status) {
    case RC_PACKMAN_COMPLETE:
	printf ("Remove complete.\n");
	break;
    case RC_PACKMAN_FAIL:
	printf ("Remove failed: %s\n", rc_packman_get_reason (hp));
	break;
    case RC_PACKMAN_ABORT:
	printf ("Remove aborted: %s\n", rc_packman_get_reason (hp));
	break;
    }
}

int main (int argc, char **argv)
{
    RCPackman *p;
    guint error;

    gtk_type_init ();

    p = RC_PACKMAN (rc_rpmman_new ());

    if ((argc == 1) || !strcmp (argv[1], "-qa")) {
        RCPackageSList *query = NULL, *iter;

        query = rc_packman_query_all (p);

        if ((error = rc_packman_get_error (p))) {
            printf ("Error %d: %s\n", error, rc_packman_get_reason (p));
            exit (error);
        }

        printf ("%-25s%-10s%-20s%-20s\n", "Package", "Epoch", "Version",
                "Release");
        printf ("%-25s%-10s%-20s%-20s\n", "-------", "-----", "-------",
                "-------");

        for (iter = query; iter; iter = iter->next) {
            RCPackage *data = (RCPackage *)(iter->data);

            printf ("%-25s%-10d%-20s%-20s\n", data->spec.name,
                    data->spec.epoch, data->spec.version, data->spec.release);
        }
    } else if (!strcmp (argv[1], "-q")) {
        RCPackage *pkg = rc_package_new ();

        if (argc < 3) {
            printf ("Usage: %s -q <name> [<version> <release>]\n", argv[0]);
            exit (-1);
        }

        pkg->spec.name = g_strdup (argv[2]);

        if (argc >= 4) {
            pkg->spec.version = g_strdup (argv[3]);
        }

        if (argc >= 5) {
            pkg->spec.release = g_strdup (argv[4]);
        }

        pkg = rc_packman_query (p, pkg);

        if (pkg->spec.installed) {
            printf ("Installed version is %s-%s-%s\n", pkg->spec.name,
                    pkg->spec.version, pkg->spec.release);
        } else {
            printf ("%s-%s-%s is not installed\n", pkg->spec.name,
                    pkg->spec.version, pkg->spec.release);
        }
    } else if (!strcmp (argv[1], "-qf")) {
        RCPackage *pkg;

        if (argc != 3) {
            printf ("Usage: %s -qf <filename>\n", argv[0]);
            exit (-1);
        }

        pkg = rc_packman_query_file (p, argv[2]);

        printf ("Package is %s-%s-%s\n", pkg->spec.name, pkg->spec.version,
                pkg->spec.release);
    }

#if 0
    if (!strncmp (argv[1], "-r", 2)) {
        static guint do_remove (void) {
            guint i;
            HP_PACKAGE_LIST *rlist = NULL;

            for (i = 2; i < argc; i++) {
                HP_ADD_PACKAGE (rlist, argv[i], NULL, NULL, 0);
            }

            if (!rlist) {
                if (!strcmp (argv[1], "-ra")) {
                    printf ("Removing all packages...\n");
                    rlist = rc_packman_query (hp, NULL);
                } else {
                    printf ("%s: -r must provide package name(s)\n", argv[0]);
                    exit (-1);
                }
            }

            rc_packman_remove (hp, rlist);

            exit (0);

            /* Not reached */
            return (FALSE);
        }

        gtk_signal_connect (GTK_OBJECT (hp), "remove_done",
                            GTK_SIGNAL_FUNC (remove_done_cb), NULL);

        gtk_timeout_add (0, (GtkFunction) do_remove, NULL);

        gtk_main ();
    } else if (!strcmp (argv[1], "-i")) {
        static guint do_install (void) {
            guint i;
            HP_FILE_LIST *ilist = NULL;

            for (i = 2; i < argc; i++) {
                HP_ADD_FILE (ilist, argv[i]);
            }

            if (!ilist) {
                printf ("%s: -i must provide filename(s)\n", argv[0]);
                exit (-1);
            }

            rc_packman_install (hp, ilist);

            exit (0);

            /* Not reached */
            return (FALSE);
        }

        gtk_signal_connect (GTK_OBJECT (hp), "pkg_installed",
                            GTK_SIGNAL_FUNC (pkg_installed_cb), NULL);
        gtk_signal_connect (GTK_OBJECT (hp), "install_done",
                            GTK_SIGNAL_FUNC (install_done_cb), NULL);

        gtk_timeout_add (0, (GtkFunction) do_install, NULL);

        gtk_main ();
    } else if (!strncmp (argv[1], "-d", 2)) {
        HP_PACKAGE_LIST *dlist = NULL;
        RCPackmanPackage *d;
        HP_DEP_LIST *iter;

        if ((argc < 3) || (argc > 5)) {
            printf ("%s: -d name [version, release]", argv[0]);
            exit (-1);
        }

        if (argc == 3) {
            HP_ADD_PACKAGE (dlist, argv[2], NULL, NULL, 0);
        } else if (argc == 4) {
            HP_ADD_PACKAGE (dlist, argv[2], argv[3], NULL, 0);
        } else {
            HP_ADD_PACKAGE (dlist, argv[2], argv[3], argv[4], 0);
        }

        dlist = rc_packman_depends (hp, dlist);

        if (!dlist) {
            printf ("Package %s doesn't exist\n", argv[2]);
            return (0);
        }

        d = (RCPackmanPackage *)(dlist->data);

        printf ("Package: %s-%s-%s\n", d->spec.name, d->spec.version,
                d->spec.release);

        printf ("\nRequires:\n");

        for (iter = d->requires; iter; iter = iter->next) {
            RCPackmanDep *dep = (RCPackmanDep *)(iter->data);

            printf ("    %-40s%-10s%-20s\n", dep->spec.name, dep->spec.version,
                    dep->spec.release);
        }
 
        printf ("\nProvides:\n");

        for (iter = d->provides; iter; iter = iter->next) {
            RCPackmanDep *dep = (RCPackmanDep *)(iter->data);

            printf ("    %-40s%-10s%-20s\n", dep->spec.name, dep->spec.version,
                    dep->spec.release);
        }

        printf ("\nConflicts:\n");

        for (iter = d->conflicts; iter; iter = iter->next) {
            RCPackmanDep *dep = (RCPackmanDep *)(iter->data);

            printf ("    %-40s%-10s%-20s\n", dep->spec.name, dep->spec.version,
                    dep->spec.release);
        }

        HP_PACKAGE_LIST_FREE (dlist);
    } else if (!strncmp (argv[1], "-t", 2)) {
        HP_PACKAGE_LIST *list = NULL;

        HP_ADD_PACKAGE (list, "joe-testpkg", "3.0", "1", 0);

        list = rc_packman_depends (hp, list);

        list = rc_packman_query (hp, list);

        list = rc_packman_depends (hp, list);

        debug_rc_packman_package_dump (list->data);

        HP_PACKAGE_LIST_FREE (list);
    } else if (!strncmp (argv[1], "-fd", 3)) {
        HP_FILE_LIST *flist = NULL;
        HP_PACKAGE_LIST *dlist = NULL;

        HP_ADD_FILE (flist, argv[2]);

        dlist = rc_packman_depends_files (hp, flist);

        debug_rc_packman_package_dump (dlist->data);

        HP_FILE_LIST_FREE (flist);
        HP_PACKAGE_LIST_FREE (dlist);
    }

#endif

    return (0);
}
