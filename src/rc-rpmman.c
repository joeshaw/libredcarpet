/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rpmman.c
 * Copyright (C) 2000, 2001 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include <config.h>

#include <gmodule.h>

#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <rpm/rpmlib.h>

#include "rc-packman-private.h"
#include "rc-rpmman.h"
#include "rc-rpmman-types.h"
#include "rc-util.h"
#include "rc-verification-private.h"

#include "rpm-rpmlead.h"
#include "rpm-signature.h"

#define GTKFLUSH {while (gtk_events_pending ()) gtk_main_iteration ();}

#undef rpmdbNextIterator

/* in rpm <= 3.0.4, the filenames are stored in a single array in the
 * RPMTAG_FILENAMES header.  In rpm >= 3.0.4, the basenames and
 * directories are stored in RPMTAG_BASENAMES and RPMTAG_DIRNAMES,
 * where RPMTAG_DIRINDEXES contains an index for each BASENAME into
 * the RPMTAG_DIRNAMES header.  This appears to be a file level
 * incompatability -- packages built with rpm 3.0.3 will not have
 * entries for RPMTAG_BASENAMES, whereas packages built with rpm 3.0.4
 * will not have entries for RPMTAG_FILENAMES.  Consequently, any
 * given client needs to know to look for both.  Since these aren't
 * all defined in all rpm versions, we'll just cheat.
 */

#define RPMTAG_FILENAMES  1027
#define RPMTAG_DIRINDEXES 1116
#define RPMTAG_BASENAMES  1117
#define RPMTAG_DIRNAMES   1118

static void rc_rpmman_class_init (RCRpmmanClass *klass);
static void rc_rpmman_init       (RCRpmman *obj);

static GtkObjectClass *parent_class;

extern gchar *rc_libdir;

guint
rc_rpmman_get_type (void)
{
    static guint type = 0;

    if (!type) {
        GtkTypeInfo type_info = {
            "RCRpmman",
            sizeof (RCRpmman),
            sizeof (RCRpmmanClass),
            (GtkClassInitFunc) rc_rpmman_class_init,
            (GtkObjectInitFunc) rc_rpmman_init,
            (GtkArgSetFunc) NULL,
            (GtkArgGetFunc) NULL,
        };

        type = gtk_type_unique (rc_packman_get_type (), &type_info);
    }

    return type;
} /* rc_rpmman_get_type */

static FD_t
rc_rpm_open (RCRpmman *rpmman, const char *path, const char *fmode,
             int flags, mode_t mode)
{
    if (rpmman->Fopen) {
        return (rpmman->Fopen (path, fmode));
    } else {
        return (rpmman->rc_fdOpen (path, flags, mode));
    }
}

static void
rc_rpm_close (RCRpmman *rpmman, FD_t fd)
{
    if (rpmman->Fclose) {
        rpmman->Fclose (fd);
    } else {
        rpmman->rc_fdClose (fd);
    }
}

static size_t
rc_rpm_read (RCRpmman *rpmman, void *buf, size_t size, size_t nmemb, FD_t fd)
{
    if (rpmman->Fread) {
        return (rpmman->Fread (buf, size, nmemb, fd));
    } else {
        return (rpmman->rc_fdRead (fd, buf, nmemb));
    }
}

typedef struct _InstallState InstallState;

struct _InstallState {
    RCPackman *packman;
    guint seqno;
    guint install_total;
    guint install_extra;
    guint remove_total;
    gboolean configuring;
};

static void *
transact_cb (const Header h, const rpmCallbackType what,
             const unsigned long amount,
             const unsigned long total,
             const void * pkgKey, void * data)
{
    const char * filename = pkgKey;
    static FD_t fd;
    InstallState *state = (InstallState *) data;
    char *filename_copy;
    const char *base;

    /* heh heh heh */
    GTKFLUSH;

    switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
        fd = rc_rpm_open (RC_RPMMAN (state->packman), filename, "r.fdio",
                          O_RDONLY, 0);
        return fd;

    case RPMCALLBACK_INST_CLOSE_FILE:
        rc_rpm_close (RC_RPMMAN (state->packman), fd);
        break;

    case RPMCALLBACK_INST_PROGRESS:
        gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                 "transact_progress", amount, total);
        GTKFLUSH;
        break;

    case RPMCALLBACK_TRANS_PROGRESS:
        if (state->configuring) {
            gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                     "transact_step", ++state->seqno,
                                     RC_PACKMAN_STEP_CONFIGURE, NULL);
            GTKFLUSH;
            if (state->seqno == state->install_total) {
                state->configuring = FALSE;
            }
        } else {
            gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                     "transact_step", ++state->seqno,
                                     RC_PACKMAN_STEP_REMOVE, NULL);
            GTKFLUSH;
        }
        break;

    case RPMCALLBACK_INST_START:
        filename_copy = g_strdup (pkgKey);
        base = basename (filename_copy);
        gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                 "transact_step", ++state->seqno,
                                 RC_PACKMAN_STEP_INSTALL, base);
        g_free (filename_copy);
        break;

    case RPMCALLBACK_TRANS_START:
        if (state->install_total) {
            state->configuring = TRUE;
        }
        break;

    case RPMCALLBACK_UNINST_START:
    case RPMCALLBACK_UNINST_STOP:
    case RPMCALLBACK_UNINST_PROGRESS:
    case RPMCALLBACK_TRANS_STOP:
        /* ignore */
        break;
    }

    return NULL;
} /* transact_cb */

static guint
transaction_add_install_packages (RCPackman *packman,
                                  rpmTransactionSet transaction,
                                  RCPackageSList *install_packages)
{
    RCPackageSList *iter;
    FD_t fd;
    Header header;
    int rc;
    guint count = 0;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    for (iter = install_packages; iter; iter = iter->next) {
        gchar *filename = ((RCPackage *)(iter->data))->package_filename;

        fd = rc_rpm_open (rpmman, filename, "r.fdio", O_RDONLY, 0);

        /* if (fd == NULL || rpmman->Ferror (fd)) { */
        if (fd == NULL) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to open %s", filename);

            return (0);
        }

        rc = rpmman->rpmReadPackageHeader (fd, &header, NULL, NULL, NULL);

        switch (rc) {
        case 1:
            rc_rpm_close (rpmman, fd);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "can't read RPM header in %s", filename);

            return (0);

        default:
            rc_rpm_close (rpmman, fd);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "%s is not installable", filename);

            return (0);

        case 0:
            rc = rpmman->rpmtransAddPackage (
                transaction, header, NULL, filename, INSTALL_UPGRADE, NULL);
            count++;
            rpmman->headerFree (header);
            rc_rpm_close (rpmman, fd);

            switch (rc) {
            case 0:
                break;

            case 1:
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "error reading from %s", filename);

                return (0);

            case 2:
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "%s requires newer rpmlib", filename);

                return (0);

            default:
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "%s is not installable", filename);

                return (0);
            }
        }
    }

    return (count);
} /* transaction_add_install_pkgs */

static gchar *
rc_package_to_rpm_name (RCPackage *package)
{
    gchar *ret = NULL;

    g_assert (package);
    g_assert (package->spec.name);

    ret = g_strdup (package->spec.name);

    if (package->spec.version) {
        gchar *tmp = g_strconcat (ret, "-", package->spec.version, NULL);
        g_free (ret);
        ret = tmp;

        if (package->spec.release) {
            tmp = g_strconcat (ret, "-", package->spec.release, NULL);
            g_free (ret);
            ret = tmp;
        }
    }

    return (ret);
} /* rc_package_to_rpm_name */

