#include <gtk/gtk.h>

#include <string.h>
#include <stdio.h>
#ifdef HAVE_LIBRPM
#include <rpm/rpmlib.h>
#endif
#include <rc-distman.h>

static void
transaction_progress_cb (RCPackman *p, int amount, int total)
{
    printf ("Transaction progress %d of %d\n", amount, total);
}

static void
transaction_step_cb (RCPackman *p, int seqno, int total)
{
    printf ("Transaction step %d of %d\n", seqno, total);
}

static void
transaction_done_cb (RCPackman *p)
{
    printf ("Transaction done.\n");
}

static void
configure_progress_cb (RCPackman *p, int amount, int total)
{
    printf ("Configure progress %d of %d\n", amount, total);
}

static void
configure_step_cb (RCPackman *p, int seqno, int total)
{
    printf ("Configure step %d of %d\n", seqno, total);
}

static void
configure_done_cb (RCPackman *p)
{
    printf ("Configure done.\n");
}

int main (int argc, char **argv)
{
    RCPackman *p;
    RCPackageSList *remove_pkgs = NULL;
    GSList *install_pkgs = NULL;
    int i = 1;
    GSList *iter;

    gtk_type_init ();

    p = rc_distman_new ();

    gtk_signal_connect (GTK_OBJECT (p), "transaction_progress",
                        GTK_SIGNAL_FUNC (transaction_progress_cb), NULL);
    gtk_signal_connect (GTK_OBJECT (p), "transaction_step",
                        GTK_SIGNAL_FUNC (transaction_step_cb), NULL);
    gtk_signal_connect (GTK_OBJECT (p), "transaction_done",
                        GTK_SIGNAL_FUNC (transaction_done_cb), NULL);
    gtk_signal_connect (GTK_OBJECT (p), "configure_progress",
                        GTK_SIGNAL_FUNC (configure_progress_cb), NULL);
    gtk_signal_connect (GTK_OBJECT (p), "configure_step",
                        GTK_SIGNAL_FUNC (configure_step_cb), NULL);
    gtk_signal_connect (GTK_OBJECT (p), "configure_done",
                        GTK_SIGNAL_FUNC (configure_done_cb), NULL);

    while (i < argc) {
        if (!(strcmp (argv[i], "-i"))) {
            i++;

            while ((i < argc) && (argv[i][0] != '-')) {
                install_pkgs = g_slist_append (install_pkgs, argv[i]);
                i++;
            }

        } else if (!(strcmp (argv[i], "-r"))) {
            i++;

            while ((i < argc) && (argv[i][0] != '-')) {
                RCPackage *pkg = rc_package_new ();

                pkg->spec.name = g_strdup (argv[i]);
                pkg = rc_packman_query (p, pkg);

                remove_pkgs = g_slist_append (remove_pkgs, pkg);
                i++;
            }
        }
    }

    for (iter = install_pkgs; iter; iter = iter->next) {
        printf ("Install: %s\n", (gchar *)(iter->data));
    }

    for (iter = remove_pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        printf ("Remove:  %s-%d:%s-%s\n", pkg->spec.name, pkg->spec.epoch,
                pkg->spec.version, pkg->spec.release);
    }

    rc_packman_transact (p, install_pkgs, remove_pkgs);

    if (rc_packman_get_error (p)) {
        printf ("RC_PACKMAN_ERROR: %s\n", rc_packman_get_reason (p));
    }

    gtk_object_destroy (GTK_OBJECT (p));

    return (0);
}
