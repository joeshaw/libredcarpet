/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-rpmman-types.h
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 */

#ifndef _RC_RPMMAN_TYPES
#define _RC_RPMMAN_TYPES

#include <rpmlib.h>

/*
 * rpmProblem is a real mess.  It's changed drastically between versions.
 * Here is an attempt to support all three.
 */

/* This one is from RPM 4.1 to the present */
typedef struct {
    char *pkgNEVR;
    char *altNEVR;
    fnpyKey key;
    rpmProblemType type;
    int ignoreProblem;
    char *str1;
    unsigned long ulong1;
} * RCrpmProblem;

/* This one was from RPM 4.0.2 to 4.0.4 */
typedef struct {
    const char *pkgNEVR;
    const char *altNEVR;
    const void *key;
    Header h;
    rpmProblemType type;
    int ignoreProblem;
    const char *str1;
    unsigned long ulong1;
} * RCrpmProblemOld;

/* This one was from RPM 3.0.x through RPM 4.0.1 */
typedef struct {
    Header h;
    Header altH;
    rpmProblemType type;
    int ignoreProblem;
    const char *str1;
    unsigned long ulong1;
} RCrpmProblemOlder;

#if RPM_VERSION < 40100
/* Definitions for RPM 4.1 and 4.2 */

typedef struct rpmts_s *rpmts;
typedef struct rpmps_s *rpmps;

/* We poke at the internals of rpmps */
struct rpmps_s {
    int numProblems;
    int numProblemsAlloced;
    rpmProblem probs;
    int nrefs;
};

typedef enum rpmVSFlags_e {
    RPMVSF_DEFAULT      = 0,
    RPMVSF_NOHDRCHK     = (1 <<  0),
    RPMVSF_NEEDPAYLOAD  = (1 <<  1),
    /* bit(s) 2-7 unused */
    RPMVSF_NOSHA1HEADER = (1 <<  8),
    RPMVSF_NOMD5HEADER  = (1 <<  9),    /* unimplemented */
    RPMVSF_NODSAHEADER  = (1 << 10),
    RPMVSF_NORSAHEADER  = (1 << 11),    /* unimplemented */
    /* bit(s) 12-15 unused */
    RPMVSF_NOSHA1       = (1 << 16),    /* unimplemented */
    RPMVSF_NOMD5        = (1 << 17),
    RPMVSF_NODSA        = (1 << 18),
    RPMVSF_NORSA        = (1 << 19)
    /* bit(s) 16-31 unused */
} rpmVSFlags;

#define _RPMVSF_NODIGESTS       \
  ( RPMVSF_NOSHA1HEADER |       \
    RPMVSF_NOMD5HEADER |        \
    RPMVSF_NOSHA1 |             \
    RPMVSF_NOMD5 )

#define _RPMVSF_NOSIGNATURES    \
  ( RPMVSF_NODSAHEADER |        \
    RPMVSF_NORSAHEADER |        \
    RPMVSF_NODSA |              \
    RPMVSF_NORSA )

#else
/* Definitions for RPM 4.0.4 */

typedef struct rpmTransactionSet_s *rpmTransactionSet;
typedef struct rpmProblemSet_s *rpmProblemSet;

/* We poke at the internals of rpmDependencyConflict */
typedef struct rpmDependencyConflict_s {
    const char * byName;
    const char * byVersion;
    const char * byRelease;
    Header byHeader;
    /*
     * These needs fields are misnamed -- they are used for the package
     * which isn't needed as well.
     */
    const char * needsName;
    const char * needsVersion;
    int needsFlags;
    const void ** suggestedPackages;
    enum {
        RPMDEP_SENSE_REQUIRES,
        RPMDEP_SENSE_CONFLICTS
    } sense;
} * rpmDependencyConflict;

#endif

/* rpmlib.h version 4.0 */

typedef struct _rpmdbMatchIterator * rc_rpmdbMatchIterator;

#define RPMDBI_PACKAGES         0
#define RPMDBI_DEPENDS          1
#define RPMDBI_LABEL            2
#define RPMDBI_ADDED            3
#define RPMDBI_REMOVED          4
#define RPMDBI_AVAILABLE        5

/* dbindex.h version 3.0.5 */

typedef struct {
    unsigned int recOffset;
    unsigned int fileNumber;
} rc_dbiIndexRecord;

typedef struct {
    rc_dbiIndexRecord *recs;
    int count;
} rc_dbiIndexSet;

typedef struct {
  /*    DB *db; */
    void *db;
    const char *indexname;
} rc_dbiIndex;

/* not in rpm < 3.0.4, I think */

typedef struct rc_FDIO_s rc_FDIO_t;

struct rc_FDIO_s {
  void *        read;
  void *       write;
  void *        seek;
  void *       close;

  void *         _fdref;
  void *       _fdderef;
  void *         _fdnew;
  void *      _fileno;

  void *        _open;
  void *       _fopen;
  void *     _ffileno;
  void *      _fflush;

  void *       _mkdir;
  void *       _chdir;
  void *       _rmdir;
  void *      _rename;
  void *      _unlink;
};

#endif
