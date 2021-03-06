/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-debman-general.c
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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

#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "rc-debman-general.h"
#include "rc-debug.h"
#include "rc-package.h"
#include "rc-dep-or.h"

/*
 * Takes a string of format [<epoch>:]<version>[-<release>], and returns those
 * three elements, if they exist
 */

void
rc_debman_parse_version (const gchar *input, guint32 *epoch, gchar **version,
                         gchar **release)
{
    gchar *epoch_ptr = NULL;
    gchar *version_ptr = NULL;
    gchar *release_ptr = NULL;
    gchar *working;

    gchar *tmp;

    working = alloca (strlen (input) + 1);

    strcpy (working, input);

    *epoch = 0;
    *version = NULL;
    *release = NULL;

    tmp = strchr (working, ':');

    if (tmp) {
        /* There's an epoch */
        *tmp = '\0';
        epoch_ptr = working;
        version_ptr = tmp + 1;
    } else {
        /* The string begins with a version */
        version_ptr = working;
    }

    tmp = strrchr (version_ptr, '-');

    if (tmp) {
        /* There's a release */
        *tmp = '\0';
        release_ptr = tmp + 1;
    }

    if (epoch_ptr) {
        *epoch = strtoul (epoch_ptr, NULL, 10);
    }

    *version = g_strdup (version_ptr);

    if (release_ptr) {
        *release = g_strdup (release_ptr);
    }
//    rc_debug (RC_DEBUG_LEVEL_DEBUG, "-- parsed %s into %d %s %s\n",
//              input, *epoch, *version, *release);
}

/*
 * Given a line from a debian control file (or from /var/lib/dpkg/status), and
 * a pointer to a RCPackageDepSList, fill in the dependency information.  It's
 * ugly but memprof says it doesn't leak.
 */

RCPackageDepSList *
rc_debman_fill_depends (gchar *input, gboolean pre)
{
    RCPackageDepSList *list = NULL;
    gchar **deps;
    guint i;

    /* All evidence indicates that the fields are comma-space separated, but if
       that ever turns out to be incorrect, we'll have to do this part more
       carefully. */
    /* Evidence has turned out incorrect; there can be arbitrary whitespace
       inserted anywhere within the depends fields -- or lack of said
       whitespace. Man, Debian is like a piece of bad fruit -- it looks great
       on the outside, but when you dig into it, you see the rot ;-) - vlad */
    /* Note that this can probably be made much prettier if we use the GNU
       regexp package; but I have no idea how much slower/faster that would
       be. - vlad */

    deps = g_strsplit (input, ",", 0);

    for (i = 0; deps[i]; i++) {
        gchar **elems;
        guint j;
        RCPackageDepSList *dep = NULL;
        RCPackageDep *the_dep;
        gchar *curdep;

        curdep = g_strstrip (deps[i]);
        elems = g_strsplit (curdep, "|", 0);

        /* For the most part, there's only one element per dep, except for the
           rare case of OR'd deps. */
        for (j = 0; elems[j]; j++) {
            RCPackageDep *depi;
            gchar *curelem;
            gchar *s1, *s2;

            gchar *depname = NULL, *deprel = NULL, *depvers = NULL;

            curelem = g_strstrip (elems[j]);
            /* A space separates the name of the dep from the relationship and
               version. */
            /* No. "alsa-headers(<<0.5)" is perfectly valid. We have to parse
             * this by hand */

            /* First we grab the dep name */
            s1 = curelem;
            while (*s1 && !isspace(*s1) && *s1 != '(') s1++;
            /* s1 now points to the first character after the full name */
            depname = g_malloc (s1 - curelem + 1);
            strncpy (depname, curelem, s1 - curelem);
            depname[s1 - curelem] = '\0';

            if (*s1) {          /* we have a relation */
                /* Skip until the opening brace */
                while (*s1 && *s1 != '(') s1++;
                s1++; /* skip the brace */
                while (*s1 && isspace(*s1)) s1++; /* skip spaces before rel */
                s2 = s1;
                while (*s2 == '=' || *s2 == '>' || *s2 == '<') s2++;
                /* s2 now points to the first char after the relation */
                deprel = g_malloc (s2 - s1 + 1);
                strncpy (deprel, s1, s2 - s1);
                deprel[s2 - s1] = '\0';

                while (*s2 && isspace (*s2)) s2++;
                /* Now we have the version string start */
                s1 = s2;
                while (*s2 && !isspace (*s2) && *s2 != ')') s2++;
                depvers = g_malloc (s2 - s1 + 1);
                strncpy (depvers, s1, s2 - s1);
                depvers[s2 - s1] = '\0';

                /* We really shouldn't ignore the rest of the string, but we
                   do. */
                /* There shouldn't be anything after this, except a closing
                   paren */
            }

            if (!deprel) {
                /* There's no version in this dependency, just a name. */
                depi = rc_package_dep_new (depname, 0, 0, NULL, NULL,
                                           RC_RELATION_ANY, 
                                           RC_CHANNEL_ANY,
                                           pre, FALSE);
            } else {
                /* We've got to parse the rest of this mess. */
                guint relation = RC_RELATION_ANY;
                gint32 epoch;
                gchar *version, *release;

                /* The possible relations are "=", ">>", "<<", ">=", and "<=",
                   decide which apply */
                switch (deprel[0]) {
                case '=':
                    relation = RC_RELATION_EQUAL;
                    break;
                case '>':
                    relation = RC_RELATION_GREATER;
                    break;
                case '<':
                    relation = RC_RELATION_LESS;
                    break;
                }

                if (deprel[1] && (deprel[1] == '=')) {
                    relation |= RC_RELATION_EQUAL;
                }

                g_free (deprel);

                /* Break the single version string up */
                rc_debman_parse_version (depvers, &epoch, &version,
                                         &release);

                g_free (depvers);

                depi = rc_package_dep_new (depname, 1, epoch, version,
                                           release, relation, 
                                           RC_CHANNEL_ANY,
                                           FALSE, FALSE);
                g_free (version);
                g_free (release);
            }

            g_free (depname);
            dep = g_slist_append (dep, depi);
        }

        g_strfreev (elems);

        if (dep) {
            if (dep->next) {
                RCDepOr *or = rc_dep_or_new (dep);
                the_dep = rc_dep_or_new_provide (or);
            } else {
                the_dep = dep->data;
                g_slist_free (dep);
            }
            list = g_slist_append (list, the_dep);
        }
    }

    g_strfreev (deps);

    return (list);
}

