/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-import.c
 *
 * Copyright (C) 2002 Ximian, Inc.
 *
 * Developed by Jon Trowbridge <trow@ximian.com>
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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
#include "rc-world-import.h"

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "rc-world.h"
#include "rc-channel-private.h"
#include "rc-debman-general.h"
#include "rc-debug.h"
#include "rc-util.h"
#include "rc-xml.h"
#include "xml-util.h"
#include "rc-package-update.h"

typedef enum _DepType DepType;

enum _DepType {
    DEP_TYPE_REQUIRES,
    DEP_TYPE_PROVIDES,
    DEP_TYPE_CONFLICTS,
    DEP_TYPE_SUGGESTS,
    DEP_TYPE_RECOMMENDS
};

static guint rc_world_parse_helix (RCWorld *, RCChannel *, gchar *);
static guint rc_world_parse_debian (RCWorld *, RCChannel *, gchar *);
static guint rc_world_parse_redhat (RCWorld *, RCChannel *, gchar *);

void
rc_world_add_packages_from_slist (RCWorld *world,
                                  RCPackageSList *slist)
{
    g_return_if_fail (world != NULL);
    
    rc_world_freeze (world);
    while (slist) {
        rc_world_add_package (world, (RCPackage *) slist->data);
        slist = slist->next;
    }
    rc_world_thaw (world);
}

guint
rc_world_add_packages_from_xml (RCWorld *world, RCChannel *channel, xmlNode *node)
{
    RCPackage *package;
    guint count = 0;
    
    g_return_val_if_fail (world != NULL, 0);

    rc_world_freeze (world);

    while (node && g_strcasecmp (node->name, "package")) {
        if (node->type != XML_ELEMENT_NODE) {
            node = node->next;
            continue;
        }

        node = node->xmlChildrenNode;
    }

    while (node) {

        if (! g_strcasecmp (node->name, "package")) {

            package = rc_xml_node_to_package (node, channel);
            if (package) {
                rc_world_add_package (world, package);
                rc_package_unref (package);
                ++count;
            }
            
        } 

        node = node->next;
    }

    rc_world_thaw (world);

    return count;
}

guint
rc_world_parse_channel (RCWorld *world,
                        RCChannel *channel, 
                        gchar *tbuf, 
                        int compressed_length)
{
    gchar *buf;
    GByteArray *byte_array = NULL;
    guint count = 0;

    g_assert (tbuf);

    if (compressed_length) {
        if (rc_uncompress_memory (tbuf, compressed_length, &byte_array)) {
            g_warning ("Uncompression failed");
            return -1;
        }

        buf = byte_array->data;
    } else {
        buf = tbuf;
    }

    switch (channel->type) {
    case RC_CHANNEL_TYPE_HELIX:
        count = rc_world_parse_helix (world, channel, buf);
        break;
    case RC_CHANNEL_TYPE_DEBIAN:
        count = rc_world_parse_debian (world, channel, buf);
        break;
    case RC_CHANNEL_TYPE_REDHAT:
        count = rc_world_parse_redhat (world, channel, buf);
        break;
    default:
        g_error ("Trying to parse unknown channel type!");
        break;
    }

    if (byte_array) {
        g_byte_array_free (byte_array, TRUE);
    }

    return count;
}

static guint
rc_world_parse_helix (RCWorld *world, RCChannel *channel, gchar *buf)
{
    xmlDoc *xml_doc;
    xmlNode *root;
    guint count = 0;

    g_assert (buf);

    if (getenv("RC_OLD_XML")) {
        xml_doc = xmlParseMemory (buf, strlen (buf));
        
        if (!xml_doc) {
            return 0;
        }
        
        root = xmlDocGetRootElement (xml_doc);
        count = rc_world_add_packages_from_xml (world, channel, root);
        
        xmlFreeDoc (xml_doc);
    }
    else {
        RCPackageSAXContext *ctx;
        RCPackageSList *packages;

        ctx = rc_package_sax_context_new(channel);
        rc_package_sax_context_parse_chunk(ctx, buf, strlen(buf));
        packages = rc_package_sax_context_done(ctx);

        count = g_slist_length(packages);
        
        rc_world_freeze(world);
        rc_world_add_packages_from_slist(world, packages);
        rc_world_thaw(world);

        rc_package_slist_unref (packages);
    }

    return count;
}

static void debian_packages_helper (gchar *mbuf, RCPackage *pkg, gchar *url_prefix);

