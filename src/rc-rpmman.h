/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rpmman.h
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

#ifndef _RC_RPMMAN_H
#define _RC_RPMMAN_H

#include <glib-object.h>
#include <gmodule.h>

#include <rpmlib.h>
#include <rpmmacro.h>

#if RPM_VERSION >= 40100
#  include <rpmts.h>
#  include <rpmps.h>
#endif

#include "rc-packman.h"
#include "rc-rpmman-types.h"

#define RC_TYPE_RPMMAN            (rc_rpmman_get_type ())
#define RC_RPMMAN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                                   RC_TYPE_RPMMAN, RCRpmman))
#define RC_RPMMAN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                   RC_TYPE_RPMMAN, \
                                   RCRpmmanClass))
#define IS_RC_RPMMAN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                                   RC_TYPE_RPMMAN))
#define IS_RC_RPMMAN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                                   RC_TYPE_RPMMAN))

typedef struct _RCRpmman      RCRpmman;
typedef struct _RCRpmmanClass RCRpmmanClass;

typedef enum {
    RC_RPMMAN_DB_NONE = 0,
    RC_RPMMAN_DB_RDONLY,
    RC_RPMMAN_DB_RDWR
} RCRpmmanDBStatus;

struct _RCRpmman {
    RCPackman parent;

    /* The RPM transaction set (>= 4.1 only) */
    rpmts rpmts;

    rpmdb db;
    int db_status;
    int lock_fd;

    gchar *rpmroot;

    guint major_version;
    guint minor_version;
    guint micro_version;

    guint version;

    guint db_watcher_cb;
    time_t db_mtime;

#ifndef STATIC_RPM
    GModule *rpm_lib;
#endif

    /*
     * Common functions
     */

    FD_t (*rc_fdOpen)(const char *, int, mode_t);
    ssize_t (*rc_fdRead)(void *, char *, size_t);
    int (*rc_fdClose)(void *);

    FD_t (*Fopen)(const char *, const char *);
    void (*Fclose)(FD_t);
    size_t (*Fread)(void *, size_t, size_t, FD_t);

    /* it turns out this isn't in 3.0.3 and there's no equivalent
     * older fd* version.  we're not really using it for anything
     * important so i'm not going to worry about it */
    /* int (*Ferror)(FD_t); */

    int (*headerGetEntry)(Header, int_32, int_32 *, void **, int_32 *);
    void (*headerFree)(Header);

    int (*readLead)(FD_t, struct rpmlead *);
    int (*rpmReadConfigFiles)(const char *, const char *);
    int (*rpmdbOpen)(const char *, rpmdb *, int, int);
    void (*rpmdbClose)(rpmdb);
    const char * (*rpmProblemString)(RCrpmProblem);
    const char * (*rpmProblemStringOld)(RCrpmProblemOld);
    const char * (*rpmProblemStringOlder)(RCrpmProblemOlder);
    int (*rpmGetRpmlibProvides)(char ***, int **, char ***);
    int (*rpmExpandNumeric)(const char *);
    int (*rpmDefineMacro)(MacroContext *, const char *, int);
    const char * (*rpmGetPath)(const char *, ...);

    /*
     * RPM 3.0.x only functions
     */

    int (*rpmdbFirstRecNum)(rpmdb);
    int (*rpmdbNextRecNum)(rpmdb, unsigned int);
    int (*rpmdbFindByLabel)(rpmdb, const char *, rc_dbiIndexSet *);
    int (*rpmdbFindPackage)(rpmdb, const char *, rc_dbiIndexSet *);
    int (*rpmdbFindByFile)(rpmdb, const char *, rc_dbiIndexSet *);
    Header (*rpmdbGetRecord)(rpmdb, unsigned int);
    unsigned int (*dbiIndexSetCount)(rc_dbiIndexSet);
    unsigned int (*dbiIndexRecordOffset)(rc_dbiIndexSet, int);
    void (*dbiFreeIndexRecord)(rc_dbiIndexSet);

    /*
     * RPM 4.x functions
     */

    int (*rpmdbGetIteratorCount)(rc_rpmdbMatchIterator);
    void (*rpmdbFreeIterator)(rc_rpmdbMatchIterator);
    Header (*rpmdbNextIterator)(rc_rpmdbMatchIterator);
    unsigned int (*rpmdbGetIteratorOffset)(rc_rpmdbMatchIterator);

    /*
     * RPM 4.0.x only functions
     */
    rc_rpmdbMatchIterator (*rpmdbInitIterator)(rpmdb, int, const void *,
                                               size_t);

    /* 
     * Pre-RPM 4.1 functions
     */
    int (*rpmReadPackageHeader)(FD_t, Header *, int *, int *, int *);
    int (*rpmtransAddPackage)(rpmTransactionSet, Header, FD_t, const void *,
                              int, rpmRelocation *);
    void (*rpmtransRemovePackage)(rpmTransactionSet, int);
    rpmTransactionSet (*rpmtransCreateSet)(rpmdb, const char *);
    void (*rpmtransFree)(rpmTransactionSet);
#if RPM_VERSION >= 40003
    int (*rpmdepCheck)(rpmTransactionSet, rpmDependencyConflict *,
                       int *);
    void (*rpmdepFreeConflicts)(rpmDependencyConflict, int);
#else
    int (*rpmdepCheck)(rpmTransactionSet, struct rpmDependencyConflict **,
                       int *);
    void (*rpmdepFreeConflicts)(struct rpmDependencyConflict *, int);
#endif
    int (*rpmdepOrder)(rpmTransactionSet);
    int (*rpmRunTransactions)(rpmTransactionSet, rpmCallbackFunction,
                              void *, rpmProblemSet, rpmProblemSet *,
                              int, int);
    void (*rpmProblemSetFree)(rpmProblemSet);
    int (*rpmReadSignatureOld)(FD_t, Header *, short);

    /*
     * RPM 4.1.x and 4.2 only functions
     */
    int (*rpmReadPackageFile) (rpmts, FD_t, const char *, Header *);
    int (*rpmReadSignature)(FD_t, Header *, short, const char **);
    rpmdbMatchIterator (*rpmtsInitIterator) (const rpmts, int, const void *,
                                             size_t);
    rpmts (*rpmtsCreate) (void);
    void (*rpmtsSetRootDir) (rpmts, const char *);
    int (*rpmtsAddInstallElement) (rpmts, Header, const void *,
                                   int, rpmRelocation *);
    int (*rpmtsAddEraseElement) (rpmts, Header, int);
    int (*rpmtsCheck) (rpmts);
    int (*rpmtsOrder) (rpmts);
    int (*rpmtsClean) (rpmts);
    int (*rpmtsRun) (rpmts, rpmps, int);
    int (*rpmtsSetNotifyCallback) (rpmts, rpmCallbackFunction, void *);
    int (*rpmtsSetFlags) (rpmts, int);
    rpmts (*rpmtsFree) (rpmts);
    rpmps (*rpmtsProblems) (rpmts);
    rpmps (*rpmpsFree) (rpmps);
    int (*rpmtsVSFlags) (rpmts);
    int (*rpmtsSetVSFlags) (rpmts, int);
};

struct _RCRpmmanClass {
    RCPackmanClass parent_class;
};

GType rc_rpmman_get_type (void);

RCRpmman *rc_rpmman_new (void);

#endif /* _RC_RPMMAN_H */
