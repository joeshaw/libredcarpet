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
//    DB *db;
    void *db;
    const char *indexname;
} rc_dbiIndex;

#endif
