#include <glib.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#ifdef HAVE_LIBRPM
#include <rpm/rpmlib.h>
#endif

#include <rc-distman.h>

/* DEBUG ONLY */
#include <rc-util.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <readline/readline.h>
#include <readline/history.h>

static void packman_test_clear (RCPackman *p, gchar *buf);
static void packman_test_find_file (RCPackman *p, gchar *buf);
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
    { "find", (command_func) packman_test_find_file,
      "Find the package which owns a file.", "find <filename>" },
    { "help", (command_func) packman_test_help,
      "Help on packman_test commands.", "help [<command>]" },
    { "install", (command_func) packman_test_install,
      "Add a file to be installed to the transaction.", "install <filename>" },
    { "list", (command_func) packman_test_list,
      "List the contents of the transaction.", "list" },
    { "query", (command_func) packman_test_query,
      "Query a package on the system.", "query <name>" },
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
transact_start_cb (RCPackman *packman, gint total)
{
    printf ("About to transact in %d steps\n", total);
}

static void
transact_step_cb (RCPackman *packman, gint seqno, RCPackmanStep step,
                  gchar *name)
{
    const char *oper;

    switch (step) {
    case RC_PACKMAN_STEP_CONFIGURE:
        oper = "Configuring";
        break;
    case RC_PACKMAN_STEP_INSTALL:
        oper = "Installing";
        break;
    case RC_PACKMAN_STEP_REMOVE:
        oper = "Removing";
        break;
    default:
        oper = "Frobbing";
        break;
    }

    printf ("%s %s (step %d)\n", oper, name ? name : "something", seqno);
}

static void
transact_progress_cb (RCPackman *packman, gulong amount, gulong total)
{
    printf ("  (%ld of %ld complete)\n", amount, total);
}

static void
transact_done_cb (RCPackman *packman)
{
    printf ("Transaction done.\n");
}

typedef enum {
    PACKMAN_TEST_DEBUG,
    PACKMAN_TEST_WARNING,
    PACKMAN_TEST_ERROR,
} PackmanTestPrintLevel;

static void
packman_test_print (PackmanTestPrintLevel level, const char *format, ...)
{
    va_list args;

    va_start (args, format);

    switch (level) {
    case PACKMAN_TEST_DEBUG:
        fprintf (stderr, "DEBUG: ");
        break;
    case PACKMAN_TEST_WARNING:
        fprintf (stderr, "WARNING: ");
        break;
    case PACKMAN_TEST_ERROR:
        fprintf (stderr, "ERROR: ");
        break;
    }
    vfprintf (stderr, format, args);
    fprintf (stderr, "\n");

    va_end (args);
}

#define CHECK_PARAMS(line, func) \
if (!line) { \
    packman_test_print ( \
        PACKMAN_TEST_ERROR, \
        "\"%s\" requires parameters (see \"help %s\")", func, func); \
    return; \
}

#define CHECK_EXTRA(line, func) \
if (line) { \
    packman_test_print ( \
        PACKMAN_TEST_WARNING, \
        "ignoring extraneous parameters to \"%s\"", func); \
}

static char *
pop_token (char *line, char **token)
{
    char *start = line;
    char *ptr;

    if (!line) {
#ifdef DEBUG
        packman_test_print (PACKMAN_TEST_DEBUG,
                            "attempt to pop from NULL input");
#endif
        *token = NULL;
        return NULL;
    }

    while (start && *start == ' ') {
        start++;
    }

    if (!start) {
        *token = NULL;
        return NULL;
    }

    ptr = strchr (start, ' ');
    if (ptr) {
        *ptr = '\0';
        ptr++;
    }

    *token = start;

    while (ptr && *ptr == ' ') {
        ptr++;
    }

    if (ptr && !*ptr) {
        ptr = NULL;
    }

    return (ptr);
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
#if 0
    printf ("%-30.30s%-8d%-15.15s%-15.15s\n", pkg->spec.name, pkg->spec.epoch,
            pkg->spec.version, pkg->spec.release);
#endif
    printf ("%s\n", rc_package_spec_to_str_static (RC_PACKAGE_SPEC (pkg)));
}

static void
packman_test_query_all (RCPackman *p, char *line)
{
    RCPackageSList *pkg_list;
    RCPackageSList *iter;

    CHECK_EXTRA (line, "query_all");

    pkg_list = rc_packman_query_all (p);

    print_pkg_list_header ();

    for (iter = pkg_list; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        print_pkg (pkg);
    }

    rc_package_slist_unref (pkg_list);
}