static guint
transaction_add_remove_packages_v4 (RCPackman *packman,
                                    rpmTransactionSet transaction,
                                    RCPackageSList *remove_packages)
{
    RCPackageSList *iter;
    guint count = 0;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        gchar *package_name = rc_package_to_rpm_name (package);
        rc_rpmdbMatchIterator mi;
        Header header;
        unsigned int offset;

        mi = rpmman->rpmdbInitIterator (
            rpmman->db, RPMDBI_LABEL, package_name, 0);

        if (rpmman->rpmdbGetIteratorCount (mi) <= 0) {
            rc_packman_set_error
                (packman, RC_PACKMAN_ERROR_ABORT,
                 "package %s does not appear to be installed (%d)",
                 package_name,
                 RC_RPMMAN (packman)->rpmdbGetIteratorCount (mi));

            rpmman->rpmdbFreeIterator (mi);

            g_free (package_name);

            return (0);
        }

        while ((header = rpmman->rpmdbNextIterator (mi))) {
            offset = rpmman->rpmdbGetIteratorOffset (mi);

            rpmman->rpmtransRemovePackage (transaction, offset);

            count++;
        }

        rpmman->rpmdbFreeIterator (mi);

        g_free (package_name);
    }

    return (count);
}

static gboolean
transaction_add_remove_packages_v3 (RCPackman *packman,
                                    rpmTransactionSet transaction,
                                    RCPackageSList *remove_packages)
{
    int rc;
    RCPackageSList *iter;
    int i;
    guint count = 0;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        gchar *package_name = rc_package_to_rpm_name (package);
        rc_dbiIndexSet matches;

        rc = rpmman->rpmdbFindByLabel (rpmman->db, package_name,
                                       &matches);

        switch (rc) {
        case 0:
            for (i = 0; i < rpmman->dbiIndexSetCount (matches); i++) {
                unsigned int offset = rpmman->dbiIndexRecordOffset (
                    matches, i);

                if (offset) {
                    rpmman->rpmtransRemovePackage (transaction, offset);
                    count++;
                } else {
                    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                          "unable to locate %s in database",
                                          package_name);

                    g_free (package_name);

                    return (0);
                }
            }

            rpmman->dbiFreeIndexRecord (matches);

            g_free (package_name);

            break;

        case 1:
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "package %s does not appear to be installed",
                                  package_name);

            g_free (package_name);

            return (0);

/*
        case 2:
*/
        default:
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unspecified error removing %s",
                                  package_name);

            g_free (package_name);

            return (0);
        }
    }

    return (count);
} /* transaction_add_remove_pkgs */

static guint
transaction_add_remove_packages (RCPackman *packman,
                                 rpmTransactionSet transaction,
                                 RCPackageSList *remove_packages)
{
    if (RC_RPMMAN (packman)->major_version == 4) {
        return (transaction_add_remove_packages_v4 (
                    packman, transaction, remove_packages));
    } else {
        return (transaction_add_remove_packages_v3 (
                    packman, transaction, remove_packages));
    }
}

static void
rc_rpmman_transact (RCPackman *packman, RCPackageSList *install_packages,
                    RCPackageSList *remove_packages)
{
    rpmTransactionSet transaction;
    int rc;
    rpmProblemSet probs = NULL;
    InstallState state;
    RCPackageSList *iter;
    struct rpmDependencyConflict *conflicts;
    int transaction_flags, problem_filter;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    transaction_flags = 0; /* Nothing interesting to do here */
    problem_filter =
        RPMPROB_FILTER_REPLACEPKG |
        RPMPROB_FILTER_REPLACEOLDFILES |
        RPMPROB_FILTER_REPLACENEWFILES |
        RPMPROB_FILTER_OLDPACKAGE;

    transaction = rpmman->rpmtransCreateSet (rpmman->db, rpmman->rpmroot);

    state.packman = packman;
    state.seqno = 0;
    state.install_total = 0;
    state.install_extra = 0;
    state.remove_total = 0;
    state.configuring = FALSE;

    if (install_packages) {
        if (!(state.install_total = transaction_add_install_packages (
                  packman, transaction, install_packages)))
        {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "error adding packages to install");

            rpmman->rpmtransFree (transaction);

            goto ERROR;
        }
    }

    /* If we're actually installing packages we should expect there to
     * be configuration steps */
    if (state.install_total) {
        state.configuring = TRUE;
    } else {
        state.configuring = FALSE;
    }

    if (remove_packages) {
        if (!(state.remove_total = transaction_add_remove_packages (
                  packman, transaction, remove_packages)))
        {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "error adding packages to remove");

            rpmman->rpmtransFree (transaction);

            goto ERROR;
        }
    }

    if (rpmman->rpmdepCheck (transaction, &conflicts, &rc) || rc) {
        struct rpmDependencyConflict *conflict = conflicts;
        guint count;
        GString *dep_info = g_string_new ("");

        for (count = 0; count < rc; count++) {
            g_string_sprintfa (dep_info, "\n%s", conflict->byName);
            if (conflict->byVersion && conflict->byVersion[0]) {
                g_string_sprintfa (dep_info, "-%s", conflict->byVersion);
                if (conflict->byRelease && conflict->byRelease[0]) {
                    g_string_sprintfa (dep_info, "-%s", conflict->byRelease);
                }
            }

            if (conflict->sense == RPMDEP_SENSE_CONFLICTS) {
                g_string_sprintfa (dep_info, " conflicts with ");
            } else { /* RPMDEP_SENSE_REQUIRES */
                if (conflict->needsFlags & RPMSENSE_PREREQ) {
                    g_string_sprintfa (dep_info, " pre-requires ");
                } else {
                    g_string_sprintfa (dep_info, " requires ");
                }
            }

            g_string_sprintfa (dep_info, "%s", conflict->needsName);

            if (conflict->needsVersion && conflict->needsVersion[0]) {
                g_string_sprintfa (dep_info, " ");

                if (conflict->needsFlags & RPMSENSE_LESS) {
                    g_string_sprintfa (dep_info, "<");
                }

                if (conflict->needsFlags & RPMSENSE_GREATER) {
                    g_string_sprintfa (dep_info, ">");
                }

                if (conflict->needsFlags & RPMSENSE_EQUAL) {
                    g_string_sprintfa (dep_info, "=");
                }

                g_string_sprintfa (dep_info, " %s", conflict->needsVersion);
            }

            conflict++;
        }

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              dep_info->str);

        g_string_free (dep_info, TRUE);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "dependencies are not met");

        rpmman->rpmdepFreeConflicts (conflicts, rc);

        rpmman->rpmtransFree (transaction);

        goto ERROR;
    }

    if (rpmman->rpmdepOrder (transaction)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "circular dependencies in selected packages");

        rpmman->rpmtransFree (transaction);

        goto ERROR;
    }

    /* Let's check for packages which are upgrades rather than new
     * installs -- we're going to get two transaction steps for these,
     * not just one */
    state.install_extra = 0;

    for (iter = install_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);

        package = rc_packman_query_file (packman, package->package_filename);

        package->spec.epoch = 0;
        g_free (package->spec.version);
        package->spec.version = NULL;
        g_free (package->spec.release);
        package->spec.release = NULL;

        if ((package = rc_packman_query (packman, package))->installed) {
            state.install_extra++;
        }

        rc_package_free (package);
    }

    gtk_signal_emit_by_name (GTK_OBJECT (packman), "transact_start",
                             state.install_total * 2 + state.install_extra +
                             state.remove_total);
    GTKFLUSH;

    rc = rpmman->rpmRunTransactions (transaction,
                                     (rpmCallbackFunction) transact_cb,
                                     (void *) &state, NULL, &probs,
                                     transaction_flags, problem_filter);

    if (rc < 0) {
        /* After looking more closely at the RPM CLI, this isn't a
         * -real- error, although I'm not sure what it is. */
#if 0
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "install payload failed");

        goto ERROR;
