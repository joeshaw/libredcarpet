/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rpmman.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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
#include <libgen.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <sys/stat.h>

#include "rc-debug.h"
#include "rc-packman-private.h"
#include "rc-rpmman.h"
#include "rc-rpmman-types.h"
#include "rc-util.h"
#include "rc-verification-private.h"
#include "rc-arch.h"

#ifndef STATIC_RPM
#include "rpm-stubs.h"
#endif

#include "rpm-rpmlead.h"
#include "rpm-signature.h"

/*
 * We define this so people can't build or use libredcarpet against a
 * newer version than we support.  Unfortunately we have to do this
 * because we do some very evil poking of private vtables which can
 * and have changed cordering from version to version.  It could be
 * very dangerous to call the wrong function.
 */
#define LATEST_SUPPORTED_RPM_VERSION 40201

#define GTKFLUSH {while (g_main_pending ()) g_main_iteration (TRUE);}

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
#define RPMTAG_REMOVETID  1129

/* Repackaging was added in RPM 4.0.3, but broken until 4.0.4. */
#define RPMTRANS_FLAG_REPACKAGE (1 << 10)

/* rpmRC for RPM 4.0.3 and 4.0.4 */
#define RPMRC_OK        0
#define RPMRC_BADMAGIC  1
#define RPMRC_FAIL      2
#define RPMRC_BADSIZE   3
#define RPMRC_SHORTREAD 4

/* rpmRC for RPM 4.1 and 4.2, where different from above */
#define RPMRC_NOTFOUND   1
#define RPMRC_NOTTRUSTED 3
#define RPMRC_NOKEY      4

/*
 * Define a compatibility enum for rpmCallbackType, since it changed to
 * a bitfield in 4.1
 */
typedef enum {
    RC_RPMCALLBACK_UNKNOWN            = 0,
    RC_RPMCALLBACK_INST_PROGRESS      = (1 << 0),
    RC_RPMCALLBACK_INST_START         = (1 << 1),
    RC_RPMCALLBACK_INST_OPEN_FILE     = (1 << 2),
    RC_RPMCALLBACK_INST_CLOSE_FILE    = (1 << 3),
    RC_RPMCALLBACK_TRANS_PROGRESS     = (1 << 4),
    RC_RPMCALLBACK_TRANS_START        = (1 << 5),
    RC_RPMCALLBACK_TRANS_STOP         = (1 << 6),
    RC_RPMCALLBACK_UNINST_PROGRESS    = (1 << 7),
    RC_RPMCALLBACK_UNINST_START       = (1 << 8),
    RC_RPMCALLBACK_UNINST_STOP        = (1 << 9),
    RC_RPMCALLBACK_REPACKAGE_PROGRESS = (1 << 10),
    RC_RPMCALLBACK_REPACKAGE_START    = (1 << 11),
    RC_RPMCALLBACK_REPACKAGE_STOP     = (1 << 12),
    RC_RPMCALLBACK_UNPACK_ERROR       = (1 << 13),
    RC_RPMCALLBACK_CPIO_ERROR         = (1 << 14)
} RCrpmCallbackType;

static void rc_rpmman_class_init (RCRpmmanClass *klass);
static void rc_rpmman_init       (RCRpmman *obj);

static GObjectClass *parent_class;

GType
rc_rpmman_get_type (void)
{
    static GType type = 0;

    if (!type) {
        static GTypeInfo type_info = {
            sizeof (RCRpmmanClass),
            NULL, NULL,
            (GClassInitFunc) rc_rpmman_class_init,
            NULL, NULL,
            sizeof (RCRpmman),
            0,
            (GInstanceInitFunc) rc_rpmman_init,
        };

        type = g_type_register_static (rc_packman_get_type (),
                                       "RCRpmman",
                                       &type_info,
                                       0);
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

static gchar *
rc_rpmman_database_filename (RCRpmman *rpmman)
{
    const char *dbpath;
    const char *dbfile;
    int lasti;
    gchar *path;

    dbpath = rpmman->rpmGetPath ("%{_dbpath}", NULL);

    if (rpmman->version < 40000)
        dbfile = "packages.rpm";
    else
        dbfile = "Packages";

    lasti = strlen (rpmman->rpmroot) - 1;

    if (rpmman->rpmroot[lasti] == '/' && dbpath[0] == '/')
        dbpath++;

    path = g_strconcat (rpmman->rpmroot, dbpath, "/", dbfile, NULL);

    return path;
}
        
static gboolean
rc_rpmman_is_database_changed (RCPackman *packman)
{
    struct stat buf;
    RCRpmman *rpmman = RC_RPMMAN (packman);
    gchar *path;

    path = rc_rpmman_database_filename (rpmman);
    stat (path, &buf);
    g_free (path);

    if (buf.st_mtime != rpmman->db_mtime) {
        rpmman->db_mtime = buf.st_mtime;
        return TRUE;
    }

    return FALSE;
}

static gboolean
database_check_func (RCRpmman *rpmman)
{
    RCPackman *packman;

    if (rc_rpmman_is_database_changed (packman = RC_PACKMAN (rpmman)))
        g_signal_emit_by_name (packman, "database_changed");

    return TRUE;
}

static void
close_database (RCRpmman *rpmman)
{
#if 0
    if (getenv ("RC_RPM_NO_DB"))
        return;
#endif

    if (rpmman->db_status == RC_RPMMAN_DB_NONE)
        return;

    rc_rpmman_is_database_changed (RC_PACKMAN (rpmman));
    rpmman->db_watcher_cb =
        g_timeout_add (5000, (GSourceFunc) database_check_func,
                       (gpointer) rpmman);

    if (rpmman->db || rpmman->rpmts) {
        rpmman->db_status = RC_RPMMAN_DB_NONE;

        if (rpmman->lock_fd) {
            rc_close (rpmman->lock_fd);
            rpmman->lock_fd = 0;
        }
    }

    if (rpmman->db) {
        rpmman->rpmdbClose (rpmman->db);
        rpmman->db = NULL;
    }

    if (rpmman->rpmts) {
        rpmman->rpmtsFree (rpmman->rpmts);
        rpmman->rpmts = NULL;
    }
}

static gboolean
open_database (RCRpmman *rpmman, gboolean write)
{
    int flags;
    gboolean root;
    struct flock fl;
    int db_fd = -1;
    gchar *db_filename = NULL;

#if 0
    if (getenv ("RC_RPM_NO_DB"))
        return FALSE;
#endif

    if (rpmman->db || rpmman->rpmts)
        close_database (rpmman);

    root = !geteuid ();

    if (!root && write) {
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "write access requires root privileges");
        goto ERROR;
    }

    if (write) {
        flags = O_RDWR;
        rpmman->db_status = RC_RPMMAN_DB_RDWR;
    }
    else {
        flags = O_RDONLY;
        rpmman->db_status = RC_RPMMAN_DB_RDONLY;
    }

    db_filename = rc_rpmman_database_filename (rpmman);

    if (!(db_fd = open (db_filename, O_RDONLY))) {
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "unable to open %s", db_filename);
        goto ERROR;
    }

    fl.l_type = F_RDLCK;
    fl.l_whence = SEEK_SET;
    fl.l_start = 0;
    fl.l_len = 0;

    if (fcntl (db_fd, F_SETLK, &fl) == -1) {
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "unable to open shared lock on %s", db_filename);
        goto ERROR;
    }

    if (rpmman->version >= 40100) {
        rpmman->rpmts = rpmman->rpmtsCreate ();

        if (!rpmman->rpmts) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "rpmtsCreate failed");
            goto ERROR;
        }

        rpmman->rpmtsSetRootDir (rpmman->rpmts, rpmman->rpmroot);

        /*
         * We don't want to use RPM >= 4.1's built-in digest/signature
         * checking because repackaged RPMs have invalid signatures and
         * we do the checking ourselves.
         */
        rpmman->rpmtsSetVSFlags (rpmman->rpmts,
                                 rpmman->rpmtsVSFlags (rpmman->rpmts) |
                                 _RPMVSF_NODIGESTS | _RPMVSF_NOSIGNATURES);
    }
    else {
        if (rpmman->rpmdbOpen (rpmman->rpmroot, &rpmman->db, flags, 0644)) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "rpmdbOpen failed");
            goto ERROR;
        }
    }

    rc_close (db_fd);
    db_fd = -1;

    if (write) {
        rpmman->lock_fd = open (db_filename, O_RDWR);

        if (rpmman->lock_fd == -1) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "open(2) failed");
            goto ERROR;
        }

        fl.l_type = F_WRLCK;
        fl.l_whence = SEEK_SET;
        fl.l_start = 0;
        fl.l_len = 0;

        if (fcntl (rpmman->lock_fd, F_SETLK, &fl) == -1) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "fcntl failed");
            goto ERROR;
        }
    }

    g_free (db_filename);
    db_filename = NULL;

    if (root && rpmman->version >= 40003) {
        if (!(rpmman->rpmExpandNumeric ("%{?__dbi_cdb:1}"))) {
            int i;

            for (i = 0; i < 16; i++) {
                gchar *filename = g_strdup_printf (
                    "%s%s/__db.0%02d", rpmman->rpmroot,
                    rpmman->rpmGetPath ("%{_dbpath}", NULL), i);
                unlink (filename);
                g_free (filename);
            }
        }
    }

    if (rpmman->db_watcher_cb) {
        g_source_remove (rpmman->db_watcher_cb);
        rpmman->db_watcher_cb = 0;
    }

    return TRUE;

  ERROR:
    rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                          "unable to open RPM database");
    rpmman->db_status = RC_RPMMAN_DB_NONE;

    if (rpmman->db) {
        rpmman->rpmdbClose (rpmman->db);
        rpmman->db = NULL;
    }

    if (rpmman->rpmts) {
        rpmman->rpmtsFree (rpmman->rpmts);
        rpmman->rpmts = NULL;
    }

    if (db_filename)
        g_free (db_filename);
    if (db_fd != -1)
        rc_close (db_fd);

    return FALSE;
}

