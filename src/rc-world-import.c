/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-world-import.c
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation
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
#include "rc-distro.h"

typedef enum _DepType DepType;

enum _DepType {
    DEP_TYPE_REQUIRES,
    DEP_TYPE_PROVIDES,
    DEP_TYPE_CONFLICTS,
    DEP_TYPE_SUGGESTS,
    DEP_TYPE_RECOMMENDS
};

static guint rc_world_parse_helix  (RCWorld *, RCChannel *, const char *);
static guint rc_world_parse_debian (RCWorld *, RCChannel *, const char *);
static guint rc_world_parse_redhat (RCWorld *, RCChannel *, const char *);

void
rc_world_add_channels_from_xml (RCWorld *world,
                                xmlNode *node)
{
    g_return_if_fail (world != NULL);
    g_return_if_fail (node != NULL);

    while (node) {
        char *tmp;
        GSList *distro_target;
        gchar *name, *alias, *id_str;
        RCChannelType type = RC_CHANNEL_TYPE_HELIX;
        gchar **targets;
        gchar **iter;
        RCChannel *channel;
        gboolean valid_channel_for_distro;

        /* Skip comments */
        if (node->type == XML_COMMENT_NODE || node->type == XML_TEXT_NODE) {
            node = node->next;
            continue;
        }

        distro_target = NULL;
        valid_channel_for_distro = FALSE;
        tmp = xml_get_prop (node, "distro_target");
        if (tmp) {
            targets = g_strsplit (tmp, ":", 0);
            g_free (tmp);

            for (iter = targets; iter && *iter; iter++) {
                distro_target = g_slist_append (distro_target, *iter);
                
                if (!strcmp (*iter, rc_distro_get_target ()))
                    valid_channel_for_distro = TRUE;
            }

            g_free (targets);
        }

        if (! valid_channel_for_distro) {

            g_slist_foreach (distro_target, (GFunc) g_free, NULL);
            g_slist_free (distro_target);
 
        } else {
            
            name = xml_get_prop(node, "name");
            
            alias = xml_get_prop(node, "alias");
            if (alias == NULL)
                alias = g_strdup ("");

            id_str = xml_get_prop(node, "id");

            tmp = xml_get_prop(node, "type");
            if (tmp) {
                if (g_strcasecmp (tmp, "helix") == 0)
                    type = RC_CHANNEL_TYPE_HELIX;
                else if (g_strcasecmp (tmp, "debian") == 0)
                    type = RC_CHANNEL_TYPE_DEBIAN;
                else if (g_strcasecmp (tmp, "redhat") == 0)
                    type = RC_CHANNEL_TYPE_REDHAT;
                else
                    type = RC_CHANNEL_TYPE_UNKNOWN;
                g_free (tmp);
            }

            channel = rc_world_add_channel (world, name, alias, 
                                            id_str, type);
            g_free (name);
            g_free (alias);
            g_free (id_str);

            channel->path = xml_get_prop(node, "path");

            tmp = xml_get_prop(node, "file_path");
            channel->file_path = rc_maybe_merge_paths(channel->path, tmp);
            g_free(tmp);
        
            tmp = xml_get_prop(node, "icon");
            channel->icon_file = rc_maybe_merge_paths(channel->path, tmp);
            g_free(tmp);

            channel->description = xml_get_prop(node, "description");

            channel->distro_target = distro_target;

            tmp = xml_get_prop(node, "pkginfo_compressed");
            if (tmp) {
                channel->pkginfo_compressed = TRUE;
                g_free (tmp);
            } else {
                channel->pkginfo_compressed = FALSE;
            }

            tmp = xml_get_prop(node, "pkgset_compressed");
            if (tmp) {
                channel->pkgset_compressed = TRUE;
                g_free (tmp);
            } else {
                channel->pkgset_compressed = FALSE;
            }
            
            tmp = xml_get_prop(node, "pkginfo_file");
            if (!tmp) {
                /* default */
                tmp = g_strdup("packageinfo.xml.gz");
                channel->pkginfo_compressed = TRUE;
            }
            channel->pkginfo_file = rc_maybe_merge_paths(channel->path, tmp);
            g_free(tmp);
            
            tmp = xml_get_prop(node, "pkgset_file");
            if (!tmp) {
                /* default */
                tmp = g_strdup("packageset.xml.gz");
                channel->pkgset_compressed = TRUE;
            }
            channel->pkgset_file = rc_maybe_merge_paths(channel->path, tmp);
            g_free(tmp);
            
            
            /* Priorities determine affinity amongst channels in dependency
               resolution. */
            tmp = xml_get_prop(node, "priority");
            if (tmp) {
                channel->priority = rc_channel_priority_parse(tmp);
                g_free(tmp);
            }
            
            tmp = xml_get_prop(node, "priority_when_unsubscribed");
            if (tmp) {
                channel->priority_unsubd = rc_channel_priority_parse(tmp);
                g_free(tmp);
            }
            
            tmp = xml_get_prop(node, "last_update");
            if (tmp)
                channel->last_update = atol(tmp);
            g_free(tmp);
        }

        node = node->next;
    }
}

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
rc_world_add_packages_from_xml (RCWorld *world,
                                RCChannel *channel,
                                xmlNode *node)
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
                if (rc_world_add_package (world, package))
                    ++count;
                rc_package_unref (package);
            }
        }
        node = node->next;
    }

    rc_world_thaw (world);

    return count;
}

