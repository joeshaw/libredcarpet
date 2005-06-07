/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-extract-packages.c
 *
 * Copyright (C) 2000-2003 Ximian, Inc.
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
#include "rc-extract-packages.h"

#include "rc-debug.h"
#include "rc-debman-general.h"
#include "rc-distro.h"
#include "rc-package.h"
#include "rc-package-match.h"
#include "rc-xml.h"
#include "xml-util.h"

#ifdef ENABLE_RPM
#  include "rc-rpmman.h"

/* apt-specific rpm header tags.  wow, this sucks. */

#define CRPMTAG_FILENAME          1000000
#define CRPMTAG_FILESIZE          1000001
#define CRPMTAG_MD5               1000005
#define CRPMTAG_SHA1              1000006

#define CRPMTAG_DIRECTORY         1000010
#define CRPMTAG_BINARY            1000011

#define CRPMTAG_UPDATE_SUMMARY    1000020
#define CRPMTAG_UPDATE_IMPORTANCE 1000021
#define CRPMTAG_UPDATE_DATE       1000022
#define CRPMTAG_UPDATE_URL        1000023

#endif

gint
rc_extract_packages_from_xml_node (xmlNode *node,
                                   RCChannel *channel,
                                   RCPackageFn callback,
                                   gpointer user_data)
{
    RCPackage *package;
    int count = 0;

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
                gboolean ok = TRUE;
                if (callback)
                    ok = callback (package, user_data);
                rc_package_unref (package);
                ++count;
                if (! ok)
                    return -1;
            }
        }
        node = node->next;
    }

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gint 
rc_extract_packages_from_helix_buffer (const guint8 *data, int len,
                                       RCChannel *channel,
                                       RCPackageFn callback,
                                       gpointer user_data)
{
    guint count = 0;
    RCPackageSAXContext *ctx;
    RCPackageSList *packages, *iter;

    if (data == NULL || len == 0)
        return 0;

    ctx = rc_package_sax_context_new (channel);
    rc_package_sax_context_parse_chunk (ctx, data, len);
    packages = rc_package_sax_context_done (ctx);
    
    count = g_slist_length (packages);
        
    if (callback) {
        for (iter = packages; iter != NULL; iter = iter->next) {
            callback ((RCPackage *) iter->data, user_data);
        }
    }
  
    rc_package_slist_unref (packages);
    g_slist_free (packages);

    return count;
}