RCPackageSection
rc_debman_section_to_package_section (const gchar *section)
{
    switch (section[0]) {
    case 'a':
        if (!strcmp (section, "admin")) {
            return (RC_SECTION_SYSTEM);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'b':
        if (!strcmp (section, "base")) {
            return (RC_SECTION_SYSTEM);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'c':
        if (!strcmp (section, "comm")) {
            return (RC_SECTION_INTERNET);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'd':
        if (!strcmp (section, "devel")) {
            return (RC_SECTION_DEVEL);
        } else if (!strcmp (section, "doc")) {
            return (RC_SECTION_DOC);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'e':
        if (!strcmp (section, "editors")) {
            return (RC_SECTION_UTIL);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'g':
        if (!strcmp (section, "games")) {
            return (RC_SECTION_GAME);
        } else if (!strcmp (section, "graphics")) {
            return (RC_SECTION_IMAGING);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'i':
        if (!strcmp (section, "interpreters")) {
            return (RC_SECTION_DEVELUTIL);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'l':
        if (!strcmp (section, "libs")) {
            return (RC_SECTION_LIBRARY);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'm':
        if (!strcmp (section, "mail")) {
            return (RC_SECTION_PIM);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'n':
        if (!strcmp (section, "net") ||
            !strcmp (section, "news"))
        {
            return (RC_SECTION_INTERNET);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'o':
        if (!strcmp (section, "oldlibs")) {
            return (RC_SECTION_LIBRARY);
        } else {
            return (RC_SECTION_MISC);
        }
    case 's':
        if (!strcmp (section, "shells")) {
            return (RC_SECTION_SYSTEM);
        } else if (!strcmp (section, "sound")) {
            return (RC_SECTION_MULTIMEDIA);
        } else {
            return (RC_SECTION_MISC);
        }
    case 't':
        if (!strcmp (section, "text")) {
            return (RC_SECTION_UTIL);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'u':
        if (!strcmp (section, "utils")) {
            return (RC_SECTION_UTIL);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'w':
        if (!strcmp (section, "web")) {
            return (RC_SECTION_INTERNET);
        } else {
            return (RC_SECTION_MISC);
        }
    case 'x':
        if (!strcmp (section, "x11")) {
            return (RC_SECTION_XAPP);
        } else {
            return (RC_SECTION_MISC);
        }
    default:
        return (RC_SECTION_MISC);
    }
}