static guint
rc_world_parse_debian (RCWorld *world, RCChannel *rcc, char *buf)
{
    /* Note that buf is NOT ours -- we can't munge it, and strdup'ing the
     * whole thing would be ufly. So, we let packages_query_helper handle
     */
    gchar *b, *ob;
    guint count = 0;

    rc_world_freeze (world);

    /* Keep looking for a "Packages: " */
    ob = buf;
    while ((b = strstr (ob, "Package: ")) != NULL) {
        RCPackage *p;
        if (b != buf && b[-1] != '\r' && b[-1] != '\n') {
            ob = b+1;
            continue;
        }
        p = rc_package_new ();
        debian_packages_helper (b, p, rcc->file_path);
        ((RCPackageUpdate *)(p->history->data))->package = p;
        ob = b+1;

        p->channel = rcc;

        rc_world_add_package (world, p);
        rc_package_unref (p);
        ++count;
    }

    rc_world_thaw (world);

    return count;
}

/*
 * This code needs to be kept super fast
 */

static void
debian_packages_helper (gchar *mbuf, RCPackage *pkg, gchar *url_prefix)
{
    gchar *ibuf = mbuf;
    gchar *buf, *p;
    RCPackageUpdate *up = NULL;
    gboolean desc = FALSE;
    int ilen;

    up = rc_package_update_new ();

    while (1) {
        /* FIXME -- handle all empty line with spaces */
        if (*ibuf == '\0' || *ibuf == '\n') break;

        buf = strchr (ibuf, '\n');
        if (buf == NULL) break; /* something bizzare  happened */

        ilen = buf - ibuf;      /* the length of the line */
        buf = ibuf;             /* start of buf again */
        buf[ilen] = '\0';       /* we probably shouldn't modify the string in-place, but
                                 * we restore it at the end */
        ibuf += ilen + 1;

        /* Downcase the sucker - I hate debian - it's SUPPOSED to be nicely
         * capitalized, but.. */
        p = buf;
        if (*p != ' ')
            while (*p != ':') *p++ = tolower(*p);

        if (!strncmp (buf, "package: ", strlen ("package: "))) {
            pkg->spec.name = g_strndup (buf + strlen ("package: "),
                                        ilen - strlen ("package: "));
        } else if (!strncmp (buf, "installed-size: ", strlen ("installed-size: "))) {
            up->installed_size = strtoul (buf +
                                          strlen ("installed-size: "),
                                          NULL, 10) * 1024;
        } else if (!strncmp (buf, "size: ", strlen ("size: "))) {
            up->package_size = strtoul(buf + strlen("size: "), NULL, 10);
        } else if (!strncmp (buf, "description: ", strlen ("description: "))) {
            pkg->summary = g_strndup (buf + strlen ("description: "),
                                      ilen - strlen ("description: "));
            desc = TRUE;
        } else if (*buf == ' ' && desc) {
            if (ilen == 2 && buf[1] == '.') {
                /* It's just a newline */
                if (pkg->description) {
                    int l = strlen (pkg->description);
                    gchar *tmp = g_malloc (l + 2);
                    strcpy (tmp, pkg->description);
                    tmp[l] = '\n';
                    tmp[l+1] = '\0';
                    g_free (pkg->description);
                    pkg->description = tmp;
                } else {
                    pkg->description = g_strndup ("\n", 1);
                }
            } else {
                /* More than a newline */
                if (pkg->description) {
                    int l = strlen (pkg->description);
                    gchar *tmp = g_malloc (l + ilen + 1);
                    strcpy (tmp, pkg->description);
                    tmp[l] = '\n';
                    strncpy (tmp + l + 1, buf+1, ilen-1);
                    tmp[l + ilen] = '\0';
                    g_free (pkg->description);
                    pkg->description = tmp;
                } else {
                    pkg->description = g_strndup (buf+1, ilen-1);
                }
            }
        } else if (!strncmp (buf, "version: ", strlen ("version: "))) {
            rc_debman_parse_version (buf + strlen ("Version: "),
                                     &pkg->spec.epoch, &pkg->spec.version,
                                     &pkg->spec.release);
        } else if (!strncmp (buf, "section: ", strlen ("section: "))) {
            gchar **tokens = g_strsplit (buf + strlen ("section: "), "/", -1);
            gint i = 0;
            gchar *sec;
            while (tokens[i+1] != NULL) i++;
            sec = g_strchomp (tokens[i]);
            p = sec;
            while (*p) *p++ = tolower(*p);
            pkg->section = rc_debman_section_to_package_section (sec);
#if 0
            if (!strcmp (sec, "admin")) {
                pkg->section = RC_SECTION_SYSTEM;
            } else if (!strcmp (sec, "base")) {
                pkg->section = RC_SECTION_SYSTEM;
            } else if (!strcmp (sec, "comm")) {
                pkg->section = RC_SECTION_INTERNET;
            } else if (!strcmp (sec, "devel")) {
                pkg->section = RC_SECTION_DEVEL;
            } else if (!strcmp (sec, "doc")) {
                pkg->section = RC_SECTION_DOC;
            } else if (!strcmp (sec, "editors")) {
                pkg->section = RC_SECTION_UTIL;
            } else if (!strcmp (sec, "electronics")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "games")) {
                pkg->section = RC_SECTION_GAME;
            } else if (!strcmp (sec, "graphics")) {
                pkg->section = RC_SECTION_IMAGING;
            } else if (!strcmp (sec, "hamradio")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "interpreters")) {
                pkg->section = RC_SECTION_DEVELUTIL;
            } else if (!strcmp (sec, "libs")) {
                pkg->section = RC_SECTION_LIBRARY;
            } else if (!strcmp (sec, "mail")) {
                pkg->section = RC_SECTION_PIM;
            } else if (!strcmp (sec, "math")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "misc")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "net")) {
                pkg->section = RC_SECTION_INTERNET;
            } else if (!strcmp (sec, "news")) {
                pkg->section = RC_SECTION_INTERNET;
            } else if (!strcmp (sec, "oldlibs")) {
                pkg->section = RC_SECTION_LIBRARY;
            } else if (!strcmp (sec, "otherosfs")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "science")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "shells")) {
                pkg->section = RC_SECTION_SYSTEM;
            } else if (!strcmp (sec, "sound")) {
                pkg->section = RC_SECTION_MULTIMEDIA;
            } else if (!strcmp (sec, "tex")) {
                pkg->section = RC_SECTION_MISC;
            } else if (!strcmp (sec, "text")) {
                pkg->section = RC_SECTION_UTIL;
            } else if (!strcmp (sec, "utils")) {
                pkg->section = RC_SECTION_UTIL;
            } else if (!strcmp (sec, "web")) {
                pkg->section = RC_SECTION_INTERNET;
            } else if (!strcmp (sec, "x11")) {
                pkg->section = RC_SECTION_XAPP;
            } else {
                pkg->section = RC_SECTION_MISC;
            }