gint
rc_extract_packages_from_helix_file (const char *filename,
                                     RCChannel *channel,
                                     RCPackageFn callback,
                                     gpointer user_data)
{
    RCBuffer *buf;
    int count;

    g_return_val_if_fail (filename != NULL, -1);

    buf = rc_buffer_map_file (filename);
    if (buf == NULL)
        return -1;

    count = rc_extract_packages_from_helix_buffer (buf->data, buf->size,
                                                   channel,
                                                   callback, user_data);
    rc_buffer_unmap_file (buf);

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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
            pkg->spec.arch = rc_arch_from_string (value->str);
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

gint 
rc_extract_packages_from_debian_buffer (const guint8 *data, int len,
                                        RCChannel *channel,
                                        RCPackageFn callback,
                                        gpointer user_data)
{
    const guint8 *iter;
    int count = 0;

    /* Keep looking for a "Package: " */
    iter = data;
    while ((iter = strstr (iter, "Package: ")) != NULL) {
        RCPackage *p;
        int off;

        p = rc_package_new ();

        /* All debian packages "have" epochs */
        p->spec.has_epoch = TRUE;

        off = fill_debian_package (p, iter,
                                   rc_channel_get_file_path (channel));

        p->channel = rc_channel_ref (channel);

        if (callback)
            callback (p, user_data);

        rc_package_unref (p);

        ++count;

        iter += off;
    }

    return count;
}

gint
rc_extract_packages_from_debian_file (const char *filename,
                                      RCChannel *channel,
                                      RCPackageFn callback,
                                      gpointer user_data)
{
    RCBuffer *buf;
    int count;

    g_return_val_if_fail (filename != NULL, -1);

    buf = rc_buffer_map_file (filename);
    if (buf == NULL)
        return -1;

    count = rc_extract_packages_from_debian_buffer (buf->data, buf->size,
                                                    channel,
                                                    callback, user_data);
    rc_buffer_unmap_file (buf);

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCPackage *
rc_extract_yum_package (const guint8 *data, int len,
                        RCPackman *packman, char *url)
{
#ifndef  ENABLE_RPM
    /* We can't support yum without rpm support */
    rc_debug (RC_DEBUG_LEVEL_ERROR, "RPM support is not enabled");
    return NULL;
#else
    RCRpmman *rpmman;
    Header h;
    RCPackage *p;
    RCPackageUpdate *pu;
    char *tmpc;
    int typ, n;

    g_return_val_if_fail (packman != NULL, NULL);

    if (!g_type_is_a (G_TYPE_FROM_INSTANCE (packman), RC_TYPE_RPMMAN)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "yum support is not available on non-RPM systems");
        return NULL;
    }

    rpmman = RC_RPMMAN (packman);
    
    h = rpmman->headerLoad (data);

    if (h == NULL) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "Unable to get header from headerCopyLoad!");
        return NULL;
    }

    rpmman->headerGetEntry (h, RPMTAG_ARCH, &typ, (void **) &tmpc, &n);

    p = rc_package_new ();

    rc_rpmman_read_header (rpmman, h, p);
    rc_rpmman_depends_fill (rpmman, h, p, TRUE);

    pu = rc_package_update_new ();
    rc_package_spec_copy (RC_PACKAGE_SPEC (pu), RC_PACKAGE_SPEC (p));
    pu->importance = RC_IMPORTANCE_SUGGESTED;
    pu->description = g_strdup ("No information available.");
    pu->package_url = url;
    
    p->history = g_slist_append (p->history, pu);

    rpmman->headerFree (h);

    return p;
#endif  
}

