#include <gtk/gtk.h>

#include <string.h>

#include "helix-rpmman.h"

void pkg_installed_cb (HelixPackman *, gchar *, gpointer);
void install_done_cb (HelixPackman *, HelixPackmanOperationStatus);
void remove_done_cb (HelixPackman *, HelixPackmanOperationStatus);

void
pkg_installed_cb (HelixPackman *hp, gchar *file, gpointer data)
{
    printf ("Installed %s\n", file);
}

void
install_done_cb (HelixPackman *hp, HelixPackmanOperationStatus status)
{
    switch (status) {
    case HELIX_PACKMAN_COMPLETE:
	printf ("Installation complete.\n");
	break;
    case HELIX_PACKMAN_FAIL:
	printf ("Installation failed: %s\n", helix_packman_get_reason (hp));
	break;
    case HELIX_PACKMAN_ABORT:
	printf ("Installation aborted: %s\n", helix_packman_get_reason (hp));
	break;
    }
}

void
remove_done_cb (HelixPackman *hp, HelixPackmanOperationStatus status)
{
    switch (status) {
    case HELIX_PACKMAN_COMPLETE:
	printf ("Remove complete.\n");
	break;
    case HELIX_PACKMAN_FAIL:
	printf ("Remove failed: %s\n", helix_packman_get_reason (hp));
	break;
    case HELIX_PACKMAN_ABORT:
	printf ("Remove aborted: %s\n", helix_packman_get_reason (hp));
	break;
    }
}

int main (int argc, char **argv)
{
    HelixPackman *hp;

    /*
    fclose (stderr);

    stderr = fopen ("/dev/null", "a+");
    */

    gtk_type_init ();

    hp = HELIX_PACKMAN (helix_rpmman_new ());

    if ((argc == 1) || !strncmp (argv[1], "-q", 2)) {
        HP_PACKAGE_LIST *query = NULL, *iter;

        if ((argc > 1) && strcmp (argv[1], "-qa")) {
            if (argc == 5) {
                HP_ADD_PACKAGE (query, argv[2], argv[3], argv[4]);
            } else if (argc == 4) {
                HP_ADD_PACKAGE (query, argv[2], argv[3], NULL);
            } else {
                HP_ADD_PACKAGE (query, argv[2], NULL, NULL);
            }

            if (!query) {
                printf ("%s: -q must provide package name(s)\n", argv[0]);
                exit (-1);
            }
        }

        printf ("%-20s%-20s%-20s\n", "Package", "Version", "Release");
        printf ("%-20s%-20s%-20s\n", "-------", "-------", "-------");

        query = helix_packman_query (hp, query);

        for (iter = query; iter; iter = iter->next) {
            HelixPackmanPackage *d = (HelixPackmanPackage *)(iter->data);

            printf ("%-20s%-20s%-20s\n", d->spec.name, d->spec.version,
                    d->spec.release);
        }

        HP_PACKAGE_LIST_FREE (query);
    } else if (!strncmp (argv[1], "-r", 2)) {
        static guint do_remove (void) {
            guint i;
            HP_PACKAGE_LIST *rlist = NULL;

            for (i = 2; i < argc; i++) {
                HP_ADD_PACKAGE (rlist, argv[i], NULL, NULL);
            }

            if (!rlist) {
                if (!strcmp (argv[1], "-ra")) {
                    printf ("Removing all packages...\n");
                    rlist = helix_packman_query (hp, NULL);
                } else {
                    printf ("%s: -r must provide package name(s)\n", argv[0]);
                    exit (-1);
                }
            }

            helix_packman_remove (hp, rlist);

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

            helix_packman_install (hp, ilist);

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
        HelixPackmanPackage *d;
        HP_DEP_LIST *iter;

        if ((argc < 3) || (argc > 5)) {
            printf ("%s: -d name [version, release]", argv[0]);
            exit (-1);
        }

        if (argc == 3) {
            HP_ADD_PACKAGE (dlist, argv[2], NULL, NULL);
        } else if (argc == 4) {
            HP_ADD_PACKAGE (dlist, argv[2], argv[3], NULL);
        } else {
            HP_ADD_PACKAGE (dlist, argv[2], argv[3], argv[4]);
        }

        dlist = helix_packman_depends (hp, dlist);

        if (!dlist) {
            printf ("Package %s doesn't exist\n", argv[2]);
            return (0);
        }

        d = (HelixPackmanPackage *)(dlist->data);

        printf ("Package: %s-%s-%s\n", d->spec.name, d->spec.version,
                d->spec.release);

        printf ("\nRequires:\n");

        for (iter = d->requires; iter; iter = iter->next) {
            HelixPackmanDep *dep = (HelixPackmanDep *)(iter->data);

            printf ("    %-40s%-10s%-20s\n", dep->spec.name, dep->spec.version,
                    dep->spec.release);
        }
 
        printf ("\nProvides:\n");

        for (iter = d->provides; iter; iter = iter->next) {
            HelixPackmanDep *dep = (HelixPackmanDep *)(iter->data);

            printf ("    %-40s%-10s%-20s\n", dep->spec.name, dep->spec.version,
                    dep->spec.release);
        }

        printf ("\nConflicts:\n");

        for (iter = d->conflicts; iter; iter = iter->next) {
            HelixPackmanDep *dep = (HelixPackmanDep *)(iter->data);

            printf ("    %-40s%-10s%-20s\n", dep->spec.name, dep->spec.version,
                    dep->spec.release);
        }

        HP_PACKAGE_LIST_FREE (dlist);
    }

    return (0);
}
