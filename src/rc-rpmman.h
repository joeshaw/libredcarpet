/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-rpmman.h
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

#ifndef _RC_RPMMAN_H
#define _RC_RPMMAN_H

#include <glib-object.h>
#include <gmodule.h>

#include <rpm/rpmlib.h>

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
    int (*readLead)(FD_t, struct rpmlead *);
    int (*rpmReadSignature)(FD_t, Header *, short);
    int (*rpmReadConfigFiles)(const char *, const char *);
    int (*rpmdbOpen)(const char *, rpmdb *, int, int);
    void (*rpmdbClose)(rpmdb);
#if RPM_VERSION >= 40002
    const char * (*rpmProblemString)(rpmProblem);
    const char * (*rpmProblemStringOld)(struct rpmProblem_s);
#else
    const char * (*rpmProblemString)(rpmProblem *);
    const char * (*rpmProblemStringOld)(rpmProblem);
#endif
    int (*rpmGetRpmlibProvides)(char ***, int **, char ***);
    int (*rpmExpandNumeric)(const char *);

    /*
     * RPMv3 only functions
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
     * RPMv4 only functions
     */

    rc_rpmdbMatchIterator (*rpmdbInitIterator)(rpmdb, int, const void *,
                                               size_t);
    int (*rpmdbGetIteratorCount)(rc_rpmdbMatchIterator);
    void (*rpmdbFreeIterator)(rc_rpmdbMatchIterator);
    Header (*rpmdbNextIterator)(rc_rpmdbMatchIterator);
    unsigned int (*rpmdbGetIteratorOffset)(rc_rpmdbMatchIterator);
};

struct _RCRpmmanClass {
    RCPackmanClass parent_class;
};

GType rc_rpmman_get_type (void);

RCRpmman *rc_rpmman_new (void);

#endif /* _RC_RPMMAN_H */
