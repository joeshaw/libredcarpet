/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-query.h
 *
 * Copyright (C) 2002 Ximian, Inc.
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

#ifndef __RC_QUERY_H__
#define __RC_QUERY_H__

#include <glib.h>

typedef enum {
    RC_QUERY_EQUAL,
    RC_QUERY_NOT_EQUAL,
    RC_QUERY_CONTAINS,
    RC_QUERY_CONTAINS_WORD,
    RC_QUERY_NOT_CONTAINS,
    RC_QUERY_NOT_CONTAINS_WORD,
    RC_QUERY_GT,
    RC_QUERY_LT,
    RC_QUERY_GT_EQ,
    RC_QUERY_LT_EQ,
    RC_QUERY_BEGIN_OR,
    RC_QUERY_END_OR,
    RC_QUERY_LAST,
    RC_QUERY_INVALID
} RCQueryType;

typedef struct _RCQueryPart RCQueryPart;
typedef struct _RCQueryEngine RCQueryEngine;

typedef gboolean (*RCQueryEngineValidateFn)   (RCQueryPart *);
typedef void     (*RCQueryEngineInitializeFn) (RCQueryPart *);
typedef void     (*RCQueryEngineFinalizeFn)   (RCQueryPart *);
typedef gboolean (*RCQueryEngineMatchFn)      (RCQueryPart *, gpointer data);

struct _RCQueryEngine {

    const char *key;

    RCQueryEngineValidateFn   validate;
    RCQueryEngineInitializeFn initialize;
    RCQueryEngineFinalizeFn   finalize;
    RCQueryEngineMatchFn      match;
};

struct _RCQueryPart {

    char        *key;
    RCQueryType type;
    char        *query_str;

    /* for internal use only */
    char          *query_str_folded;
    RCQueryEngine *engine;
    gpointer       data;
    guint          processed : 1;
};


RCQueryType rc_query_type_from_string (const char *str);

const char *rc_query_type_to_string   (RCQueryType type);

gboolean    rc_query_type_int_compare (RCQueryType type,
                                       gint x, gint y);


RCQueryPart *rc_query_part_new (const char  *key,
                                RCQueryType  type,
                                const char  *query_str);
void         rc_query_part_free (RCQueryPart *part);

/* Useful pre-defined RCQueryEngine components. */

gboolean    rc_query_match_string    (RCQueryPart *part, const char *str);
gboolean    rc_query_match_string_ci (RCQueryPart *part, const char *str); /* case-insensitive */

gboolean    rc_query_validate_bool   (RCQueryPart *part);
gboolean    rc_query_match_bool      (RCQueryPart *part, gboolean val);


/* The query_parts array should be terminated by type RC_QUERY_LAST.
   The query_engine array should be NULL-key terminated. */

gboolean    rc_query_begin (RCQueryPart   *query_parts,
                            RCQueryEngine *query_engine);

gboolean    rc_query_match (RCQueryPart   *query_parts,  
                            RCQueryEngine *query_engine, 
                            gpointer        data);

void        rc_query_end   (RCQueryPart   *query_parts,
                            RCQueryEngine *query_engine);

#endif /* __RC_QUERY_H__ */