static void
pretty_print_pkg (RCPackage *pkg)
{
    RCPackageDepSList *iter;

    printf ("%-15s%-25.25s%-15s%-25d\n", "Name:", pkg->spec.name,
            "Epoch:", pkg->spec.epoch);
    printf ("%-15s%-25.25s%-15s%-25.25s\n", "Version:", pkg->spec.version,
            "Release:", pkg->spec.release);
    printf ("%-15s%-25.25s%-15s%-25d\n", "Section:",
            rc_package_section_to_string (pkg->section), "Size:",
            pkg->installed_size);
    printf ("\n");
    printf ("Summary:\n%s\n\n", pkg->summary);
    printf ("Description:\n%s\n\n", pkg->description);

    for (iter = pkg->requires; iter; iter = iter->next) {
        RCPackageDep *dep = (RCPackageDep *)(iter->data);
        printf ("Requires: %s %s %d %s %s\n", dep->spec.name,
                rc_package_relation_to_string (dep->relation, FALSE),
                dep->spec.epoch, dep->spec.version, dep->spec.release);
    }
    for (iter = pkg->provides; iter; iter = iter->next) {
        RCPackageDep *dep = (RCPackageDep *)(iter->data);
        printf ("Provides: %s %s %d %s %s\n", dep->spec.name,
                rc_package_relation_to_string (dep->relation, FALSE),
                dep->spec.epoch, dep->spec.version, dep->spec.release);
    }
    for (iter = pkg->conflicts; iter; iter = iter->next) {
        RCPackageDep *dep = (RCPackageDep *)(iter->data);
        printf ("Conflicts: %s %s %d %s %s\n", dep->spec.name,
                rc_package_relation_to_string (dep->relation, FALSE),
                dep->spec.epoch, dep->spec.version, dep->spec.release);
    }
}

static void
packman_test_query (RCPackman *p, char *line)
{
    RCPackageSList *packages;
    GSList *iter;
    char *name;

    CHECK_PARAMS (line, "query");

    line = pop_token (line, &name);

    packages = rc_packman_query (p, name);

    if (!packages) {
        printf ("No package matching name \"%s\" found\n", name);
        return;
    }

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        pretty_print_pkg (package);
    }

    rc_package_slist_unref (packages);
}

static void
packman_test_find_file (RCPackman *p, char *line)
{
    RCPackage *package;
    char *file;

    CHECK_PARAMS (line, "find");

    line = pop_token (line, &file);

    CHECK_EXTRA (line, "find");

    package = rc_packman_find_file (p, file);

    if (!package) {
        return;
    }

    pretty_print_pkg (package);

    rc_package_unref (package);
}

static void
packman_test_query_file (RCPackman *p, char *line)
{
    struct stat buf;
    char *file;
    RCPackage *pkg;

    CHECK_PARAMS (line, "query_file");

    line = pop_token (line, &file);

    CHECK_EXTRA (line, "query_file");

    if (stat (file, &buf)) {
        packman_test_print (
            PACKMAN_TEST_ERROR,
            "could not stat local file \"%s\"", file);
        return;
    }

    pkg = rc_packman_query_file (p, file);

    if (!pkg) {
        return;
    }

    pretty_print_pkg (pkg);

    rc_package_unref (pkg);
}

static void
packman_test_help (RCPackman *p, char *line)
{
    guint i;
    gboolean handled = FALSE;
    char *command;

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

    line = pop_token (line, &command);

    CHECK_EXTRA (line, "help");

    for (i = 0; i < num_commands; i++) {
        if (!strcmp (commands[i].name, command)) {
            printf ("Description:\n  %s\n\n", commands[i].help);
            printf ("Syntax:\n  %s\n", commands[i].syntax);
            handled = TRUE;
            break;
        }
    }

    if (!handled) {
        packman_test_print (
            PACKMAN_TEST_ERROR,
            "unknown command \"%s\"", command);
    }
}

static void
packman_test_quit (RCPackman *p, char *line)
{
    CHECK_EXTRA (line, "quit");

    exit (0);
}

static void
packman_test_install (RCPackman *p, char *line)
{
    struct stat buf;
    char *file;
    RCPackage *package;

    CHECK_PARAMS (line, "install");

    line = pop_token (line, &file);

    CHECK_EXTRA (line, "install");

    if (stat (file, &buf)) {
        packman_test_print (
            PACKMAN_TEST_ERROR,
            "could not stat local file \"%s\"", file);
        return;
    }

//    package = rc_package_new ();
    package = rc_packman_query_file (p, file);

    package->package_filename = g_strdup (file);

    transaction.install_pkgs = g_slist_append (transaction.install_pkgs,
                                               package);
}