RCChannel *
rc_world_add_channel_from_buffer (RCWorld *world,
                                  const char *channel_name,
                                  const char *alias,
                                  const char *id,
                                  RCChannelType type,
                                  const char *tbuf,
                                  gint compressed_length)
{
    RCChannel *channel;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel_name && *channel_name, NULL);
    g_return_val_if_fail (alias != NULL, NULL);
    g_return_val_if_fail (tbuf != NULL, NULL);

    channel = rc_world_add_channel (world,
                                    channel_name,
                                    alias,
                                    id,
                                    type);

    rc_world_add_packages_from_buffer (world, 
                                       channel,
                                       tbuf,
                                       compressed_length);
    return channel;
}

gboolean
rc_world_add_packages_from_buffer (RCWorld *world,
                                   RCChannel *channel,
                                   const char *tbuf, 
                                   int compressed_length)
{
    const char *buf;
    GByteArray *byte_array = NULL;
    guint count = -1;
    RCChannelType type;

    g_return_val_if_fail (world != NULL, FALSE);
    g_return_val_if_fail (tbuf != NULL, FALSE);

    if (compressed_length) {
        if (rc_uncompress_memory (tbuf, compressed_length, &byte_array)) {
            rc_debug (RC_DEBUG_LEVEL_WARNING, "Uncompression failed");
            return FALSE;
        }

        buf = byte_array->data;
    } else {
        buf = tbuf;
    }

    if (channel == RC_CHANNEL_SYSTEM)
        type = RC_CHANNEL_TYPE_HELIX;
    else
        type = channel->type;

    switch (type) {
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
        g_warning ("Trying to parse unknown channel type!");
        break;
    }

    if (byte_array) {
        g_byte_array_free (byte_array, TRUE);
    }

    return count >= 0;
}