static gchar *
rc_package_to_rpm_name (RCPackage *package)
{
    gchar *ret = NULL;

    g_assert (package);
    g_assert (package->spec.nameq);

    ret = g_strdup (g_quark_to_string (package->spec.nameq));

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

typedef struct {
    /* For reading out of a package file */
    FD_t fd;

    /* For reading out the package database */
    rc_rpmdbMatchIterator mi; /* Only for RPM v4 */
    rc_dbiIndexSet matches;   /* Only for RPM v3 */

    GSList *headers;
} HeaderInfo;

static void
header_free_cb (gpointer data, gpointer user_data)
{
    Header h = data;
    RCRpmman *rpmman = RC_RPMMAN (user_data);

    rpmman->headerFree (h);
}

static void
rc_rpmman_header_info_free (RCRpmman *rpmman, HeaderInfo *hi)
{
    gboolean free_headers = TRUE;

    if (hi->fd)
        rc_rpm_close (rpmman, hi->fd);

    if (hi->mi) {
        /*
         * rpmdbFreeIterator() frees the headers for us, so we only want to
         * free them when it's not called.
         */
        rpmman->rpmdbFreeIterator (hi->mi);
        free_headers = FALSE;
    }

    if (hi->matches.count)
        rpmman->dbiFreeIndexRecord (hi->matches);

    if (free_headers)
        g_slist_foreach (hi->headers, header_free_cb, rpmman);

    g_slist_free (hi->headers);

    g_free (hi);
}

static HeaderInfo *
rc_rpmman_read_package_file (RCRpmman *rpmman, const char *filename)
{
    FD_t fd;
    Header header;
    gboolean is_source;
    HeaderInfo *hi;
    
    if (!rc_file_exists (filename)) {
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "file '%s' does not exist", filename);
        return NULL;
    }

    fd = rc_rpm_open (rpmman, filename, "r.fdio", O_RDONLY, 0444);

    if (fd == NULL) {
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "unable to open package '%s'", filename);
        return NULL;
    }

    if (rpmman->version >= 40100) {
        rpmts ts;
        int rc;
        char *tmp = NULL;
        guint32 count = 0;

        ts = rpmman->rpmtsCreate ();

        if (!ts) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "rpmtsCreate() failed");
            rc_rpm_close (rpmman, fd);
            return NULL;
        }

        rpmman->rpmtsSetRootDir (ts, rpmman->rpmroot);

        /*
         * We don't want to use RPM >= 4.1's built-in digest/signature
         * checking because repackaged RPMs have invalid signatures and
         * we do the checking ourselves.
         */
        rpmman->rpmtsSetVSFlags (ts, rpmman->rpmtsVSFlags (ts) |
                                 _RPMVSF_NODIGESTS | _RPMVSF_NOSIGNATURES);

        rc = rpmman->rpmReadPackageFile (ts, fd, NULL, &header);

        rpmman->rpmtsFree (ts);

        if (rc != 0) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "unable to read package header");
            rc_rpm_close (rpmman, fd);
            return NULL;
        }

        rpmman->headerGetEntry (header, RPMTAG_SOURCEPACKAGE, NULL,
                                (void **) &tmp, &count);

        if (count)
            is_source = TRUE;
        else
            is_source = FALSE;
    }
    else {
        if (rpmman->rpmReadPackageHeader (fd, &header,
                                          &is_source, NULL, NULL)) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "unable to read package header");

            rc_rpm_close (rpmman, fd);
            
            return NULL;
        }
    }

    if (is_source) {
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "source packages are not supported");

        rc_rpm_close (rpmman, fd);

        return NULL;
    }

    hi = g_new0 (HeaderInfo, 1);
    hi->fd = fd;
    hi->headers = g_slist_append (hi->headers, header);

    return hi;
}

static HeaderInfo *
rc_rpmman_find_system_headers_v4 (RCRpmman *rpmman, const char *name)
{
    rc_rpmdbMatchIterator mi = NULL;
    Header header;
    HeaderInfo *hi;

    if (rpmman->version >= 40100) {
        g_return_val_if_fail (rpmman->rpmts != NULL, NULL);
        mi = rpmman->rpmtsInitIterator (rpmman->rpmts, RPMDBI_LABEL, name, 0);
    } else {
        g_return_val_if_fail (rpmman->db != NULL, NULL);
        mi = rpmman->rpmdbInitIterator (rpmman->db, RPMDBI_LABEL, name, 0);
    }

    if (!mi)
        return NULL;

    hi = g_new0 (HeaderInfo, 1);
    hi->mi = mi;

    while ((header = rpmman->rpmdbNextIterator (mi)))
        hi->headers = g_slist_prepend (hi->headers, header);

    /*
     * We can't free the match iterator here because it'll free the
     * headers and we need those.  That's why we keep state around
     * and defer the freeing until after we're done with the headers
     */

    return hi;
}

static HeaderInfo *
rc_rpmman_find_system_headers_v3 (RCRpmman *rpmman, const char *name)
{
    rc_dbiIndexSet matches;
    guint i;
    HeaderInfo *hi;

    g_return_val_if_fail (rpmman->db != NULL, NULL);

    switch (rpmman->rpmdbFindByLabel (rpmman->db, name, &matches)) {
    case -1:
        rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database search");

        return NULL;

    case 1:
        return NULL;

    default:
        break;
    }

    hi = g_new0 (HeaderInfo, 1);
    hi->matches = matches;

    for (i = 0; i < matches.count; i++) {
        Header header;

        if (!(header = rpmman->rpmdbGetRecord (rpmman->db,
                                               matches.recs[i].recOffset))) {
            rc_packman_set_error (RC_PACKMAN (rpmman), RC_PACKMAN_ERROR_ABORT,
                                  "unable to fetch RPM header from database");

            return NULL;
        }

        hi->headers = g_slist_prepend (hi->headers, header);
    }

    return hi;
}

/*
 * Make sure you've called open_database() before calling this or you'll
 * get back a NULL list of headers inexplicably.
 */
static HeaderInfo *
rc_rpmman_find_system_headers (RCRpmman *rpmman, const char *name)
{
    if (rpmman->major_version == 4)
        return rc_rpmman_find_system_headers_v4 (rpmman, name);
    else
        return rc_rpmman_find_system_headers_v3 (rpmman, name);
}


typedef struct _InstallState InstallState;

struct _InstallState {
    RCPackman *packman;
    guint seqno;
    guint install_total;
    guint install_extra;
    guint remove_total;
    guint remove_extra;
    guint true_total;
    gboolean installing;
};

static RCrpmCallbackType
convert_callback_type (RCRpmman *rpmman, int what)
{
    if (rpmman->version >= 40100)
        return (RCrpmCallbackType) what;
    else {
        /*
         * Sigh.  Symbolically these are worthless, because we could have
         * been built against RPM 4.1 -- which means the symbols would have
         * different values -- so we need to refer to these by integer value.
         */
        switch (what) {
        case 0:
            return RC_RPMCALLBACK_INST_PROGRESS;
        case 1:
            return RC_RPMCALLBACK_INST_START;
        case 2:
            return RC_RPMCALLBACK_INST_OPEN_FILE;
        case 3:
            return RC_RPMCALLBACK_INST_CLOSE_FILE;
        case 4:
            return RC_RPMCALLBACK_TRANS_PROGRESS;
        case 5:
            return RC_RPMCALLBACK_TRANS_START;
        case 6:
            return RC_RPMCALLBACK_TRANS_STOP;
        case 7:
            return RC_RPMCALLBACK_UNINST_PROGRESS;
        case 8:
            return RC_RPMCALLBACK_UNINST_START;
        case 9:
            return RC_RPMCALLBACK_UNINST_STOP;
        default:
            return RC_RPMCALLBACK_UNKNOWN;
        }
    }
}

static void *
transact_cb (const Header h, const int rpm_what,
             const unsigned long amount,
             const unsigned long total,
             const void * pkgKey, void * data)
{
    const char * filename = pkgKey;
    static FD_t fd;
    InstallState *state = (InstallState *) data;
    char *filename_copy;
    const char *base;
    RCrpmCallbackType what;

    what = convert_callback_type (RC_RPMMAN (state->packman), rpm_what);

    switch (what) {
    case RC_RPMCALLBACK_INST_OPEN_FILE:
        fd = rc_rpm_open (RC_RPMMAN (state->packman), filename, "r.fdio",
                          O_RDONLY, 0);
        return fd;

    case RC_RPMCALLBACK_INST_CLOSE_FILE:
        rc_rpm_close (RC_RPMMAN (state->packman), fd);
        break;

    case RC_RPMCALLBACK_INST_PROGRESS:
        g_signal_emit_by_name (state->packman, "transact_progress",
                               amount, total);
        GTKFLUSH;
        break;

    case RC_RPMCALLBACK_TRANS_PROGRESS:
        if (state->seqno < state->true_total) {
            RCPackmanStep step;
            if (!state->installing || (state->seqno >= state->install_total)) {
                step = RC_PACKMAN_STEP_REMOVE;
            } else {
                step = RC_PACKMAN_STEP_PREPARE;
            }
            g_signal_emit_by_name (state->packman, "transact_step",
                                   ++state->seqno, step, NULL);
            GTKFLUSH;
        }
        break;

    case RC_RPMCALLBACK_INST_START:
        if (state->seqno < state->true_total) {
            filename_copy = g_strdup (pkgKey);
            base = basename (filename_copy);
            g_signal_emit_by_name (state->packman, "transact_step",
                                   ++state->seqno, RC_PACKMAN_STEP_INSTALL, base);
            g_free (filename_copy);
        }
        break;

    case RC_RPMCALLBACK_UNKNOWN:
    case RC_RPMCALLBACK_TRANS_START:
    case RC_RPMCALLBACK_TRANS_STOP:
    case RC_RPMCALLBACK_UNINST_PROGRESS:
    case RC_RPMCALLBACK_UNINST_START:
    case RC_RPMCALLBACK_UNINST_STOP:
    case RC_RPMCALLBACK_REPACKAGE_PROGRESS:
    case RC_RPMCALLBACK_REPACKAGE_START:
    case RC_RPMCALLBACK_REPACKAGE_STOP:
    case RC_RPMCALLBACK_UNPACK_ERROR:
    case RC_RPMCALLBACK_CPIO_ERROR:
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
    RCRpmman *rpmman = RC_RPMMAN (packman);
    RCPackageSList *iter;
    int count = 0;

    for (iter = install_packages; iter; iter = iter->next) {
        RCPackage *package = iter->data;
        char *filename = package->package_filename;
        HeaderInfo *hi;
        Header header;
        int rc;

        hi = rc_rpmman_read_package_file (rpmman, filename);

        if (!hi) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to add %s for installation",
                                  filename);
            return 0;
        }

        header = hi->headers->data;

        if (rpmman->version >= 40100) {
            rc = rpmman->rpmtsAddInstallElement (rpmman->rpmts,
                                                 header,
                                                 filename,
                                                 !package->install_only,
                                                 NULL);
        }
        else {
            rc = rpmman->rpmtransAddPackage (transaction,
                                             header,
                                             NULL,
                                             filename,
                                             package->install_only ? 0 : INSTALL_UPGRADE,
                                             NULL);
        }

        count++;
        rc_rpmman_header_info_free (rpmman, hi);

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

    return (count);
} /* transaction_add_install_pkgs */

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
        HeaderInfo *hi;
        GSList *hiter;

        hi = rc_rpmman_find_system_headers (rpmman, package_name);

        if (!hi || !hi->headers) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "package %s does not appear to be installed",
                                  package_name);
            if (hi)
                rc_rpmman_header_info_free (rpmman, hi);
            g_free (package_name);

            return 0;
        }

        for (hiter = hi->headers; hiter; hiter = hiter->next) {
            Header header = hiter->data;
            unsigned int offset = rpmman->rpmdbGetIteratorOffset (hi->mi);

            if (rpmman->version >= 40100)
                rpmman->rpmtsAddEraseElement (rpmman->rpmts, header, offset);
            else
                rpmman->rpmtransRemovePackage (transaction, offset);

            count++;
        }

        if (hi)
            rc_rpmman_header_info_free (rpmman, hi);

        g_free (package_name);
    }

    return (count);
}