#endif
            g_strfreev (tokens);
        } else if (!strncmp (buf, "depends: ", strlen ("depends: "))) {
            pkg->requires = g_slist_concat (
                pkg->requires,
                rc_debman_fill_depends (buf + strlen ("depends: ")));
        } else if (!strncmp (buf, "recommends: ", strlen ("recommends: "))) {
            pkg->recommends = g_slist_concat (
                pkg->recommends,
                rc_debman_fill_depends (buf + strlen ("recommends: ")));
        } else if (!strncmp (buf, "suggests: ", strlen ("suggests: "))) {
            pkg->suggests = g_slist_concat (
                pkg->suggests,
                rc_debman_fill_depends (buf + strlen ("suggests: ")));
        } else if (!strncmp (buf, "pre-depends: ", strlen ("pre-depends: "))) {
            RCPackageDepSList *tmp, *iter;

            tmp = rc_debman_fill_depends (buf + strlen ("pre-depends: "));
            for (iter = tmp; iter; iter = iter->next) {
                RCPackageDep *dep = (RCPackageDep *)(iter->data);

                dep->pre = TRUE;
            }
            pkg->requires = g_slist_concat (
                pkg->requires, tmp);
        } else if (!strncmp (buf, "conflicts: ", strlen ("conflicts: "))) {
            pkg->conflicts = g_slist_concat (
                pkg->conflicts,
                rc_debman_fill_depends (buf + strlen ("conflicts: ")));
        } else if (!strncmp (buf, "provides: ", strlen ("provides: "))) {
            pkg->provides = g_slist_concat (
                pkg->provides,
                rc_debman_fill_depends (buf + strlen ("provides: ")));

        } else if (!strncmp (buf, "filename: ", strlen ("filename: "))) {
            /* Build a new update with just this version */
            if (url_prefix) {
                up->package_url = g_strconcat (url_prefix, "/",
                                               buf + strlen ("filename: "),
                                               NULL);
            } else {
                up->package_url = g_strndup (buf + strlen ("filename: "),
                                             ilen - strlen ("filename: "));
            }
        } else if (!strncmp (buf, "md5sum: ", strlen ("md5sum: "))) {
            up->md5sum = g_strndup (buf + strlen ("md5sum: "),
                                    ilen - strlen ("md5sum: "));
        }

        buf[ilen] = '\n';
    }

    up->importance = RC_IMPORTANCE_SUGGESTED;
    up->description = g_strdup ("Upstream Debian release");
    rc_package_spec_copy (&up->spec, &pkg->spec);
    rc_package_add_update (pkg, up);

    /* Make sure to provide myself, for the dep code! */
    pkg->provides =
        g_slist_append (pkg->provides, rc_package_dep_new_from_spec
                        (&pkg->spec,
                         RC_RELATION_EQUAL));
}


static guint
rc_world_parse_redhat (RCWorld *world, RCChannel *rcc, char *buf)
{
    g_error ("No redhat channel support!");
    return 0;
}