#endif
    } else if (rc > 0) {
        guint count;
        rpmProblem *problem = probs->probs;
        GString *report = g_string_new ("");

        for (count = 0; count < probs->numProblems; count++) {
            g_string_sprintfa (report, "\n%s",
                               rpmman->rpmProblemString (*problem));
            problem++;
        }

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT, report->str);

        g_string_free (report, TRUE);

        rpmman->rpmProblemSetFree (probs);

        goto ERROR;
    }

    rpmman->rpmProblemSetFree (probs);

    gtk_signal_emit_by_name (GTK_OBJECT (packman), "transact_done");
    GTKFLUSH;

    rpmman->rpmtransFree (transaction);

    return;

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Unable to complete RPM transaction");
} /* rc_rpmman_transact */

static RCPackageSection
rc_rpmman_section_to_package_section (const gchar *rpmsection)
{
    char *major_section, *minor_section, *ptr;
    RCPackageSection ret = RC_SECTION_MISC;

    if ((ptr = strchr (rpmsection, '/'))) {
        major_section = g_strndup (rpmsection, ptr - rpmsection);
        minor_section = g_strdup (ptr + 1);
    } else {
        major_section = g_strdup (rpmsection);
        minor_section = NULL;
    }

    ptr = major_section;
    while (*ptr) {
        *ptr++ = tolower (*ptr);
    }
    ptr = minor_section;
    while (ptr && *ptr) {
        *ptr++ = tolower (*ptr);
    }

    if (!major_section || !major_section[0]) {
        goto DONE;
    }

    switch (major_section[0]) {
    case 'a':
        if (!strcmp (major_section, "amusements")) {
            ret = RC_SECTION_GAME;
            goto DONE;
        }
        if (!strcmp (major_section, "applications")) {
            if (!minor_section || !minor_section[0]) {
                goto DONE;
            }

            switch (minor_section[0]) {
            case 'a':
                if (!strcmp (minor_section, "archiving")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            case 'c':
                if (!strcmp (minor_section, "communications")) {
                    ret = RC_SECTION_INTERNET;
                    goto DONE;
                }
                if (!strcmp (minor_section, "cryptography")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            case 'e':
                if (!strcmp (minor_section, "editors")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                if (!strcmp (minor_section, "engineering")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            case 'f':
                if (!strcmp (minor_section, "file")) {
                    ret = RC_SECTION_SYSTEM;
                    goto DONE;
                }
                if (!strcmp (minor_section, "finance")) {
                    ret = RC_SECTION_OFFICE;
                    goto DONE;
                }
                goto DONE;

            case 'g':
                if (!strcmp (minor_section, "graphics")) {
                    ret = RC_SECTION_IMAGING;
                    goto DONE;
                }
                goto DONE;

            case 'i':
                if (!strcmp (minor_section, "internet")) {
                    ret = RC_SECTION_INTERNET;
                    goto DONE;
                }
                goto DONE;

            case 'm':
                if (!strcmp (minor_section, "multimedia")) {
                    ret = RC_SECTION_MULTIMEDIA;
                    goto DONE;
                }
                goto DONE;

            case 'o':
                if (!strcmp (minor_section, "office")) {
                    ret = RC_SECTION_OFFICE;
                    goto DONE;
                }
                goto DONE;

            case 'p':
                if (!strcmp (minor_section, "productivity")) {
                    ret = RC_SECTION_PIM;
                    goto DONE;
                }
                if (!strcmp (minor_section, "publishing")) {
                    ret = RC_SECTION_OFFICE;
                    goto DONE;
                }
                goto DONE;

            case 's':
                if (!strcmp (minor_section, "sound")) {
                    ret = RC_SECTION_MULTIMEDIA;
                    goto DONE;
                }
                if (!strcmp (minor_section, "system")) {
                    ret = RC_SECTION_SYSTEM;
                    goto DONE;
                }
                goto DONE;

            case 't':
                if (!strcmp (minor_section, "text")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            default:
                goto DONE;
            }
        }

    case 'd':
        if (!strcmp (major_section, "documentation")) {
            ret = RC_SECTION_DOC;
            goto DONE;
        }
        if (!strcmp (major_section, "development")) {
            if (!minor_section || !minor_section[0]) {
                goto DONE;
            }

            switch (minor_section[0]) {
            case 'd':
                if (!strcmp (minor_section, "debuggers")) {
                    ret = RC_SECTION_DEVELUTIL;
                    goto DONE;
                }
                goto DONE;

            case 'l':
                if (!strcmp (minor_section, "languages")) {
                    ret = RC_SECTION_DEVELUTIL;
                    goto DONE;
                }
                if (!strcmp (minor_section, "libraries")) {
                    ret = RC_SECTION_DEVEL;
                    goto DONE;
                }
                goto DONE;

            case 's':
                if (!strcmp (minor_section, "system")) {
                    ret = RC_SECTION_DEVEL;
                    goto DONE;
                }
                goto DONE;

            case 't':
                if (!strcmp (minor_section, "tools")) {
                    ret = RC_SECTION_DEVELUTIL;
                    goto DONE;
                }
                goto DONE;

            default:
                goto DONE;
            }
        }

    case 'l':
        if (!strcmp (major_section, "libraries")) {
            ret = RC_SECTION_LIBRARY;
            goto DONE;
        }
        goto DONE;

    case 's':
        if (!strcmp (major_section, "system environment")) {
            ret = RC_SECTION_SYSTEM;
            goto DONE;
        }
        goto DONE;

    case 'u':
        if (!strncmp (major_section, "user interface",
                      strlen ("user interface"))) {
            ret = RC_SECTION_XAPP;
            goto DONE;
        }
        goto DONE;

    case 'x':
        if (!strcmp (major_section, "x11")) {
            if (!minor_section || !minor_section[0]) {
                ret = RC_SECTION_XAPP;
                goto DONE;
            }

            switch (minor_section[0]) {
            case 'g':
                if (!strcmp (minor_section, "graphics")) {
                    ret = RC_SECTION_IMAGING;
                    goto DONE;
                }
                ret = RC_SECTION_XAPP;
                goto DONE;

            case 'u':
                if (!strcmp (minor_section, "utilities")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                ret = RC_SECTION_XAPP;
                goto DONE;

            default:
                goto DONE;
            }
        }
    }

  DONE:
    g_free (major_section);
    g_free (minor_section);
    return (ret);
}

/* Helper function to read values out of an rpm header safely.  If any of the
   paramaters are NULL, they're ignored.  Remember, you don't need to free any
   of the elements, since they are just pointers into the rpm header structure.
   Just remember to free the header later! */

static void
rc_rpmman_read_header (RCRpmman *rpmman, Header header, gchar **name,
                       guint32 *epoch, gchar **version, gchar **release,
                       RCPackageSection *section, guint32 *size,
                       gchar **summary, gchar **description)
{
    int_32 type, count;

    if (name) {
        gchar *tmpname;

        g_free (*name);
        *name = NULL;

        rpmman->headerGetEntry (header, RPMTAG_NAME, &type, (void **)&tmpname,
                                &count);

        if (count && (type == RPM_STRING_TYPE) && tmpname && tmpname[0]) {
            *name = g_strdup (tmpname);
        }
    }

    if (epoch) {
        guint32 *tmpepoch;

        *epoch = 0;

        rpmman->headerGetEntry (header, RPMTAG_EPOCH, &type,
                                (void **)&tmpepoch, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *epoch = *tmpepoch;
        }
    }

    if (version) {
        gchar *tmpver;

        g_free (*version);
        *version = NULL;

        rpmman->headerGetEntry (header, RPMTAG_VERSION, &type,
                                (void **)&tmpver, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpver && tmpver[0]) {
            *version = g_strdup (tmpver);
        }
    }

    if (release) {
        gchar *tmprel;

        g_free (*release);
        *release = NULL;

        rpmman->headerGetEntry (header, RPMTAG_RELEASE, &type,
                                (void **)&tmprel, &count);

        if (count && (type == RPM_STRING_TYPE) && tmprel && tmprel[0]) {
            *release = g_strdup (tmprel);
        }
    }

    if (section) {
        gchar *tmpsection;

        *section = RC_SECTION_MISC;

        rpmman->headerGetEntry (header, RPMTAG_GROUP, &type,
                                (void **)&tmpsection, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpsection &&
            tmpsection[0])
        {
            *section = rc_rpmman_section_to_package_section (tmpsection);
        } else {
            *section = RC_SECTION_MISC;
        }
    }

    if (size) {
        guint32 *tmpsize;

        *size = 0;

        rpmman->headerGetEntry (header, RPMTAG_SIZE, &type, (void **)&tmpsize,
                                &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *size = *tmpsize;
        }
    }

    if (summary) {
        gchar *tmpsummary;

        g_free (*summary);
        *summary = NULL;

        rpmman->headerGetEntry (header, RPMTAG_SUMMARY, &type,
                                (void **)&tmpsummary, &count);

        if (count && (type == RPM_STRING_TYPE)) {
            *summary = g_strdup (tmpsummary);
        }
    }

    if (description) {
        gchar *tmpdescription;

        g_free (*description);
        *description = NULL;

        rpmman->headerGetEntry (header, RPMTAG_DESCRIPTION, &type,
                                (void **)&tmpdescription, &count);

        if (count && (type == RPM_STRING_TYPE)) {
            *description = g_strdup (tmpdescription);
        }
    }
}

/* Takes an array of strings, which may be of the form <version>, or
   <epoch>:<version>, or <version>-<release>, or <epoch>:<version>-<release>,
   and does the right thing, breaking them into the right parts and returning
   the parts in the arrays epochs, versions, and releases. */

static void
parse_versions (char **inputs, guint32 **epochs, char ***versions,
                char ***releases, guint count)
{
    guint i;

    *versions = g_new0 (char *, count + 1);
    *releases = g_new0 (char *, count + 1);
    *epochs = g_new0 (guint32, count);

    for (i = 0; i < count; i++) {
        char *vptr = NULL, *rptr = NULL;

        if (!inputs[i] || !inputs[i][0]) {
            continue;
        }

        if ((vptr = strchr (inputs[i], ':'))) {
            char *endptr;
            (*epochs)[i] = strtoul (inputs[i], &endptr, 10);
            g_assert (endptr == vptr);
            vptr++;
        } else {
            vptr = inputs[i];
        }

        if ((rptr = strchr (vptr, '-'))) {
            (*versions)[i] = g_strndup (vptr, rptr - vptr);
            (*releases)[i] = g_strdup (rptr + 1);
        } else {
            (*versions)[i] = g_strdup (vptr);
        }
    }
}

static gboolean
in_set (gchar *item, const gchar **set) {
    const gchar **iter;

    for (iter = set; *iter; iter++) {
        if (strncmp (*iter, item, strlen (*iter)) == 0) {
            return (TRUE);
        }
    }

    return (FALSE);
}

static void
depends_fill_helper (RCRpmman *rpmman, Header header, int names_tag,
                     int versions_tag, int flags_tag, RCPackageDepSList **deps)
{
    char **names = NULL, **verrels = NULL, **versions, **releases;
    guint32 *epochs;
    int *relations = NULL;
    guint32 names_count = 0, versions_count = 0, flags_count = 0;
    int i;

    rpmman->headerGetEntry (header, names_tag, NULL, (void **)&names,
                            &names_count);
    if (versions_tag) {
        rpmman->headerGetEntry (header, versions_tag, NULL, (void **)&verrels,
                                &versions_count);
        if (flags_tag) {
            rpmman->headerGetEntry (header, flags_tag, NULL,
                                    (void **)&relations, &flags_count);
        }
    }

    if (names_count == 0) {
        return;
    }

    parse_versions (verrels, &epochs, &versions, &releases, versions_count);

    for (i = 0; i < names_count; i++) {
        RCPackageDep *dep;
        RCPackageRelation relation = RC_RELATION_ANY;

        if (!strncmp (names[i], "rpmlib(", strlen ("rpmlib("))) {
            continue;
        }

        if (versions_tag && versions_count) {
            if (flags_tag && flags_count) {
                if (relations[i] & RPMSENSE_LESS) {
                    relation |= RC_RELATION_LESS;
                }
                if (relations[i] & RPMSENSE_GREATER) {
                    relation |= RC_RELATION_GREATER;
                }
                if (relations[i] & RPMSENSE_EQUAL) {
                    relation |= RC_RELATION_EQUAL;
                }
            }
            dep = rc_package_dep_new (names[i], epochs[i], versions[i],
                                      releases[i], relation);
        } else {
            dep = rc_package_dep_new (names[i], 0, NULL, NULL, relation);
        }

        *deps = g_slist_prepend (*deps, dep);
    }

    free (names);
    free (verrels);

    g_strfreev (versions);
    g_strfreev (releases);
    g_free (epochs);
}

static void
rc_rpmman_depends_fill (RCRpmman *rpmman, Header header, RCPackage *package)
{
    RCPackageDep *dep;

    /* Shouldn't ever ask for dependencies unless you know what you really
       want (name, version, release) */
    g_assert (package->spec.name);
    g_assert (package->spec.version);
    g_assert (package->spec.release);

    depends_fill_helper (rpmman, header, RPMTAG_REQUIRENAME,
                         RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS,
                         &package->requires);

    depends_fill_helper (rpmman, header, RPMTAG_PROVIDENAME,
                         RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS,
                         &package->provides);

    depends_fill_helper (rpmman, header, RPMTAG_CONFLICTNAME,
                         RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS,
                         &package->conflicts);

    depends_fill_helper (rpmman, header, RPMTAG_OBSOLETENAME,
                         RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS,
                         &package->conflicts);

    /* Provide myself for the benefit of the dependency code */
    dep = rc_package_dep_new (package->spec.name, package->spec.epoch,
                              package->spec.version,
                              package->spec.release, RC_RELATION_EQUAL);
    package->provides = g_slist_prepend (package->provides, dep);

    /* First stab at handling the pesky file dependencies */
    {
        const gchar *file_dep_set[] = {
            "/bin/",
            "/usr/bin/",
            "/usr/X11R6/bin/",
            "/sbin/",
            "/usr/sbin/",
            "/lib/",
            "/usr/games/",
            "/usr/share/dict/words",
            "/etc/",
            "/opt/gnome/bin",
            "/opt/gnome/sbin",
            "/opt/gnome/etc",
            "/opt/gnome/games",
            "/usr/local/bin",   /* /usr/local shouldn't be required, but */
            "/usr/local/sbin",  /* apparently msc linux uses it */
            NULL
        };

        gchar **basenames, **dirnames;
        guint32 *dirindexes;
        int count, i;

        rpmman->headerGetEntry (header, RPMTAG_BASENAMES, NULL,
                                (void **)&basenames, &count);
        rpmman->headerGetEntry (header, RPMTAG_DIRNAMES, NULL,
                                (void **)&dirnames, NULL);
        rpmman->headerGetEntry (header, RPMTAG_DIRINDEXES, NULL,
                                (void **)&dirindexes, NULL);

        for (i = 0; i < count; i++) {
            gchar *tmp = g_strconcat (dirnames[dirindexes[i]], basenames[i],
                                      NULL);

            if (in_set (tmp, file_dep_set)) {
                dep = rc_package_dep_new (tmp, 0, NULL, NULL,
                                          RC_RELATION_ANY);

                package->provides = g_slist_prepend (package->provides, dep);
            }

            g_free (tmp);
        }

        free (basenames);
        free (dirnames);

        rpmman->headerGetEntry (header, RPMTAG_FILENAMES, NULL,
                                (void **)&basenames, &count);

        for (i = 0; i < count; i++) {
            if (in_set (basenames[i], file_dep_set)) {
                dep = rc_package_dep_new (basenames[i], 0, NULL, NULL,
                                          RC_RELATION_ANY);

                package->provides = g_slist_prepend (package->provides, dep);
            }
        }

        free (basenames);
    }
} /* rc_rpmman_depends_fill */

/* Query for information about a package (version, release, installed status,
   installed size...).  If you specify more than just a name, it tries to match
   those criteria. */

static gboolean
rc_rpmman_check_match (RCRpmman *rpmman, Header header, RCPackage *package)
{
    gchar *version = NULL, *release = NULL;
    gchar *summary = NULL, *description = NULL;
    guint32 size = 0, epoch = 0;
    RCPackageSection section = RC_SECTION_MISC;

    rc_rpmman_read_header (rpmman, header, NULL, &epoch, &version, &release,
                           &section, &size, &summary, &description);

    if ((!package->spec.version || !strcmp (package->spec.version, version)) &&
        (!package->spec.release || !strcmp (package->spec.release, release)) &&
        (!package->spec.epoch || package->spec.epoch == epoch)) {

        g_free (package->spec.version);
        g_free (package->spec.release);

        package->spec.epoch = epoch;
        package->spec.version = version;
        package->spec.release = release;
        package->section = section;
        package->summary = summary;
        package->description = description;
        package->installed = TRUE;
        package->installed_size = size;

        rc_rpmman_depends_fill (rpmman, header, package);

        return (TRUE);
    } else {
        g_free (version);
        g_free (release);
        g_free (summary);
        g_free (description);
    }

    return (FALSE);
}

static RCPackage *
rc_rpmman_query_v4 (RCPackman *packman, RCPackage *package)
{
    rc_rpmdbMatchIterator mi = NULL;
    Header header;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    mi = rpmman->rpmdbInitIterator (rpmman->db, RPMDBI_LABEL,
                                    package->spec.name, 0);

    if (!mi) {
        package->installed = FALSE;

        return (package);
    }

    while ((header = rpmman->rpmdbNextIterator (mi))) {
        if (rc_rpmman_check_match (rpmman, header, package)) {
            rpmman->rpmdbFreeIterator (mi);

            package->installed = TRUE;

            return (package);
        }
    }

    package->installed = FALSE;

    rpmman->rpmdbFreeIterator (mi);

    return (package);
} /* rc_packman_query */

static RCPackage *
rc_rpmman_query_v3 (RCPackman *packman, RCPackage *package)
{
    rc_dbiIndexSet matches;
    guint i;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    switch (rpmman->rpmdbFindPackage (rpmman->db, package->spec.name,
                                      &matches)) {
    case -1:
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database search");

        goto ERROR;

    case 1:
        package->installed = FALSE;

        return (package);

    default:
        break;
    }

    for (i = 0; i < matches.count; i++) {
        Header header;

        if (!(header = rpmman->rpmdbGetRecord (rpmman->db,
                                               matches.recs[i].recOffset))) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to fetch RPM header from database");

            goto ERROR;
        }

        if (rc_rpmman_check_match (RC_RPMMAN (packman), header, package)) {
            rpmman->headerFree (header);
            rpmman->dbiFreeIndexRecord (matches);
            return (package);
        }

        rpmman->headerFree (header);
    }

    package->installed = FALSE;

    rpmman->dbiFreeIndexRecord (matches);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    package->installed = FALSE;

    return (package);
} /* rc_packman_query */

static RCPackage *
rc_rpmman_query (RCPackman *packman, RCPackage *package)
{
    if (RC_RPMMAN (packman)->major_version == 4) {
        return (rc_rpmman_query_v4 (packman, package));
    } else {
        return (rc_rpmman_query_v3 (packman, package));
    }
}

/* Query a file for rpm header information */

static RCPackage *
rc_rpmman_query_file (RCPackman *packman, const gchar *filename)
{
    FD_t fd;
    Header header;
    RCPackage *package;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    fd = rc_rpm_open (rpmman, filename, "r.fdio", O_RDONLY, 0444);

    if (rpmman->rpmReadPackageHeader (fd, &header, NULL, NULL, NULL)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to read package header");

        goto ERROR;
    }

    package = rc_package_new ();

    rc_rpmman_read_header (rpmman, header, &package->spec.name,
                           &package->spec.epoch, &package->spec.version,
                           &package->spec.release, &package->section,
                           &package->installed_size, &package->summary,
                           &package->description);

    rc_rpmman_depends_fill (rpmman, header, package);

    rpmman->headerFree (header);
    rc_rpm_close (rpmman, fd);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "File query failed");

    return (NULL);
} /* rc_packman_query_file */

/* Query all of the packages on the system */

static RCPackageSList *
rc_rpmman_query_all_v4 (RCPackman *packman)
{
    RCPackageSList *list = NULL;
    rc_rpmdbMatchIterator mi = NULL;
    Header header;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    mi = rpmman->rpmdbInitIterator (rpmman->db, RPMDBI_PACKAGES, NULL, 0);

    if (!mi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database search");

        goto ERROR;
    }

    while ((header = rpmman->rpmdbNextIterator (mi))) {
        RCPackage *package = rc_package_new ();

        rc_rpmman_read_header (rpmman, header,
                               &package->spec.name, &package->spec.epoch,
                               &package->spec.version, &package->spec.release,
                               &package->section,
                               &package->installed_size, &package->summary,
                               &package->description);

        package->installed = TRUE;

        rc_rpmman_depends_fill (rpmman, header, package);

        list = g_slist_prepend (list, package);
    }

    rpmman->rpmdbFreeIterator(mi);

    return (list);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    return (NULL);
} /* rc_rpmman_query_all */

static RCPackageSList *
rc_rpmman_query_all_v3 (RCPackman *packman)
{
    RCPackageSList *list = NULL;
    guint recno;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    if (!(recno = rpmman->rpmdbFirstRecNum (rpmman->db))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to access RPM database");

        goto ERROR;
    }

    for (; recno; recno = rpmman->rpmdbNextRecNum (rpmman->db, recno)) {
        RCPackage *package;
        Header header;
        
        if (!(header = rpmman->rpmdbGetRecord (rpmman->db, recno))) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "Unable to read RPM database entry");

            rc_package_slist_free (list);

            goto ERROR;
        }

        package = rc_package_new ();

        rc_rpmman_read_header (rpmman, header,
                               &package->spec.name, &package->spec.epoch,
                               &package->spec.version, &package->spec.release,
                               &package->section, &package->installed_size,
                               &package->summary, &package->description);

        package->installed = TRUE;

        rc_rpmman_depends_fill (rpmman, header, package);

        list = g_slist_prepend (list, package);

        rpmman->headerFree(header);
    }

    return (list);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    return (NULL);
} /* rc_rpmman_query_all */

static RCPackageSList *
rc_rpmman_query_all (RCPackman *packman)
{
    RCPackageSList *packages;
#if 0
    RCPackage *internal;
    int *relations;
    char **verrels, **names;
    char **versions, **releases;
    guint32 *epochs;
    int i;
    int count;
#endif

    if (RC_RPMMAN (packman)->major_version == 4) {
        packages = rc_rpmman_query_all_v4 (packman);
    } else {
        packages = rc_rpmman_query_all_v3 (packman);
    }

#if 0
    internal = rc_package_new ();

    internal->spec.name = g_strdup ("rpmlib-internal");

    count = RC_RPMMAN (packman)->rpmGetRpmlibProvides (
        &names, &relations, &verrels);

    parse_versions (verrels, &epochs, &versions, &releases, count);

    for (i = 0; i < count; i++) {
        RCPackageDep *dep;
        RCPackageRelation relation = RC_RELATION_ANY;

        if (relations[i] & RPMSENSE_LESS) {
            relation |= RC_RELATION_LESS;
        }
        if (relations[i] & RPMSENSE_GREATER) {
            relation |= RC_RELATION_GREATER;
        }
        if (relations[i] & RPMSENSE_EQUAL) {
            relation |= RC_RELATION_EQUAL;
        }

        dep = rc_package_dep_new (names[i], epochs[i], versions[i],
                                  releases[i], relation);

        internal->provides = g_slist_prepend (internal->provides, dep);
    }

    free (names);
    free (verrels);

    g_free (epochs);
    g_strfreev (versions);
    g_strfreev (releases);

    packages = g_slist_prepend (packages, internal);
#endif

    return packages;
}

static gboolean
split_rpm (RCPackman *packman, RCPackage *package, gchar **signature_filename,
           gchar **payload_filename, guint8 **md5sum, guint32 *size)
{
    FD_t rpm_fd = NULL;
    int signature_fd;
    int payload_fd;
    struct rpmlead lead;
    Header signature_header;
    gchar *buf;
    guint32 count = 0;
    guchar buffer[128];
    ssize_t num_bytes;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    if (!package->package_filename) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "no file name specified");

        return (FALSE);
    }

    rpm_fd = rc_rpm_open (rpmman, package->package_filename, "r.fdio",
                          O_RDONLY, 0);

    /* if (!rpm_fd || rpmman->Ferror (rpm_fd)) { */
    if (rpm_fd == NULL) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to open %s", package->package_filename);

        return (FALSE);
    }

    if (rpmman->readLead (rpm_fd, &lead)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to read from %s",
                              package->package_filename);

        rc_rpm_close (rpmman, rpm_fd);

        return (FALSE);
    }

    if (rpmman->rpmReadSignature (rpm_fd, &signature_header,
                                  lead.signature_type) ||
        (signature_header == NULL))
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "failed to read RPM signature section");

        return (FALSE);
    }

    rpmman->headerGetEntry (signature_header, RPMSIGTAG_GPG, NULL,
                            (void **)&buf, &count);

    if (count > 0) {
        if ((signature_fd = mkstemp (*signature_filename)) == -1) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "failed to create temporary signature file");

            rpmman->headerFree (signature_header);

            rc_rpm_close (rpmman, rpm_fd);

            return (FALSE);
        }

        if (!rc_write (signature_fd, buf, count)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "failed to write temporary signature file");

            rpmman->headerFree (signature_header);

            rc_rpm_close (rpmman, rpm_fd);

            return (FALSE);
        }

        close (signature_fd);
    } else {
        g_free (*signature_filename);

        *signature_filename = NULL;
    }

    if ((payload_fd = mkstemp (*payload_filename)) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "failed to create temporary payload file");

        rpmman->headerFree (signature_header);

        g_free (*payload_filename);

        *payload_filename = NULL;
    } else {
        while ((num_bytes = rc_rpm_read (rpmman, buffer, sizeof (buffer[0]),
                                         sizeof (buffer), rpm_fd)) > 0)
        {
            if (!rc_write (payload_fd, buffer, num_bytes)) {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "unable to write temporary payload "
                                      "file");

                rc_close (payload_fd);

                rpmman->headerFree (signature_header);

                rc_rpm_close (rpmman, rpm_fd);

                return (FALSE);
            }
        }

        rc_close (payload_fd);
    }

    rc_rpm_close (rpmman, rpm_fd);

    count = 0;

    rpmman->headerGetEntry (signature_header, RPMSIGTAG_MD5, NULL,
                            (void **)&buf, &count);

    if (count > 0) {
        *md5sum = g_new (guint8, count);

        memcpy (*md5sum, buf, count);
    }

    count = 0;

    rpmman->headerGetEntry (signature_header, RPMSIGTAG_SIZE, NULL,
                            (void **)&buf, &count);

    if (count > 0) {
        *size = *((guint32 *)buf);
    } else {
        *size = 0;
    }

    rpmman->headerFree (signature_header);

    return (TRUE);
}