static gboolean
transaction_add_remove_packages_v3 (RCPackman *packman,
                                    rpmTransactionSet transaction,
                                    RCPackageSList *remove_packages)
{
    RCPackageSList *iter;
    int i;
    guint count = 0;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    for (iter = remove_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        gchar *package_name = rc_package_to_rpm_name (package);
        HeaderInfo *hi;

        hi = rc_rpmman_find_system_headers (rpmman, package_name);

        if (!hi || !hi->headers) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "package %s does not appear to be installed",
                                  package_name);
            if (hi)
                rc_rpmman_header_info_free (rpmman, hi);
            g_free (package_name);

            return 0;
        }

        for (i = 0; i < rpmman->dbiIndexSetCount (hi->matches); i++) {
            unsigned int offset = rpmman->dbiIndexRecordOffset (hi->matches, i);

            if (offset) {
                rpmman->rpmtransRemovePackage (transaction, offset);
                count++;
            } else {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "unable to locate %s in database",
                                      package_name);

                rc_rpmman_header_info_free (rpmman, hi);
                g_free (package_name);

                return 0;
            }
        }

        rc_rpmman_header_info_free (rpmman, hi);
        g_free (package_name);
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
render_problems (RCPackman *packman, gpointer prob_ptr)
{
    guint count;
#if RPM_VERSION >= 40100
    rpmps probs = prob_ptr;
#else
    rpmProblemSet probs = prob_ptr;
#endif
    GString *report = g_string_new ("");
    RCRpmman *rpmman = RC_RPMMAN (packman);

    for (count = 0; count < probs->numProblems; count++) {
        const char *str;

        if (rpmman->version >= 40100) {
            RCrpmProblem problem = ((RCrpmProblem) probs->probs) + count;

            str = rpmman->rpmProblemString (problem);
        }
        else if (rpmman->version >= 40002) {
            RCrpmProblemOld problem = ((RCrpmProblemOld) probs->probs) + count;

            str = rpmman->rpmProblemStringOld (problem);
        }
        else {
            RCrpmProblemOlder *problem = ((RCrpmProblemOlder *) probs->probs) + count;
            
            str = rpmman->rpmProblemStringOlder (*problem);
        }

        g_string_append_printf (report, "\n%s", str);
    }

    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT, report->str);

    g_string_free (report, TRUE);
}

static void
rc_rpmman_transact (RCPackman *packman, RCPackageSList *install_packages,
                    RCPackageSList *remove_packages, int flags)
{
    rpmTransactionSet transaction = NULL;
    rpmps ps = NULL;
    rpmProblemSet probs = NULL;
    int rc;
    InstallState state;
    RCPackageSList *iter;
    int transaction_flags = 0, problem_filter;
    RCRpmman *rpmman = RC_RPMMAN (packman);
    RCPackageSList *real_remove_packages = NULL;
    RCPackageSList *obsoleted = NULL;
    gboolean close_db = FALSE;

    if (rpmman->db_status != RC_RPMMAN_DB_RDWR) {
        if (!open_database (rpmman, TRUE))
            /* open_database sets intelligent error messages */
            goto ERROR;
        close_db = TRUE;
    }

    if (getenv ("RC_JUST_KIDDING") || flags & RC_TRANSACT_FLAG_NO_ACT)
        transaction_flags |= RPMTRANS_FLAG_TEST;

    /*
     * Repackaging was added in RPM 4.0.3, but broken until 4.0.4.
     *
     * FIXME: Should we warn or abort or do something if we don't support
     * repackaging?
     */
    if (rpmman->version >= 40004) {
        if (flags & RC_TRANSACT_FLAG_REPACKAGE) {
            const char *repackage_dir;
            char *macro;

            repackage_dir = rc_packman_get_repackage_dir (packman);
            if (!repackage_dir)
                repackage_dir = "/tmp";

            macro = g_strconcat ("_repackage_dir ", repackage_dir, NULL);
            rpmman->rpmDefineMacro (NULL, macro, RMIL_GLOBAL);
            g_free (macro);

            transaction_flags |= RPMTRANS_FLAG_REPACKAGE;
        }
    }

    problem_filter =
        /* This isn't really a problem, and we'll trust RC to do the
           right thing here */
        RPMPROB_FILTER_OLDPACKAGE |
        RPMPROB_FILTER_REPLACEPKG;

    state.packman = packman;
    state.seqno = 0;
    state.install_total = 0;
    state.install_extra = 0;
    state.remove_total = 0;
    state.remove_extra = 0;
    state.installing = FALSE;

    if (rpmman->version < 40100)
        transaction = rpmman->rpmtransCreateSet (rpmman->db, rpmman->rpmroot);

    if (install_packages &&
        !(state.install_total = transaction_add_install_packages (
              packman, transaction, install_packages)))
    {
        rc_packman_set_error (
            packman, RC_PACKMAN_ERROR_ABORT,
            "error processing to-be-installed packages for test transaction");

        if (transaction)
            rpmman->rpmtransFree (transaction);

        goto ERROR;
    }

    /* If any of the remove packages are obsoleted by any of the
     * install packages, we want to filter them out */
    real_remove_packages = g_slist_copy (remove_packages);

    if (real_remove_packages && install_packages) {
        GSList *install_iter;

        for (install_iter = install_packages; install_iter;
             install_iter = install_iter->next)
        {
            RCPackage *install_package = (RCPackage *)(install_iter->data);
            int i;

            if (install_package->obsoletes_a) {
                for (i = 0; i < install_package->obsoletes_a->len; i++) {
                    RCPackageDep *obsolete = install_package->obsoletes_a->data[i];
                    GSList *remove_iter;

                    for (remove_iter = real_remove_packages; remove_iter;
                         remove_iter = remove_iter->next)
                    {
                        RCPackage *remove_package =
                            (RCPackage *)(remove_iter->data);
                        RCPackageDep *prov;

                        prov = rc_package_dep_new (
                            g_quark_to_string (remove_package->spec.nameq),
                            remove_package->spec.has_epoch,
                            remove_package->spec.epoch,
                            remove_package->spec.version,
                            remove_package->spec.release,
                            RC_RELATION_EQUAL, RC_CHANNEL_ANY,
                            FALSE, FALSE);

                        if (rc_package_dep_verify_relation (
                                packman, obsolete, prov)) {
                            obsoleted =
                                g_slist_prepend (obsoleted, remove_package);
                        }

                        rc_package_dep_unref (prov);
                    }
                }
            }
        }
    }

    for (iter = obsoleted; iter; iter = iter->next) {
        RCPackage *obsolete = (RCPackage *)(iter->data);

        real_remove_packages = g_slist_remove (real_remove_packages, obsolete);
	state.remove_extra++;
    }

    g_slist_free (obsoleted);

    if (real_remove_packages &&
        !(state.remove_total = transaction_add_remove_packages (
              packman, transaction, real_remove_packages)))
    {
        rc_packman_set_error (
            packman, RC_PACKMAN_ERROR_ABORT,
            "error processing to-be-removed packages for test transaction");

        if (transaction)
            rpmman->rpmtransFree (transaction);

        goto ERROR;
    }

    /* Let's check for packages which are upgrades rather than new
     * installs -- we're going to get two transaction steps for these,
     * not just one */
    state.install_extra = 0;

    for (iter = install_packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
        RCPackage *file_package, *inst_package;
        RCPackageSList *packages;

        /* FIXME: what is the logic here now?  Ugh ugh ugh */

        file_package =
            rc_packman_query_file (packman, package->package_filename);

        packages = rc_packman_query (
            packman, g_quark_to_string (file_package->spec.nameq));
        if (packages) {
            inst_package = packages->data;

            if (inst_package->installed &&
                rc_packman_version_compare (
                    packman,
                    RC_PACKAGE_SPEC (file_package),
                    RC_PACKAGE_SPEC (inst_package)))
            {
                state.install_extra++;
            }

            rc_package_slist_unref (packages);
        }

        rc_package_unref (file_package);
    }

    /* trust me */
    state.true_total = state.install_total * 2 + state.install_extra +
        state.remove_total + state.remove_extra;

    /* If there are no packages to be removed, we go right to
     * installing */

    if (!real_remove_packages)
        state.installing = TRUE;

    if (rpmman->version >= 40100) {
        if (rpmman->rpmtsCheck (rpmman->rpmts)) {
            ps = rpmman->rpmtsProblems (rpmman->rpmts);
            render_problems (packman, ps);
            rpmman->rpmpsFree (ps);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "transaction has unmet dependencies");


            goto ERROR;
        }

        if (rpmman->rpmtsOrder (rpmman->rpmts)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "circular dependencies were detected "
                                  "in the selected packages");


            goto ERROR;
        }
    }
    else {
#if RPM_VERSION >= 40003
        rpmDependencyConflict conflicts;
#else
        struct rpmDependencyConflict *conflicts;
#endif

        if (rpmman->rpmdepCheck (transaction, &conflicts, &rc) || rc) {
#if RPM_VERSION >= 40003
            rpmDependencyConflict conflict = conflicts;
#else
            struct rpmDependencyConflict *conflict = conflicts;
#endif
            guint count;
            GString *dep_info = g_string_new ("");

            for (count = 0; count < rc; count++) {
                g_string_append_printf (dep_info, "\n%s", conflict->byName);
                if (conflict->byVersion && conflict->byVersion[0]) {
                    g_string_append_printf (dep_info, "-%s",
                                            conflict->byVersion);
                    if (conflict->byRelease && conflict->byRelease[0]) {
                        g_string_append_printf (dep_info, "-%s",
                                                conflict->byRelease);
                    }
                }

                if (conflict->sense == RPMDEP_SENSE_CONFLICTS) {
                    g_string_append_printf (dep_info, " conflicts with ");
                } else { /* RPMDEP_SENSE_REQUIRES */
                    if (conflict->needsFlags & RPMSENSE_PREREQ) {
                        g_string_append_printf (dep_info, " pre-requires ");
                    } else {
                        g_string_append_printf (dep_info, " requires ");
                    }
                }

                g_string_append_printf (dep_info, "%s", conflict->needsName);

                if (conflict->needsVersion && conflict->needsVersion[0]) {
                    g_string_append_printf (dep_info, " ");
                    
                    if (conflict->needsFlags & RPMSENSE_LESS) {
                        g_string_append_printf (dep_info, "<");
                    }
                    
                    if (conflict->needsFlags & RPMSENSE_GREATER) {
                        g_string_append_printf (dep_info, ">");
                    }
                    
                    if (conflict->needsFlags & RPMSENSE_EQUAL) {
                        g_string_append_printf (dep_info, "=");
                    }
                    
                    g_string_append_printf (dep_info, " %s",
                                            conflict->needsVersion);
                }
                
                conflict++;
            }

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  dep_info->str);

            g_string_free (dep_info, TRUE);

            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "transaction has unmet dependencies");
            
            rpmman->rpmdepFreeConflicts (conflicts, rc);
            
            rpmman->rpmtransFree (transaction);
            
            goto ERROR;
        }
        
        if (rpmman->rpmdepOrder (transaction)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "circular dependencies were detected "
                                  "in the selected packages");

            rpmman->rpmtransFree (transaction);

            goto ERROR;
        }
    }

    g_signal_emit_by_name (packman, "transact_start", state.true_total);
    GTKFLUSH;

    if (rpmman->version >= 40100) {
        rpmman->rpmtsSetNotifyCallback (rpmman->rpmts,
                                        (rpmCallbackFunction) transact_cb,
                                        &state);
        rpmman->rpmtsSetFlags (rpmman->rpmts, transaction_flags);
        rpmman->rpmtsClean (rpmman->rpmts);
        rc = rpmman->rpmtsRun (rpmman->rpmts, NULL, problem_filter);

        switch (rc) {
        case 0:
            /* Success */
            break;
        case -1:
            /* Error, but no idea why. */
            goto ERROR;
        default:
            /* New problem set */
            ps = rpmman->rpmtsProblems (rpmman->rpmts);
            render_problems (packman, ps);
            rpmman->rpmpsFree (ps);
            goto ERROR;
        }
    }
    else {
        rc = rpmman->rpmRunTransactions (transaction,
                                         (rpmCallbackFunction) transact_cb,
                                         (void *) &state, NULL, &probs,
                                         transaction_flags, problem_filter);

        switch (rc) {
        case 0:
            /* Success */
            break;
        case -1:
            /* Error, but no idea why. */
            goto ERROR;
        default:
            /* Error, with probs */
            render_problems (packman, probs);
            rpmman->rpmProblemSetFree (probs);
            goto ERROR;
        }

        rpmman->rpmProblemSetFree (probs);
    }
        
    if (transaction)
        rpmman->rpmtransFree (transaction);

    while (state.seqno < state.true_total) {
        g_signal_emit_by_name (packman, "transact_step",
                               ++state.seqno, RC_PACKMAN_STEP_UNKNOWN, NULL);
        GTKFLUSH;
    }

    g_signal_emit_by_name (packman, "transact_done");
    GTKFLUSH;

    g_slist_free (real_remove_packages);

    if (close_db)
        close_database (rpmman);

    return;

  ERROR:
    rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                         "Unable to complete RPM transaction");

    if (close_db)
        close_database (rpmman);

    g_slist_free (real_remove_packages);
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

