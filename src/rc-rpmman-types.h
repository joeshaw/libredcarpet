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

#include <rpm/rpmlib.h>

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