static RCVerificationSList *
rc_rpmman_verify (RCPackman *packman, RCPackage *package)
{
    RCVerificationSList *ret = NULL;
    RCVerification *verification = NULL;
    gchar *signature_filename = g_strdup ("/tmp/rpm-sig-XXXXXX");
    gchar *payload_filename = g_strdup ("/tmp/rpm-data-XXXXXX");
    guint8 *md5sum = NULL;
    guint32 size;

    if (!split_rpm (packman, package, &signature_filename, &payload_filename,
                    &md5sum, &size))
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "couldn't prepare package signature");

        if (signature_filename) {
            unlink (signature_filename);
        }

        if (payload_filename) {
            unlink (payload_filename);
        }

        g_free (signature_filename);
        g_free (payload_filename);

        g_free (md5sum);

        goto ERROR;
    }

    if (signature_filename) {
        verification = rc_verify_gpg (payload_filename, signature_filename);

        ret = g_slist_append (ret, verification);
    }

    if (md5sum) {
        verification = rc_verify_md5 (payload_filename, md5sum);

        ret = g_slist_append (ret, verification);
    }

    if (size > 0) {
        verification = rc_verify_size (payload_filename, size);

        ret = g_slist_append (ret, verification);
    }

    if (signature_filename) {
        unlink (signature_filename);
    }

    if (payload_filename) {
        unlink (payload_filename);
    }

    g_free (signature_filename);
    g_free (payload_filename);

    g_free (md5sum);

    return (ret);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Couldn't verify signatures");

    return (NULL);
} /* rc_rpmman_verify */

