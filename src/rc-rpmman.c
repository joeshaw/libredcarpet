/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include "rc-packman-private.h"
#include "rc-rpmman.h"
#include "rc-util.h"

#include <fcntl.h>
#include <string.h>

#include <rpm/rpmurl.h>
#include <rpm/rpmmacro.h>
#include <rpm/misc.h>
#include <rpm/header.h>
#include <rpm/rpmlib.h>

#ifdef HAVE_RPM_4_0
#include "rpmlead-4-0-x.h"
#include "signature-4-0-x.h"
#else
#include "rpmlead-3-0-x.h"
#include "signature-3-0-x.h"
#endif

static void rc_rpmman_class_init (RCRpmmanClass *klass);
static void rc_rpmman_init       (RCRpmman *obj);

static GtkObjectClass *rc_rpmman_parent;

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
    RCPackman *p;
    gint seqno;
    gint install_total;
    gint remove_total;
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
    InstallState *state = (InstallState *)data;

    switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
        fd = fdOpen(filename, O_RDONLY, 0);
        return fd;

    case RPMCALLBACK_INST_CLOSE_FILE:
        fdClose(fd);
        break;

    case RPMCALLBACK_INST_PROGRESS:
        rc_packman_transaction_progress (state->p, amount, total);
        if (amount == total) {
            rc_packman_transaction_step (state->p, ++state->seqno,
                                         state->install_total +
                                         state->remove_total);
        }
        break;

    case RPMCALLBACK_TRANS_PROGRESS:
        if (state->configuring) {
            rc_packman_configure_step (state->p, ++state->seqno,
                                       state->install_total);
            if (state->seqno == state->install_total) {
                state->configuring = FALSE;
                state->seqno = 0;
                rc_packman_configure_done (state->p);
                rc_packman_transaction_step (state->p, 0,
                                             state->install_total +
                                             state->remove_total);
            }
        } else {
            rc_packman_transaction_step (state->p, ++state->seqno,
                                         state->install_total +
                                         state->remove_total);
        }
        break;

    case RPMCALLBACK_INST_START:
        break;

    case RPMCALLBACK_TRANS_START:
        state->configuring = TRUE;
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

static gboolean
transaction_add_install_pkgs (RCPackman *p, rpmTransactionSet transaction,
                              GSList *install_pkgs)
{
    GSList *iter;
    FD_t fd;
    Header hdr;
    int rc;

    for (iter = install_pkgs; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);

        fd = Fopen (filename, "r.ufdio");

        if (fd == NULL || Ferror (fd)) {
            /* FIXME: Fatal error opening the file */
            return (FALSE);
        }

        rc = rpmReadPackageHeader (fd, &hdr, NULL, NULL, NULL);

        switch (rc) {
        case 1:
            Fclose (fd);
            /* FIXME: not an RPM package? */
            return (FALSE);
            break;

        default:
            Fclose (fd);
            /* FIXME: cannot be installed? */
            return (FALSE);
            break;

        case 0:
            rc = rpmtransAddPackage (transaction, hdr, NULL, filename, 1,
                                     NULL);
            headerFree (hdr);
            Fclose (fd);

            switch (rc) {
            case 0:
                break;

            case 1:
                /* FIXME: Error reading from file */
                return (FALSE);
                break;

            case 2:
                /* FIXME: Requires newer RPM */
                return (FALSE);
                break;
            }
        }
    }

    return (TRUE);
} /* transaction_add_install_pkgs */

static gchar *
rc_package_to_rpm_name (RCPackage *pkg)
{
    gchar *ret = NULL;

    g_assert (pkg);
    g_assert (pkg->spec.name);

    ret = g_strdup (pkg->spec.name);

    if (pkg->spec.version) {
        gchar *tmp = g_strconcat (ret, "-", pkg->spec.version, NULL);
        g_free (ret);
        ret = tmp;

        if (pkg->spec.release) {
            tmp = g_strconcat (ret, "-", pkg->spec.release, NULL);
            g_free (ret);
            ret = tmp;
        }
    }

    return (ret);
} /* rc_package_to_rpm_name */