void
rc_rpmman_read_header (RCRpmman *rpmman, Header header, RCPackage *package)
{
    int_32 type, count;
    char *tmpc;
    guint32 *tmpi;

    rpmman->headerGetEntry (header, RPMTAG_NAME, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0])
        package->spec.nameq = g_quark_from_string (tmpc);
    else
        package->spec.nameq = 0;

    rpmman->headerGetEntry (header, RPMTAG_EPOCH, &type, (void **)&tmpi,
                            &count);
    if (count && (type == RPM_INT32_TYPE)) {
        package->spec.epoch = *tmpi;
        package->spec.has_epoch = 1;
    } else
        package->spec.has_epoch = 0;

    g_free (package->spec.version);
    rpmman->headerGetEntry (header, RPMTAG_VERSION, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0])
        package->spec.version = g_strdup (tmpc);
    else
        package->spec.version = NULL;

    g_free (package->spec.release);
    rpmman->headerGetEntry (header, RPMTAG_RELEASE, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0])
        package->spec.release = g_strdup (tmpc);
    else
        package->spec.release = NULL;

    rpmman->headerGetEntry (header, RPMTAG_ARCH, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0])
        package->arch = rc_arch_from_string (tmpc);
    else
        package->arch = RC_ARCH_UNKNOWN;

    rpmman->headerGetEntry (header, RPMTAG_GROUP, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0])
        package->section = rc_rpmman_section_to_package_section (tmpc);
    else
        package->section = RC_SECTION_MISC;

    rpmman->headerGetEntry (header, RPMTAG_SIZE, &type, (void **)&tmpi,
                            &count);
    if (count && (type == RPM_INT32_TYPE))
        package->installed_size = *tmpi;
    else
        package->installed_size = 0;

    g_free (package->summary);
    rpmman->headerGetEntry (header, RPMTAG_SUMMARY, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0]) {
        /* charset nonsense */
        if (g_utf8_validate (tmpc, -1, NULL))
            package->summary = g_strdup (tmpc);
        else {
            package->summary = g_convert_with_fallback (tmpc, -1,
                                                        "UTF-8",
                                                        "ISO-8859-1",
                                                        "?", NULL, NULL, NULL);
        }
    }
    else
        package->summary = NULL;

    g_free (package->description);
    rpmman->headerGetEntry (header, RPMTAG_DESCRIPTION, &type, (void **)&tmpc,
                            &count);
    if (count && (type == RPM_STRING_TYPE) && tmpc && tmpc[0]) {
        /* charset nonsense */
        if (g_utf8_validate (tmpc, -1, NULL))
            package->description = g_strdup (tmpc);
        else {
            package->description = g_convert_with_fallback (tmpc, -1,
                                                            "UTF-8",
                                                            "ISO-8859-1",
                                                            "?", NULL, NULL,
                                                            NULL);
        }
    }
    else
        package->description = NULL;
}

/* Takes an array of strings, which may be of the form <version>, or
   <epoch>:<version>, or <version>-<release>, or <epoch>:<version>-<release>,
   and does the right thing, breaking them into the right parts and returning
   the parts in the arrays epochs, versions, and releases. */

static gboolean
parse_version (const char *input, gboolean *has_epoch, guint32 *epoch,
               char **version, char **release)
{
    const char *vptr = NULL, *rptr = NULL;

    g_return_val_if_fail (input, FALSE);
    g_return_val_if_fail (input[0], FALSE);

    *has_epoch = 0;

    if ((vptr = strchr (input, ':'))) {
        /* We -might- have an epoch here */
        char *endptr;

        *epoch = strtoul (input, &endptr, 10);

        if (endptr != vptr) {
            /* No epoch here, just a : in the version string */
            *epoch = 0;
            vptr = input;
        } else {
            vptr++;
            *has_epoch = 1;
        }
    } else {
        vptr = input;
    }

    if ((rptr = strchr (vptr, '-'))) {
        *version = g_strndup (vptr, rptr - vptr);
        *release = g_strdup (rptr + 1);
    } else {
        *version = g_strdup (vptr);
    }

    return (TRUE);
}

static void
parse_versions (char **inputs, gboolean **has_epochs, guint32 **epochs,
                char ***versions, char ***releases, guint count)
{
    guint i;

    *versions = g_new0 (char *, count + 1);
    *releases = g_new0 (char *, count + 1);
    *epochs = g_new0 (guint32, count);
    *has_epochs = g_new0 (gboolean, count);

    for (i = 0; i < count; i++) {
        if (inputs[i] && inputs[i][0]) {
            parse_version (inputs[i], &((*has_epochs)[i]), &((*epochs)[i]),
                           &((*versions)[i]), &((*releases)[i]));
        }
    }
}

