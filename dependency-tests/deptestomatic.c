/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * deptestomatic.c
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

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <glib-object.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "libredcarpet.h"

#include "xml-util.h"

static GHashTable *channel_hash = NULL;
static RCPackman *packman = NULL;

static RCWorld *world = NULL;

static void
mark_as_system_cb (RCPackage *package, gpointer user_data)
{
    package->installed = TRUE;
    package->channel = NULL;
}

static void
load_channel (const char *name,
              const char *filename,
              const char *type,
              gboolean system_packages)
{
    RCChannel *channel;
    RCChannelType channel_type;
    gboolean is_compressed;
    struct stat stat_buf;
    int file_size;
    guint count;
    char *buffer;
    FILE *in;

    is_compressed =  (strlen (filename) >=  3)
        && !strcmp (filename + strlen (filename) - 3, ".gz");

    if (stat (filename, &stat_buf)) {
        g_warning ("Can't open file '%s'", filename);
        return;
    }

    file_size = stat_buf.st_size;

    in = fopen (filename, "r");
    if (in == NULL) {
        g_warning ("Can't open file '%s' -- very strange!", filename);
        return;
    }
    
    buffer = g_malloc (file_size + 1);
    if (fread (buffer, 1, file_size, in) != file_size) {
        g_warning ("Couldn't read all %d bytes from file '%s'", file_size, filename);
        g_free (buffer);
        return;
    }
    buffer[file_size] = '\0';
    
    fclose (in);

    channel_type = RC_CHANNEL_TYPE_HELIX;

    if (type) {
        if (g_strcasecmp(type, "helix") == 0)
            channel_type = RC_CHANNEL_TYPE_HELIX;
        else if (g_strcasecmp(type, "debian") == 0)
            channel_type = RC_CHANNEL_TYPE_DEBIAN;
    }

    channel = rc_world_add_channel_from_buffer (world,
                                                name, "foo", 666,
                                                channel_type, buffer,
                                                is_compressed ? file_size : 0);

    g_free (buffer);

    if (channel == NULL) {
        g_print ("Couldn't load packages from %s\n", filename);
        return;
    }
    
    count = rc_channel_package_count (channel);

    if (system_packages) {
        rc_world_foreach_package (world,
                                  channel,
                                  mark_as_system_cb,
                                  NULL);
    }

    g_print ("Loaded %d package%s from %s\n",
             count, count == 1 ? "" : "s", filename);

    if (channel_hash == NULL) {
        channel_hash = g_hash_table_new (g_str_hash, g_str_equal);
    }

    g_hash_table_insert (channel_hash, g_strdup (name), channel);
}

static RCPackage *
get_package (const char *channel_name, const char *package_name)
{
    RCChannel *channel;
    RCPackage *package;

    if (channel_hash == NULL) {
        g_warning ("Can't find package '%s' in channel '%s': no channels defined!",
                   package_name, channel_name);
        return NULL;
    }

    channel = g_hash_table_lookup (channel_hash, channel_name);
    if (channel == NULL) {
        g_warning ("Can't find package '%s': channel '%s' not defined",
                   package_name, channel_name);
        return NULL;
    }

    package = rc_world_get_package (world, channel, package_name);

    if (package == NULL) {
        g_warning ("Can't find package '%s' in channel '%s': no such package",
                   package_name, channel_name);
        return NULL;
    }

    return package;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
print_sep (void)
{
    g_print ("\n------------------------------------------------\n\n");
}

static gboolean done_setup = FALSE;

static void
parse_xml_setup (xmlNode *node)
{
    g_assert (! g_strcasecmp (node->name, "setup"));

    if (done_setup) {
        g_warning ("Multiple <setup>..</setup> sections not allowed!");
        exit (-1);
    }
    done_setup = TRUE;

    node = node->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE) {
            node = node->next;
            continue;
        }

        if (! g_strcasecmp (node->name, "system")) {
            xmlChar *file = xml_get_prop (node, "file");
            g_assert (file);
            load_channel ("SYSTEM", file, RC_CHANNEL_TYPE_HELIX, TRUE);
            g_free (file);
        } else if (! g_strcasecmp (node->name, "channel")) {
            xmlChar *name = xml_get_prop (node, "name");
            xmlChar *file = xml_get_prop (node, "file");
            xmlChar *type = xml_get_prop (node, "type");
            g_assert (name);
            g_assert (file);
            load_channel (name, file, type, FALSE);
            g_free (name);
            g_free (file);
            g_free (type);
        } else if (! g_strcasecmp (node->name, "force-install")) {
            xmlChar *channel_name = xml_get_prop (node, "channel");
            xmlChar *package_name = xml_get_prop (node, "package");
            RCPackage *package;

            g_assert (channel_name);
            g_assert (package_name);

            package = get_package (channel_name, package_name);
            if (package) {
                g_print (">!> Force-installing %s from channel %s\n",
                         package_name, channel_name);
                package->channel = NULL;
                package->installed = TRUE;
            } else {
                g_warning ("Unknown package %s::%s",
                           channel_name, package_name);
            }

            g_free (channel_name);
            g_free (package_name);
        } else if (! g_strcasecmp (node->name, "force-uninstall")) {
            xmlChar *package_name = xml_get_prop (node, "package");
            RCPackage *package;

            g_assert (package_name);
            package = get_package ("SYSTEM", package_name);
            
            if (! package) {
                g_warning ("Can't force-install uninstalled package '%s'\n",
                           package_name);
            } else {
                g_print (">!> Force-uninstalling '%s'", package_name);
                rc_world_remove_package (world, package);
            }


            g_free (package_name);
        } else {
            g_warning ("Unrecognized tag '%s' in setup", node->name);
        }

        node = node->next;
    }
}