static void
packman_test_remove (RCPackman *p, char *line)
{
    RCPackageSList *packages;
    GSList *iter;
    char *name;

    CHECK_PARAMS (line, "remove");

    line = pop_token (line, &name);

    packages = rc_packman_query (p, name);

    if (!packages) {
        packman_test_print (
            PACKMAN_TEST_ERROR,
            "package \"%s\" is not installed", name);
        return;
    }

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        transaction.remove_pkgs = g_slist_prepend (transaction.remove_pkgs,
                                                   package);
    }
}

static void
packman_test_clear (RCPackman *p, char *line)
{
    CHECK_EXTRA (line, "clear");

    rc_package_slist_unref (transaction.install_pkgs);
    transaction.install_pkgs = NULL;

    rc_package_slist_unref (transaction.remove_pkgs);
    transaction.remove_pkgs = NULL;

    printf ("Transaction cleared\n");
}

static void
packman_test_list (RCPackman *p, char *line)
{
    GSList *iter;

    CHECK_EXTRA (line, "list");

    for (iter = transaction.install_pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        printf ("Install: %s\n", pkg->package_filename);
    }

    for (iter = transaction.remove_pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);

        printf (" Remove: %s %d %s %s\n", pkg->spec.name, pkg->spec.epoch,
                pkg->spec.version, pkg->spec.release);
    }
}

static void
packman_test_run (RCPackman *p, char *line)
{
    char *test;
    gboolean flag = TRUE;

    line = pop_token (line, &test);

    if (test && !strcmp (test, "test"))
        flag = FALSE;

    rc_packman_transact (p, transaction.install_pkgs, transaction.remove_pkgs,
                         flag);

    if (rc_packman_get_error (p)) {
        packman_test_print (
            PACKMAN_TEST_ERROR,
            "%s", rc_packman_get_reason (p));
    }

    rc_package_slist_unref (transaction.install_pkgs);
    transaction.install_pkgs = NULL;

    rc_package_slist_unref (transaction.remove_pkgs);
    transaction.remove_pkgs = NULL;
}

char *
packman_completion_generator (const char *text, int state)
{
    static int list_index, len;
    char *name;

    if (state == 0) {
        list_index = 0;
        len = strlen (text);
    }

    while ((name = commands[list_index].name) != NULL) {
        list_index++;
        if (strncmp (name, text, len) == 0) {
            return (strdup (name));
        }
    }

    return NULL;
}

char **
packman_completion (char *text, int start, int end)
{
    char **matches = NULL;

    if (start == 0) {
        matches = rl_completion_matches (text, packman_completion_generator);
    }

    return matches;
}

int main (int argc, char **argv)
{
    RCPackman *p;
    gboolean done = FALSE;

    g_type_init ();

    transaction.install_pkgs = NULL;
    transaction.remove_pkgs = NULL;

    p = rc_distman_new ();

    if (rc_packman_get_error (p)) {
        printf ("ERROR: %s\n", rc_packman_get_reason (p));
        exit (-1);
    }

    g_signal_connect (p, "transact_start",
                      (GCallback) transact_start_cb, NULL);
    g_signal_connect (p, "transact_step",
                      (GCallback) transact_step_cb, NULL);
    g_signal_connect (p, "transact_progress",
                      (GCallback) transact_progress_cb, NULL);
    g_signal_connect (p, "transact_done",
                      (GCallback) transact_done_cb, NULL);

    rl_readline_name = "packman_test";
    rl_attempted_completion_function = (CPPFunction *)packman_completion;

    while (!done) {
        char *buf, *line, *command;
        guint i;
        gboolean handled = FALSE;

        buf = readline ("> ");

        line = buf;

        if (!line) {
            printf ("\n");
            done = TRUE;
            continue;
        }

        if (!*line) {
            free (buf);
            continue;
        }

        add_history (line);

        line = pop_token (line, &command);

        for (i = 0; i < num_commands; i++) {
            if (!strcmp (commands[i].name, command)) {
                commands[i].func (p, line);
                handled = TRUE;
                break;
            }
        }

        if (!handled) {
            printf ("ERROR: unknown command \"%s\"\n", command);
        }

        free (buf);
    }

    g_object_unref (p);

    return (0);
}