static gboolean
rc_rpmman_parse_version (RCPackman    *packman,
                         const gchar  *input,
                         gboolean     *has_epoch,
                         guint32      *epoch,
                         gchar       **version,
                         gchar       **release)
{
    return parse_version (input, has_epoch, epoch, version, release);
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
free_n (char **strv, int len)
{
    int i;

    if (!strv)
        return;

    for (i = 0; i < len; i++)
        g_free (strv[i]);

    g_free (strv);
}

static void
depends_fill_helper (RCRpmman *rpmman, Header header, int names_tag,
                     int versions_tag, int flags_tag, RCPackageDepSList **deps)
{
    char **names = NULL, **verrels = NULL, **versions, **releases;
    gboolean *has_epochs;
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

    parse_versions (verrels, &has_epochs, &epochs, &versions, &releases,
                    versions_count);

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
            dep = rc_package_dep_new (names[i], has_epochs[i], epochs[i],
                                      versions[i], releases[i], relation,
                                      RC_CHANNEL_ANY, FALSE, FALSE);
        } else {
            dep = rc_package_dep_new (names[i], 0, 0, NULL, NULL, relation,
                                      RC_CHANNEL_ANY, FALSE, FALSE);
        }

        *deps = g_slist_prepend (*deps, dep);
    }

    free (names);
    free (verrels);

    free_n (versions, versions_count);
    free_n (releases, versions_count);
    g_free (epochs);
    g_free (has_epochs);
}

void
rc_rpmman_depends_fill (RCRpmman *rpmman, Header header, RCPackage *package)
{
    RCPackageDep *dep;
    RCPackageDepSList *requires = NULL, *provides = NULL,
        *conflicts = NULL, *obsoletes = NULL;

    /* Shouldn't ever ask for dependencies unless you know what you really
       want (name, version, release) */
    g_assert (package->spec.nameq);
    g_assert (package->spec.version);
    g_assert (package->spec.release);

    depends_fill_helper (rpmman, header, RPMTAG_REQUIRENAME,
                         RPMTAG_REQUIREVERSION, RPMTAG_REQUIREFLAGS,
                         &requires);

    depends_fill_helper (rpmman, header, RPMTAG_PROVIDENAME,
                         RPMTAG_PROVIDEVERSION, RPMTAG_PROVIDEFLAGS,
                         &provides);

    depends_fill_helper (rpmman, header, RPMTAG_CONFLICTNAME,
                         RPMTAG_CONFLICTVERSION, RPMTAG_CONFLICTFLAGS,
                         &conflicts);

    depends_fill_helper (rpmman, header, RPMTAG_OBSOLETENAME,
                         RPMTAG_OBSOLETEVERSION, RPMTAG_OBSOLETEFLAGS,
                         &obsoletes);

    /* RPM versions prior to 4.0 don't make each package provide
     * itself automatically, so we have to add the dependency
     * ourselves */
    if (rpmman->version < 40000) {
        dep = rc_package_dep_new (
            g_quark_to_string (package->spec.nameq),
            package->spec.has_epoch, package->spec.epoch,
            package->spec.version, package->spec.release, RC_RELATION_EQUAL,
            package->channel, FALSE, FALSE);
        provides = g_slist_prepend (provides, dep);
    }

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
            "/usr/share/magic.mime",
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
        gboolean dont_filter;
        
        dont_filter = getenv ("RC_PLEASE_DONT_FILTER_FILE_DEPS") != NULL;

        rpmman->headerGetEntry (header, RPMTAG_BASENAMES, NULL,
                                (void **)&basenames, &count);
        rpmman->headerGetEntry (header, RPMTAG_DIRNAMES, NULL,
                                (void **)&dirnames, NULL);
        rpmman->headerGetEntry (header, RPMTAG_DIRINDEXES, NULL,
                                (void **)&dirindexes, NULL);

        for (i = 0; i < count; i++) {
            gchar *tmp = g_strconcat (dirnames[dirindexes[i]], basenames[i],
                                      NULL);

            if (dont_filter || in_set (tmp, file_dep_set)) {
                dep = rc_package_dep_new (tmp, 0, 0, NULL, NULL,
                                          RC_RELATION_ANY, RC_CHANNEL_ANY,
                                          FALSE, FALSE);

                provides = g_slist_prepend (provides, dep);
            }

            g_free (tmp);
        }

        free (basenames);
        free (dirnames);

        rpmman->headerGetEntry (header, RPMTAG_FILENAMES, NULL,
                                (void **)&basenames, &count);

        for (i = 0; i < count; i++) {
            if (in_set (basenames[i], file_dep_set)) {
                dep = rc_package_dep_new (basenames[i], 0, 0, NULL, NULL,
                                          RC_RELATION_ANY, RC_CHANNEL_ANY,
                                          FALSE, FALSE);

                provides = g_slist_prepend (provides, dep);
            }
        }

        free (basenames);
    }

    package->requires_a =
        rc_package_dep_array_from_slist (&requires);
    package->provides_a =
        rc_package_dep_array_from_slist (&provides);
    package->conflicts_a =
        rc_package_dep_array_from_slist (&conflicts);
    package->obsoletes_a =
        rc_package_dep_array_from_slist (&obsoletes);
} /* rc_rpmman_depends_fill */

static RCPackageSList *
rc_rpmman_query (RCPackman *packman, const char *name)
{
    RCPackageSList *ret = NULL;
    gboolean close_db = FALSE;
    HeaderInfo *hi;
    GSList *iter;

    if ((RC_RPMMAN (packman))->db_status < RC_RPMMAN_DB_RDONLY) {
        if (!open_database (RC_RPMMAN (packman), FALSE)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to query packages");
            return NULL;
        }
        close_db = TRUE;
    }

    hi = rc_rpmman_find_system_headers (RC_RPMMAN (packman), name);
    if (hi) {
        for (iter = hi->headers; iter; iter = iter->next) {
            Header header = iter->data;
            RCPackage *package;
            
            package = rc_package_new ();
            rc_rpmman_read_header (RC_RPMMAN (packman), header, package);
            package->installed = TRUE;

            ret = g_slist_prepend (ret, package);
        }

        rc_rpmman_header_info_free (RC_RPMMAN (packman), hi);
    }

    if (close_db)
        close_database (RC_RPMMAN (packman));

    return ret;
}

/* Query a file for rpm header information */

static RCPackage *
rc_rpmman_query_file (RCPackman *packman, const gchar *filename)
{
    RCRpmman *rpmman = RC_RPMMAN (packman);
    HeaderInfo *hi;
    Header header;
    RCPackage *package;

    hi = rc_rpmman_read_package_file (rpmman, filename);

    if (!hi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "Unable to query file");
        return NULL;
    }

    header = hi->headers->data;

    package = rc_package_new ();

    rc_rpmman_read_header (rpmman, header, package);

    rc_rpmman_depends_fill (rpmman, header, package);

    rc_rpmman_header_info_free (rpmman, hi);

    return (package);
} /* rc_packman_query_file */

/* Query all of the packages on the system */

static RCPackageSList *
rc_rpmman_query_all_v4 (RCPackman *packman)
{
    RCPackageSList *list = NULL;
    rc_rpmdbMatchIterator mi = NULL;
    Header header;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    if (rpmman->version >= 40100) {
        mi = rpmman->rpmtsInitIterator (rpmman->rpmts, RPMDBI_PACKAGES,
                                        NULL, 0);
    }
    else
        mi = rpmman->rpmdbInitIterator (rpmman->db, RPMDBI_PACKAGES, NULL, 0);

    if (!mi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to initialize database search");

        goto ERROR;
    }

    while ((header = rpmman->rpmdbNextIterator (mi))) {
        RCPackage *package = rc_package_new ();

        rc_rpmman_read_header (rpmman, header, package);

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

            rc_package_slist_unref (list);
            g_slist_free (list);

            goto ERROR;
        }

        package = rc_package_new ();

        rc_rpmman_read_header (rpmman, header, package);

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
    gboolean close_db = FALSE;

    if ((RC_RPMMAN (packman))->db_status < RC_RPMMAN_DB_RDONLY) {
        if (!open_database (RC_RPMMAN (packman), FALSE)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to query packages");
            return NULL;
        }
        close_db = TRUE;
    }

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

    if (close_db)
        close_database (RC_RPMMAN (packman));

    return packages;
}

static gboolean
rc_rpmman_package_is_repackaged (RCPackman *packman, RCPackage *package)
{
    RCRpmman *rpmman = RC_RPMMAN (packman);
    HeaderInfo *hi;
    Header header = NULL;
    char *buf;
    guint32 count;

    if (!package->package_filename) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "no file name specified");
        return FALSE;
    }

    hi = rc_rpmman_read_package_file (rpmman, package->package_filename);

    if (!hi) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to open %s", package->package_filename);
        return FALSE;
    }

    header = hi->headers->data;

    rpmman->headerGetEntry (header, RPMTAG_REMOVETID, NULL,
                            (void **) &buf, &count);

    rc_rpmman_header_info_free (rpmman, hi);

    return count > 0 ? TRUE : FALSE;
}