/* This was stolen from RPM */
/* And then slightly hacked on by me */
/* And then hacked on more by me */

/* compare alpha and numeric segments of two versions */
/* return 1: a is newer than b */
/*        0: a and b are the same version */
/*       -1: b is newer than a */
static int
vercmp(const char * a, const char * b)
{
    char oldch1, oldch2;
    char * str1, * str2;
    char * one, * two;
    int rc;
    int isnum;
    guint alen, blen;

    /* easy comparison to see if versions are identical */
    if (!strcmp(a, b)) return 0;

    alen = strlen (a);
    blen = strlen (b);

    str1 = alloca(alen + 1);
    str2 = alloca(blen + 1);

    strcpy(str1, a);
    strcpy(str2, b);

    /* Take care of broken Mandrake releases */
    if ((alen > 3) && !strcmp (a + alen - 3, "mdk")) {
        str1[alen - 3] = '\0';
    }
    if ((blen > 3) && !strcmp (b + blen - 3, "mdk")) {
        str2[blen - 3] = '\0';
    }

    one = str1;
    two = str2;

    /* loop through each version segment of str1 and str2 and compare them */
    while (*one && *two) {
        while (*one && !isalnum(*one)) one++;
        while (*two && !isalnum(*two)) two++;

#if 0
        /* Looks like we don't need this now, but just in case */

        while (*one && !isalnum(*one)) one++;
        while (!strncmp (one, "mdk", 3)) {
            one += 3;
            while (*one && !isalnum(*one)) one++;
        }
        while (*two && !isalnum(*two)) two++;
        while (!strncmp (two, "mdk", 3)) {
            two += 3;
            while (*two && !isalnum(*two)) two++;
        }
#endif

        str1 = one;
        str2 = two;

        /* grab first completely alpha or completely numeric segment */
        /* leave one and two pointing to the start of the alpha or numeric */
        /* segment and walk str1 and str2 to end of segment */
        if (isdigit(*str1)) {
            while (*str1 && isdigit(*str1)) str1++;
            while (*str2 && isdigit(*str2)) str2++;
            isnum = 1;
        } else {
            while (*str1 && isalpha(*str1)) str1++;
            while (*str2 && isalpha(*str2)) str2++;
            isnum = 0;
        }

        /* save character at the end of the alpha or numeric segment */
        /* so that they can be restored after the comparison */
        oldch1 = *str1;
        *str1 = '\0';
        oldch2 = *str2;
        *str2 = '\0';

        /* Here's how we handle comparing numeric and non-numeric
         * segments -- non-numeric (ximian.1) always sorts higher than
         * numeric (0_helix_1).  Original code is #if 0'd out
         * below. */
        if (one == str1) {
            if (isdigit (*two)) {
                return (1);
            } else {
                return (-1);
            }
        }
        if (two == str2) {
            if (isdigit (*one)) {
                return (-1);
            } else {
                return (1);
            }
        }
#if 0
        /* take care of the case where the two version segments are */
        /* different types: one numeric and one alpha */
        if (one == str1) return -1;	/* arbitrary */
        if (two == str2) return -1;
#endif

        if (isnum) {
            /* this used to be done by converting the digit segments */
            /* to ints using atoi() - it's changed because long  */
            /* digit segments can overflow an int - this should fix that. */

            /* throw away any leading zeros - it's a number, right? */
            while (*one == '0') one++;
            while (*two == '0') two++;

            /* whichever number has more digits wins */
            if (strlen(one) > strlen(two)) return 1;
            if (strlen(two) > strlen(one)) return -1;
        }

        /* strcmp will return which one is greater - even if the two */
        /* segments are alpha or if they are numeric.  don't return  */
        /* if they are equal because there might be more segments to */
        /* compare */
        rc = strcmp(one, two);
        if (rc) return rc;

        /* restore character that was replaced by null above */
        *str1 = oldch1;
        one = str1;
        *str2 = oldch2;
        two = str2;
    }

    /* this catches the case where all numeric and alpha segments have */
    /* compared identically but the segment sepparating characters were */
    /* different */
    if ((!*one) && (!*two)) return 0;

    /* whichever version still has characters left over wins */
    if (!*one) return -1; else return 1;
}

