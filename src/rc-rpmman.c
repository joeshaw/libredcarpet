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

#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#include <rpm/rpmlib.h>

#include "rc-packman-private.h"
#include "rc-rpmman.h"
#include "rc-util.h"
#include "rc-verification-private.h"

#ifdef HAVE_RPM_4_0
#include "rpmlead-4-0-x.h"
#include "signature-4-0-x.h"
#else
#include "rpmlead-3-0-x.h"
#include "signature-3-0-x.h"
#endif

#define GTKFLUSH {while (gtk_events_pending ()) gtk_main_iteration ();}

static void rc_rpmman_class_init (RCRpmmanClass *klass);
static void rc_rpmman_init       (RCRpmman *obj);

static GtkObjectClass *parent_class;

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

    /* heh heh heh */
    GTKFLUSH;

    switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
        fd = fdOpen(filename, O_RDONLY, 0);
        return fd;

    case RPMCALLBACK_INST_CLOSE_FILE:
        fdClose(fd);
        break;

    case RPMCALLBACK_INST_PROGRESS:
        gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                 "transact_progress", amount, total);
        GTKFLUSH;
        break;

    case RPMCALLBACK_TRANS_PROGRESS:
        if (state->configuring) {
            gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                     "configure_step", NULL, ++state->seqno);
            GTKFLUSH;
            if (state->seqno == state->install_total) {
                state->configuring = FALSE;
                state->seqno = 0;
                gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                         "configure_done");
                GTKFLUSH;
                gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                         "transact_start",
                                         state->install_total +
                                         state->install_extra +
                                         state->remove_total);
                GTKFLUSH;
            }
        } else {
            gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                     "transact_step", FALSE, NULL,
                                     ++state->seqno);
            GTKFLUSH;
        }
        break;

    case RPMCALLBACK_INST_START:
        gtk_signal_emit_by_name (GTK_OBJECT (state->packman),
                                 "transact_step", TRUE, pkgKey,
                                 ++state->seqno);
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

    for (iter = install_packages; iter; iter = iter->next) {
        gchar *filename = ((RCPackage *)(iter->data))->package_filename;

        fd = fdOpen (filename, O_RDONLY, 0);

        if (fd == NULL || Ferror (fd)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to open %s", filename);

            return (0);
        }

        rc = rpmReadPackageHeader (fd, &header, NULL, NULL, NULL);

        switch (rc) {
        case 1:
            fdClose (fd);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "can't read RPM header in %s", filename);

            return (0);

        default:
            fdClose (fd);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "%s is not installable", filename);

            return (0);

        case 0:
            rc = rpmtransAddPackage (transaction, header, NULL, filename, 1,
                                     NULL);
            count++;
            headerFree (header);
            fdClose (fd);

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

#ifdef HAVE_RPM_4_0

static guint
transaction_add_remove_packages (RCPackman *packman,
                                 rpmTransactionSet transaction,
                                 RCPackageSList *remove_packages)
{
    RCPackageSList *iter;
    guint count = 0;

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        gchar *package_name = rc_package_to_rpm_name (package);
        rpmdbMatchIterator mi;
        Header header;
        unsigned int offset;

        mi = rpmdbInitIterator (RC_RPMMAN (packman)->db, RPMDBI_LABEL,
                                package_name, 0);

        if (rpmdbGetIteratorCount (mi) <= 0) {
            rc_packman_set_error
                (packman, RC_PACKMAN_ERROR_ABORT,
                 "package %s does not appear to be installed (%d)",
                 package_name, rpmdbGetIteratorCount (mi));

            rpmdbFreeIterator (mi);

            g_free (package_name);

            return (0);
        }

/* So, this sucks, but apparently having multiple packages that are
 * exactly the same is a fairly common occurance in RPM land (ie, same
 * name, version, and release).  This came up twice tonight, so I
 * guess I'm going to have to ignore this case for now. */
#if 0
        if (count > 1) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "%s refers to %d packages", package_name,
                                  count);

            rpmdbFreeIterator (mi);

            g_free (package_name);

            return (0);
        }
#endif

        while (header = rpmdbNextIterator (mi)) {
            offset = rpmdbGetIteratorOffset (mi);

            rpmtransRemovePackage (transaction, offset);

            count++;
        }

        rpmdbFreeIterator (mi);

        g_free (package_name);
    }

    return (count);
}