static gboolean
split_rpm (RCPackman *packman, RCPackage *package, gchar **signature_filename,
           gchar **payload_filename, guint8 **md5sum, guint32 *size)
{
    FD_t rpm_fd = NULL;
    int signature_fd = -1;
    int payload_fd = -1;
    struct rpmlead lead;
    Header signature_header = NULL;
    char *buf = NULL;
    guint32 count = 0;
    guchar buffer[128];
    ssize_t num_bytes;
    RCRpmman *rpmman = RC_RPMMAN (packman);
    int rc;
    const char *msg = NULL;

    if (!package->package_filename) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "no file name specified");
        goto ERROR;
    }

    rpm_fd = rc_rpm_open (rpmman, package->package_filename, "r.fdio",
                          O_RDONLY, 0);

    if (rpm_fd == NULL) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to open %s", package->package_filename);
        goto ERROR;
    }

    if (rpmman->readLead (rpm_fd, &lead)) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "unable to read from %s",
                              package->package_filename);
        goto ERROR;
    }

    *signature_filename = NULL;
    *payload_filename = NULL;
    *md5sum = NULL;
    *size = 0;

    if (rpmman->version >= 40100) {
        rc = rpmman->rpmReadSignature (rpm_fd, &signature_header,
                                       lead.signature_type, &msg);
    }
    else {
        rc = rpmman->rpmReadSignatureOld (rpm_fd, &signature_header,
                                          lead.signature_type);
    }

    if (!rc && signature_header != NULL) {
        rpmman->headerGetEntry (signature_header, RPMSIGTAG_GPG, NULL,
                                (void **)&buf, &count);

        if (count > 0) {
            if ((signature_fd = g_file_open_tmp ("rpm-sig-XXXXXX",
                                                 signature_filename,
                                                 NULL)) == -1)
            {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "failed to create temporary signature "
                                      "file");
                goto ERROR;
            }
            
            if (!rc_write (signature_fd, buf, count)) {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "failed to write temporary signature "
                                      "file");
                goto ERROR;
            }

            rc_close (signature_fd);
            signature_fd = -1;
        }

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

        if (count > 0)
            *size = *((guint32 *)buf);
        
        rpmman->headerFree (signature_header);
        signature_header = NULL;
    }
    else {
        /* msg is an empty string on success */
        if (msg && *msg)
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_NONE, msg);

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "failed to read RPM signature section");
        goto ERROR;
    }

    if ((payload_fd = g_file_open_tmp ("rpm-data-XXXXXX",
                                       payload_filename, NULL)) == -1)
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "failed to create temporary payload file");
        goto ERROR;
    } else {
        while ((num_bytes = rc_rpm_read (rpmman, buffer, sizeof (buffer[0]),
                                         sizeof (buffer), rpm_fd)) > 0)
        {
            if (!rc_write (payload_fd, buffer, num_bytes)) {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "unable to write temporary payload "
                                      "file");
                goto ERROR;
            }
        }

        rc_close (payload_fd);
        payload_fd = -1;
    }

    rc_rpm_close (rpmman, rpm_fd);
    rpm_fd = NULL;

    return TRUE;

  ERROR:
    if (rpm_fd)
        rc_rpm_close (rpmman, rpm_fd);
    if (signature_fd != -1)
        rc_close (signature_fd);
    if (payload_fd != -1)
        rc_close (payload_fd);
    if (signature_header)
        rpmman->headerFree (signature_header);
    if (*signature_filename) {
        unlink (*signature_filename);
        g_free (*signature_filename);
        *signature_filename = NULL;
    }
    if (*payload_filename) {
        unlink (*payload_filename);
        g_free (*payload_filename);
        *payload_filename = NULL;
    }

    return FALSE;
}

static RCVerificationSList *
rc_rpmman_verify (RCPackman *packman, RCPackage *package, guint32 type)
{
    RCVerificationSList *ret = NULL;
    RCVerification *verification = NULL;
    gchar *signature_filename = NULL;
    gchar *payload_filename = NULL;
    guint8 *md5sum = NULL;
    guint32 size;

    /*
     * Repackaged RPMs don't have correct md5sums, signatures, or file
     * sizes, so just skip over verification if it is.
     */
    if (rc_rpmman_package_is_repackaged (packman, package))
        return NULL;
    
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

        rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                              "Couldn't verify signatures");

        verification = rc_verification_new ();

        verification->type = RC_VERIFICATION_TYPE_SANITY;
        verification->status = RC_VERIFICATION_STATUS_FAIL;
        verification->info = g_strdup ("Invalid RPM file");

        ret = g_slist_append (ret, verification);

        return ret;
    }

    if (signature_filename && (type & RC_VERIFICATION_TYPE_GPG)) {
        verification = rc_verify_gpg (payload_filename, signature_filename);

        ret = g_slist_append (ret, verification);
    }

    if (md5sum && (type & RC_VERIFICATION_TYPE_MD5)) {
        verification = rc_verify_md5 (payload_filename, md5sum);

        ret = g_slist_append (ret, verification);
    }

    if ((size > 0) && (type & RC_VERIFICATION_TYPE_SIZE)) {
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

#ifndef STRICT_RPM_ORDER
    /* Take care of broken Mandrake releases */
    if ((alen > 3) && !strcmp (a + alen - 3, "mdk")) {
        str1[alen - 3] = '\0';
    }
    if ((blen > 3) && !strcmp (b + blen - 3, "mdk")) {
        str2[blen - 3] = '\0';
    }
#endif

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

        /* This should only happen if someone is changing the string */
        /* behind our back.  It should be a _very_ rare race condition */
        if (one == str1) return -1; /* arbitrary */

        /* take care of the case where the two version segments are */
        /* different types: one numeric and one alpha */

        /* Here's how we handle comparing numeric and non-numeric
         * segments -- non-numeric (ximian.1) always sorts higher than
         * numeric (0_helix_1). */
        if (two == str2)
#ifdef STRICT_RPM_ORDER
            return (isnum ? 1 : -1);
#else
            return (isnum ? -1 : 1);
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
    gint rc = 0;

    g_assert (spec1);
    g_assert (spec2);
    
    if (spec1->nameq || spec2->nameq) {
        if (spec1->nameq == spec2->nameq)
            rc = 0;
        else
            rc = strcmp (spec1->nameq ? g_quark_to_string (spec1->nameq) : "",
                         spec2->nameq ? g_quark_to_string (spec2->nameq) : "");
    }
    if (rc) return rc;
    
    /* WARNING: This is partially broken, because you cannot
     * tell the difference between an epoch of zero and no epoch */
    /* NOTE: when the code is changed, foo->spec.epoch will be an
     * existance test for the epoch and foo->spec.epoch > 0 will 
     * be a test for > 0.  Right now these are the same, but
     * please leave the code as-is. */

    if (spec1->has_epoch && spec2->has_epoch) {
        rc = spec1->epoch - spec2->epoch;
    } else if (spec1->has_epoch && spec1->epoch > 0) {
        rc = 1;
    } else if (spec2->has_epoch && spec2->epoch > 0) {
        rc = -1;
    }
    if (rc) return rc;

    rc = vercmp (spec1->version ? spec1->version : "",
                 spec2->version ? spec2->version : "");
    if (rc) return rc;

    if (spec1->release && *(spec1->release) && spec2->release &&
        *(spec2->release)) {
        rc = vercmp (spec1->release ? spec1->release : "",
                     spec2->release ? spec2->release : "");
    }
    return rc;
}

static RCPackage *
rc_rpmman_find_file_v4 (RCPackman *packman, const gchar *filename)
{
    rc_rpmdbMatchIterator mi = NULL;
    Header header;
    RCPackage *package;
    RCRpmman *rpmman = RC_RPMMAN (packman);

    if (rpmman->version >= 40100) {
        mi = rpmman->rpmtsInitIterator (rpmman->rpmts, RPMTAG_BASENAMES,
                                        filename, 0);
    }
    else {
        mi = rpmman->rpmdbInitIterator (rpmman->db, RPMTAG_BASENAMES,
                                        filename, 0);
    }

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

    rc_rpmman_read_header (rpmman, header, package);

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

    rc_rpmman_read_header (rpmman, header, package);

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
    RCPackage *ret;
    gboolean close_db = FALSE;

    if ((RC_RPMMAN (packman))->db_status < RC_RPMMAN_DB_RDONLY) {
        if (!open_database (RC_RPMMAN (packman), FALSE)) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "unable to find file");
            return NULL;
        }
        close_db = TRUE;
    }

    if (RC_RPMMAN (packman)->major_version == 4) {
        ret = rc_rpmman_find_file_v4 (packman, filename);
    } else {
        ret = rc_rpmman_find_file_v3 (packman, filename);
    }

    if (close_db)
        close_database (RC_RPMMAN (packman));

    return ret;
}

static RCPackageFileSList *
rc_rpmman_file_list (RCPackman *packman, RCPackage *package)
{
    RCRpmman *rpmman = RC_RPMMAN (packman);
    gboolean close_db = FALSE;
    char *package_name = NULL;
    HeaderInfo *hi = NULL;
    Header header;
    RCPackageFileSList *file_list = NULL;
    RCPackageFile *file;
    int count, i;
    char **basenames = NULL;
    char **dirnames = NULL;
    gint32 *dirindexes = NULL;
    gint32 *filesizes = NULL;
    char **md5sums = NULL;
    char **owners = NULL;
    char **groups = NULL;
    gint32 *uids = NULL;
    gint32 *gids = NULL;
    guint16 *filemodes = NULL;
    gint32 *filemtimes = NULL;
    gint32 *fileflags = NULL;

    g_return_val_if_fail (package, NULL);

    if (!package->installed) {
        if (!package->package_filename) {
            rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                  "no file list information available for "
                                  "package '%s'",
                                  g_quark_to_string (package->spec.nameq));
            return NULL;
        }

        hi = rc_rpmman_read_package_file (rpmman, package->package_filename);
    }
    else {
        if ((RC_RPMMAN (packman))->db_status < RC_RPMMAN_DB_RDONLY) {
            if (!open_database (RC_RPMMAN (packman), FALSE)) {
                rc_packman_set_error (packman, RC_PACKMAN_ERROR_ABORT,
                                      "unable to query packages");
                return NULL;
            }
            close_db = TRUE;
        }
        
        package_name = rc_package_to_rpm_name (package);
        hi = rc_rpmman_find_system_headers (rpmman, package_name);
    }

    if (!hi)
        goto cleanup;
    
    if (g_slist_length (hi->headers) > 1) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, "Package '%s' matches %d entries",
                  package_name, g_slist_length (hi->headers));
    }

    header = (Header) hi->headers->data;

    if (!rpmman->headerGetEntry (header, RPMTAG_BASENAMES, NULL,
                                 (void **) &basenames, &count)) {
        rc_debug (RC_DEBUG_LEVEL_WARNING, "Package '%s' contains no files",
                  g_quark_to_string (package->spec.nameq));
        goto cleanup;
    }

    /* Ugh. */
    rpmman->headerGetEntry (header, RPMTAG_DIRNAMES, NULL,
                            (void **) &dirnames, NULL);
    rpmman->headerGetEntry (header, RPMTAG_DIRINDEXES, NULL,
                            (void **) &dirindexes, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILESIZES, NULL,
                            (void **) &filesizes, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEMD5S, NULL,
                            (void **) &md5sums, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEUSERNAME, NULL,
                            (void **) &owners, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEGROUPNAME, NULL,
                            (void **) &groups, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEUIDS, NULL,
                            (void **) &uids, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEGIDS, NULL,
                            (void **) &gids, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEMODES, NULL,
                            (void **) &filemodes, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEMTIMES, NULL,
                            (void **) &filemtimes, NULL);
    rpmman->headerGetEntry (header, RPMTAG_FILEFLAGS, NULL,
                            (void **) &fileflags, NULL);

    for (i = 0; i < count; i++) {
        file = rc_package_file_new ();

        file->filename = g_strdup_printf ("%s%s", dirnames[dirindexes[i]],
                                          basenames[i]);
        file->size = filesizes[i];
        file->md5sum = g_strdup (md5sums[i]);
        file->mode = filemodes[i];
        file->mtime = filemtimes[i];

        if (fileflags[i] & RPMFILE_GHOST)
            file->ghost = TRUE;
        else
            file->ghost = FALSE;

        if (uids)
            file->uid = uids[i];
        else {
            if (rpmman->unameToUid (owners[i], &file->uid) < 0) {
                rc_debug (RC_DEBUG_LEVEL_WARNING,
                          "User '%s' does not exist; assuming root",
                          owners[i]);
                file->uid = 0;
            }
        }

        if (gids)
            file->gid = gids[i];
        else {
            if (rpmman->gnameToGid (groups[i], &file->gid) < 0) {
                rc_debug (RC_DEBUG_LEVEL_WARNING,
                          "Group '%s' does not exist; assuming root",
                          groups[i]);
                file->gid = 0;
            }
        }

        file_list = g_slist_prepend (file_list, file);
    }

    file_list = g_slist_reverse (file_list);