static gint
rc_rpmman_version_compare (RCPackman *packman,
                           RCPackageSpec *spec1,
                           RCPackageSpec *spec2)
{
    return (rc_packman_generic_version_compare (spec1, spec2, vercmp));
}

static RCPackage *
rc_rpmman_find_file_v4 (RCPackman *packman, const gchar *filename)
{
    rc_rpmdbMatchIterator mi = NULL;
    Header header;
    RCPackage *package;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    mi = rpmman->rpmdbInitIterator (rpmman->db, RPMTAG_BASENAMES, filename, 0);

    if (!mi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database iterator");

        goto ERROR;
    }

    header = rpmman->rpmdbNextIterator (mi);

    if (rpmman->rpmdbNextIterator (mi)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "found owners != 1");

        rpmman->rpmdbFreeIterator (mi);

        goto ERROR;
    }

    package = rc_package_new ();

    rc_rpmman_read_header (rpmman, header, &package->spec.name,
                           &package->spec.epoch, &package->spec.version,
                           &package->spec.release, &package->section,
                           &package->installed_size, &package->summary,
                           &package->description);

    rpmman->rpmdbFreeIterator (mi);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Find file failed");

    return (NULL);
}

static RCPackage *
rc_rpmman_find_file_v3 (RCPackman *packman, const gchar *filename)
{
    rc_dbiIndexSet matches;
    Header header;
    RCPackage *package;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    if (rpmman->rpmdbFindByFile (rpmman->db, filename, &matches) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "RPM database search failed");

        goto ERROR;
    }

    if (matches.count != 1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "found owners != 1");

        goto ERROR;
    }

    if (!(header = rpmman->rpmdbGetRecord (rpmman->db,
                                           matches.recs[0].recOffset))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "read of RPM header failed");

        goto ERROR;
    }

    package = rc_package_new ();

    rc_rpmman_read_header (rpmman, header, &package->spec.name,
                           &package->spec.epoch, &package->spec.version,
                           &package->spec.release, &package->section,
                           &package->installed_size, &package->summary,
                           &package->description);

    rpmman->dbiFreeIndexRecord (matches);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Find file failed");

    return (NULL);
}