#else /* !HAVE_RPM_4_0 */

static gboolean
transaction_add_remove_packages (RCPackman *packman,
                                 rpmTransactionSet transaction,
                                 RCPackageSList *remove_packages)
{
    int rc;
    RCPackageSList *iter;
    int i;
    guint count = 0;

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        gchar *package_name = rc_package_to_rpm_name (package);
        dbiIndexSet matches;

        rc = rpmdbFindByLabel (RC_RPMMAN (packman)->db, package_name,
                               &matches);

        switch (rc) {
        case 0:
            /* So we're no longer bothering to check for multiple
             * matches anymore (see the comment above for the RPM 4.x
             * case */

            for (i = 0; i < dbiIndexSetCount (matches); i++) {
                unsigned int offset = dbiIndexRecordOffset (matches, i);

                if (offset) {
                    rpmtransRemovePackage (transaction, offset);
                    count++;
                } else {
                    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                          "unable to locate %s in database",
                                          package_name);

                    g_free (package_name);

                    return (0);
                }
            }

            dbiFreeIndexRecord (matches);

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

#endif /* !HAVE_RPM_4_0 */

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

    transaction = rpmtransCreateSet (rpmman->db, RC_RPMMAN (packman)->rpmroot);

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

            rpmtransFree (transaction);

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

            rpmtransFree (transaction);

            goto ERROR;
        }
    }

    if (rpmdepCheck (transaction, &conflicts, &rc) || rc) {
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

        rpmdepFreeConflicts (conflicts, rc);

        rpmtransFree (transaction);

        goto ERROR;
    }

    if (rpmdepOrder (transaction)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "circular dependencies in selected packages");

        rpmtransFree (transaction);

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

    /* If we're installing any packages at all, expect to get the
       configure steps first.  Otherwise, you'll go directly to
       transacts for the removed packages. */
    if (state.install_total) {
        gtk_signal_emit_by_name (GTK_OBJECT (packman), "configure_start",
                                 state.install_total);
    } else {
        gtk_signal_emit_by_name (GTK_OBJECT (packman), "transact_start",
                                 state.remove_total);
    }

    rc = rpmRunTransactions (transaction, (rpmCallbackFunction) transact_cb,
                             (void *) &state, NULL, &probs, transaction_flags,
                             problem_filter);

    if (rc < 0) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "install payload failed");

        goto ERROR;
    } else if (rc > 0) {
        guint count;
        rpmProblem *problem = probs->probs;
        GString *report = g_string_new ("");

        for (count = 0; count < probs->numProblems; count++) {
            g_string_sprintfa (report, "\n%s", rpmProblemString (*problem));
            problem++;
        }

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT, report->str);

        g_string_free (report, TRUE);

        rpmProblemSetFree (probs);

        goto ERROR;
    }

    rpmProblemSetFree (probs);

    gtk_signal_emit_by_name (GTK_OBJECT (packman), "transact_done");
    GTKFLUSH;

    rpmtransFree (transaction);

    return;

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Unable to complete RPM transaction");
} /* rc_rpmman_transact */

