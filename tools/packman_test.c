#include <gtk/gtk.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef HAVE_LIBRPM
#include <rpm/rpmlib.h>
#endif

#include <rc-distman.h>

#include <readline.h>
#include <history.h>

static void packman_test_clear (RCPackman *p, gchar *buf);
static void packman_test_help (RCPackman *p, gchar *buf);
static void packman_test_install (RCPackman *p, gchar *buf);
static void packman_test_remove (RCPackman *p, gchar *buf);
static void packman_test_list (RCPackman *p, gchar *buf);
static void packman_test_query (RCPackman *p, gchar *buf);
static void packman_test_query_all (RCPackman *p, gchar *buf);
static void packman_test_query_file (RCPackman *p, gchar *buf);
static void packman_test_quit (RCPackman *p, gchar *buf);
static void packman_test_run (RCPackman *p, gchar *buf);

typedef void (*command_func)(RCPackman *p, gchar *buf);

struct _Command {
    gchar *name;
    command_func func;
    gchar *help;
    gchar *syntax;
};

typedef struct _Command Command;

Command commands[] = {
    { "clear", (command_func) packman_test_clear,
      "Clear the pending transaction.", "clear" },
    { "help", (command_func) packman_test_help,
      "Help on packman_test commands.", "help [<command>]" },
    { "install", (command_func) packman_test_install,
      "Add a file to be installed to the transaction.", "install <filename>" },
    { "list", (command_func) packman_test_list,
      "List the contents of the transaction.", "list" },
    { "query", (command_func) packman_test_query,
      "Query a package on the system.",
      "query [<name> [epoch=<epoch>] [version=<version>] [release=<release>]]"
    },
    { "query_all", (command_func) packman_test_query_all,
      "Query all of the packages on the system.", "query_all" },
    { "query_file", (command_func) packman_test_query_file,
      "Query a local package file.", "query_file <filename>" },
    { "quit", (command_func) packman_test_quit,
      "Quit packman_test.", "quit" },
    { "remove", (command_func) packman_test_remove,
      "Add a package to be removed to the transaction", "remove <package>" },
    { "run", (command_func) packman_test_run,
      "Run the pending transaction", "run" },
    { (gchar *)NULL, (command_func)NULL, (gchar *)NULL, (gchar *)NULL }
};

struct _Transaction {
    GSList *install_pkgs;
    RCPackageSList *remove_pkgs;
};

typedef struct _Transaction Transaction;

Transaction transaction;

guint num_commands = sizeof (commands) / sizeof (Command) - 1;

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

static gchar **
pop_token (gchar *buf)
{
    gchar **tokens;

    buf = g_strstrip (buf);

    tokens = g_strsplit (buf, " ", 1);

#ifdef DEBUG
    printf ("DEBUG: pop_token has \"%s\" and \"%s\"\n", tokens[0], tokens[1]);
#endif

    return (tokens);
}

static void
print_pkg_list_header ()
{
    printf ("%-30s%-8s%-15s%-15s\n", "Package", "Epoch", "Version", "Release");
    printf ("%-30s%-8s%-15s%-15s\n", "-------", "-----", "-------", "-------");
}

static void
print_pkg (RCPackage *pkg)
{
    printf ("%-30.30s%-8d%-15.15s%-15.15s\n", pkg->spec.name, pkg->spec.epoch,
            pkg->spec.version, pkg->spec.release);
}

static void
packman_test_query_all (RCPackman *p, gchar *buf)
{
    RCPackageSList *pkg_list;
    RCPackageSList *iter;

    if (buf) {
        printf ("ERROR: extraneous characters after \"query_all\"\n");
        return;
    }
    
    pkg_list = rc_packman_query_all (p);

    print_pkg_list_header ();

    for (iter = pkg_list; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        print_pkg (pkg);
    }

    rc_package_slist_free (pkg_list);
}