static void
assemble_install_cb (RCPackage *package,
                     RCPackageStatus status,
                     gpointer user_data)
{
    GList **list = (GList **) user_data;
    char *str;

    str = g_strdup_printf ("%-7s %s",
                           package->channel == NULL ? "|flag" : "install",
                           rc_package_to_str_static (package));

    *list = g_list_prepend (*list, str);
}

static void
assemble_uninstall_cb (RCPackage *package,
                       RCPackageStatus status,
                       gpointer user_data)
{
    GList **list = (GList **) user_data;
    char *str;

    str = g_strdup_printf ("%-7s %s",
                           package->channel == NULL ? "remove" : "|unflag",
                           rc_package_to_str_static (package));

    *list = g_list_prepend (*list, str);
}

static void
assemble_upgrade_cb (RCPackage *pkg1,
                     RCPackageStatus status1,
                     RCPackage *pkg2,
                     RCPackageStatus status2,
                     gpointer user_data)
{
    GList **list = (GList **) user_data;
    char *str, *p1, *p2;

    p1 = rc_package_to_str (pkg1);
    p2 = rc_package_to_str (pkg2);

    str = g_strdup_printf ("%-7s %s => %s",
                           "upgrade", p2, p1);

    g_free (p1);
    g_free (p2);

    *list = g_list_prepend (*list, str);
}

static void
print_important (gpointer user_data)
{
    g_print (">!> %s\n", (const char *) user_data);
}

static void
soln_cb (RCResolverContext *context, gpointer user_data)
{
    if (rc_resolver_context_is_valid (context)) {

        GList *items = NULL;
        
        gint *count = user_data;
        g_print (">!> Solution #%d:\n", *count);
        ++*count;
        
        rc_resolver_context_foreach_install (context, assemble_install_cb, &items);
        rc_resolver_context_foreach_uninstall (context, assemble_uninstall_cb, &items);
        rc_resolver_context_foreach_upgrade (context, assemble_upgrade_cb, &items);

        items = g_list_sort (items, (GCompareFunc) strcmp);
        g_list_foreach (items, (GFunc) print_important, NULL);
        g_list_foreach (items, (GFunc) g_free, NULL);
        g_list_free (items);

    } else {
        g_print (">!> Failed Attempt:\n");
    }

    g_print (">!> installs=%d, upgrades=%d, uninstalls=%d\n",
             rc_resolver_context_install_count (context),
             rc_resolver_context_upgrade_count (context),
             rc_resolver_context_uninstall_count (context));

    g_print ("download size=%.1fk, install size=%.1fk\n",
             context->download_size / 1024.0,
             context->install_size / 1024.0);

    g_print ("total priority=%d, min priority=%d, max priority=%d\n",
             context->total_priority,
             context->min_priority,
             context->max_priority);

    g_print ("other penalties=%d\n",
             context->other_penalties);

    g_print ("- - - - - - - - - -\n");
    
    rc_resolver_context_spew_info (context);
    g_print ("\n");
}

static void
report_solutions (RCResolver *resolver)
{
    int count = 1;
    GSList *iter;

    g_return_if_fail (resolver);
    
    if (resolver->best_context) {
        g_print ("\nBest Solution:\n\n");
        soln_cb (resolver->best_context, &count);

        g_print ("\nOther Valid Solutions:\n\n");

        if (g_slist_length (resolver->complete_queues) < 20) {
            for (iter = resolver->complete_queues; iter != NULL; iter = iter->next) {
                RCResolverQueue *queue = iter->data;
                if (queue->context != resolver->best_context) 
                    soln_cb (queue->context, &count);
            }
        }
    }

    if (g_slist_length (resolver->invalid_queues) < 20) {
        for (iter = resolver->invalid_queues; iter != NULL; iter = iter->next) {
            RCResolverQueue *queue = iter->data;
            g_print ("Failed Solution:\n");
            rc_resolver_context_spew_info (queue->context);
            g_print ("\n");
        }
    }
}

static void
trial_upgrade_cb (RCPackage *original, RCPackage *upgrade, gpointer user_data)
{
    RCResolver *resolver = user_data;
    char *p1, *p2;

    rc_resolver_add_package_to_install (resolver, upgrade);

    p1 = rc_package_to_str (original);
    p2 = rc_package_to_str (upgrade);
    g_print (">!> Upgrading %s => %s\n", p1, p2);
    g_free (p1);
    g_free (p2);
}