static RCPackageSection
rc_rpmman_section_to_package_section (const gchar *rpmsection)
{
    gchar **sections;
    RCPackageSection ret = RC_SECTION_MISC;

    sections = g_strsplit (rpmsection, "/", 1);

    if (!sections[0] || !sections[0][0]) {
        goto DONE;
    }

    switch (sections[0][0]) {
    case 'A':
        if (!g_strcasecmp (sections[0], "Amusements")) {
            ret = RC_SECTION_GAME;
            goto DONE;
        }
        if (!g_strcasecmp (sections[0], "Applications")) {
            if (!sections[1] || !sections[1][0]) {
                goto DONE;
            }

            switch (sections[1][0]) {
            case 'A':
                if (!g_strcasecmp (sections[1], "Archiving")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            case 'C':
                if (!g_strcasecmp (sections[1], "Communications")) {
                    ret = RC_SECTION_INTERNET;
                    goto DONE;
                }
                if (!g_strcasecmp (sections[1], "Cryptography")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            case 'E':
                if (!g_strcasecmp (sections[1], "Editors")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                if (!g_strcasecmp (sections[1], "Engineering")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            case 'F':
                if (!g_strcasecmp (sections[1], "File")) {
                    ret = RC_SECTION_SYSTEM;
                    goto DONE;
                }
                if (!g_strcasecmp (sections[1], "Finance")) {
                    ret = RC_SECTION_OFFICE;
                    goto DONE;
                }
                goto DONE;

            case 'G':
                if (!g_strcasecmp (sections[1], "Graphics")) {
                    ret = RC_SECTION_IMAGING;
                    goto DONE;
                }
                goto DONE;

            case 'I':
                if (!g_strcasecmp (sections[1], "Internet")) {
                    ret = RC_SECTION_INTERNET;
                    goto DONE;
                }
                goto DONE;

            case 'M':
                if (!g_strcasecmp (sections[1], "Multimedia")) {
                    ret = RC_SECTION_MULTIMEDIA;
                    goto DONE;
                }
                goto DONE;

            case 'O':
                if (!g_strcasecmp (sections[1], "Office")) {
                    ret = RC_SECTION_OFFICE;
                    goto DONE;
                }
                goto DONE;

            case 'P':
                if (!g_strcasecmp (sections[1], "Productivity")) {
                    ret = RC_SECTION_PIM;
                    goto DONE;
                }
                if (!g_strcasecmp (sections[1], "Publishing")) {
                    ret = RC_SECTION_OFFICE;
                    goto DONE;
                }
                goto DONE;

            case 'S':
                if (!g_strcasecmp (sections[1], "Sound")) {
                    ret = RC_SECTION_MULTIMEDIA;
                    goto DONE;
                }
                if (!g_strcasecmp (sections[1], "System")) {
                    ret = RC_SECTION_SYSTEM;
                    goto DONE;
                }
                goto DONE;

            case 'T':
                if (!g_strcasecmp (sections[1], "Text")) {
                    ret = RC_SECTION_UTIL;
                    goto DONE;
                }
                goto DONE;

            default:
                goto DONE;
            }
        }

    case 'D':
        if (!g_strcasecmp (sections[0], "Documentation")) {
            ret = RC_SECTION_DOC;
            goto DONE;
        }
        if (!g_strcasecmp (sections[0], "Development")) {
            if (!sections[1] || !sections[1][0]) {
                goto DONE;
            }

            switch (sections[1][0]) {
            case 'D':
                if (!g_strcasecmp (sections[1], "Debuggers")) {
                    ret = RC_SECTION_DEVELUTIL;
                    goto DONE;
                }
                goto DONE;

            case 'L':
                if (!g_strcasecmp (sections[1], "Languages")) {
                    ret = RC_SECTION_DEVELUTIL;
                    goto DONE;
                }
                if (!g_strcasecmp (sections[1], "Libraries")) {
                    ret = RC_SECTION_DEVEL;
                    goto DONE;
                }
                goto DONE;

            case 'S':
                if (!g_strcasecmp (sections[1], "System")) {
                    ret = RC_SECTION_DEVEL;
                    goto DONE;
                }
                goto DONE;

            case 'T':
                if (!g_strcasecmp (sections[1], "Tools")) {
                    ret = RC_SECTION_DEVELUTIL;
                    goto DONE;
                }
                goto DONE;

            default:
                goto DONE;
            }
        }

    case 'L':
        if (!g_strcasecmp (sections[0], "Libraries")) {
            ret = RC_SECTION_LIBRARY;
            goto DONE;
        }
        goto DONE;

    case 'S':
        if (!g_strcasecmp (sections[0], "System Environment")) {
            ret = RC_SECTION_SYSTEM;
            goto DONE;
        }
        goto DONE;

    case 'U':
        if (!g_strncasecmp (sections[0], "User Interface",
                            strlen ("User Interface"))) {
            ret = RC_SECTION_XAPP;
            goto DONE;
        }
        goto DONE;

    case 'X':
        if (!g_strcasecmp (sections[0], "X11")) {
            if (!sections[1] || !sections[1][0]) {
                ret = RC_SECTION_XAPP;
                goto DONE;
            }

            switch (sections[1][0]) {
            case 'G':
                if (!g_strcasecmp (sections[1], "Graphics")) {
                    ret = RC_SECTION_IMAGING;
                    goto DONE;
                }
                ret = RC_SECTION_XAPP;
                goto DONE;

            case 'U':
                if (!g_strcasecmp (sections[1], "Utilities")) {
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
    g_strfreev (sections);
    return (ret);
}

/* Helper function to read values out of an rpm header safely.  If any of the
   paramaters are NULL, they're ignored.  Remember, you don't need to free any
   of the elements, since they are just pointers into the rpm header structure.
   Just remember to free the header later! */

static void
rc_rpmman_read_header (Header header, gchar **name, guint32 *epoch,
                       gchar **version, gchar **release,
                       RCPackageSection *section, guint32 *size,
                       gchar **summary, gchar **description)
{
    int_32 type, count;

    if (name) {
        gchar *tmpname;

        g_free (*name);
        *name = NULL;

        headerGetEntry (header, RPMTAG_NAME, &type, (void **)&tmpname, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpname && tmpname[0]) {
            *name = g_strdup (tmpname);
        }
    }

    if (epoch) {
        guint32 *tmpepoch;

        *epoch = 0;

        headerGetEntry (header, RPMTAG_EPOCH, &type, (void **)&tmpepoch, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *epoch = *tmpepoch;
        }
    }

    if (version) {
        gchar *tmpver;

        g_free (*version);
        *version = NULL;

        headerGetEntry (header, RPMTAG_VERSION, &type, (void **)&tmpver, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpver && tmpver[0]) {
            *version = g_strdup (tmpver);
        }
    }

    if (release) {
        gchar *tmprel;

        g_free (*release);
        *release = NULL;

        headerGetEntry (header, RPMTAG_RELEASE, &type, (void **)&tmprel, &count);

        if (count && (type == RPM_STRING_TYPE) && tmprel && tmprel[0]) {
            *release = g_strdup (tmprel);
        }
    }

    if (section) {
        gchar *tmpsection;

        *section = RC_SECTION_MISC;

        headerGetEntry (header, RPMTAG_GROUP, &type, (void **)&tmpsection,
                        &count);

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

        headerGetEntry (header, RPMTAG_SIZE, &type, (void **)&tmpsize, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *size = *tmpsize;
        }
    }

    if (summary) {
        gchar *tmpsummary;

        g_free (*summary);
        *summary = NULL;

        headerGetEntry (header, RPMTAG_SUMMARY, &type, (void **)&tmpsummary,
                        &count);

        if (count && (type == RPM_STRING_TYPE)) {
            *summary = g_strdup (tmpsummary);
        }
    }

    if (description) {
        gchar *tmpdescription;

        g_free (*description);
        *description = NULL;

        headerGetEntry (header, RPMTAG_DESCRIPTION, &type,
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

/* FIXME: I can make this faster with str[r]chr when I have more
 * time */

static void
parse_versions (gchar **inputs, guint32 **epochs, gchar ***versions,
                gchar ***releases, guint count)
{
    guint i;

    *versions = g_new0 (gchar *, count + 1);
    *releases = g_new0 (gchar *, count + 1);
    *epochs = g_new0 (guint32, count);

    for (i = 0; i < count; i++) {
        gchar **t1, **t2;

        if (!inputs[i] || !inputs[i][0]) {
            continue;
        }

        t1 = g_strsplit (inputs[i], ":", 1);

        if (t1[1]) {
            (*epochs)[i] = strtoul (t1[0], NULL, 10);
            t2 = g_strsplit (t1[1], "-", 1);
        } else {
            t2 = g_strsplit (t1[0], "-", 1);
        }

        (*versions)[i] = g_strdup (t2[0]);
        (*releases)[i] = g_strdup (t2[1]);

        g_strfreev (t1);
        g_strfreev (t2);
    }
}

static void
rc_rpmman_depends_fill (Header header, RCPackage *package)
{
    gchar **names, **verrels, **versions, **releases;
    guint32 *epochs;
    gint *relations;
    guint32 count;
    guint i;
    RCPackageDepItem *dep;
    RCPackageDep *depl = NULL;

    /* Shouldn't ever ask for dependencies unless you know what you really
       want (name, version, release) */

    g_assert (package->spec.name);
    g_assert (package->spec.version);
    g_assert (package->spec.release);

    /* Better safe than sorry, eh? */

    rc_package_dep_slist_free (package->requires);
    package->requires = NULL;
    rc_package_dep_slist_free (package->conflicts);
    package->conflicts = NULL;
    rc_package_dep_slist_free (package->provides);
    package->provides = NULL;

    /* Query the RPM database for the packages required by this package, and
       all associated information */

    headerGetEntry (header, RPMTAG_REQUIREFLAGS, NULL, (void **)&relations,
                    &count);
    headerGetEntry (header, RPMTAG_REQUIRENAME, NULL, (void **)&names, NULL);
    headerGetEntry (header, RPMTAG_REQUIREVERSION, NULL, (void **)&verrels, NULL);

    /* Some packages, such as setup and termcap, store a NULL in the
       RPMTAG_REQUIREVERSION, yet set the count to 1. That's pretty damned
       broken, so if that's the case, we'll set the count to zero, as we
       should. *sigh* */

    if (verrels == NULL) {
        count = 0;
    }

    parse_versions (verrels, &epochs, &versions, &releases, count);

    for (i = 0; i < count; i++) {
        RCPackageRelation relation = 0;

        if (relations[i] & RPMSENSE_LESS) {
            relation |= RC_RELATION_LESS;
        }
        if (relations[i] & RPMSENSE_GREATER) {
            relation |= RC_RELATION_GREATER;
        }
        if (relations[i] & RPMSENSE_EQUAL) {
            relation |= RC_RELATION_EQUAL;
        }

        if (versions[i] && !versions[i][0]) {
            g_free (versions[i]);
            versions[i] = NULL;
        }

        if (releases[i] && !releases[i][0]) {
            g_free (releases[i]);
            releases[i] = NULL;
        }

        if (names[i][0] == '/') {
            /* This is a broken RPM file dependency.  I hate it, I don't know
               how I want to support it, but I don't intend to drop to the
               filesystem and verify it, or ask vlad to do so.  For now I'm
               just going to ignore it. */
        } else if (!strncmp (names[i], "rpmlib(", strlen ("rpmlib("))) {
            /* This is a "seekret" message for rpmlib only */
        } else {
            /* Add the dependency to the list of dependencies */
            dep = rc_package_dep_item_new (names[i], epochs[i], versions[i],
                                           releases[i], relation);

            depl = g_slist_append (NULL, dep);

            package->requires = g_slist_append (package->requires, depl);
        }

        g_free (versions[i]);
        g_free (releases[i]);
    }

    free (names);
    free (verrels);

    g_free (versions);
    g_free (releases);
    g_free (epochs);

    names = NULL;
    verrels = NULL;

    /* Provide myself */

    dep = rc_package_dep_item_new (package->spec.name, package->spec.epoch,
                                   package->spec.version,
                                   package->spec.release, RC_RELATION_EQUAL);

    depl = g_slist_append (NULL, dep);

    package->provides = g_slist_append (package->provides, depl);

    /* RPM doesn't do versioned provides (as of 3.0.4), so we only need to find
       out the name of things that this package provides */

    headerGetEntry (header, RPMTAG_PROVIDENAME, NULL, (void **)&names, &count);

    for (i = 0; i < count; i++) {
        dep = rc_package_dep_item_new (names[i], 0, NULL, NULL,
                                       RC_RELATION_ANY);

        depl = g_slist_append (NULL, dep);

        package->provides = g_slist_append (package->provides, depl);
    }

    /* Only have to free the char** ones */

    free (names);
    names = NULL;

    /* Find full information on anything that this package conflicts with */

    headerGetEntry (header, RPMTAG_CONFLICTFLAGS, NULL, (void **)&relations,
                    &count);
    headerGetEntry (header, RPMTAG_CONFLICTNAME, NULL, (void **)&names, &count);
    headerGetEntry (header, RPMTAG_CONFLICTVERSION, NULL, (void **)&verrels,
                    &count);

    parse_versions (verrels, &epochs, &versions, &releases, count);

    for (i = 0; i < count; i++) {
        RCPackageRelation relation = 0;

        if (relations[i] & RPMSENSE_LESS) {
            relation |= RC_RELATION_LESS;
        }
        if (relations[i] & RPMSENSE_GREATER) {
            relation |= RC_RELATION_GREATER;
        }
        if (relations[i] & RPMSENSE_EQUAL) {
            relation |= RC_RELATION_EQUAL;
        }

        if (versions[i] && !versions[i][0]) {
            versions[i] = NULL;
        }

        if (releases[i] && !releases[i][0]) {
            releases[i] = NULL;
        }

        dep = rc_package_dep_item_new (names[i], epochs[i], versions[i],
                                       releases[i], relation);

        depl = g_slist_append (NULL, dep);

        package->conflicts = g_slist_append (package->conflicts, depl);

        g_free (versions[i]);
        g_free (releases[i]);
    }

    /* Only have to free the char** ones */

    free (names);
    free (verrels);

    g_free (versions);
    g_free (releases);
    g_free (epochs);
} /* rc_rpmman_depends_fill */

/* Query for information about a package (version, release, installed status,
   installed size...).  If you specify more than just a name, it tries to match
   those criteria. */

static gboolean
rc_rpmman_check_match (Header header, RCPackage *package)
{
    gchar *version = NULL, *release = NULL;
    gchar *summary = NULL, *description = NULL;
    guint32 size = 0, epoch = 0;
    RCPackageSection section = RC_SECTION_MISC;

    rc_rpmman_read_header (header, NULL, &epoch, &version, &release, &section,
                           &size, &summary, &description);

    /* FIXME: this could potentially be a big problem if an rpm can have
       just an epoch (serial), and no version/release.  I'm choosing to
       ignore that for now, because I need to get this stuff done and it
       doesn't seem very likely, but... */

    if ((!package->spec.version || !strcmp (package->spec.version, version)) &&
        (!package->spec.release || !strcmp (package->spec.release, release)) &&
        (!package->spec.epoch || package->spec.epoch == epoch)) {

        g_free (package->spec.version);
        g_free (package->spec.release);

        package->spec.epoch = epoch;
        package->spec.version = version;
        package->spec.release = release;
        package->summary = summary;
        package->description = description;
        package->installed = TRUE;
        package->installed_size = size;

        rc_rpmman_depends_fill (header, package);

        return (TRUE);
    } else {
        g_free (version);
        g_free (release);
        g_free (summary);
        g_free (description);
    }

    return (FALSE);
}

#ifdef HAVE_RPM_4_0

static RCPackage *
rc_rpmman_query (RCPackman *packman, RCPackage *package)
{
    rpmdbMatchIterator mi = NULL;
    Header header;

    mi = rpmdbInitIterator (RC_RPMMAN (packman)->db, RPMDBI_LABEL,
                            package->spec.name, 0);

    /* I think this is an error? */
    /* I guess not */

    if (!mi) {
#if 0
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database search");
#endif

#if 0
        goto ERROR;
#else
        return (package);
#endif
    }

    while ((header = rpmdbNextIterator (mi))) {
        if (rc_rpmman_check_match (header, package)) {
            rpmdbFreeIterator (mi);

            package->installed = TRUE;

            return (package);
        }
    }

    package->installed = FALSE;

    rpmdbFreeIterator (mi);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    package->installed = FALSE;

    return (package);
} /* rc_packman_query */

#else /* !HAVE_RPM_4_0 */

static RCPackage *
rc_rpmman_query (RCPackman *packman, RCPackage *package)
{
    dbiIndexSet matches;
    guint i;

    switch (rpmdbFindPackage (RC_RPMMAN (packman)->db,
                              package->spec.name, &matches)) {
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

        if (!(header = rpmdbGetRecord (RC_RPMMAN (packman)->db,
                                       matches.recs[i].recOffset))) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to fetch RPM header from database");

            goto ERROR;
        }

        if (rc_rpmman_check_match (header, package)) {
            headerFree (header);
            dbiFreeIndexRecord (matches);
            return (package);
        }

        headerFree (header);
    }

    package->installed = FALSE;

    dbiFreeIndexRecord (matches);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    package->installed = FALSE;

    return (package);
} /* rc_packman_query */

#endif /* !HAVE_RPM_4_0 */

/* Query a file for rpm header information */

static RCPackage *
rc_rpmman_query_file (RCPackman *packman, const gchar *filename)
{
    FD_t fd;
    Header header;
    RCPackage *package;

    fd = fdOpen (filename, O_RDONLY, 0444);

    if (rpmReadPackageHeader (fd, &header, NULL, NULL, NULL)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to read package header");

        goto ERROR;
    }

    package = rc_package_new ();

    rc_rpmman_read_header (header, &package->spec.name, &package->spec.epoch,
                           &package->spec.version, &package->spec.release,
                           &package->section, &package->installed_size,
                           &package->summary, &package->description);

    rc_rpmman_depends_fill (header, package);

    headerFree (header);
    fdClose (fd);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "File query failed");

    return (NULL);
} /* rc_packman_query_file */

/* Query all of the packages on the system */

#ifdef HAVE_RPM_4_0

static RCPackageSList *
rc_rpmman_query_all (RCPackman *packman)
{
    RCPackageSList *list = NULL;
    rpmdbMatchIterator mi = NULL;
    Header header;

    mi = rpmdbInitIterator (RC_RPMMAN (packman)->db, RPMDBI_PACKAGES,
                            NULL, 0);

    if (!mi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database search");

        goto ERROR;
    }

    while ((header = rpmdbNextIterator (mi))) {
        RCPackage *package = rc_package_new ();

        rc_rpmman_read_header (header, &package->spec.name, &package->spec.epoch,
                               &package->spec.version, &package->spec.release,
                               &package->section,
                               &package->installed_size, &package->summary,
                               &package->description);

        package->installed = TRUE;

        rc_rpmman_depends_fill (header, package);

        list = g_slist_append (list, package);
    }

    rpmdbFreeIterator(mi);

    return (list);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    return (NULL);
} /* rc_rpmman_query_all */

#else /* !HAVE_RPM_4_0 */

static RCPackageSList *
rc_rpmman_query_all (RCPackman *packman)
{
    RCPackageSList *list = NULL;
    guint recno;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    if (!(recno = rpmdbFirstRecNum (rpmman->db))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to access RPM database");

        goto ERROR;
    }

    for (; recno; recno = rpmdbNextRecNum (rpmman->db, recno)) {
        RCPackage *package;
        Header header;
        
        if (!(header = rpmdbGetRecord (rpmman->db, recno))) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "Unable to read RPM database entry");

            rc_package_slist_free (list);

            goto ERROR;
        }

        package = rc_package_new ();

        rc_rpmman_read_header (header, &package->spec.name,
                               &package->spec.epoch, &package->spec.version,
                               &package->spec.release, &package->section,
                               &package->installed_size, &package->summary,
                               &package->description);

        package->installed = TRUE;

        rc_rpmman_depends_fill (header, package);

        list = g_slist_append (list, package);

        headerFree(header);
    }

    return (list);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "System query failed");

    return (NULL);
} /* rc_rpmman_query_all */

#endif /* !HAVE_RPM_4_0 */

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

    if (!package->package_filename) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "no file name specified");

        return (FALSE);
    }

    rpm_fd = fdOpen (package->package_filename, O_RDONLY, 0);

    if (!rpm_fd || Ferror (rpm_fd)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to open %s", package->package_filename);

        return (FALSE);
    }

    if (readLead (rpm_fd, &lead)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to read from %s",
                              package->package_filename);

        fdClose (rpm_fd);

        return (FALSE);
    }

    if (rpmReadSignature (rpm_fd, &signature_header, lead.signature_type) ||
        (signature_header == NULL))
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "failed to read RPM signature section");

        return (FALSE);
    }

    headerGetEntry (signature_header, RPMSIGTAG_GPG, NULL, (void **)&buf,
                    &count);

    if (count > 0) {
        if ((signature_fd = mkstemp (*signature_filename)) == -1) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "failed to create temporary signature file");

            headerFree (signature_header);

            fdClose (rpm_fd);

            return (FALSE);
        }

        if (!rc_write (signature_fd, buf, count)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "failed to write temporary signature file");

            headerFree (signature_header);

            fdClose (rpm_fd);

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

        headerFree (signature_header);

        g_free (*payload_filename);

        *payload_filename = NULL;
    } else {
        while ((num_bytes = fdRead (rpm_fd, buffer, sizeof (buffer))) > 0) {
            if (!rc_write (payload_fd, buffer, num_bytes)) {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "unable to write temporary payload "
                                      "file");

                rc_close (payload_fd);

                headerFree (signature_header);

                fdClose (rpm_fd);

                return (FALSE);
            }
        }
    }

    fdClose (rpm_fd);

    count = 0;

    headerGetEntry (signature_header, RPMSIGTAG_MD5, NULL, (void **)&buf,
                    &count);

    if (count > 0) {
        *md5sum = g_new (guint8, count);

        memcpy (*md5sum, buf, count);
    } else {
        *md5sum = NULL;
    }

    count = 0;

    headerGetEntry (signature_header, RPMSIGTAG_SIZE, NULL, (void **)&buf,
                    &count);

    if (count > 0) {
        *size = *((guint32 *)buf);
    } else {
        *size = 0;
    }

    headerFree (signature_header);

    return (TRUE);
}