static void
pretty_print_pkg (RCPackage *pkg)
{
    printf ("%-15s%-25.25s%-15s%-25d\n", "Name:", pkg->spec.name,
            "Epoch:", pkg->spec.epoch);
    printf ("%-15s%-25.25s%-15s%-25.25s\n", "Version:", pkg->spec.version,
            "Release:", pkg->spec.release);
    printf ("%-15s%-25.25s%-15s%-25d\n", "Section:",
            sectable[pkg->spec.section].name, "Size:",
            pkg->spec.installed_size);
    printf ("\n");
    printf ("Summary:\n%s\n\n", pkg->summary);
    printf ("Description:\n%s\n", pkg->description);
}

static RCPackage *
packman_test_make_pkg (RCPackman *p, gchar *buf)
{
    RCPackage *pkg;
    gchar **tokens;
    gchar *attribs;

    tokens = pop_token (buf);

    pkg = rc_package_new ();
    pkg->spec.name = g_strdup (tokens[0]);

    attribs = g_strdup (tokens[1]);

    g_strfreev (tokens);

    while (attribs) {
        gchar **parts;

        tokens = pop_token (attribs);
        g_free (attribs);
        attribs = g_strdup (tokens[1]);
        parts = g_strsplit (tokens[0], "=", 1);
        g_strfreev (tokens);

        if (!parts[1]) {
            printf ("ERROR: must provide attributes as <attribute>=<value>\n");
            g_strfreev (parts);
            rc_package_free (pkg);
            return (NULL);
        }

        if (!(strcmp (parts[0], "epoch"))) {
            pkg->spec.epoch = g_strtod (parts[1], NULL);
            g_strfreev (parts);
            continue;
        }

        if (!(strcmp (parts[0], "version"))) {
            pkg->spec.version = g_strdup (parts[1]);
            g_strfreev (parts);
            continue;
        }

        if (!(strcmp (parts[0], "release"))) {
            pkg->spec.release = g_strdup (parts[1]);
            g_strfreev (parts);
            continue;
        }

        printf ("ERROR: unknown attribute \"%s\"\n", parts[0]);
        g_strfreev (parts);
        rc_package_free (pkg);
        return (NULL);
    }

    pkg = rc_packman_query (p, pkg);

    return (pkg);
}

static void
packman_test_query (RCPackman *p, gchar *buf)
{
    RCPackage *pkg;

    if (!buf) {
        printf ("ERROR: \"query\" requires parameters (see \"help query\")\n");
        return;
    }

    pkg = packman_test_make_pkg (p, buf);

    if (!pkg) {
        return;
    }

    if (pkg->spec.installed) {
        pretty_print_pkg (pkg);
    } else {
        printf ("Package \"%s\" not found\n", pkg->spec.name);
    }

    rc_package_free (pkg);
}

static void
packman_test_query_file (RCPackman *p, gchar *line)
{
    struct stat buf;
    gchar **tokens;
    RCPackage *pkg;

    tokens = pop_token (line);

    if (tokens[1]) {
        printf ("ERROR: extraneous characters after \"%s\"\n", tokens[0]);
        g_strfreev (tokens);
        return;
    }

    if (stat (tokens[0], &buf)) {
        printf ("ERROR: could not stat local file \"%s\"\n", tokens[0]);
        g_strfreev (tokens);
        return;
    }

    pkg = rc_packman_query_file (p, tokens[0]);

    pretty_print_pkg (pkg);

    rc_package_free (pkg);
    g_strfreev (tokens);
}

static void
packman_test_help (RCPackman *p, gchar *line)
{
    gchar **tokens;
    guint i;
    gboolean handled = FALSE;

    if (!line) {
        guint count = 0;

        printf ("Available commands:\n");

        for (i = 0; i < num_commands; i++) {
            printf ("%-14.14s  ", commands[i].name);
            if (++count == 5) {
                printf ("\n");
                count = 0;
            }
        }

        if (count) {
            printf ("\n");
        }
        
        return;
    }

    tokens = pop_token (line);

    if (tokens[1]) {
        printf ("ERROR: extraneous characters after \"%s\"\n", tokens[0]);
        g_strfreev (tokens);
        return;
    }

    for (i = 0; i < num_commands; i++) {
        if (!strcmp (commands[i].name, tokens[0])) {
            printf ("Description:\n  %s\n\n", commands[i].help);
            printf ("Syntax:\n  %s\n", commands[i].syntax);
            handled = TRUE;
            break;
        }
    }

    if (!handled) {
        printf ("ERROR: unknown command \"%s\"\n", tokens[0]);
    }

    g_strfreev (tokens);
}