gint
rc_extract_packages_from_aptrpm_buffer (const guint8 *data, int len,
                                        RCPackman *packman,
                                        RCChannel *channel,
                                        RCPackageFn callback,
                                        gpointer user_data)
{
#ifndef ENABLE_RPM
    /* We can't support apt-rpm without rpm support */
    rc_debug (RC_DEBUG_LEVEL_ERROR, "RPM support is not enabled");
    return -1;
#else
    RCRpmman *rpmman;
    int count = 0;
    const int hdrmagic_len = 8;
    const char *hdrmagic;
    const guint8 *cur_ptr;


    g_return_val_if_fail (packman != NULL, -1);

    if (!g_type_is_a (G_TYPE_FROM_INSTANCE (packman), RC_TYPE_RPMMAN)) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "apt-rpm support is not available on non-RPM systems");
        return -1;
    }

    rpmman = RC_RPMMAN (packman);

    if (len < hdrmagic_len) {
        rc_debug (RC_DEBUG_LEVEL_ERROR,
                  "Data is too small to possibly be correct");
        return 0;
    }

    /*
     * The apt-rpm pkglist files are a set of rpm headers, each prefixed
     * with the header magic, one right after the other.  If opened on disk,
     * they can be iterated using headerRead().  Since we have an in-memory
     * buffer, we use headerCopyLoad to read them.  We could, potentially,
     * use headerLoad(); but I'm unsure as to what happens when headerFree
     * is called on a Header returned from headerLoad.  It may be a small
     * memory savings to do so.
     */

    /* Skip the inital RPM header magic */
    hdrmagic = data;
    cur_ptr = data + hdrmagic_len;

    while (cur_ptr != NULL) {
        Header h;
        RCPackage *p;
        RCPackageUpdate *pu;
        int bytesleft, i;
        char *tmpc;
        int typ, n;
        char *archstr;
        char *filename;

        h = rpmman->headerLoad (cur_ptr);

        if (h == NULL) {
            rc_debug (RC_DEBUG_LEVEL_ERROR,
                      "Unable to get header from headerCopyLoad!");
            return 0;
        }

        rpmman->headerGetEntry (h, RPMTAG_ARCH, &typ, (void **) &tmpc, &n);

        if (n && typ == RPM_STRING_TYPE)
            archstr = tmpc;
        else {
            rc_debug (RC_DEBUG_LEVEL_WARNING, "No arch available!");
            goto cleanup;
        }

        rpmman->headerGetEntry (h, CRPMTAG_FILENAME, &typ, (void **)&tmpc, &n);
        if (n && (typ == RPM_STRING_TYPE) && tmpc && tmpc[0]) {
            if (g_utf8_validate (tmpc, -1, NULL)) {
                filename = g_strdup (tmpc);
            } else {
                filename = g_convert_with_fallback (tmpc, -1,
                                                    "UTF-8",
                                                    "ISO-8859-1",
                                                    "?", NULL, NULL,
                                                    NULL);
            }
        } else {
            filename = g_strdup_printf ("%s-%s-%s.%s.rpm",
                                        g_quark_to_string (p->spec.nameq),
                                        p->spec.version,
                                        p->spec.release,
                                        archstr);
        }

        p = rc_package_new ();

        rc_rpmman_read_header (rpmman, h, p);
        rc_rpmman_depends_fill (rpmman, h, p, TRUE);

        p->channel = rc_channel_ref (channel);

        pu = rc_package_update_new ();
        rc_package_spec_copy (RC_PACKAGE_SPEC (pu), RC_PACKAGE_SPEC (p));
        pu->importance = RC_IMPORTANCE_SUGGESTED;
        pu->description = g_strdup ("No information available.");
        pu->package_url = g_strdup_printf ("%s/%s",
                                           rc_channel_get_file_path (channel),
                                           filename);
        p->history = g_slist_append (p->history, pu);

        if (callback)
            callback (p, user_data);

        rc_package_unref (p);

        ++count;

    cleanup:
        rpmman->headerFree (h);

        /* This chunk of ugly could be removed if a) memmem() was portable;
         * or b) if rpmlib didn't suck, and I could figure out how much
         * data it read from the buffer.
         */
        bytesleft = len - (cur_ptr - data);
        for (i = 0; i < bytesleft - hdrmagic_len; i++) {
            if (memcmp (cur_ptr + i, hdrmagic, hdrmagic_len) == 0) {
                /* We found a match */
                cur_ptr = cur_ptr + i + hdrmagic_len;
                break;
            }
        }

        if (i >= bytesleft - hdrmagic_len) {
            /* No match was found */
            cur_ptr = NULL;
        }
    }

    return count;
#endif
}