static RCVerificationSList *
rc_rpmman_verify (RCPackman *packman, RCPackage *package)
{
    RCVerificationSList *ret = NULL;
    RCVerification *verification = NULL;
    gchar *signature_filename = g_strdup ("/tmp/rpm-sig-XXXXXX");
    gchar *payload_filename = g_strdup ("/tmp/rpm-data-XXXXXX");
    guint8 *md5sum;
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

	/* take care of the case where the two version segments are */
	/* different types: one numeric and one alpha */
	if (one == str1) return -1;	/* arbitrary */
	if (two == str2) return -1;

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

#ifdef HAVE_RPM_4_0

static RCPackage *
rc_rpmman_find_file (RCPackman *packman, const gchar *filename)
{
    rpmdbMatchIterator mi = NULL;
    Header header;
    RCPackage *package;

    mi = rpmdbInitIterator (RC_RPMMAN (packman)->db, RPMTAG_BASENAMES,
                            filename, 0);

    if (!mi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database iterator");

        goto ERROR;
    }

    header = rpmdbNextIterator (mi);

    if (rpmdbNextIterator (mi)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "found owners != 1");

        rpmdbFreeIterator (mi);

        goto ERROR;
    }

    package = rc_package_new ();

    rc_rpmman_read_header (header, &package->spec.name, &package->spec.epoch,
                           &package->spec.version, &package->spec.release,
                           &package->section, &package->installed_size,
                           &package->summary, &package->description);

    rpmdbFreeIterator (mi);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Find file failed");

    return (NULL);
}