static RCPackage *
rc_rpmman_find_file (RCPackman *packman, const gchar *filename)
{
    if (RC_RPMMAN (packman)->major_version == 4) {
        return (rc_rpmman_find_file_v4 (packman, filename));
    } else {
        return (rc_rpmman_find_file_v3 (packman, filename));
    }
}

static void
rc_rpmman_destroy (GtkObject *obj)
{
    RCRpmman *rpmman = RC_RPMMAN (obj);

    if (!getenv ("RC_NO_RPM_DB")) {
        rpmman->rpmdbClose (rpmman->db);
    }

#ifndef STATIC_RPM
#if 0
    /* If we do this we just look bad, sigh.  I can't really blame
     * rpmlib for not being designed to be dlopen'd.  Much as I like
     * to blame rpmlib for things. */
    g_module_close (rpmman->rpm_lib);
#endif
#endif

    if (GTK_OBJECT_CLASS (parent_class)->destroy)
        (* GTK_OBJECT_CLASS (parent_class)->destroy) (obj);
}

static void
rc_rpmman_class_init (RCRpmmanClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    RCPackmanClass *packman_class = (RCPackmanClass *) klass;

    object_class->destroy = rc_rpmman_destroy;

    parent_class = gtk_type_class (rc_packman_get_type ());

    packman_class->rc_packman_real_transact = rc_rpmman_transact;
    packman_class->rc_packman_real_query = rc_rpmman_query;
    packman_class->rc_packman_real_query_all = rc_rpmman_query_all;
    packman_class->rc_packman_real_query_file = rc_rpmman_query_file;
    packman_class->rc_packman_real_version_compare = rc_rpmman_version_compare;
    packman_class->rc_packman_real_verify = rc_rpmman_verify;
    packman_class->rc_packman_real_find_file = rc_rpmman_find_file;
} /* rc_rpmman_class_init */

#ifdef STATIC_RPM

static void
load_fake_syms (RCRpmman *rpmman)
{
    rpmman->rc_fdOpen = &fdOpen;
    rpmman->rc_fdRead = &fdRead;
    rpmman->rc_fdClose = &fdClose;
    /* rpmman->Ferror = &Ferror; */
    rpmman->headerGetEntry = &headerGetEntry;
    rpmman->headerFree = &headerFree;
    rpmman->rpmReadPackageHeader = &rpmReadPackageHeader;
    rpmman->rpmtransAddPackage = &rpmtransAddPackage;
    rpmman->rpmtransRemovePackage = &rpmtransRemovePackage;
    rpmman->rpmtransCreateSet = &rpmtransCreateSet;
    rpmman->rpmtransFree = &rpmtransFree;
    rpmman->rpmdepCheck = &rpmdepCheck;
    rpmman->rpmdepFreeConflicts = &rpmdepFreeConflicts;
    rpmman->rpmdepOrder = &rpmdepOrder;
    rpmman->rpmRunTransactions = &rpmRunTransactions;
    rpmman->rpmProblemSetFree = &rpmProblemSetFree;
    rpmman->readLead = &readLead;
    rpmman->rpmReadSignature = &rpmReadSignature;
    rpmman->rpmReadConfigFiles = &rpmReadConfigFiles;
    rpmman->rpmdbOpen = &rpmdbOpen;
    rpmman->rpmdbClose = &rpmdbClose;
    rpmman->rpmProblemString = &rpmProblemString;
#if 0
    rpmman->rpmGetRpmlibProvides = &rpmGetRpmlibProvides;
#endif

#ifdef RC_RPM4

    rpmman->rpmdbInitIterator = &rpmdbInitIterator;
    rpmman->rpmdbGetIteratorCount = &rpmdbGetIteratorCount;
    rpmman->rpmdbFreeIterator = &rpmdbFreeIterator;
    rpmman->rpmdbNextIterator = &XrpmdbNextIterator;
    rpmman->rpmdbGetIteratorOffset = &rpmdbGetIteratorOffset;

#else

    rpmman->rpmdbFirstRecNum = &rpmdbFirstRecNum;
    rpmman->rpmdbNextRecNum = &rpmdbNextRecNum;
    rpmman->rpmdbFindByLabel = &rpmdbFindByLabel;
    rpmman->rpmdbFindPackage = &rpmdbFindPackage;
    rpmman->rpmdbFindByFile = &rpmdbFindByFile;
    rpmman->rpmdbGetRecord = &rpmdbGetRecord;
    rpmman->dbiIndexSetCount = &dbiIndexSetCount;
    rpmman->dbiIndexRecordOffset = &dbiIndexRecordOffset;
    rpmman->dbiFreeIndexRecord = &dbiFreeIndexRecord;

#endif
}

