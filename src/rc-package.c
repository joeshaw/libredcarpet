/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-package.c: Dealing with individual packages...
 *
 * Copyright (C) 2000 Helix Code, Inc.
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

#include <libredcarpet/rc-package.h>
#include <libredcarpet/xml-util.h>

#include <gnome-xml/tree.h>

const gchar *
rc_package_section_to_string (RCPackageSection section)
{
    switch (section) {
    case SECTION_OFFICE:
        return ("SECTION_OFFICE");
        break;
    case SECTION_IMAGING:
        return ("SECTION_IMAGINE");
        break;
    case SECTION_PIM:
        return ("SECTION_PIM");
        break;
    case SECTION_GAME:
        return ("SECTION_GAME");
        break;
    case SECTION_MULTIMEDIA:
        return ("SECTION_MULTIMEDIA");
        break;
    case SECTION_INTERNET:
        return ("SECTION_INTERNET");
        break;
    case SECTION_UTIL:
        return ("SECTION_UTIL");
        break;
    case SECTION_SYSTEM:
        return ("SECTION_SYSTEM");
        break;
    case SECTION_DOC:
        return ("SECTION_DOC");
        break;
    case SECTION_DEVEL:
        return ("SECTION_DEVEL");
        break;
    case SECTION_DEVELUTIL:
        return ("SECTION_DEVELUTIL");
        break;
    case SECTION_LIBRARY:
        return ("SECTION_LIBRARY");
        break;
    case SECTION_XAPP:
        return ("SECTION_XAPP");
        break;
    default:
        return ("SECTION_MISC");
        break;
    }
}

RCPackageSection
rc_string_to_package_section (gchar *section)
{
    if (!strcmp (section, "SECTION_OFFICE")) {
        return (SECTION_OFFICE);
    } else if (!strcmp (section, "SECTION_IMAGING")) {
        return (SECTION_IMAGING);
    } else if (!strcmp (section, "SECTION_PIM")) {
        return (SECTION_PIM);
    } else if (!strcmp (section, "SECTION_GAME")) {
        return (SECTION_GAME);
    } else if (!strcmp (section, "SECTION_MULTIMEDIA")) {
        return (SECTION_MULTIMEDIA);
    } else if (!strcmp (section, "SECTION_INTERNET")) {
        return (SECTION_INTERNET);
    } else if (!strcmp (section, "SECTION_UTIL")) {
        return (SECTION_UTIL);
    } else if (!strcmp (section, "SECTION_SYSTEM")) {
        return (SECTION_SYSTEM);
    } else if (!strcmp (section, "SECTION_DOC")) {
        return (SECTION_DOC);
    } else if (!strcmp (section, "SECTION_DEVEL")) {
        return (SECTION_DEVEL);
    } else if (!strcmp (section, "SECTION_DEVELUTIL")) {
        return (SECTION_DEVELUTIL);
    } else if (!strcmp (section, "SECTION_LIBRARY")) {
        return (SECTION_LIBRARY);
    } else if (!strcmp (section, "SECTION_XAPP")) {
        return (SECTION_XAPP);
    } else {
        return (SECTION_MISC);
    }
}

RCPackage *
rc_package_new (void)
{
    RCPackage *rcp = g_new0 (RCPackage, 1);

    return (rcp);
} /* rc_package_new */

RCPackage *
rc_package_copy (RCPackage *old_pkg)
{
    RCPackage *pkg = rc_package_new ();

    g_return_val_if_fail (old_pkg, NULL);

    rc_package_spec_copy ((RCPackageSpec *)old_pkg, (RCPackageSpec *)pkg);

    pkg->section = old_pkg->section;

    pkg->installed = old_pkg->installed;

    pkg->installed_size = old_pkg->installed_size;

    pkg->subchannel = old_pkg->subchannel;

    pkg->requires = rc_package_dep_slist_copy (old_pkg->requires);
    pkg->provides = rc_package_dep_slist_copy (old_pkg->provides);
    pkg->conflicts = rc_package_dep_slist_copy (old_pkg->conflicts);

    pkg->suggests = rc_package_dep_slist_copy (old_pkg->suggests);
    pkg->recommends = rc_package_dep_slist_copy (old_pkg->recommends);

    pkg->summary = g_strdup (old_pkg->summary);
    pkg->description = g_strdup (old_pkg->description);

    pkg->history = rc_package_update_slist_copy (old_pkg->history);

    pkg->hold = old_pkg->hold;

    pkg->package_filename = g_strdup (old_pkg->package_filename);
    pkg->signature_filename = g_strdup (old_pkg->signature_filename);

    return (pkg);
}