static void
parse_xml_trial (xmlNode *node)
{
    RCResolver *resolver;
    gboolean verify = FALSE;

    g_assert (! g_strcasecmp (node->name, "trial"));

    if (! done_setup) {
        g_warning ("Any trials must be preceeded by the setup!");
        exit (-1);
    }

    print_sep ();

    resolver = rc_resolver_new ();
    rc_resolver_set_world (resolver, world);

    node = node->xmlChildrenNode;
    while (node) {
        if (node->type != XML_ELEMENT_NODE) {
            node = node->next;
            continue;
        }

        if (! g_strcasecmp (node->name, "verify")) {

            verify = TRUE;

        } else if (! g_strcasecmp (node->name, "current")) {

            xmlChar *channel_name = xml_get_prop (node, "channel");
            RCChannel *channel;

            channel = g_hash_table_lookup (channel_hash, channel_name);
            if (channel != NULL) {
                rc_resolver_set_current_channel (resolver, channel);
            } else {
                g_warning ("Unknown channel '%s' (current)", channel_name);
            }

            g_free (channel_name);

        } else if (! g_strcasecmp (node->name, "subscribe")) {

            xmlChar *channel_name = xml_get_prop (node, "channel");
            RCChannel *channel;

            channel = g_hash_table_lookup (channel_hash, channel_name);
            if (channel != NULL) {
                rc_channel_set_subscription (channel, TRUE);
            } else {
                g_warning ("Unknown channel '%s' (subscribe)", channel_name);
            }

            g_free (channel_name);
        
        } else if (! g_strcasecmp (node->name, "install")) {

            xmlChar *channel_name = xml_get_prop (node, "channel");
            xmlChar *package_name = xml_get_prop (node, "package");
            RCPackage *package;

            g_assert (channel_name);
            g_assert (package_name);

            package = get_package (channel_name, package_name);
            if (package) {
                g_print (">!> Installing %s from channel %s\n", package_name, channel_name);
                rc_resolver_add_package_to_install (resolver, package);
            } else {
                g_warning ("Unknown package %s::%s", channel_name, package_name);
            }

            g_free (channel_name);
            g_free (package_name);

        } else if (! g_strcasecmp (node->name, "uninstall")) {

            xmlChar *package_name = xml_get_prop (node, "package");
            RCPackage *package;

            g_assert (package_name);

            package = get_package ("SYSTEM", package_name);
            if (package) {
                g_print (">!> Uninstalling %s\n", package_name);
                rc_resolver_add_package_to_remove (resolver, package);
            } else {
                g_warning ("Unknown system package %s", package_name);
            }

            g_free (package_name);

        } else if (! g_strcasecmp (node->name, "upgrade")) {
            
            int count;

            g_print (">!> Checking for upgrades...\n");

            count = rc_world_foreach_system_upgrade (world,
                                                     trial_upgrade_cb,
                                                     resolver);
            
            if (count == 0)
                g_print (">!> System is up-to-date, no upgrades required\n");
            else
                g_print (">!> Upgrading %d package%s\n", count, count > 1 ? "s" : "");

        } else {
            g_warning ("Unknown tag '%s' in trial", node->name);
        }

        node = node->next;
    }

    if (verify)
        rc_resolver_verify_system (resolver);
    else
        rc_resolver_resolve_dependencies (resolver);

    report_solutions (resolver);
    rc_resolver_free (resolver);
}

static void
parse_xml_test (xmlNode *node)
{
    g_assert (! g_strcasecmp (node->name, "test"));

    node = node->xmlChildrenNode;

    while (node) {
        if (node->type == XML_ELEMENT_NODE) {
            if (! g_strcasecmp (node->name, "setup")) {
                parse_xml_setup (node);
            } else if (! g_strcasecmp (node->name, "trial")) {
                parse_xml_trial (node);
            } else {
                g_warning ("Unknown tag '%s' in test", node->name);
            }
        }
        
        node = node->next;
    }
}

static void
process_xml_test_file (const char *filename)
{
    xmlDocPtr xml_doc;
    xmlNode *root;

    xml_doc = xmlParseFile (filename);
    if (xml_doc == NULL) {
        g_warning ("Can't parse test file '%s'", filename);
        exit (-1);
    }

    root = xmlDocGetRootElement (xml_doc);
    
    parse_xml_test (root);
    
    xmlFreeDoc (xml_doc);
}

int
main (int argc, char *argv[])
{
    g_type_init ();

    rc_distro_parse_xml (NULL, 0);

    packman = rc_distman_new ();
    rc_packman_set_packman (packman);

    world = rc_world_new ();

    if (argc != 2) {
        g_print ("Usage: deptestomatic testfile.xml\n");
        exit (-1);
    }

    process_xml_test_file (argv[1]);

    rc_world_free (world);
    rc_package_spew_leaks ();

    return 0;
}