static void
packman_test_quit (RCPackman *p, gchar *line)
{
    if (line) {
        printf ("ERROR: extraneous characters after \"quit\"\n");
        return;
    }

    exit (0);
}

static void
packman_test_install (RCPackman *p, gchar *line)
{
    struct stat buf;
    gchar **tokens;

    tokens = pop_token (line);

    if (tokens[1]) {
        printf ("ERROR: extraneous characters after \"%s\"\n", tokens[0]);
        g_strfreev (tokens);
        return;
    }

    if (stat (tokens[0], &buf)) {
        printf ("ERROR: could not stat local file \"%s\"\n", tokens[0]);
        g_strfreev (tokens);
        return;
    }

    transaction.install_pkgs = g_slist_append (transaction.install_pkgs,
                                               g_strdup (tokens[0]));

    g_strfreev (tokens);
}

static void
packman_test_remove (RCPackman *p, gchar *line)
{
    RCPackage *pkg;

    pkg = packman_test_make_pkg (p, line);

    if (pkg->spec.installed) {
        transaction.remove_pkgs = g_slist_append (transaction.remove_pkgs,
                                                  pkg);
    } else {
        printf ("ERROR: package \"%s\" is not installed\n", pkg->spec.name);
        rc_package_free (pkg);
    }
}

static void
packman_test_clear (RCPackman *p, gchar *line)
{
    if (line) {
        printf ("ERROR: extraneous characters after \"clear\"\n");
        return;
    }

    g_slist_foreach (transaction.install_pkgs, (GFunc) g_free, NULL);
    g_slist_free (transaction.install_pkgs);
    transaction.install_pkgs = NULL;

    rc_package_slist_free (transaction.remove_pkgs);
    transaction.remove_pkgs = NULL;

    printf ("Transaction cleared\n");
}

static void
packman_test_list (RCPackman *p, gchar *line)
{
    GSList *iter;

    if (line) {
        printf ("ERROR: extraneous characters after \"list\"\n");
        return;
    }

    for (iter = transaction.install_pkgs; iter; iter = iter->next) {
        gchar *name = (gchar *)(iter->data);

        printf ("Install: %s\n", name);
    }

    for (iter = transaction.remove_pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        printf (" Remove: %s %d %s %s\n", pkg->spec.name, pkg->spec.epoch,
                pkg->spec.version, pkg->spec.release);
    }
}

static void
packman_test_run (RCPackman *p, gchar *line)
{
    if (line) {
        printf ("ERROR: extraneous characters after \"run\"\n");
        return;
    }

    rc_packman_transact (p, transaction.install_pkgs, transaction.remove_pkgs);

    if (rc_packman_get_error (p)) {
        printf ("ERROR: %s\n", rc_packman_get_reason (p));
    }

    g_slist_foreach (transaction.install_pkgs, (GFunc) g_free, NULL);
    g_slist_free (transaction.install_pkgs);
    transaction.install_pkgs = NULL;

    rc_package_slist_free (transaction.remove_pkgs);
    transaction.remove_pkgs = NULL;
}

int main (int argc, char **argv)
{
    RCPackman *p;
    char *buf;
    gboolean done = FALSE;

    gtk_type_init ();

    transaction.install_pkgs = NULL;
    transaction.remove_pkgs = NULL;

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

    while (!done) {
        gchar **tokens;
        guint i;
        gboolean handled = FALSE;

        buf = readline ("> ");

        if (!buf) {
            printf ("\n");
            done = TRUE;
            continue;
        }

        add_history (buf);

        tokens = pop_token (buf);

        for (i = 0; i < num_commands; i++) {
            if (!strcmp (commands[i].name, tokens[0])) {
                commands[i].func (p, tokens[1]);
                handled = TRUE;
                break;
            }
        }

        if (!handled) {
            printf ("ERROR: unknown command \"%s\"\n", tokens[0]);
        }

        g_strfreev (tokens);
        g_free (buf);
    }

    gtk_object_destroy (GTK_OBJECT (p));

    return (0);
}