void
rc_package_free (RCPackage *rcp)
{
    g_return_if_fail (rcp);

    rc_package_spec_free_members (RC_PACKAGE_SPEC (rcp));

    rc_package_dep_slist_free (rcp->requires);
    rc_package_dep_slist_free (rcp->provides);
    rc_package_dep_slist_free (rcp->conflicts);

    rc_package_dep_slist_free (rcp->suggests);
    rc_package_dep_slist_free (rcp->recommends);

    g_free (rcp->summary);
    g_free (rcp->description);

    rc_package_update_slist_free (rcp->history);

    g_free (rcp->package_filename);
    g_free (rcp->signature_filename);

    g_free (rcp);
} /* rc_package_free */

void
rc_package_slist_free (RCPackageSList *rcpsl)
{
    g_slist_foreach (rcpsl, (GFunc) rc_package_free, NULL);

    g_slist_free (rcpsl);
} /* rc_package_slist_free */

RCPackageUpdateSList *
rc_package_slist_sort (RCPackageSList *rcpusl)
{
    return (g_slist_sort (rcpusl,
                          (GCompareFunc) rc_package_spec_compare_name));
}

RCPackageImportance 
rc_package_get_highest_importance(RCPackage *package)
{
    GSList *i;
    RCPackageImportance highest = RC_IMPORTANCE_INVALID;

    /* FIXME: This probably isn't the right value to return, but
       I can't think of anything better to return. */
    g_return_val_if_fail(package, RC_IMPORTANCE_INVALID);

    i = package->history;
    while (i) {
	RCPackageUpdate *up = i->data;

	if (up->importance == RC_IMPORTANCE_MAX)
	    highest = RC_IMPORTANCE_NECESSARY;
	else if (up->importance < highest || highest == RC_IMPORTANCE_INVALID)
	    highest = up->importance;

	i = i->next;
    }

    return highest;
} /* rc_package_get_highest_importance */

RCPackageImportance 
rc_package_get_highest_importance_from_current(RCPackage *package,
					       RCPackageSList *system_pkgs)
{
    GSList *i;
    RCPackage *system_package = NULL;
    RCPackageImportance highest = RC_IMPORTANCE_INVALID;

    /* FIXME: This probably isn't the right value to return, but
       I can't think of anything better to return. */
    g_return_val_if_fail(package, RC_IMPORTANCE_INVALID);

    i = system_pkgs;
    while (i) {
	RCPackageSpec *s = i->data;

	if (g_strcasecmp(s->name, RC_PACKAGE_SPEC(package)->name) == 0) {
	    system_package = (RCPackage *) s;
	    break;
	}

	i = i->next;
    }

    if (!system_package)
	return RC_IMPORTANCE_INVALID;

    i = package->history;
    while (i) {
	RCPackageUpdate *up = i->data;

	if (rc_package_spec_compare(
	    RC_PACKAGE_SPEC(up), RC_PACKAGE_SPEC(system_package)) > 0) {
	    if (up->importance == RC_IMPORTANCE_MAX)
		highest = RC_IMPORTANCE_NECESSARY;
	    else if (up->importance < highest || 
		     highest == RC_IMPORTANCE_INVALID)
		highest = up->importance;
	}
	
	i = i->next;
    }

    return highest;
} /* rc_package_get_highest_importance_from_current */

gint
rc_package_compare_func (gconstpointer a, gconstpointer b)
{
    RCPackage *one = (RCPackage *)(a);
    RCPackage *two = (RCPackage *)(b);

    return rc_package_spec_compare (&one->spec, &two->spec);
}

static void
util_hash_to_list (gpointer a, gpointer b, gpointer c)
{
    GSList **l = (GSList **) c;
    *l = g_slist_append (*l, b);
}

RCPackageSList *
rc_package_hash_table_by_spec_to_list (RCPackageHashTableBySpec *ht)
{
    RCPackageSList *l = NULL;

    g_hash_table_foreach (ht, util_hash_to_list, &l);

    return l;
}

RCPackageSList *
rc_package_hash_table_by_string_to_list (RCPackageHashTableByString *ht)
{
    RCPackageSList *l = NULL;

    g_hash_table_foreach (ht, util_hash_to_list, &l);

    return l;
}

RCPackageUpdate *
rc_package_get_latest_update(RCPackage *package)
{
    g_return_if_fail(package);
    g_return_if_fail(package->history);

    return (RCPackageUpdate *) package->history->data;
} /* rc_package_get_latest_update */

