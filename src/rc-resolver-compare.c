/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-resolver-compare.c
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
#include "rc-resolver-compare.h"

static int
num_cmp (double a, double b)
{
    return (b < a) - (a < b);
}

static int
rev_num_cmp (double a, double b)
{
    return (a < b) - (b < a);
}

static double
churn_factor (RCResolverContext *a)
{
    return a->upgrade_count
        + 2.0 * a->install_count
        + 4.0 * a->uninstall_count;
}

int
rc_resolver_context_partial_cmp (RCResolverContext *a,
                                 RCResolverContext *b)
{
    int cmp;

    g_return_val_if_fail (a != NULL, 0);
    g_return_val_if_fail (b != NULL, 0);

    if (a == b)
        return 0;

    /* High numbers are good... we don't want solutions containing
       low-priority channels. */
    cmp = num_cmp (a->min_priority, b->min_priority);
    if (cmp)
        return cmp;
    
    /* High numbers are bad.  Less churn is better. */
    cmp = rev_num_cmp (churn_factor (a), churn_factor (b));
    if (cmp)
        return cmp;

    /* High numbers are bad.  Bigger #s means more penalties. */
    cmp = rev_num_cmp (a->other_penalties, b->other_penalties);
    if (cmp)
        return cmp;
    
    return 0;
}

int
rc_resolver_context_cmp (RCResolverContext *a,
                         RCResolverContext *b)
{
    int cmp;

    g_return_val_if_fail (a != NULL, 0);
    g_return_val_if_fail (b != NULL, 0);

    if (a == b)
        return 0;

    cmp = rc_resolver_context_partial_cmp (a, b);
    if (cmp)
        return cmp;

    /* High numbers are bad.  Smaller downloads are best. */
    cmp = rev_num_cmp (a->download_size, b->download_size);
    if (cmp)
        return cmp;

    /* High numbers are bad.  Less disk space consumed is good. */
    cmp = rev_num_cmp (a->install_size, b->install_size);
    if (cmp)
        return cmp;

    return 0;
}
