/*
 * rc-debman-general.c: Things that are useful even if dpkg support isn't enabled
 *
 *
 *  Copyright (C) 2000 Helix Code Inc.
 *  Copyright (C) 2001 Ximian Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include <glib.h>

#include <libredcarpet/rc-debman-general.h>

/*
 * Takes a string of format [<epoch>:]<version>[-<release>], and returns those
 * three elements, if they exist
 */

void
rc_debman_parse_version (gchar *input, guint32 *epoch, gchar **version,
                         gchar **release)
{
    gchar **t1, **t2;

    *epoch = 0;
    *version = NULL;
    *release = NULL;

    /* Check if there's an epoch set */
    t1 = g_strsplit (input, ":", 1);

    if (t1[1]) {
        /* There is an epoch */
        *epoch = strtoul (t1[0], NULL, 10);
        /* Split the version and release */
        t2 = g_strsplit (t1[1], "-", 1);
    } else {
        /* No epoch, split the version and release */
        t2 = g_strsplit (t1[0], "-", 1);
    }

    /* Version definitely gets set to something */
    *version = g_strdup (t2[0]);
    /* Set release (this may be dup'ing NULL) */
    *release = g_strdup (t2[1]);

    g_strfreev (t2);
    g_strfreev (t1);
}

/*
 * Given a line from a debian control file (or from /var/lib/dpkg/status), and
 * a pointer to a RCPackageDepSList, fill in the dependency information.  It's
 * ugly but memprof says it doesn't leak.
 */

RCPackageDepSList *
rc_debman_fill_depends (gchar *input, RCPackageDepSList *list)
{
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
        RCPackageDep *dep = NULL;
        gchar *curdep;

        curdep = g_strstrip (deps[i]);
        elems = g_strsplit (curdep, "|", 0);

        /* For the most part, there's only one element per dep, except for the
           rare case of OR'd deps. */
        for (j = 0; elems[j]; j++) {
            RCPackageDepItem *depi;
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
                depi = rc_package_dep_item_new (depname, 0, NULL, NULL,
                                                RC_RELATION_ANY);
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

                depi = rc_package_dep_item_new (depname, epoch, version,
                                                release, relation);
                g_free (version);
                g_free (release);
            }

            g_free (depname);
            dep = g_slist_append (dep, depi);
        }

        g_strfreev (elems);

        list = g_slist_append (list, dep);
    }

    g_strfreev (deps);

    return (list);
}