gint
rc_extract_packages_from_aptrpm_file (const char *filename,
                                      RCPackman *packman,
                                      RCChannel *channel,
                                      RCPackageFn callback,
                                      gpointer user_data)
{
    RCBuffer *buf;
    int count;

    g_return_val_if_fail (filename != NULL, -1);
    g_return_val_if_fail (packman != NULL, -1);

    buf = rc_buffer_map_file (filename);
    if (buf == NULL)
        return -1;

    count = rc_extract_packages_from_aptrpm_buffer (buf->data, buf->size,
                                                    packman, channel,
                                                    callback, user_data);

    rc_buffer_unmap_file (buf);

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gint
rc_extract_packages_from_undump_buffer (const guint8 *data, int len,
                                        RCChannelAndSubdFn channel_callback,
                                        RCPackageFn package_callback,
                                        RCPackageMatchFn lock_callback,
                                        gpointer user_data)
{

    xmlDoc *doc;
    xmlNode *dump_node;
    RCChannel *system_channel = NULL;
    RCChannel *current_channel = NULL;
    xmlNode *channel_node;
    int count = 0;

    doc = rc_parse_xml_from_buffer (data, len);
    if (doc == NULL)
        return -1;

    dump_node = xmlDocGetRootElement (doc);
    if (dump_node == NULL)
        return -1;

    if (g_strcasecmp (dump_node->name, "world")) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "Unrecognized top-level node for undump: '%s'",
                  dump_node->name);
        return -1;
    }

    channel_node = dump_node->xmlChildrenNode;

    while (channel_node != NULL) {

        if (! g_strcasecmp (channel_node->name, "locks")) {
            xmlNode *lock_node = channel_node->xmlChildrenNode;

            while (lock_node) {
                RCPackageMatch *lock;

                lock = rc_package_match_from_xml_node (lock_node);

                if (lock_callback)
                    lock_callback (lock, user_data);

                lock_node = lock_node->next;
            }

        } else if (! g_strcasecmp (channel_node->name, "system_packages")) {

            int subcount;

            if (!system_channel) {
                system_channel = rc_channel_new ("@system",
                                                 "System Packages",
                                                 "@system",
                                                 "System Packages");
                rc_channel_set_system (system_channel);
                rc_channel_set_hidden (system_channel);
            }

            if (channel_callback)
                channel_callback (system_channel, FALSE, user_data);
            
            subcount = rc_extract_packages_from_xml_node (channel_node,
                                                          system_channel,
                                                          package_callback,
                                                          user_data);

            if (subcount < 0) {
                /* Do something clever */
                g_assert_not_reached ();
            }
            
            count += subcount;

        } else if (! g_strcasecmp (channel_node->name, "channel")) {

            char *name;
            char *alias;
            char *id_str;
            static guint32 dummy_id = 0xdeadbeef;
            char *subd_str;
            int subd;
            char *priority_str;
            char *priority_unsubd_str;
            
            name = xml_get_prop (channel_node, "name");
            alias = xml_get_prop (channel_node, "alias");
            
            id_str = xml_get_prop (channel_node, "id");
            if (id_str == NULL) {
                id_str = g_strdup_printf ("dummy:%d", dummy_id);
                ++dummy_id;
            }

            subd_str = xml_get_prop (channel_node, "subscribed");
            subd = subd_str ? atoi (subd_str) : 0;

            priority_str = xml_get_prop (channel_node, "priority_base");
            priority_unsubd_str = xml_get_prop (channel_node, "priority_unsubd");

            current_channel = rc_channel_new (id_str, name, alias, NULL);

            if (current_channel) {
                int subd_priority, unsubd_priority;

                subd_priority = priority_str ? atoi (priority_str) : 0;
                unsubd_priority = priority_unsubd_str ?
                    atoi (priority_unsubd_str) : 0;
                
                rc_channel_set_priorities (current_channel,
                                           subd_priority, unsubd_priority);

                if (channel_callback)
                    channel_callback (current_channel, subd, user_data);

                if (package_callback) {
                    int subcount;
                    subcount = rc_extract_packages_from_xml_node (channel_node,
                                                                  current_channel,
                                                                  package_callback,
                                                                  user_data);
                    if (subcount < 0) {
                        /* FIXME: do something clever */
                        g_assert_not_reached ();
                    }
                    count += subcount;
                }
            }

            g_free (name);
            g_free (alias);
            g_free (id_str);
            g_free (subd_str);

            g_free (priority_str);
            g_free (priority_unsubd_str);
        }

        channel_node = channel_node->next;
    }

    xmlFreeDoc (doc);

    return count;
}

