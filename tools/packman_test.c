#include <gtk/gtk.h>

#include <string.h>
#include <stdio.h>
#ifdef HAVE_LIBRPM
#include <rpm/rpmlib.h>
#endif
#include <rc-distman.h>

void pkg_installed_cb (RCPackman *, gchar *, gpointer);
void install_done_cb (RCPackman *);
void remove_done_cb (RCPackman *);

void
pkg_installed_cb (RCPackman *hp, gchar *file, gpointer data)
{
    printf ("Installed %s\n", file);
}

void
install_done_cb (RCPackman *hp)
{
    switch (rc_packman_get_error (hp)) {
    case RC_PACKMAN_ERROR_NONE:
        printf ("Installation complete.\n");
        break;
    default:
        printf ("Installation error: %s\n", rc_packman_get_reason (hp));
        break;
    }
}

void
remove_done_cb (RCPackman *hp)
{
    switch (rc_packman_get_error (hp)) {
    case RC_PACKMAN_ERROR_NONE:
	printf ("Remove complete.\n");
	break;
    default:
	printf ("Remove error: %s\n", rc_packman_get_reason (hp));
	break;
    }
}

int main (int argc, char **argv)
{
    RCPackman *p;
    guint error;

    gtk_type_init ();

    p = rc_distman_new ();

    if ((argc == 3) && !strcmp (argv[1], "-initdb")) {
#ifdef HAVE_LIBRPM
        rpmdbInit (argv[2], 0644);
#else
	printf ("RPM support was not compiled in.\n");
#endif
    } else if ((argc == 1) || !strcmp (argv[1], "-qa")) {
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

        rc_package_slist_free (query);
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
            printf ("Installed size is %u\n", pkg->spec.installed_size);
            printf ("Summary: %s\nDescription: %s\n", pkg->summary,
                    pkg->description);
            printf ("Section: %d\n", pkg->spec.section);
        } else {
            printf ("%s-%s-%s is not installed\n", pkg->spec.name,
                    pkg->spec.version, pkg->spec.release);
        }

        rc_package_free (pkg);
    } else if (!strcmp (argv[1], "-qf")) {
        RCPackage *pkg;

        if (argc != 3) {
            printf ("Usage: %s -qf <filename>\n", argv[0]);
            exit (-1);
        }

        pkg = rc_packman_query_file (p, argv[2]);

        printf ("Package is %s-%s-%s\n", pkg->spec.name, pkg->spec.version,
                pkg->spec.release);
    } else if (!strncmp (argv[1], "-r", 2)) {
        static guint do_remove (void) {
            guint i;
            RCPackageSList *rlist = NULL;

            for (i = 2; i < argc; i++) {
                RCPackage *pkg = rc_package_new ();

                pkg->spec.name = g_strdup (argv[i]);

                rlist = g_slist_append (rlist, pkg);
            }

            if (!rlist) {
                if (!strcmp (argv[1], "-ra")) {
                    printf ("Removing all packages...\n");
                    rlist = rc_packman_query (p, NULL);
                } else {
                    printf ("%s: -r must provide package name(s)\n", argv[0]);
                    exit (-1);
                }
            }

            rc_packman_remove (p, rlist);

            exit (0);

            /* Not reached */
            return (FALSE);
        }

        gtk_signal_connect (GTK_OBJECT (p), "remove_done",
                            GTK_SIGNAL_FUNC (remove_done_cb), NULL);

        gtk_timeout_add (0, (GtkFunction) do_remove, NULL);

        gtk_main ();
    } else if (!strcmp (argv[1], "-i")) {
        static guint do_install (void) {
            guint i;
            GSList *ilist = NULL;

            for (i = 2; i < argc; i++) {
                ilist = g_slist_append (ilist, argv[i]);
            }

            if (!ilist) {
                printf ("%s: -i must provide filename(s)\n", argv[0]);
                exit (-1);
            }

            rc_packman_install (p, ilist);

            exit (0);

            /* Not reached */
            return (FALSE);
        }

        gtk_signal_connect (GTK_OBJECT (p), "pkg_installed",
                            GTK_SIGNAL_FUNC (pkg_installed_cb), NULL);
        gtk_signal_connect (GTK_OBJECT (p), "install_done",
                            GTK_SIGNAL_FUNC (install_done_cb), NULL);

        gtk_timeout_add (0, (GtkFunction) do_install, NULL);

        gtk_main ();
    }

    gtk_object_destroy (p);

    return (0);
}