#ifdef HAVE_RPM_4_0

static gboolean
transaction_add_remove_pkgs (RCPackman *p, rpmTransactionSet transaction,
                             rpmdb db, RCPackageSList *remove_pkgs)
{
    RCPackageSList *iter;

    for (iter = remove_pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);
        gchar *pkg_name = rc_package_to_rpm_name (pkg);
        rpmdbMatchIterator mi;
        Header hdr;
        unsigned int offset;
	int count;

        mi = rpmdbInitIterator (db, RPMDBI_LABEL, pkg_name, 0);
        count = rpmdbGetIteratorCount (mi);

        if (count <= 0) {
            /* FIXME: package is not installed */
            rpmdbFreeIterator (mi);
            return (FALSE);
        }

        if (count > 1) {
            /* FIXME: we've specified multiple packages? */
            rpmdbFreeIterator (mi);
            return (FALSE);
        }

        hdr = rpmdbNextIterator (mi);
        offset = rpmdbGetIteratorOffset (mi);

        rpmtransRemovePackage (transaction, offset);
        rpmdbFreeIterator (mi);
    }

    return (TRUE);
}

#else /* !HAVE_RPM_4_0 */

static gboolean
transaction_add_remove_pkgs (RCPackman *p, rpmTransactionSet transaction,
                             rpmdb db, RCPackageSList *remove_pkgs)
{
    int rc;
    RCPackageSList *iter;
    int i;
    int count;

    for (iter = remove_pkgs; iter; iter = iter->next) {
        RCPackage *pkg = (RCPackage *)(iter->data);
        gchar *pkg_name = rc_package_to_rpm_name (pkg);
        dbiIndexSet matches;

        rc = rpmdbFindByLabel (db, pkg_name, &matches);

        g_free (pkg_name);

        switch (rc) {
        case 1:
            /* FIXME: package is not installed */
            return (FALSE);
            break;

        case 2:
            /* FIXME: couldn't find that package? */
            return (FALSE);
            break;

        default:
            count = 0;
            for (i = 0; i < dbiIndexSetCount (matches); i++) {
                if (dbiIndexRecordOffset (matches, i)) {
                    count++;
                }
            }

            if (count > 1) {
                /* FIXME: multiple matches!  (fucking rpm) */
                return (FALSE);
            }

            for (i = 0; i < dbiIndexSetCount (matches); i++) {
                unsigned int offset = dbiIndexRecordOffset (matches, i);

                if (offset) {
                    rpmtransRemovePackage (transaction, offset);
                }
            }

            dbiFreeIndexRecord (matches);
            break;
        }
    }

    return (TRUE);
} /* transaction_add_remove_pkgs */

#endif /* !HAVE_RPM_4_0 */

static void
rc_rpmman_transact (RCPackman *p, GSList *install_pkgs,
                    RCPackageSList *remove_pkgs)
{
    rpmdb db = NULL;
    rpmTransactionSet transaction;
    int rc;
    rpmProblemSet probs = NULL;
    InstallState *state;

    if (rpmdbOpen (RC_RPMMAN (p)->rpmroot, &db, O_RDWR | O_CREAT, 0644)) {
        /* FIXME: Fatal error opening the database */
        return;
    }

    transaction = rpmtransCreateSet (db, RC_RPMMAN (p)->rpmroot);

    if (!(transaction_add_install_pkgs (p, transaction, install_pkgs))) {
        /* FIXME: Uh oh */
        return;
    }

    if (!(transaction_add_remove_pkgs (p, transaction, db, remove_pkgs))) {
        /* FIXME: Uh oh */
        return;
    }

    if (rpmdepOrder (transaction)) {
        /* FIXME: Failed to order transaction */
        return;
    }

    state = g_new0 (InstallState, 1);
    state->p = p;
    state->seqno = 0;
    state->install_total = g_slist_length (install_pkgs);
    state->remove_total = g_slist_length (remove_pkgs);

    rc_packman_configure_step (p, 0, state->install_total);

    rc = rpmRunTransactions (transaction, transact_cb, (void *) state,
                             NULL, &probs, 0, 0);

    rc_packman_transaction_done (p);

    rpmdbClose (db);
} /* rc_rpmman_transact */

