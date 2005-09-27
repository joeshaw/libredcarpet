/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-query.c
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

#include <config.h>
#include "rc-query.h"

#include <string.h>
#include "rc-debug.h"

static struct QueryTypeStrings {
    RCQueryType type;
    const char *str;
} query2str[] = {

    { RC_QUERY_EQUAL, "is" },
    { RC_QUERY_EQUAL, "eq" },
    { RC_QUERY_EQUAL, "==" },
    { RC_QUERY_EQUAL, "=" },

    { RC_QUERY_NOT_EQUAL, "is not" },
    { RC_QUERY_NOT_EQUAL, "ne" },
    { RC_QUERY_NOT_EQUAL, "!=" },

    { RC_QUERY_CONTAINS, "contains" },

    { RC_QUERY_CONTAINS_WORD, "contains_word" },

    { RC_QUERY_NOT_CONTAINS, "!contains" },

    { RC_QUERY_NOT_CONTAINS_WORD, "!contains_word" },
    
    { RC_QUERY_GT, ">" },
    { RC_QUERY_GT, "gt" },

    { RC_QUERY_LT, "<" },
    { RC_QUERY_LT, "lt" },

    { RC_QUERY_GT_EQ, ">="},
    { RC_QUERY_GT_EQ, "gteq"},

    { RC_QUERY_LT_EQ, "<="},
    { RC_QUERY_LT_EQ, "lteq"},

    { RC_QUERY_BEGIN_OR, "begin-or" },
    { RC_QUERY_END_OR,   "end-or" },

    { RC_QUERY_INVALID,   NULL }
};
        

RCQueryType
rc_query_type_from_string (const char *str)
{
    int i;
    g_return_val_if_fail (str && *str, RC_QUERY_INVALID);
    for (i = 0; query2str[i].type != RC_QUERY_INVALID; ++i) {
        if (! g_strcasecmp (str, query2str[i].str))
            return query2str[i].type;
    }
    return RC_QUERY_INVALID;
}

const char *
rc_query_type_to_string (RCQueryType type)
{
    int i;

    g_return_val_if_fail (type != RC_QUERY_INVALID, "[Invalid]");
    g_return_val_if_fail (type != RC_QUERY_LAST, "[Invalid:Last]");

    for (i = 0; query2str[i].type != RC_QUERY_LAST; ++i) {
        if (query2str[i].type == type)
            return query2str[i].str;
    }

    return "[Invalid:NotFound]";
}