#else

static gboolean
load_rpm_syms (RCRpmman *rpmman)
{
    if (!g_module_symbol (rpmman->rpm_lib, "Fopen",
                          ((gpointer)&rpmman->Fopen))) {
        if (!g_module_symbol (rpmman->rpm_lib, "fdOpen",
                              ((gpointer)&rpmman->rc_fdOpen))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "fdRead",
                              ((gpointer)&rpmman->rc_fdRead))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "fdClose",
                              ((gpointer)&rpmman->rc_fdClose))) {
            return (FALSE);
        }
        rpmman->Fopen = NULL;
        rpmman->Fclose = NULL;
        rpmman->Fread = NULL;
    } else {
        if (!g_module_symbol (rpmman->rpm_lib, "Fread",
                              ((gpointer)&rpmman->Fread))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "Fclose",
                              ((gpointer)&rpmman->Fclose))) {
            return (FALSE);
        }
        rpmman->rc_fdOpen = NULL;
        rpmman->rc_fdRead = NULL;
        rpmman->rc_fdClose = NULL;
    }
    /*
    if (!g_module_symbol (rpmman->rpm_lib, "Ferror",
                          ((gpointer)&rpmman->Ferror)))
    {
        return (FALSE);
    }
    */

    if (!g_module_symbol (rpmman->rpm_lib, "headerGetEntry",
                          ((gpointer)&rpmman->headerGetEntry))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "headerFree",
                          ((gpointer)&rpmman->headerFree))) {
        return (FALSE);
    }

    if (!g_module_symbol (rpmman->rpm_lib, "rpmReadPackageHeader",
                          ((gpointer)&rpmman->rpmReadPackageHeader))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmtransAddPackage",
                          ((gpointer)&rpmman->rpmtransAddPackage))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmtransRemovePackage",
                          ((gpointer)&rpmman->rpmtransRemovePackage))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmtransCreateSet",
                          ((gpointer)&rpmman->rpmtransCreateSet))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmtransFree",
                          ((gpointer)&rpmman->rpmtransFree))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmdepCheck",
                          ((gpointer)&rpmman->rpmdepCheck))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmdepFreeConflicts",
                          ((gpointer)&rpmman->rpmdepFreeConflicts))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmdepOrder",
                          ((gpointer)&rpmman->rpmdepOrder))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmRunTransactions",
                          ((gpointer)&rpmman->rpmRunTransactions))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmProblemSetFree",
                          ((gpointer)&rpmman->rpmProblemSetFree))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "readLead",
                          ((gpointer)&rpmman->readLead))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmReadSignature",
                          ((gpointer)&rpmman->rpmReadSignature))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmReadConfigFiles",
                          ((gpointer)&rpmman->rpmReadConfigFiles))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmdbOpen",
                          ((gpointer)&rpmman->rpmdbOpen))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmdbClose",
                          ((gpointer)&rpmman->rpmdbClose))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmProblemString",
                          ((gpointer)&rpmman->rpmProblemString))) {
        return (FALSE);
    }
#if 0
    if (!g_module_symbol (rpmman->rpm_lib, "rpmGetRpmlibProvides",
                          ((gpointer)&rpmman->rpmGetRpmlibProvides))) {
        return (FALSE);
    }
#endif

    if (rpmman->major_version == 4) {
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbInitIterator",
                              ((gpointer)&rpmman->rpmdbInitIterator))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbGetIteratorCount",
                              ((gpointer)&rpmman->rpmdbGetIteratorCount))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbFreeIterator",
                              ((gpointer)&rpmman->rpmdbFreeIterator))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "XrpmdbNextIterator",
                              ((gpointer)&rpmman->rpmdbNextIterator))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbGetIteratorOffset",
                              ((gpointer)&rpmman->rpmdbGetIteratorOffset))) {
            return (FALSE);
        }
    }

    if (rpmman->major_version == 3) {
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbFirstRecNum",
                              ((gpointer)&rpmman->rpmdbFirstRecNum))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbNextRecNum",
                              ((gpointer)&rpmman->rpmdbNextRecNum))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbFindByLabel",
                              ((gpointer)&rpmman->rpmdbFindByLabel))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbFindPackage",
                              ((gpointer)&rpmman->rpmdbFindPackage))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbFindByFile",
                              ((gpointer)&rpmman->rpmdbFindByFile))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbGetRecord",
                              ((gpointer)&rpmman->rpmdbGetRecord))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "dbiIndexSetCount",
                              ((gpointer)&rpmman->dbiIndexSetCount))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "dbiIndexRecordOffset",
                              ((gpointer)&rpmman->dbiIndexRecordOffset))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "dbiFreeIndexRecord",
                              ((gpointer)&rpmman->dbiFreeIndexRecord))) {
            return (FALSE);
        }
    }

    return (TRUE);
}

#endif

static void
parse_rpm_version (RCRpmman *rpmman, const gchar *version)
{
    const char *nptr = version;
    char *endptr = NULL;

    rpmman->major_version = -1;
    rpmman->minor_version = -1;
    rpmman->micro_version = -1;

    if (nptr && *nptr) {
        rpmman->major_version = strtoul (nptr, &endptr, 10);
        nptr = endptr;
        while (*nptr && !isalnum (*nptr)) {
            nptr++;
        }
    }
    if (nptr && *nptr) {
        rpmman->minor_version = strtoul (nptr, &endptr, 10);
        nptr = endptr;
        while (*nptr && !isalnum (*nptr)) {
            nptr++;
        }
    }
    if (nptr && *nptr) {
        rpmman->micro_version = strtoul (nptr, &endptr, 10);
    }
}

static void
rc_rpmman_init (RCRpmman *obj)
{
    RCPackman *packman = RC_PACKMAN (obj);
    gchar *tmp;
    int flags;
    gchar **rpm_version;
    gchar *so_file;

#ifdef STATIC_RPM

    extern char *RPMVERSION;

    parse_rpm_version (obj, RPMVERSION);

    load_fake_syms (obj);

#else

    if (!g_module_supported ()) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dynamic module loading not supported");
        return;
    }

    so_file = g_strdup_printf ("%s/rc-rpm-helper.so", rc_libdir);

    obj->rpm_lib = g_module_open (so_file, 0);

    g_free (so_file);

    if (!obj->rpm_lib) {
        so_file = g_strdup_printf ("%s/rc-rpm-helper-with-rpmio.so",
                                   rc_libdir);

        obj->rpm_lib = g_module_open (so_file, 0);

        g_free (so_file);
    }

    if (!obj->rpm_lib) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "unable to open rpm library");
        return;
    }

    if (!(g_module_symbol (obj->rpm_lib, "RPMVERSION",
                           ((gpointer)&rpm_version))))
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "unable to determine rpm version");
        return;
    }

    parse_rpm_version (obj, *rpm_version);

    if (!load_rpm_syms (obj)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "unable to load all symbols from rpm library");
        return;
    }

#endif

    obj->rpmReadConfigFiles (NULL, NULL);

    tmp = getenv ("RC_RPM_ROOT");

    if (tmp) {
        obj->rpmroot = tmp;
    } else {
        obj->rpmroot = "/";
    }

    /* If we're not root we can't open the database for writing */
    if (geteuid ()) {
        flags = O_RDONLY;
    } else {
        flags = O_RDWR;
    }

    if (!getenv ("RC_NO_RPM_DB")) {
        if (obj->rpmdbOpen (obj->rpmroot, &obj->db, flags, 0644)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "unable to open RPM database");
        }
    }

    rc_packman_set_file_extension(packman, "rpm");

    rc_package_dep_system_is_rpmish (TRUE);
} /* rc_rpmman_init */

RCRpmman *
rc_rpmman_new (void)
{
    RCRpmman *rpmman =
        RC_RPMMAN (gtk_type_new (rc_rpmman_get_type ()));

    return rpmman;
} /* rc_rpmman_new */
