/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-arch.c
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

#include <sys/utsname.h>

#include <glib.h>
#include <string.h>

#include "rc-arch.h"

typedef struct {
    const char *name;
    RCArch      arch;
} RCArchAndName;

/* The "canonical" name we want to use for a given architecture should
 * be the first entry for that architecture in this table */
static RCArchAndName arch_table[] = {
    { "noarch",  RC_ARCH_NOARCH },
    { "all",     RC_ARCH_NOARCH },
    { "i386",    RC_ARCH_I386 },
    { "i486",    RC_ARCH_I486 },
    { "i586",    RC_ARCH_I586 },
    { "i686",    RC_ARCH_I686 },
    { "athlon",  RC_ARCH_ATHLON },
    { "ppc",     RC_ARCH_PPC },
    { "ppc64",   RC_ARCH_PPC64 },
    { "sparc",   RC_ARCH_SPARC },
    { "sun4c",   RC_ARCH_SPARC },
    { "sun4d",   RC_ARCH_SPARC },
    { "sun4m",   RC_ARCH_SPARC },
    { "sparc64", RC_ARCH_SPARC64 },
    { "sun4u",   RC_ARCH_SPARC64 },
    { "sparcv9", RC_ARCH_SPARC64 },
    { "unknown", RC_ARCH_UNKNOWN },
    { 0 },
};

typedef struct {
    RCArch arch;
    RCArch compat_arch;
} RCArchAndCompatArch;

/* _NOARCH should never be listed in this table (other than as the
 * terminator), as it will automatically be added.  Every architecture
 * is implicitly compatible with itself.  Compatible architectures
 * should be listed in most-preferred to least-preferred order. */
static RCArchAndCompatArch compat_table[] = {
    { RC_ARCH_I486,    RC_ARCH_I386 },
    { RC_ARCH_I586,    RC_ARCH_I486 },
    { RC_ARCH_I586,    RC_ARCH_I386 },
    { RC_ARCH_I686,    RC_ARCH_I586 },
    { RC_ARCH_I686,    RC_ARCH_I486 },
    { RC_ARCH_I686,    RC_ARCH_I386 },
    { RC_ARCH_ATHLON,  RC_ARCH_I686 },
    { RC_ARCH_ATHLON,  RC_ARCH_I586 },
    { RC_ARCH_ATHLON,  RC_ARCH_I486 },
    { RC_ARCH_ATHLON,  RC_ARCH_I386 },
    { RC_ARCH_PPC64,   RC_ARCH_PPC },
    { RC_ARCH_SPARC64, RC_ARCH_SPARC },
    { RC_ARCH_NOARCH,  RC_ARCH_NOARCH },
};

RCArch
rc_arch_from_string (const char *arch_name)
{
    RCArchAndName *iter;

    g_return_val_if_fail (arch_name, RC_ARCH_UNKNOWN);

    iter = arch_table;
    while (iter->name) {
        if (!strcmp (iter->name, arch_name))
            return iter->arch;
        iter++;
    }

    return RC_ARCH_UNKNOWN;
}

const char *
rc_arch_to_string (RCArch arch)
{
    RCArchAndName *iter;

    iter = arch_table;
    while (iter->name) {
        if (iter->arch == arch)
            return iter->name;
        iter++;
    }

    return "unknown";
}

RCArch
rc_arch_get_system_arch (void)
{
    struct utsname buf;
    static gboolean checked = FALSE;
    static RCArch arch;

    if (!checked) {
        if (uname (&buf) < 0)
            arch = RC_ARCH_UNKNOWN;
        else
            arch = rc_arch_from_string (buf.machine);
        checked = TRUE;
    }
    
    return arch;
}

GSList *
rc_arch_get_compat_list (RCArch arch)
{
    RCArchAndCompatArch *iter;
    GSList *ret = NULL;

    ret = g_slist_prepend (ret, GINT_TO_POINTER (arch));

    iter = compat_table;
    while (iter->arch != RC_ARCH_NOARCH) {
        if (iter->arch == arch)
            ret = g_slist_prepend (ret, GINT_TO_POINTER (iter->compat_arch));
        iter++;
    }

    ret = g_slist_prepend (ret, GINT_TO_POINTER (RC_ARCH_NOARCH));
    ret = g_slist_reverse (ret);

    return ret;
}

gint
rc_arch_get_compat_score (GSList *compat_arch_list, RCArch arch)
{
    guint score = 0;
    GSList *iter;

    iter = compat_arch_list;
    while (iter) {
        if (GPOINTER_TO_INT (iter->data) == arch)
            return score;
        score++;
        iter = iter->next;
    }

    return (guint) -1;
}