cleanup:
    if (package_name)
        g_free (package_name);

    if (hi)
        rc_rpmman_header_info_free (rpmman, hi);

    if (close_db)
        close_database (rpmman);

    return file_list;
}
    

gboolean
rc_rpmman_lock (RCPackman *packman)
{
    return (open_database (RC_RPMMAN (packman), TRUE));
}

void
rc_rpmman_unlock (RCPackman *packman)
{
    close_database (RC_RPMMAN (packman));
}

static void
rc_rpmman_finalize (GObject *obj)
{
    RCRpmman *rpmman = RC_RPMMAN (obj);

    close_database (rpmman);

#ifndef STATIC_RPM
#if 0
    /* If we do this we just look bad, sigh.  I can't really blame
     * rpmlib for not being designed to be dlopen'd.  Much as I like
     * to blame rpmlib for things. */
    g_module_close (rpmman->rpm_lib);
#endif
#endif

    if (rpmman->db_watcher_cb) {
        g_source_remove (rpmman->db_watcher_cb);
        rpmman->db_watcher_cb = 0;
    }

    if (parent_class->finalize)
        parent_class->finalize (obj);
}

static void
rc_rpmman_class_init (RCRpmmanClass *klass)
{
    GObjectClass *object_class = (GObjectClass *) klass;
    RCPackmanClass *packman_class = (RCPackmanClass *) klass;

    object_class->finalize = rc_rpmman_finalize;

    parent_class = g_type_class_peek_parent (klass);

    packman_class->rc_packman_real_transact = rc_rpmman_transact;
    packman_class->rc_packman_real_query = rc_rpmman_query;
    packman_class->rc_packman_real_query_all = rc_rpmman_query_all;
    packman_class->rc_packman_real_query_file = rc_rpmman_query_file;
    packman_class->rc_packman_real_version_compare = rc_rpmman_version_compare;
    packman_class->rc_packman_real_parse_version = rc_rpmman_parse_version;
    packman_class->rc_packman_real_verify = rc_rpmman_verify;
    packman_class->rc_packman_real_find_file = rc_rpmman_find_file;
    packman_class->rc_packman_real_lock = rc_rpmman_lock;
    packman_class->rc_packman_real_unlock = rc_rpmman_unlock;
    packman_class->rc_packman_real_is_database_changed =
        rc_rpmman_is_database_changed;
    packman_class->rc_packman_real_file_list = rc_rpmman_file_list;
} /* rc_rpmman_class_init */

#ifdef STATIC_RPM

static void
load_fake_syms (RCRpmman *rpmman)
{
    rpmman->rc_fdOpen = &fdOpen;
    rpmman->rc_fdRead = &fdRead;
    rpmman->rc_fdClose = &fdClose;
    /* Look for hdrVec in load_rpm_syms for an explanation of what's
     * going on */
#if RPM_VERSION >= 40100 && RPM_VERSION <= 40201 /* RPM 4.1 to 4.2.1 inclusive */
    /* FIXME: untested */
    rpmman->headerGetEntry = ((void **)hdrVec)[16];
    rpmman->headerFree = ((void **)hdrVec)[2];
    rpmman->headerSizeof = ((void **)hdrVec)[6];
    rpmman->headerLoad = ((void **)hdrVec)[10];
#elif RPM_VERSION >= 40004 /* RPM 4.0.4 only */
    /* FIXME: untested */
    rpmman->headerGetEntry = ((void **)hdrVec)[15];
    rpmman->headerFree = ((void **)hdrVec)[1];
    rpmman->headerSizeof = ((void **)hdrVec)[5];
    rpmman->headerLoad = ((void **)hdrVec)[9];
#else
    rpmman->headerGetEntry = &headerGetEntry;
    rpmman->headerFree = &headerFree;
    rpmman->headerSizeof = &headerSizeof;
    rpmman->headerLoad = &headerLoad;
#endif
    rpmman->readLead = &readLead;
    rpmman->rpmReadConfigFiles = &rpmReadConfigFiles;
    rpmman->rpmdbOpen = &rpmdbOpen;
    rpmman->rpmdbClose = &rpmdbClose;
    rpmman->rpmProblemString = &rpmProblemString;
    rpmman->rpmProblemStringOld = &rpmProblemString;
    rpmman->rpmProblemStringOlder = &rpmProblemString;
    rpmman->rpmExpandNumeric = &rpmExpandNumeric;
    rpmman->rpmDefineMacro = &rpmDefineMacro;
    rpmman->rpmGetPath = &rpmGetPath;
    rpmman->unameToUid = &unameToUid;
    rpmman->gnameToUid = &gnameToGid;

#if RPM_VERSION >= 40100
    rpmman->rpmReadPackageFile = &rpmReadPackageFile;
    rpmman->rpmReadSignature = &rpmReadSignature;
    rpmman->rpmtsInitIterator = &rpmtsInitIterator;
    rpmman->rpmtsCreate = &rpmtsCreate;
    rpmman->rpmtsSetRootDir = &rpmtsSetRootDir;
    rpmman->rpmtsAddInstallElement = &rpmtsAddInstallElement;
    rpmman->rpmtsAddEraseElement = &rpmtsAddEraseElement;
    rpmman->rpmtsCheck = &rpmtsCheck;
    rpmman->rpmtsOrder = &rpmtsOrder;
    rpmman->rpmtsRun = &rpmtsRun;
    rpmman->rpmtsClean = &rpmtsClean;
    rpmman->rpmtsSetNotifyCallback = &rpmtsSetNotifyCallback;
    rpmman->rpmtsSetFlags = &rpmtsSetFlags;
    rpmman->rpmtsFree = &rpmtsFree;
    rpmman->rpmtsProblems = &rpmtsProblems;
    rpmman->rpmtsVSFlags = &rpmtsVSFlags;
    rpmman->rpmtsSetVSFlags = &rpmtsSetVSFlags;
    rpmman->rpmpsFree = &rpmpsFree;
#else
#  if RPM_VERSION >= 40000
    rpmman->rpmdbInitIterator = &rpmdbInitIterator;
#  endif
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
    rpmman->rpmReadSignatureOld = &rpmReadSignature;
#endif

#if RPM_VERSION >= 40000
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

    /* RPM 4.0.4 marked our two header manipulation functions as
     * static, which is a bit of a pain.  The workaround is to look
     * for hdrVec, a globally exported virtual function table, and
     * find the functions we need by absolute offset.  It's not very
     * pretty, and could quite possibly break with successive releases
     * of RPM, which is why I check for 4.0.4 exactly, rather than
     * 4.0.4 and all later versions.  Blah.
     *
     * This is still a problem in RPM 4.1 and 4.2, except that the
     * vtable is locaed in librpmdb now.
     */
    if (rpmman->version >= 40004) {
        void **(*hdrfuncs);

        if (!g_module_symbol (rpmman->rpm_lib, "hdrVec",
                              ((gpointer)&hdrfuncs))) {
            return (FALSE);
        }

        if (rpmman->version == 40004) { /* RPM 4.0.4 only */
            rpmman->headerFree = *(*hdrfuncs + 1);
            rpmman->headerGetEntry = *(*hdrfuncs + 15);
            rpmman->headerSizeof = *(*hdrfuncs + 5);
            rpmman->headerLoad = *(*hdrfuncs + 9);
        }
        else if (rpmman->version >= 40100 &&
                 rpmman->version <= 40201) { /* RPM 4.1-4.2.1 inclusive */
            rpmman->headerFree = *(*hdrfuncs + 2);
            rpmman->headerGetEntry = *(*hdrfuncs + 16);
            rpmman->headerSizeof = *(*hdrfuncs + 6);
            rpmman->headerLoad = *(*hdrfuncs + 10);
        }
        else
            g_assert_not_reached ();
    } else {
        if (!g_module_symbol (rpmman->rpm_lib, "headerGetEntry",
                              ((gpointer)&rpmman->headerGetEntry))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "headerFree",
                              ((gpointer)&rpmman->headerFree))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "headerSizeof",
                              ((gpointer)&rpmman->headerSizeof))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "headerLoad",
                              ((gpointer)&rpmman->headerLoad))) {
            return (FALSE);
        }
    }

    if (!g_module_symbol (rpmman->rpm_lib, "readLead",
                          ((gpointer)&rpmman->readLead))) {
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
    if (!g_module_symbol (rpmman->rpm_lib, "rpmProblemString",
                          ((gpointer)&rpmman->rpmProblemStringOld))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmProblemString",
                          ((gpointer)&rpmman->rpmProblemStringOlder))) {
        return (FALSE);
    }
#if 0
    if (!g_module_symbol (rpmman->rpm_lib, "rpmGetRpmlibProvides",
                          ((gpointer)&rpmman->rpmGetRpmlibProvides))) {
        return (FALSE);
    }