xmlNode *
rc_package_to_xml_node (RCPackage *package)
{
    xmlNode *package_node;
    xmlNode *tmp_node;
    RCPackageUpdateSList *history_iter;
    RCPackageDepSList *dep_iter;

    package_node = xmlNewNode (NULL, "package");

    xmlNewTextChild (package_node, NULL, "name", package->spec.name);

    xmlNewTextChild (package_node, NULL, "summary", package->summary);

    xmlNewTextChild (package_node, NULL, "description", package->description);

    xmlNewTextChild (package_node, NULL, "section",
                     rc_package_section_to_string (package->section));

    if (package->history) {
        tmp_node = xmlNewChild (package_node, NULL, "history", NULL);
        for (history_iter = package->history; history_iter;
             history_iter = history_iter->next)
        {
            RCPackageUpdate *update = (RCPackageUpdate *)(history_iter->data);
            xmlAddChild (tmp_node, rc_package_update_to_xml_node (update));
        }
    }

    if (package->requires) {
        tmp_node = xmlNewChild (package_node, NULL, "requires", NULL);
        for (dep_iter = package->requires; dep_iter;
             dep_iter = dep_iter->next)
        {
            RCPackageDep *dep = (RCPackageDep *)(dep_iter->data);

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->recommends) {
        tmp_node = xmlNewChild (package_node, NULL, "recommends", NULL);
        for (dep_iter = package->recommends; dep_iter;
             dep_iter = dep_iter->next)
        {
            RCPackageDep *dep = (RCPackageDep *)(dep_iter->data);

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->suggests) {
        tmp_node = xmlNewChild (package_node, NULL, "suggests", NULL);
        for (dep_iter = package->suggests; dep_iter;
             dep_iter = dep_iter->next)
        {
            RCPackageDep *dep = (RCPackageDep *)(dep_iter->data);

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->conflicts) {
        tmp_node = xmlNewChild (package_node, NULL, "conflicts", NULL);
        for (dep_iter = package->conflicts; dep_iter;
             dep_iter = dep_iter->next)
        {
            RCPackageDep *dep = (RCPackageDep *)(dep_iter->data);

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->provides) {
        tmp_node = xmlNewChild (package_node, NULL, "provides", NULL);
        for (dep_iter = package->provides; dep_iter;
             dep_iter = dep_iter->next)
        {
            RCPackageDep *dep = (RCPackageDep *)(dep_iter->data);

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    return (package_node);
}

RCPackage *
rc_xml_node_to_package (const xmlNode *node, const RCSubchannel *subchannel)
{
    RCPackage *package;
    const xmlNode *iter;

    if (g_strcasecmp (node->name, "package")) {
        return (NULL);
    }

    package = rc_package_new ();

    package->subchannel = subchannel;

#if LIBXML_VERSION < 20000
    iter = node->childs;
#else
    iter = node->children;
#endif

    while (iter) {
        if (!g_strcasecmp (iter->name, "name")) {
            package->spec.name = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "summary")) {
            package->summary = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "description")) {
            package->description = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "section")) {
            gchar *tmp = xml_get_content (iter);
            package->section = rc_string_to_package_section (tmp);
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "history")) {
            const xmlNode *iter2;
#if LIBXML_VERSION < 20000
            iter2 = iter->childs;
#else
            iter2 = iter->children;
#endif
            while (iter2) {
                RCPackageUpdate *update;

                update = rc_xml_node_to_package_update (iter2, package);

                package->history =
                    g_slist_append (package->history, update);

                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "requires")) {
            const xmlNode *iter2;
#if LIBXML_VERSION < 20000
            iter2 = iter->childs;
#else
            iter2 = iter->children;
#endif
            while (iter2) {
                package->requires =
                    g_slist_append (package->requires,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "recommends")) {
            const xmlNode *iter2;
#if LIBXML_VERSION < 20000
            iter2 = iter->childs;
#else
            iter2 = iter->children;
#endif
            while (iter2) {
                package->recommends =
                    g_slist_append (package->recommends,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "suggests")) {
            const xmlNode *iter2;
#if LIBXML_VERSION < 20000
            iter2 = iter->childs;
#else
            iter2 = iter->children;
#endif
            while (iter2) {
                package->suggests =
                    g_slist_append (package->suggests,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "conflicts")) {
            const xmlNode *iter2;
#if LIBXML_VERSION < 20000
            iter2 = iter->childs;
#else
            iter2 = iter->children;
#endif
            while (iter2) {
                package->conflicts =
                    g_slist_append (package->conflicts,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "provides")) {
            const xmlNode *iter2;
#if LIBXML_VERSION < 20000
            iter2 = iter->childs;
#else
            iter2 = iter->children;
#endif
            while (iter2) {
                package->provides =
                    g_slist_append (package->provides,
                                    rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }
        } else {
            /* FIXME: do we want to bitch to the user here? */
        }

        iter = iter->next;
    }

    package->spec.epoch =
        ((RCPackageUpdate *)package->history->data)->spec.epoch;
    package->spec.version = g_strdup (
        ((RCPackageUpdate *)package->history->data)->spec.version);
    package->spec.release = g_strdup (
        ((RCPackageUpdate *)package->history->data)->spec.release);

    package->subchannel = subchannel;

    return (package);
}