#else /* !HAVE_RPM_4_0 */

static RCPackage *
rc_rpmman_find_file (RCPackman *packman, const gchar *filename)
{
    dbiIndexSet matches;
    Header header;
    RCPackage *package;

    if (rpmdbFindByFile (RC_RPMMAN (packman)->db, filename, &matches) == -1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "RPM database search failed");

        goto ERROR;
    }

    if (matches.count != 1) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "found owners != 1");

        goto ERROR;
    }

    if (!(header = rpmdbGetRecord (RC_RPMMAN (packman)->db,
                                   matches.recs[0].recOffset))) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "read of RPM header failed");

        goto ERROR;
    }

    package = rc_package_new ();

    rc_rpmman_read_header (header, &package->spec.name, &package->spec.epoch,
                           &package->spec.version, &package->spec.release,
                           &package->section, &package->installed_size,
                           &package->summary, &package->description);

    dbiFreeIndexRecord (matches);

    return (package);

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                          "Find file failed");

    return (NULL);
}

#endif

static void
rc_rpmman_destroy (GtkObject *obj)
{
    RCRpmman *rpmman = RC_RPMMAN (obj);

    g_free (rpmman->rpmroot);

    rpmdbClose (rpmman->db);

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

static void
rc_rpmman_init (RCRpmman *obj)
{
    RCPackman *packman = RC_PACKMAN (obj);
    gchar *tmp;
    int flags;

    rpmReadConfigFiles (NULL, NULL);

    tmp = getenv ("RC_RPM_ROOT");

    if (tmp) {
        obj->rpmroot = g_strdup (tmp);
    } else {
        obj->rpmroot = g_strdup ("/");
    }

    /* If we're not root we can't open the database for writing */
    if (geteuid ()) {
        flags = O_RDONLY;
    } else {
        flags = O_RDWR;
    }

    if (!getenv ("RC_NO_RPM_DB")) {
        if (rpmdbOpen (obj->rpmroot, &obj->db, flags, 0644)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                                  "unable to open RPM database");
        }
    }

    packman->priv->features =
        RC_PACKMAN_FEATURE_PRE_CONFIG |
        RC_PACKMAN_FEATURE_PKG_PROGRESS;

    rc_package_dep_system_is_rpmish (TRUE);
} /* rc_rpmman_init */

RCRpmman *
rc_rpmman_new (void)
{
    RCRpmman *rpmman =
        RC_RPMMAN (gtk_type_new (rc_rpmman_get_type ()));

    return rpmman;
} /* rc_rpmman_new */