gint
rc_extract_packages_from_undump_file (const char *filename,
                                      RCChannelAndSubdFn channel_callback,
                                      RCPackageFn package_callback,
                                      RCPackageMatchFn lock_callback,
                                      gpointer user_data)
{
    RCBuffer *buf;
    int count;

    g_return_val_if_fail (filename != NULL, -1);

    buf = rc_buffer_map_file (filename);
    if (buf == NULL)
        return -1;

    count = rc_extract_packages_from_undump_buffer (buf->data, buf->size,
                                                    channel_callback,
                                                    package_callback,
                                                    lock_callback,
                                                    user_data);
    rc_buffer_unmap_file (buf);

    return count;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
package_into_hash (RCPackage *pkg, 
                   RCPackman *packman,
                   GHashTable *hash)
{
    gpointer nameq;
    RCPackage *hashed_pkg;

    nameq = GINT_TO_POINTER (RC_PACKAGE_SPEC (pkg)->nameq);
    hashed_pkg = g_hash_table_lookup (hash, nameq);
    if (hashed_pkg == NULL) {
        g_hash_table_insert (hash, nameq, rc_package_ref (pkg));
    } else if (rc_packman_version_compare (packman,
                                           RC_PACKAGE_SPEC (pkg),
                                           RC_PACKAGE_SPEC (hashed_pkg)) > 0) {
        g_hash_table_replace (hash, nameq, rc_package_ref (pkg));
        rc_package_unref (hashed_pkg);
    }
}

struct HashRecurseInfo {
    RCPackman *packman;
    GHashTable *hash;
};

static gboolean
hash_recurse_cb (RCPackage *pkg, gpointer user_data)
{
    struct HashRecurseInfo *info = user_data;
    package_into_hash (pkg, info->packman, info->hash);
    return TRUE;
}

struct HashIterInfo {
    RCPackageFn callback;
    gpointer user_data;
    int count;
};

static void
hash_iter_cb (gpointer key, gpointer val, gpointer user_data)
{
    RCPackage *pkg = val;
    struct HashIterInfo *info = user_data;

    if (info->callback)
        info->callback (pkg, info->user_data);

    rc_package_unref (pkg);
    ++info->count;
}


static void
add_fake_history (RCPackage *pkg)
{
    RCPackageUpdate *up;

    up = rc_package_update_new ();
    rc_package_spec_copy (&up->spec, &pkg->spec);
    up->importance = RC_IMPORTANCE_SUGGESTED;
    rc_package_add_update (pkg, up);
}

typedef struct {
    RCPackageFn user_callback;
    gpointer    user_data;
    const gchar *path;
} PackagesFromDirInfo;

static gboolean
packages_from_dir_cb (RCPackage *package, gpointer user_data)
{
    PackagesFromDirInfo *info = user_data;
    RCPackageUpdate *update;

    /* Set package path */
    update = rc_package_get_latest_update (package);
    if (update && update->package_url)
        package->package_filename = g_build_path (G_DIR_SEPARATOR_S,
                                                  info->path,
                                                  update->package_url,
                                                  NULL);
    if (info->user_callback)
        return info->user_callback (package, info->user_data);

    return TRUE;
}

gint
rc_extract_packages_from_directory (const char *path,
                                    RCChannel *channel,
                                    RCPackman *packman,
                                    gboolean recursive,
                                    RCPackageFn callback,
                                    gpointer user_data)
{
    GDir *dir;
    GHashTable *hash;
    struct HashIterInfo info;
    const char *filename;
    char *magic;
    gboolean distro_magic, pkginfo_magic;
    
    g_return_val_if_fail (path && *path, -1);
    g_return_val_if_fail (channel != NULL, -1);

    /*
      Check for magic files that indicate how to treat the
      directory.  The files aren't read -- it is sufficient that
      they exist.
    */

    magic = g_strconcat (path, "/RC_SKIP", NULL);
    if (g_file_test (magic, G_FILE_TEST_EXISTS)) {
        g_free (magic);
        return 0;
    }
    g_free (magic);

    magic = g_strconcat (path, "/RC_RECURSIVE", NULL);
    if (g_file_test (magic, G_FILE_TEST_EXISTS))
        recursive = TRUE;
    g_free (magic);
    
    magic = g_strconcat (path, "/RC_BY_DISTRO", NULL);
    distro_magic = g_file_test (magic, G_FILE_TEST_EXISTS);
    g_free (magic);

    pkginfo_magic = TRUE;
    magic = g_strconcat (path, "/RC_IGNORE_PKGINFO", NULL);
    if (g_file_test (magic, G_FILE_TEST_EXISTS))
        pkginfo_magic = FALSE;
    g_free (magic);

    /* If distro_magic is set, we search for packages in two
       subdirectories of path: path/distro-target (i.e.
       path/redhat-9-i386) and path/x-cross.
    */

#if 0      
    if (distro_magic) {
        char *distro_path, *cross_distro_path;
        gboolean found_distro_magic = FALSE;
        int count = 0, c;

        distro_path = g_strconcat (path, "/", rc_distro_get_target (), NULL);
        if (g_file_test (distro_path, G_FILE_TEST_IS_DIR)) {
            found_distro_magic = TRUE;

            c = rc_extract_packages_from_directory (distro_path,
                                                    channel, packman,
                                                    callback, user_data);
            if (c >= 0)
                count += c;
        }

        cross_distro_path = g_strconcat (path, "/x-distro", NULL);
        if (g_file_test (cross_distro_path, G_FILE_TEST_IS_DIR)) {
            c = rc_extract_packages_from_directory (cross_distro_path,
                                                    channel, packman,
                                                    callback, user_data);
            if (c >= 0)
                count += c;
        }

        g_free (cross_distro_path);
        g_free (distro_path);

        return count;
    }
#endif

    /* If pkginfo_magic is set and if a packageinfo.xml or
       packageinfo.xml.gz file exists in the directory, use it
       instead of just scanning the files in the directory
       looking for packages. */

    if (pkginfo_magic) {
        int i, count;
        gchar *pkginfo_path;
        const gchar *pkginfo[] = { "packageinfo.xml",
                                   "packageinfo.xml.gz",
                                   NULL };

        for (i = 0; pkginfo[i]; i++) {
            pkginfo_path = g_build_path (G_DIR_SEPARATOR_S, path, pkginfo[i], NULL);
            if (g_file_test (pkginfo_path, G_FILE_TEST_EXISTS))
                break;

            g_free (pkginfo_path);
            pkginfo_path = NULL;
        }

        if (pkginfo_path) {
            PackagesFromDirInfo info;

            info.user_callback = callback;
            info.user_data = user_data;
            info.path = path;

            count = rc_extract_packages_from_helix_file (pkginfo_path,
                                                         channel,
                                                         packages_from_dir_cb,
                                                         &info);
            g_free (pkginfo_path);
            return count;
        }
    }

    dir = g_dir_open (path, 0, NULL);
    if (dir == NULL)
        return -1;

    hash = g_hash_table_new (NULL, NULL);

    while ( (filename = g_dir_read_name (dir)) ) {
        gchar *file_path;

        file_path = g_strconcat (path, "/", filename, NULL);

        if (recursive && g_file_test (file_path, G_FILE_TEST_IS_DIR)) {
            struct HashRecurseInfo info;
            info.packman = packman;
            info.hash = hash;
            rc_extract_packages_from_directory (file_path,
                                                channel,
                                                packman,
                                                TRUE,
                                                hash_recurse_cb,
                                                &info);
        } else if (g_file_test (file_path, G_FILE_TEST_IS_REGULAR)) {
            RCPackage *pkg;

            pkg = rc_packman_query_file (packman, file_path, TRUE);
            if (pkg != NULL) {
                pkg->channel = rc_channel_ref (channel);
                pkg->package_filename = g_strdup (file_path);
                pkg->local_package = FALSE;
                add_fake_history (pkg);
                package_into_hash (pkg, packman, hash);
                rc_package_unref (pkg);
            }
        }

        g_free (file_path);
    }

    g_dir_close (dir);
   
    info.callback = callback;
    info.user_data = user_data;
    info.count = 0;

    /* Walk across the hash and:
       1) Invoke the callback on each package
       2) Unref each package
    */
    g_hash_table_foreach (hash, hash_iter_cb, &info);

    g_hash_table_destroy (hash);

    return info.count;
}