#endif
    if (!g_module_symbol (rpmman->rpm_lib, "rpmExpandNumeric",
                          ((gpointer)&rpmman->rpmExpandNumeric))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmDefineMacro",
                          ((gpointer) &rpmman->rpmDefineMacro))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "rpmGetPath",
                          ((gpointer) &rpmman->rpmGetPath))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "unameToUid",
                          ((gpointer) &rpmman->unameToUid))) {
        return (FALSE);
    }
    if (!g_module_symbol (rpmman->rpm_lib, "gnameToGid",
                          ((gpointer) &rpmman->gnameToGid))) {
        return (FALSE);
    }

    if (rpmman->version >= 40100) {
        if (!g_module_symbol (rpmman->rpm_lib, "rpmReadPackageFile",
                              ((gpointer)&rpmman->rpmReadPackageFile))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmReadSignature",
                              ((gpointer)&rpmman->rpmReadSignature))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsInitIterator",
                              ((gpointer)&rpmman->rpmtsInitIterator))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbNextIterator",
                              ((gpointer)&rpmman->rpmdbNextIterator))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsCreate",
                              ((gpointer)&rpmman->rpmtsCreate))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsSetRootDir",
                              ((gpointer)&rpmman->rpmtsSetRootDir))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsAddInstallElement",
                              ((gpointer)&rpmman->rpmtsAddInstallElement))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsAddEraseElement",
                              ((gpointer)&rpmman->rpmtsAddEraseElement))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsCheck",
                              ((gpointer)&rpmman->rpmtsCheck))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsOrder",
                              ((gpointer)&rpmman->rpmtsOrder))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsRun",
                              ((gpointer)&rpmman->rpmtsRun))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsClean",
                              ((gpointer)&rpmman->rpmtsClean))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsSetNotifyCallback",
                              ((gpointer)&rpmman->rpmtsSetNotifyCallback))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsSetFlags",
                              ((gpointer)&rpmman->rpmtsSetFlags))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsFree",
                              ((gpointer)&rpmman->rpmtsFree))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsProblems",
                              ((gpointer)&rpmman->rpmtsProblems))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmpsFree",
                              ((gpointer)&rpmman->rpmpsFree))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsVSFlags",
                              ((gpointer)&rpmman->rpmtsVSFlags))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmtsSetVSFlags",
                              ((gpointer)&rpmman->rpmtsSetVSFlags))) {
            return (FALSE);
        }
    } else {
        if (rpmman->version >= 40000) {
            if (!g_module_symbol (rpmman->rpm_lib, "rpmdbInitIterator",
                                  ((gpointer)&rpmman->rpmdbInitIterator))) {
                return (FALSE);
            }
            if (!g_module_symbol (rpmman->rpm_lib, "XrpmdbNextIterator",
                                  ((gpointer)&rpmman->rpmdbNextIterator))) {
                return (FALSE);
            }
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
        if (!g_module_symbol (rpmman->rpm_lib, "rpmReadSignature",
                              ((gpointer)&rpmman->rpmReadSignatureOld))) {
            return (FALSE);
        }
    }

    if (rpmman->version >= 40000) {
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbGetIteratorCount",
                              ((gpointer)&rpmman->rpmdbGetIteratorCount))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbFreeIterator",
                              ((gpointer)&rpmman->rpmdbFreeIterator))) {
            return (FALSE);
        }
        if (!g_module_symbol (rpmman->rpm_lib, "rpmdbGetIteratorOffset",
                              ((gpointer)&rpmman->rpmdbGetIteratorOffset))) {
            return (FALSE);
        }
    }
    else if (rpmman->major_version == 3) {
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
    char *tmp;

    rpmman->major_version = 0;
    rpmman->minor_version = 0;
    rpmman->micro_version = 0;

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

    tmp = g_strdup_printf ("%d%02d%02d", rpmman->major_version,
                           rpmman->minor_version, rpmman->micro_version);
    rpmman->version = atoi (tmp);
    g_free (tmp);
}

#define OBJECTDIR "/tmp"

#ifndef STATIC_RPM
static char *
write_objects (void)
{
    GByteArray *buf;
    int fd = -1;
    char *object_dir = NULL;
    char *file = NULL;
    mode_t mask;

    /* First we need to create a secure directory */

    mask = umask (0077);
    object_dir = g_strdup (OBJECTDIR "/rc-rpm-XXXXXX");
    if (!rc_mkdtemp (object_dir)) {
        g_free (object_dir);
        object_dir = NULL;
        goto ERROR;
    }

    umask (mask);

    /* Now, write out each of the objects */
    {
        typedef struct {
            const char   *obj_name;
            const char   *obj_data;
            unsigned int  obj_len;
        } ObjectInfo;

        ObjectInfo objects[] = {
            { "rc-{rpm_rpmio_rpmdb}-4.2.so",
              rc_rpm_rpmio_rpmdb_4_2_so,
              rc_rpm_rpmio_rpmdb_4_2_so_len },
            { "rc-{rpm_rpmio_rpmdb}-4.1.so",
              rc_rpm_rpmio_rpmdb_4_1_so,
              rc_rpm_rpmio_rpmdb_4_1_so_len },
            { "rc-{rpm_rpmio_rpmdb}-4.0.4.so",
              rc_rpm_rpmio_rpmdb_4_0_4_so,
              rc_rpm_rpmio_rpmdb_4_0_4_so_len },
            { "rc-{rpm_rpmio_rpmdb}-4.0.3.so",
              rc_rpm_rpmio_rpmdb_4_0_3_so,
              rc_rpm_rpmio_rpmdb_4_0_3_so_len },
            { "rc-{rpm_rpmio}.so.0",
              rc_rpm_rpmio_so_0,
              rc_rpm_rpmio_so_0_len },
            { "rc-{rpm}.so.0",
              rc_rpm_so_0,
              rc_rpm_so_0_len },
            { 0 }
        };

        ObjectInfo *cur_obj;
        
        for (cur_obj = objects; cur_obj->obj_name; cur_obj++) {
            file = g_strconcat (object_dir, "/", cur_obj->obj_name, NULL);
            fd = open (file, O_RDWR | O_CREAT | O_EXCL, S_IRWXU);

            if (fd < 0)
                goto ERROR;

            g_free (file);
            file = NULL;

            if (rc_uncompress_memory (cur_obj->obj_data,
                                      cur_obj->obj_len,
                                      &buf))
                goto ERROR;

            if (!rc_write (fd, buf->data, buf->len))
                goto ERROR;

            rc_close (fd);
            fd = -1;
            g_byte_array_free (buf, TRUE);
            buf = NULL;
        }
    }                                      

    return object_dir;

  ERROR:
    if (file)
        g_free (file);
    if (fd != -1)
        rc_close (fd);
    if (buf)
        g_byte_array_free (buf, TRUE);
    if (object_dir)
        rc_rmdir (object_dir);

    return NULL;
}
#endif

static void
rc_rpmman_init (RCRpmman *obj)
{
    RCPackman *packman = RC_PACKMAN (obj);
    gchar *tmp;
    int capabilities = 0;
#ifndef STATIC_RPM
    gchar **rpm_version;
    gchar *so_file;
    char *object_dir;
    const char *objects[] = {
        "rc-{rpm_rpmio_rpmdb}-4.2.so",
        "rc-{rpm_rpmio_rpmdb}-4.1.so",
        "rc-{rpm_rpmio_rpmdb}-4.0.4.so",
        "rc-{rpm_rpmio_rpmdb}-4.0.3.so",
        "rc-{rpm_rpmio}.so.0",
        "rc-{rpm}.so.0",
        NULL };
    const char **iter;
#endif

#ifdef STATIC_RPM

    /* Used to use RPMVERSION rathre than rpmEVR, until that broke in
     * RPM 4.0.4.  rpmEVR seems to contain the same thing? */
    extern const char *rpmEVR;

    parse_rpm_version (obj, rpmEVR);

    load_fake_syms (obj);

#else

    if (!g_module_supported ()) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "dynamic module loading not supported");
        return;
    }

    if (!(object_dir = write_objects ())) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "unable to write rpm stub files");
        return;
    }

    iter = objects;

    while (*iter && !obj->rpm_lib) {
        so_file = g_strdup_printf ("%s/%s", object_dir, *iter);
        obj->rpm_lib = g_module_open (so_file, G_MODULE_BIND_LAZY);
        g_free (so_file);
        iter++;
    }

    rc_rmdir (object_dir);
    g_free (object_dir);

    if (!obj->rpm_lib) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "unable to open rpm library");
        return;
    }

    /* Used to use RPMVERSION rather than rpmEVR, until that broke in
     * RPM 4.0.4.  rpmEVR seems to contain the same thing? */
    if (!(g_module_symbol (obj->rpm_lib, "rpmEVR",
                           ((gpointer)&rpm_version))))
    {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "unable to determine rpm version");
        return;
    }

    parse_rpm_version (obj, *rpm_version);

#if RPM_VERSION > LATEST_SUPPORTED_RPM_VERSION
#  error This version of RPM is not yet supported by libredcarpet
#endif

    if (obj->version > LATEST_SUPPORTED_RPM_VERSION) {
        rc_packman_set_error (packman, RC_PACKMAN_ERROR_FATAL,
                              "this version of RPM is not yet supported");
        return;
    }

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

    rc_packman_set_file_extension(packman, "rpm");

    capabilities = RC_PACKMAN_CAP_PROVIDE_ALL_VERSIONS |
        RC_PACKMAN_CAP_IGNORE_ABSENT_EPOCHS;

    /* Repackaging was added in RPM 4.0.3, but broken until 4.0.4 */
    if (obj->version >= 40004)
        capabilities |= RC_PACKMAN_CAP_ROLLBACK;

    rc_packman_set_capabilities(packman, capabilities);

    obj->db_status = RC_RPMMAN_DB_NONE;

    /* initialize the mtime */
    obj->db_mtime = 0;
    rc_rpmman_is_database_changed (packman);

    /* Watch for changes */
    obj->db_watcher_cb =
        g_timeout_add (5000, (GSourceFunc) database_check_func,
                       (gpointer) obj);

} /* rc_rpmman_init */

RCRpmman *
rc_rpmman_new (void)
{
    RCRpmman *rpmman =
        RC_RPMMAN (g_object_new (rc_rpmman_get_type (), NULL));

    return rpmman;
} /* rc_rpmman_new */