gboolean
rc_query_type_int_compare (RCQueryType type,
                           gint x, gint y)
{
    g_return_val_if_fail (type != RC_QUERY_CONTAINS
                          && type != RC_QUERY_NOT_CONTAINS, FALSE);

    switch (type) {

    case RC_QUERY_EQUAL:
        return x == y;
        
    case RC_QUERY_NOT_EQUAL:
        return x != y;

    case RC_QUERY_GT:
        return x > y;

    case RC_QUERY_LT:
        return x < y;

    case RC_QUERY_GT_EQ:
        return x >= y;

    case RC_QUERY_LT_EQ:
        return x <= y;

    default:
        g_assert_not_reached ();
        return FALSE;
    }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCQueryPart *
rc_query_part_new (const char  *key,
                   RCQueryType  type,
                   const char  *query_str)
{
    RCQueryPart *part;

    g_return_val_if_fail (type != RC_QUERY_LAST, NULL);
    g_return_val_if_fail (type != RC_QUERY_INVALID, NULL);

    if (!(type == RC_QUERY_BEGIN_OR || type == RC_QUERY_END_OR)) {
        g_return_val_if_fail (key != NULL, NULL);
        g_return_val_if_fail (query_str != NULL, NULL);
    }

    part = g_new0 (RCQueryPart, 1);
    part->key = key != NULL ? g_strdup (key) : NULL;
    part->type = type;
    part->query_str = query_str != NULL ? g_strdup (query_str) : NULL;

    return part;
}

void
rc_query_part_free (RCQueryPart *part)
{
    g_return_if_fail (part != NULL);

    g_free (part->key);
    g_free (part->query_str);

    g_free (part);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static char *
strstr_word (const char *haystack, const char *needle)
{
    const char *hay = haystack;
    while (*hay) {
        char *n = strstr (hay, needle);
        gboolean failed = FALSE;

        if (n == NULL)
            return NULL;

        if (n != haystack) {
            char *prev = g_utf8_prev_char (n);
            if (g_unichar_isalnum (g_utf8_get_char (prev)))
                failed = TRUE;
        }

        if (! failed) {
            char *next = n + strlen (needle);
            if (*next && g_unichar_isalnum (g_utf8_get_char (next)))
                failed = TRUE;
        }

        if (! failed)
            return n;

        hay = g_utf8_next_char (hay);
    }

    return NULL;
}

gboolean
rc_query_match_string (RCQueryPart *part,
                       const char *str)
{
    if (part->type == RC_QUERY_CONTAINS) {
        return strstr (str, part->query_str) != NULL;
    } else if (part->type == RC_QUERY_NOT_CONTAINS) {
        return strstr (str, part->query_str) == NULL;
    } else if (part->type == RC_QUERY_CONTAINS_WORD) {
        return strstr_word (str, part->query_str) != NULL;
    } else if (part->type == RC_QUERY_NOT_CONTAINS_WORD) {
        return strstr_word (str, part->query_str) == NULL;
    }
    
    if (rc_query_type_int_compare (part->type,
                                    strcmp (part->query_str, str),
                                    0))
        return TRUE;
    else
        return rc_query_type_int_compare (part->type,
                                           g_pattern_match_simple (
                                               part->query_str, str),
                                           TRUE);
}

gboolean
rc_query_match_string_ci (RCQueryPart *part,
                          const char *str)
{
    char *str_folded;
    int rv;

    if (part->query_str_folded == NULL) {
        part->query_str_folded = g_utf8_casefold (part->query_str, -1);
    }

    str_folded = g_utf8_casefold (str, -1);

    if (part->type == RC_QUERY_CONTAINS) {
        rv =  strstr (str_folded, part->query_str_folded) != NULL;
    } else if (part->type == RC_QUERY_NOT_CONTAINS) {
        rv = strstr (str_folded, part->query_str_folded) == NULL;
    } else if (part->type == RC_QUERY_CONTAINS_WORD) {
        rv = strstr_word (str_folded, part->query_str_folded) != NULL;
    } else if (part->type == RC_QUERY_NOT_CONTAINS_WORD) {
        rv = strstr_word (str_folded, part->query_str_folded) == NULL;
    } else {
        rv = rc_query_type_int_compare (part->type,
                                         strcmp (part->query_str_folded, str_folded),
                                         0);

        if (!rv) {
            rv = rc_query_type_int_compare (part->type,
                                             g_pattern_match_simple (
                                                 part->query_str_folded,
                                                 str_folded),
                                             TRUE);
        }
    }

    g_free (str_folded);

    return rv;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_query_validate_bool (RCQueryPart *part)
{
    if (part->type != RC_QUERY_EQUAL
        && part->type != RC_QUERY_NOT_EQUAL)
        return FALSE;

    if (! g_strcasecmp (part->query_str, "true")) {
        part->data = GINT_TO_POINTER (1);
    } else if (! g_strcasecmp (part->query_str, "false")) {
        part->data = GINT_TO_POINTER (0);
    } else {
        return FALSE;
    }

    return TRUE;
}

gboolean
rc_query_match_bool (RCQueryPart *part,
                     gboolean      val)
{
    gboolean match;

    match = part->data == (val ? GINT_TO_POINTER (1) : GINT_TO_POINTER (0));
    
    if (part->type == RC_QUERY_EQUAL)
        return match;
    else if (part->type == RC_QUERY_NOT_EQUAL)
        return ! match;

    g_assert_not_reached ();
    return FALSE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static RCQueryEngine *
lookup_engine (const char     *key, 
               RCQueryEngine *query_engine)
{
    int i;

    for (i = 0; query_engine[i].key != NULL; ++i) {
        if (! g_strcasecmp (key, query_engine[i].key))
            return &query_engine[i];
    }

    return NULL;
}

gboolean
rc_query_begin (RCQueryPart *query_parts,
                RCQueryEngine *query_engine)
{
    int i, or_depth = 0;

    g_return_val_if_fail (query_engine != NULL, FALSE);

    if (query_parts == NULL)
        return TRUE;

    /* Sweep through and validate the parts. */
    for (i = 0; query_parts[i].type != RC_QUERY_LAST; ++i) {

        RCQueryEngine *eng = NULL;

        if (query_parts[i].type == RC_QUERY_BEGIN_OR) {

            if (or_depth > 0) {
                rc_debug (RC_DEBUG_LEVEL_WARNING, 
                          "Nested 'or' not allowed in queries.");
                return FALSE;
            }

            ++or_depth;

        } else if (query_parts[i].type == RC_QUERY_END_OR) {

            --or_depth;

            if (or_depth < 0) {
                rc_debug (RC_DEBUG_LEVEL_WARNING,
                          "Extra 'or' terminator found in query.");
                return FALSE;
            }

        } else {

            eng = lookup_engine (query_parts[i].key, query_engine);

            if (eng) {
                
                if (eng->validate && ! eng->validate (&query_parts[i]))
                    return FALSE;

                if (eng->match == NULL) {
                    rc_debug (RC_DEBUG_LEVEL_WARNING,
                              "Key \"%s\" lacks a match function.", query_parts[i].key);
                    return FALSE;
                }

           } else {
               rc_debug (RC_DEBUG_LEVEL_WARNING,
                         "Unknown part \"%s\"", query_parts[i].key);
                return FALSE;
            }
        }

        query_parts[i].query_str_folded = NULL;
        query_parts[i].engine           = eng;
        query_parts[i].processed        = FALSE;
    }

    if (or_depth > 0) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Unterminated 'or' in expression.");
        return FALSE;
    }

    /* If we made it this far, our query must be OK.  Call each part initializer. */
    for (i = 0; query_parts[i].type != RC_QUERY_LAST; ++i) {
        
        if (query_parts[i].engine && query_parts[i].engine->initialize)
            query_parts[i].engine->initialize (&query_parts[i]);

     
    }

    return TRUE;
}

gboolean
rc_query_match (RCQueryPart   *query_parts,
                RCQueryEngine *query_engine,
                gpointer        data)
{
    int i;
    int or_depth = 0, or_expr_count = 0;
    gboolean or_val = FALSE;

    g_return_val_if_fail (query_engine != NULL, FALSE);

    if (query_parts == NULL)
        return TRUE;

    for (i = 0; query_parts[i].type != RC_QUERY_LAST; ++i) {

        if (! query_parts[i].processed) {

            if (query_parts[i].type == RC_QUERY_BEGIN_OR) {

                ++or_depth;
                or_expr_count = 0;
                or_val = FALSE;

            } else if (query_parts[i].type == RC_QUERY_END_OR) {

                --or_depth;
                if (or_expr_count > 0 && ! or_val)
                    return FALSE;

            } else {

                RCQueryEngine *engine = query_parts[i].engine;
                gboolean matched;
            
                g_assert (engine != NULL);
                g_assert (engine->match != NULL);

                matched = engine->match (&query_parts[i], data);

                if (or_depth > 0) {
                    ++or_expr_count;
                    if (matched)
                        or_val = TRUE;
                } else {
                    if (! matched)
                        return FALSE;
                }
            }
        }
    }

    return TRUE;
}

void
rc_query_end (RCQueryPart *query_parts,
              RCQueryEngine *query_engine)
{
    int i;

    g_return_if_fail (query_engine != NULL);

    if (query_parts == NULL)
        return;

    for (i = 0; query_parts[i].key != NULL; ++i) {

        if (query_parts[i].engine && query_parts[i].engine->finalize)
            query_parts[i].engine->finalize (&query_parts[i]);

        query_parts[i].engine = NULL;

        if (query_parts[i].query_str_folded) {
            g_free (query_parts[i].query_str_folded);
            query_parts[i].query_str_folded = NULL;
        }
    }
}