static guint
rc_world_parse_helix (RCWorld *world, RCChannel *channel, const char *buf)
{
    xmlDoc *xml_doc;
    xmlNode *root;
    guint count = 0;

    g_assert (buf);

    if (getenv("RC_OLD_XML")) {
        xml_doc = xmlParseMemory (buf, strlen (buf));
        
        if (!xml_doc) {
            return -1;
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
        g_slist_free (packages);
    }

    return count;
}

static int
fill_debian_package (RCPackage  *pkg,
                     const char *buf,
                     const char *url_prefix)
{
    const char *ibuf;
    RCPackageUpdate *up = NULL;
    RCPackageSList *requires = NULL, *provides = NULL, *conflicts = NULL,
        *suggests = NULL, *recommends = NULL;

    up = rc_package_update_new ();

    ibuf = buf;
    while (1) {
        char *key;
        GString *value = NULL;
        const char *p;
        int ind;

        /* Linebreaks indicate the end of a package block. */
        if (*ibuf == '\0' || *ibuf == '\n') break;

        p = strchr (ibuf, ':');

        /* Something bad happened, we're supposed to have a colon. */
        if (!p) break;

        /* Copy the name of the key and lowercase it */
        key = g_ascii_strdown (ibuf, p - ibuf);
        
        /* Move past the colon and any spaces */
        ibuf = p;
        while (*ibuf && (*ibuf == ':' || *ibuf == ' ')) ibuf++;

        ind = 0;
        while ((p = strchr (ibuf, '\n'))) {
            if (!value)
                value = g_string_new ("");

            g_string_append_len (value, ibuf, p - ibuf);
            ind += p - ibuf;

            ibuf = p;

            /* Move past the newline */
            ibuf++;

            /* Check to see if this is a continuation of the previous line */
            if (*ibuf == ' ') {
                /* It is.  Move past the space */
                ibuf++;

                /*
                 * This is a hack.  Description is special because it's
                 * intended to be multiline and user-visible.  So if we're
                 * dealing with description, add a newline.
                 */

                if (strncmp (key, "description",
                             strlen ("description")) == 0) {
                    g_string_append_c (value, '\n');

                    /*
                     * A period on a line by itself indicates that it
                     * should be a blank line.  A newline will follow the
                     * period, so we'll just skip over it.
                     */
                    if (*ibuf == '.')
                        ibuf++;
                }
            }
            else {
                /* It isn't.  Break out. */
                break;
            }
        }

        if (!strncmp (key, "package", strlen ("package"))) {
            pkg->spec.nameq = g_quark_from_string (value->str);
        } else if (!strncmp (key, "installed-size",
                             strlen ("installed-size"))) {
            up->installed_size = strtoul (value->str, NULL, 10) * 1024;
        } else if (!strncmp (key, "size", strlen ("size"))) {
            up->package_size = strtoul(value->str, NULL, 10);
        } else if (!strncmp (key, "description", strlen ("description"))) {
            char *newline;

            /*
             * We only want the first line for the summary, and all the
             * other lines for the description.
             */

            newline = strchr (value->str, '\n');
            if (!newline) {
                pkg->summary = g_strdup (value->str);
                pkg->description = g_strconcat (value->str, "\n", NULL);
            }
            else {
                pkg->summary = g_strndup (value->str, newline - value->str);
                pkg->description = g_strconcat (newline + 1, "\n", NULL);
            }
        } else if (!strncmp (key, "version", strlen ("version"))) {
            guint epoch;
            rc_debman_parse_version (value->str,
                                     &epoch, &pkg->spec.version,
                                     &pkg->spec.release);
            pkg->spec.epoch = epoch;
        } else if (!strncmp (key, "section", strlen ("section"))) {
            pkg->section = rc_debman_section_to_package_section (value->str);
        } else if (!strncmp (key, "depends", strlen ("depends"))) {
            requires = g_slist_concat (
                requires,
                rc_debman_fill_depends (value->str, FALSE));
        } else if (!strncmp (key, "recommends", strlen ("recommends"))) {
            recommends = g_slist_concat (
                recommends,
                rc_debman_fill_depends (value->str, FALSE));
        } else if (!strncmp (key, "suggests", strlen ("suggests"))) {
            suggests = g_slist_concat (
                suggests,
                rc_debman_fill_depends (value->str, FALSE));
        } else if (!strncmp (key, "pre-depends", strlen ("pre-depends"))) {
            requires = g_slist_concat (
                requires,
                rc_debman_fill_depends (value->str, TRUE));
        } else if (!strncmp (key, "conflicts", strlen ("conflicts"))) {
            conflicts = g_slist_concat (
                conflicts,
                rc_debman_fill_depends (value->str, FALSE));
        } else if (!strncmp (key, "provides", strlen ("provides"))) {
            provides = g_slist_concat (
                provides,
                rc_debman_fill_depends (value->str, FALSE));
        } else if (!strncmp (key, "filename", strlen ("filename"))) {
            /* Build a new update with just this version */
            if (url_prefix) {
                up->package_url = g_strconcat (url_prefix, "/",
                                               value->str,
                                               NULL);
            } else {
                up->package_url = g_strdup (value->str);
            }
        } else if (!strncmp (key, "md5sum", strlen ("md5sum"))) {
            up->md5sum = g_strdup (value->str);
        } else if (!strncmp (key, "architecture", strlen ("architecture"))) {
            pkg->arch = rc_arch_from_string (value->str);
        }

        g_string_free (value, TRUE);
    }

    up->importance = RC_IMPORTANCE_SUGGESTED;
    up->description = g_strdup ("Upstream Debian release");
    rc_package_spec_copy (&up->spec, &pkg->spec);
    rc_package_add_update (pkg, up);

    /* Make sure to provide myself, for the dep code! */
    provides =
        g_slist_append (provides, rc_package_dep_new_from_spec (
                            &pkg->spec, RC_RELATION_EQUAL,
                            pkg->channel, FALSE, FALSE));

    pkg->requires_a = rc_package_dep_array_from_slist (&requires);
    pkg->provides_a = rc_package_dep_array_from_slist (&provides);
    pkg->conflicts_a = rc_package_dep_array_from_slist (&conflicts);
    pkg->obsoletes_a = rc_package_dep_array_from_slist (NULL);
    pkg->suggests_a = rc_package_dep_array_from_slist (&suggests);
    pkg->recommends_a = rc_package_dep_array_from_slist (&recommends);

    /* returns the number of characters we processed */
    return ibuf - buf;
}

static guint
rc_world_parse_debian (RCWorld *world, RCChannel *rcc, const char *buf)
{
    const char *iter;
    guint count = 0;

    rc_world_freeze (world);

    /* Keep looking for a "Package: " */
    iter = buf;
    while ((iter = strstr (iter, "Package: ")) != NULL) {
        RCPackage *p;
        int off;

        p = rc_package_new ();

        /* All debian packages "have" epochs */
        p->spec.has_epoch = TRUE;

        off = fill_debian_package (p, iter, rcc->file_path);

        p->channel = rc_channel_ref (rcc);

        rc_world_add_package (world, p);
        rc_package_unref (p);
        ++count;

        iter += off;
    }

    rc_world_thaw (world);

    return count;
}

static guint
rc_world_parse_redhat (RCWorld *world, RCChannel *rcc, const char *buf)
{
    g_warning ("No redhat channel support!");
    return 0;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
set_pkg_file_cb (RCPackage *pkg,
                 gpointer   user_data)
{
    RCPackageUpdate *update = rc_package_get_latest_update (pkg);

    g_free (pkg->package_filename);
    pkg->package_filename = g_strconcat ((gchar *) user_data,
                                         "/",
                                         update->package_url,
                                         NULL);
    pkg->local_package = FALSE;
}

static void
add_packages_from_hash_cb (gpointer key,
                           gpointer val,
                           gpointer user_data)
{
    RCPackage *pkg = val;
    RCWorld *world = user_data;
    RCPackageUpdate *update;

    update = rc_package_update_new ();
    rc_package_spec_copy (RC_PACKAGE_SPEC (update),
                          RC_PACKAGE_SPEC (pkg));
    pkg->history = g_slist_prepend (pkg->history, update);

    rc_world_add_package (world, pkg);
    rc_package_unref (pkg);
}

static void
refresh_channel_from_dir (RCChannel *channel)
{
    RCWorld *world = rc_get_world ();
    const char *directory;
    const char *filename;
    char *pkginfo_file;
    gboolean is_compressed = FALSE;

    directory = rc_channel_get_description (channel);

    /* Check if there is a file named packageinfo.xml.gz or
       packageinfo.xml is in the directory. */
    pkginfo_file = g_strconcat (directory, "/packageinfo.xml.gz", NULL);
    if (g_file_test (pkginfo_file, G_FILE_TEST_EXISTS)) {
        is_compressed = TRUE;
    } else {
        g_free (pkginfo_file);
        pkginfo_file = g_strconcat (directory, "/packageinfo.xml", NULL);
        if (! g_file_test (pkginfo_file, G_FILE_TEST_EXISTS)) {
            g_free (pkginfo_file);
            pkginfo_file = NULL;
        }
    }

    if (pkginfo_file) {

        /* Read the package info from the packageinfo.xml file. */
        
        RCBuffer *buf;
        buf = rc_buffer_map_file (pkginfo_file);
        if (buf == NULL)
            goto cleanup;

        channel->type = RC_CHANNEL_TYPE_HELIX;

        rc_world_remove_packages (world, channel);

        rc_world_add_packages_from_buffer (world,
                                           channel,
                                           buf->data,
                                           is_compressed ? buf->size : 0);
        rc_buffer_unmap_file (buf);

        rc_world_foreach_package (world,
                                  channel,
                                  set_pkg_file_cb,
                                  (gpointer) directory);

    } else {

        /* Walk the directory and scan each file, checking to see
           if it is a package. */

        GDir *dir;
        GHashTable *hash;
        RCPackman *packman = rc_world_get_packman (world);

        dir = g_dir_open (directory, 0, NULL);
        if (dir == NULL)
            goto cleanup;

        hash = g_hash_table_new (NULL, NULL);

        rc_world_remove_packages (world, channel);
    
        while ( (filename = g_dir_read_name (dir)) ) {
            RCPackage *pkg = NULL;
            gchar *full_path = g_strconcat (directory, "/", filename, NULL);
            char *ext;

            ext = strrchr (full_path, '.');

            /* Read the synthetic package XML */
            if (ext && !strcmp (ext, ".synpkg")) {
                char *buf;
                RCPackageSAXContext *ctx;
                RCPackageSList *packages;
                
                if (g_file_get_contents (full_path, &buf, NULL, NULL)) {
                    
                    ctx = rc_package_sax_context_new (RC_CHANNEL_SYSTEM);
                    rc_package_sax_context_parse_chunk (ctx, buf, strlen (buf));
                    g_free (buf);
                    packages = rc_package_sax_context_done (ctx);

                    /* This is stupid. */
                    g_assert (g_slist_length (packages) <= 1);

                    if (packages)
                        pkg = packages->data;

                    if (pkg && ! rc_package_is_synthetic (pkg)) {
                        rc_debug (RC_DEBUG_LEVEL_WARNING,
                                  "Found non-synthetic package in '%s' - ignoring",
                                  full_path);
                        rc_package_unref (pkg);
                        pkg = NULL;
                    }

                    g_slist_free (packages);
                }

            } else {
            
                pkg = rc_packman_query_file (rc_world_get_packman (world),
                                             full_path);
            }
            
            if (pkg) {
                gpointer nameq = GINT_TO_POINTER (RC_PACKAGE_SPEC (pkg)->nameq);
                RCPackage *curr_pkg;

                curr_pkg = g_hash_table_lookup (hash, nameq);
                if (curr_pkg == NULL
                    || rc_packman_version_compare (packman,
                                                   RC_PACKAGE_SPEC (pkg),
                                                   RC_PACKAGE_SPEC (curr_pkg)) > 0) {

                    pkg->channel = rc_channel_ref (channel);
                    pkg->package_filename = g_strdup (full_path);
                    pkg->local_package = FALSE;

                    if (curr_pkg == NULL) {
                        g_hash_table_insert (hash, nameq, pkg);
                    } else {
                        g_hash_table_replace (hash, nameq, pkg);
                        rc_package_unref (curr_pkg);
                    }
                }
            }
            
            g_free (full_path);
        }

        g_hash_table_foreach (hash, add_packages_from_hash_cb, world);
        
        g_hash_table_destroy (hash);
        g_dir_close (dir);
    }

 cleanup:
    g_free (pkginfo_file);
}

RCChannel *
rc_world_add_channel_from_directory (RCWorld *world,
                                     const char *channel_name,
                                     const char *alias,
                                     const char *directory)
{
    RCChannel *channel;
    char *id_str;

    g_return_val_if_fail (world != NULL, NULL);
    g_return_val_if_fail (channel_name && *channel_name, NULL);
    g_return_val_if_fail (alias != NULL, NULL);
    g_return_val_if_fail (directory != NULL, NULL);

    id_str = g_strdup_printf("mounted:%s", directory);

    channel = rc_world_add_channel (world,
                                    channel_name,
                                    alias,
                                    id_str,
                                    RC_CHANNEL_TYPE_UNKNOWN);

    g_free (id_str);

    channel->description = g_strdup (directory);
    channel->refresh_magic = refresh_channel_from_dir;
    channel->transient = TRUE;
    channel->subscribed = TRUE;
    channel->priority = 12800;

    refresh_channel_from_dir (channel);

    return channel;
}
