#include <gnome-xml/tree.h>

#include <gtk/gtk.h>
#include "libredcarpet.h"

#include <stdio.h>

int
main (int argc, char **argv)
{
    RCPackman *packman;
    RCPackageSList *packages = NULL;
    RCPackageSList *iter;
    RCPackageUpdate *pkg_up;
    xmlDocPtr doc;
    xmlNode *channel_node;
    xmlNode *subchannel_node;

    gtk_type_init ();

    packman = rc_distman_new ();

    packages = rc_packman_query_all (packman);

    channel_node = xmlNewNode (NULL, "channel");
    subchannel_node = xmlNewNode (NULL, "subchannel");
    xmlAddChild (channel_node, subchannel_node);

    for (iter = packages; iter; iter = iter->next) {
        RCPackage *package = (RCPackage *)(iter->data);
	xmlNode *package_node;

        pkg_up = rc_package_update_new ();

        rc_package_spec_copy ((RCPackageSpec *)pkg_up,
                              (RCPackageSpec *)package);

        pkg_up->package_url = g_strdup ("file://dummy.rpm");
        pkg_up->importance = RC_IMPORTANCE_SUGGESTED;
        pkg_up->description = g_strdup ("No information available.");
        pkg_up->package_size = 0;

        package->history = g_slist_append (package->history, pkg_up);

	package_node = rc_package_to_xml_node (package);

	xmlAddChild (subchannel_node, package_node);
    }

    doc = xmlNewDoc ("1.0");
    xmlDocSetRootElement (doc, channel_node);
    xmlDocDump (stdout, doc);

    exit (0);
}
    