/* Helper function to read values out of an rpm header safely.  If any of the
   paramaters are NULL, they're ignored.  Remember, you don't need to free any
   of the elements, since they are just pointers into the rpm header structure.
   Just remember to free the header later! */

static void
rc_rpmman_read_header (Header hdr, gchar **name, guint32 *epoch,
                       gchar **version, gchar **release,
                       RCPackageSection *section, guint32 *size,
                       gchar **summary, gchar **description)
{
    int_32 type, count;

    if (name) {
        gchar *tmpname;

        g_free (*name);
        *name = NULL;

        headerGetEntry (hdr, RPMTAG_NAME, &type, (void **)&tmpname, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpname && tmpname[0]) {
            *name = g_strdup (tmpname);
        }
    }

    if (epoch) {
        guint32 *tmpepoch;

        *epoch = 0;

        headerGetEntry (hdr, RPMTAG_EPOCH, &type, (void **)&tmpepoch, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *epoch = *tmpepoch;
        }
    }

    if (version) {
        gchar *tmpver;

        g_free (*version);
        *version = NULL;

        headerGetEntry (hdr, RPMTAG_VERSION, &type, (void **)&tmpver, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpver && tmpver[0]) {
            *version = g_strdup (tmpver);
        }
    }

    if (release) {
        gchar *tmprel;

        g_free (*release);
        *release = NULL;

        headerGetEntry (hdr, RPMTAG_RELEASE, &type, (void **)&tmprel, &count);

        if (count && (type == RPM_STRING_TYPE) && tmprel && tmprel[0]) {
            *release = g_strdup (tmprel);
        }
    }

    if (section) {
        gchar *tmpsection;

        *section = SECTION_MISC;

        headerGetEntry (hdr, RPMTAG_GROUP, &type, (void **)&tmpsection,
                        &count);

        if (count && (type == RPM_STRING_TYPE) && tmpsection &&
            tmpsection[0]) {
            if (!g_strcasecmp (tmpsection, "Amusements/Games")) {
                *section = SECTION_GAME;
            } else if (!g_strcasecmp (tmpsection, "Amusements/Graphics")) {
                *section = SECTION_IMAGING;
            } else if (!g_strcasecmp (tmpsection, "Applications/Archiving")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection,
                                      "Applications/Communications")) {
                *section = SECTION_INTERNET;
            } else if (!g_strcasecmp (tmpsection, "Applications/Databases")) {
                *section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (tmpsection, "Applications/Editors")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection, "Applications/Emulators")) {
                *section = SECTION_GAME;
            } else if (!g_strcasecmp (tmpsection,
                                      "Applications/Engineering")) {
                *section = SECTION_MISC;
            } else if (!g_strcasecmp (tmpsection, "Applications/File")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection, "Applications/Internet")) {
                *section = SECTION_INTERNET;
            } else if (!g_strcasecmp (tmpsection, "Applications/Multimedia")) {
                *section = SECTION_MULTIMEDIA;
            } else if (!g_strcasecmp (tmpsection,
                                      "Applications/Productivity")) {
                *section = SECTION_PIM;
            } else if (!g_strcasecmp (tmpsection, "Applications/Publishing")) {
                *section = SECTION_MISC;
            } else if (!g_strcasecmp (tmpsection, "Applications/System")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection, "Applications/Text")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection, "Development/Debuggers")) {
                *section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (tmpsection, "Development/Languages")) {
                *section = SECTION_DEVEL;
            } else if (!g_strcasecmp (tmpsection, "Development/Libraries")) {
                *section = SECTION_LIBRARY;
            } else if (!g_strcasecmp (tmpsection, "Development/System")) {
                *section = SECTION_DEVEL;
            } else if (!g_strcasecmp (tmpsection, "Development/Tools")) {
                *section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Base")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Daemons")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Kernel")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Libraries")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Shells")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "User Interface/Desktops")) {
                *section = SECTION_XAPP;
            } else if (!g_strcasecmp (tmpsection,
                                      "User Interface/X")) {
                *section = SECTION_XAPP;
            } else if (!g_strcasecmp (tmpsection,
                                      "User Interface/X Hardware Support")) {
                *section = SECTION_XAPP;
            } else {
                *section = SECTION_MISC;
            }
        } else {
            *section = SECTION_MISC;
        }
    }

    if (size) {
        guint32 *tmpsize;

        *size = 0;

        headerGetEntry (hdr, RPMTAG_SIZE, &type, (void **)&tmpsize, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *size = *tmpsize;
        }
    }

    if (summary) {
        gchar *tmpsummary;

        g_free (*summary);
        *summary = NULL;

        headerGetEntry (hdr, RPMTAG_SUMMARY, &type, (void **)&tmpsummary,
                        &count);

        if (count && (type == RPM_STRING_TYPE)) {
            *summary = g_strdup (tmpsummary);
        }
    }

    if (description) {
        gchar *tmpdescription;

        g_free (*description);
        *description = NULL;

        headerGetEntry (hdr, RPMTAG_DESCRIPTION, &type,
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
            break;
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
rc_rpmman_depends_fill (Header hdr, RCPackage *pkg)
{
    gchar **names, **verrels, **versions, **releases;
    guint32 *epochs;
    gint *relations;
    gint32 count;
    guint i;
    RCPackageDepItem *dep;
    RCPackageDep *depl = NULL;

    /* Shouldn't ever ask for dependencies unless you know what you really
       want (name, version, release) */

    g_assert (pkg->spec.name);
    g_assert (pkg->spec.version);
    g_assert (pkg->spec.release);

    /* Better safe than sorry, eh? */

    rc_package_dep_slist_free (pkg->requires);
    pkg->requires = NULL;
    rc_package_dep_slist_free (pkg->conflicts);
    pkg->conflicts = NULL;
    rc_package_dep_slist_free (pkg->provides);
    pkg->provides = NULL;

    /* Query the RPM database for the packages required by this package, and
       all associated information */

    headerGetEntry (hdr, RPMTAG_REQUIREFLAGS, NULL, (void **)&relations,
                    &count);
    headerGetEntry (hdr, RPMTAG_REQUIRENAME, NULL, (void **)&names, &count);
    headerGetEntry (hdr, RPMTAG_REQUIREVERSION, NULL, (void **)&verrels,
                    &count);

    /* Break this useless information into version and release fields.
       FIXME: this needs to do the epoch too */

    /* Some packages, such as setup and termcap, store a NULL in the
       RPMTAG_REQUIREVERSION, yet set the count to 1. That's pretty damned
       broken, so if that's the case, we'll set the count to zero, as we
       should. *sigh* */

    if (verrels == NULL)
	count = 0;

    parse_versions (verrels, &epochs, &versions, &releases, count);

    for (i = 0; i < count; i++) {
        RCPackageRelation relation = 0;

//        printf ("%s %d %s %s\n", verrels[i], epochs[i], versions[i], releases[i]);
                
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
        } else {
            /* Add the dependency to the list of dependencies */

            dep = rc_package_dep_item_new (names[i], epochs[i], versions[i],
                                           releases[i], relation);

            depl = g_slist_append (NULL, dep);

            pkg->requires = g_slist_append (pkg->requires, depl);
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

    dep = rc_package_dep_item_new (pkg->spec.name, pkg->spec.epoch,
                                   pkg->spec.version, pkg->spec.release,
                                   RC_RELATION_EQUAL);

    depl = g_slist_append (NULL, dep);

    pkg->provides = g_slist_append (pkg->provides, depl);

    /* RPM doesn't do versioned provides (as of 3.0.4), so we only need to find
       out the name of things that this package provides */

    headerGetEntry (hdr, RPMTAG_PROVIDENAME, NULL, (void **)&names, &count);

    for (i = 0; i < count; i++) {
        dep = rc_package_dep_item_new (names[i], 0, NULL, NULL,
                                       RC_RELATION_ANY);

        depl = g_slist_append (NULL, dep);

        pkg->provides = g_slist_append (pkg->provides, depl);
    }

    /* Only have to free the char** ones */

    free (names);
    names = NULL;

    /* Find full information on anything that this package conflicts with */

    headerGetEntry (hdr, RPMTAG_CONFLICTFLAGS, NULL, (void **)&relations,
                    &count);
    headerGetEntry (hdr, RPMTAG_CONFLICTNAME, NULL, (void **)&names, &count);
    headerGetEntry (hdr, RPMTAG_CONFLICTVERSION, NULL, (void **)&verrels,
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

        pkg->conflicts = g_slist_append (pkg->conflicts, depl);

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
rc_rpmman_check_match (Header hdr, RCPackage *pkg)
{
    gchar *version = NULL, *release = NULL;
    gchar *summary = NULL, *description = NULL;
    guint32 size = 0, epoch = 0;
    RCPackageSection section = SECTION_MISC;

    rc_rpmman_read_header (hdr, NULL, &epoch, &version, &release, &section,
                           &size, &summary, &description);

    /* FIXME: this could potentially be a big problem if an rpm can have
       just an epoch (serial), and no version/release.  I'm choosing to
       ignore that for now, because I need to get this stuff done and it
       doesn't seem very likely, but... */

    if ((!pkg->spec.version || !strcmp (pkg->spec.version, version)) &&
        (!pkg->spec.release || !strcmp (pkg->spec.release, release)) &&
        (!pkg->spec.epoch || pkg->spec.epoch == epoch)) {

        g_free (pkg->spec.version);
        g_free (pkg->spec.release);

        pkg->spec.epoch = epoch;
        pkg->spec.version = version;
        pkg->spec.release = release;
        pkg->summary = summary;
        pkg->description = description;
        pkg->spec.installed = TRUE;
        pkg->spec.installed_size = size;

        rc_rpmman_depends_fill (hdr, pkg);

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
rc_rpmman_query (RCPackman *p, RCPackage *pkg)
{
    rpmdb db;
    rpmdbMatchIterator mi = NULL;
    Header hdr;

    if (rpmdbOpen (RC_RPMMAN (p)->rpmroot, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open RPM database");
        return (pkg);
    }

    mi = rpmdbInitIterator (db, RPMDBI_LABEL, pkg->spec.name, 0);

    if (!mi) {
        pkg->spec.installed = FALSE;
        pkg->spec.installed_size = 0;
        return (pkg);
    }

    while ((hdr = rpmdbNextIterator (mi))) {
        if (rc_rpmman_check_match (hdr, pkg)) {
            rpmdbFreeIterator (mi);
            rpmdbClose (db);
            return (pkg);
        }
    }

    pkg->spec.installed = FALSE;
    pkg->spec.installed_size = 0;

    rpmdbFreeIterator (mi);
    rpmdbClose (db);

    return (pkg);
} /* rc_packman_query */

#else /* !HAVE_RPM_4_0 */

static RCPackage *
rc_rpmman_query (RCPackman *p, RCPackage *pkg)
{
    rpmdb db;
    dbiIndexSet matches;
    guint i;

    if (rpmdbOpen (RC_RPMMAN (p)->rpmroot, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open RPM database");
        return (pkg);
    }

    if (rpmdbFindPackage (db, pkg->spec.name, &matches)) {
        rpmdbClose (db);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to perform RPM database search");
        return (pkg);
    }

    for (i = 0; i < matches.count; i++) {
        Header hdr;

        if (!(hdr = rpmdbGetRecord (db, matches.recs[i].recOffset))) {
            rpmdbClose (db);
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to fetch RPM header from database");
            return (pkg);
        }

        if (rc_rpmman_check_match (hdr, pkg)) {
            headerFree (hdr);
            dbiFreeIndexRecord (matches);
            rpmdbClose (db);
            return (pkg);
        }
    }

    pkg->spec.installed = FALSE;
    pkg->spec.installed_size = 0;

    dbiFreeIndexRecord (matches);
    rpmdbClose (db);

    return (pkg);
} /* rc_packman_query */

#endif /* !HAVE_RPM_4_0 */

/* Query a file for rpm header information */

static RCPackage *
rc_rpmman_query_file (RCPackman *p, gchar *filename)
{
    FD_t fd;
    Header hdr;
    RCPackage *pkg = rc_package_new ();

    fd = fdOpen (filename, O_RDONLY, 0444);

    if (rpmReadPackageHeader (fd, &hdr, NULL, NULL, NULL)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to read package header");
        rc_package_free (pkg);
        return (NULL);
    }

    rc_rpmman_read_header (hdr, &pkg->spec.name, &pkg->spec.epoch,
                           &pkg->spec.version, &pkg->spec.release,
                           &pkg->spec.section,
                           &pkg->spec.installed_size, &pkg->summary,
                           &pkg->description);

    rc_rpmman_depends_fill (hdr, pkg);

    headerFree (hdr);
    fdClose (fd);

    return (pkg);
} /* rc_packman_query_file */

/* Query all of the packages on the system */

#ifdef HAVE_RPM_4_0

static RCPackageSList *
rc_rpmman_query_all (RCPackman *p)
{
    rpmdb db;
    RCPackageSList *list = NULL;
    rpmdbMatchIterator mi = NULL;
    Header hdr;

    if (rpmdbOpen (RC_RPMMAN (p)->rpmroot, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open RPM database");
        return (NULL);
    }

    mi = rpmdbInitIterator (db, RPMDBI_PACKAGES, NULL, 0);

    if (!mi) {
        rpmdbClose (db);
        return (NULL);
    }

    while ((hdr = rpmdbNextIterator (mi))) {
        RCPackage *pkg = rc_package_new ();

        rc_rpmman_read_header (hdr, &pkg->spec.name, &pkg->spec.epoch,
                               &pkg->spec.version, &pkg->spec.release,
                               &pkg->spec.section,
                               &pkg->spec.installed_size, &pkg->summary,
                               &pkg->description);
	pkg->spec.installed = TRUE;

        rc_rpmman_depends_fill (hdr, pkg);

        list = g_slist_append (list, pkg);
    }

    rpmdbClose (db);

    return (list);
} /* rc_rpmman_query_all */

#else /* !HAVE_RPM_4_0 */

static RCPackageSList *
rc_rpmman_query_all (RCPackman *p)
{
    rpmdb db;
    RCPackageSList *list = NULL;
    guint recno;

    if (rpmdbOpen (RC_RPMMAN (p)->rpmroot, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open RPM database");
        return (NULL);
    }

    if (!(recno = rpmdbFirstRecNum (db))) {
        rpmdbClose (db);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to access RPM database");
        return (NULL);
    }

    for (; recno; recno = rpmdbNextRecNum (db, recno)) {
        Header hdr;
        RCPackage *pkg = rc_package_new ();
        
        if (!(hdr = rpmdbGetRecord (db, recno))) {
            rpmdbClose (db);
            rc_package_slist_free (list);
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to read RPM database entry");
            return (NULL);
        }

        rc_rpmman_read_header (hdr, &pkg->spec.name, &pkg->spec.epoch,
                               &pkg->spec.version, &pkg->spec.release,
                               &pkg->spec.section,
                               &pkg->spec.installed_size, &pkg->summary,
                               &pkg->description);
	pkg->spec.installed = TRUE;

        rc_rpmman_depends_fill (hdr, pkg);

        list = g_slist_append (list, pkg);
#if 1
	/* So, this line seems to have issues. It frees data that the package
	   needs, and doing so ends up corrupting the linked list. Ick. */
        /* Ok, I think I've fixed this.  I need to test it, but can't, because
           I don't have a convenient RPM-based machine.  Suck. */
        headerFree(hdr);
#endif
    }

    rpmdbClose (db);

    return (list);
} /* rc_rpmman_query_all */

#endif /* !HAVE_RPM_4_0 */

static gint
rc_rpmman_version_compare (RCPackman *p, RCPackageSpec *s1, RCPackageSpec *s2)
{
    int stat;
    gchar *v1 = s1->version, *r1 = s1->release, *v2 = s2->version,
        *r2 = s2->release;

    g_return_val_if_fail(p, -1);

    /* Are we requiring that they provide strings for v1, r1, v2, and r2? If
       so, we can just remove the following code and put some
       g_return_val_if_fails instead. Otherwise... 
    */

    /* I don't think we should require that; this code looks correct.  Any
       non-NULL string should be interpreted as a "later" version than a NULL
       string. */

    if (v1 && !v2)
        return 1;
    else if (v2 && !v1)
        return -1;
    else if (!v1 && !v2)
        stat = 0;
    else
        stat = rpmvercmp(v1, v2);

    if (stat)
        return stat;
    
    if (r1 && !r2)
        return 1;
    else if (r2 && !r1)
        return -1;
    else if (!r1 && !r2)
        return 0;
    else
        return rpmvercmp(r1, r2);
}

static RCVerificationSList *
rc_rpmman_verify (RCPackman *p, gchar *filename)
{
    struct rpmlead lead;
    FD_t in_fd = NULL;
    Header sig_hdr = NULL;
    int data_fd = 0;
    int sig_fd = 0;
    RCVerificationSList *ret = NULL;
    RCVerification *rcv = NULL;
    char *sig_name = g_strdup ("/tmp/rpm-sig-XXXXXX");
    char *data_name = g_strdup ("/tmp/rpm-data-XXXXXX");
    gint32 count;
    guchar buffer[128];
    ssize_t num_bytes;
    gboolean gpg_sig = FALSE, md5_sig = FALSE, size_sig = FALSE;
    guint8 *md5 = NULL;
    guint32 size = 0;
    gchar *buf;

    in_fd = Fopen (filename, "r.ufdio");
    if (in_fd == NULL || Ferror (in_fd)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open the RPM for verification");
        goto END;
    }

    if (readLead (in_fd, &lead)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to read the RPM file for verification");
        goto END;
    }

    if (rpmReadSignature (in_fd, &sig_hdr, lead.signature_type) ||
        (sig_hdr == NULL))
    {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to read the RPM signature section");
        goto END;
    }

    count = 0;
    headerGetEntry (sig_hdr, RPMSIGTAG_GPG, NULL, (void **)&buf, &count);
    if (count > 0) {
        if ((sig_fd = mkstemp (sig_name)) == -1) {
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to create a temporary signature "
                                  "file");
            goto END;
        }
        if (!rc_write (sig_fd, buf, count)) {
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to write a temporary signature "
                                  "file");
            goto END;
        }
        close (sig_fd);
        gpg_sig = TRUE;
    }

    count = 0;
    headerGetEntry (sig_hdr, RPMSIGTAG_MD5, NULL, (void **)&buf, &count);
    if (count > 0) {
        md5_sig = TRUE;
        md5 = g_new (guint8, count);
        memcpy (md5, buf, count);
    }

    count = 0;
    headerGetEntry (sig_hdr, RPMSIGTAG_SIZE, NULL, (void **)&buf, &count);
    if (count > 0) {
        size_sig = TRUE;
        size = *((guint32 *)buf);
    }

    if (gpg_sig || md5_sig || size_sig) {
        if ((data_fd = mkstemp (data_name)) == -1) {
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to create temporary payload file");
            goto END;
        }

        while ((num_bytes = Fread (buffer, sizeof (char), sizeof (buffer),
                                   in_fd)) > 0) {
            if (!rc_write (data_fd, buffer, num_bytes)) {
                rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                      "Unable to write a temporary payload "
                                      "file");
                goto END;
            }
        }

        close (data_fd);
    }

    if (gpg_sig) {
        rcv = rc_verify_gpg (data_name, sig_name);

        ret = g_slist_append (ret, rcv);
    }

    if (md5_sig) {
        rcv = rc_verify_md5 (data_name, md5);

        ret = g_slist_append (ret, rcv);
    }

    if (size_sig) {
        rcv = rc_verify_size (data_name, size);

        ret = g_slist_append (ret, rcv);
    }

  END:
    if (gpg_sig) {
        unlink (sig_name);
    }
    if (gpg_sig || md5_sig || size_sig) {
        unlink (data_name);
    }
    if (in_fd) {
        Fclose (in_fd);
    }
    g_free (sig_name);
    g_free (data_name);
    g_free (md5);
    headerFree (sig_hdr);
    if (sig_fd) {
        close (sig_fd);
    }
    if (data_fd) {
        close (data_fd);
    }

    return (ret);
} /* rc_rpmman_verify */

static void
rc_rpmman_destroy (GtkObject *obj)
{
    RCRpmman *r = RC_RPMMAN (obj);

    g_free (r->rpmroot);

    if (GTK_OBJECT_CLASS (rc_rpmman_parent)->destroy)
        (* GTK_OBJECT_CLASS (rc_rpmman_parent)->destroy) (obj);
}

static void
rc_rpmman_class_init (RCRpmmanClass *klass)
{
    GtkObjectClass *object_class = (GtkObjectClass *) klass;
    RCPackmanClass *hpc = (RCPackmanClass *) klass;

    object_class->destroy = rc_rpmman_destroy;

    rc_rpmman_parent = gtk_type_class (rc_packman_get_type ());

    hpc->rc_packman_real_transact = rc_rpmman_transact;
    hpc->rc_packman_real_query = rc_rpmman_query;
    hpc->rc_packman_real_query_all = rc_rpmman_query_all;
    hpc->rc_packman_real_query_file = rc_rpmman_query_file;
    hpc->rc_packman_real_version_compare = rc_rpmman_version_compare;
    hpc->rc_packman_real_verify = rc_rpmman_verify;
} /* rc_rpmman_class_init */

static void
rc_rpmman_init (RCRpmman *obj)
{
    RCPackman *p = RC_PACKMAN (obj);
    /*
    rpmdb db;
    */

    gchar *tmp;

    rpmReadConfigFiles( NULL, NULL );

    tmp = getenv ("RC_RPM_ROOT");

    if (tmp) {
        obj->rpmroot = g_strdup (tmp);
    } else {
        obj->rpmroot = g_strdup ("/");
    }

    p->pre_config = TRUE;
    p->pkg_progress = TRUE;
    p->post_config = FALSE;
} /* rc_rpmman_init */

RCRpmman *
rc_rpmman_new (void)
{
    RCRpmman *new =
        RC_RPMMAN (gtk_type_new (rc_rpmman_get_type ()));

    return new;
} /* rc_rpmman_new */
