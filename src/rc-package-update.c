#include <stdlib.h>

#include <gnome-xml/tree.h>

#include <libredcarpet/rc-package-update.h>
#include <libredcarpet/xml-util.h>

RCPackageUpdate *
rc_package_update_new ()
{
    RCPackageUpdate *rcpu = g_new0 (RCPackageUpdate, 1);

    return (rcpu);
} /* rc_package_update_new */

RCPackageUpdate *
rc_package_update_copy (RCPackageUpdate *old)
{
    RCPackageUpdate *new = rc_package_update_new ();

    rc_package_spec_copy ((RCPackageSpec *) old, (RCPackageSpec *) new);

    new->package_url = g_strdup (old->package_url);
    new->package_size = old->package_size;

    new->installed_size = old->installed_size;

    new->signature_url = g_strdup (old->signature_url);
    new->signature_size = old->signature_size;

    new->md5sum = g_strdup (old->md5sum);

    new->importance = old->importance;

    new->description = g_strdup (old->description);

    return (new);
}

void
rc_package_update_free (RCPackageUpdate *update)
{
    rc_package_spec_free_members(RC_PACKAGE_SPEC (update));

    g_free (update->package_url);

    g_free (update->signature_url);

    g_free (update->md5sum);

    g_free (update->description);

    g_free (update);
} /* rc_package_update_free */

RCPackageUpdateSList *
rc_package_update_slist_copy (RCPackageUpdateSList *old)
{
    RCPackageUpdateSList *iter;
    RCPackageUpdateSList *new_list = NULL;

    for (iter = old; iter; iter = iter->next) {
        RCPackageUpdate *old_update = (RCPackageUpdate *)(iter->data);
        RCPackageUpdate *new = rc_package_update_copy (old_update);

        new_list = g_slist_append (new_list, new);
    }

    return (new_list);
}

void
rc_package_update_slist_free (RCPackageUpdateSList *rcpusl)
{
    g_slist_foreach (rcpusl, (GFunc) rc_package_update_free, NULL);

    g_slist_free (rcpusl);
} /* rc_package_update_slist_free */

RCPackageUpdateSList *
rc_package_update_slist_sort (RCPackageUpdateSList *rcpusl)
{
    RCPackageUpdateSList *list = NULL;

    list = g_slist_sort (rcpusl, (GCompareFunc) rc_package_spec_compare_name);

    return (list);
}

RCPackageImportance
rc_string_to_package_importance (gchar *importance)
{
    if (!g_strcasecmp (importance, "necessary")) {
        return (RC_IMPORTANCE_NECESSARY);
    } else if (!g_strcasecmp (importance, "urgent")) {
        return (RC_IMPORTANCE_URGENT);
    } else if (!g_strcasecmp (importance, "suggested")) {
        return (RC_IMPORTANCE_SUGGESTED);
    } else if (!g_strcasecmp (importance, "feature")) {
        return (RC_IMPORTANCE_FEATURE);
    } else if (!g_strcasecmp (importance, "minor")) {
        return (RC_IMPORTANCE_MINOR);
    } else if (!g_strcasecmp (importance, "new")) {
        return (RC_IMPORTANCE_NEW);
    } else {
        return (RC_IMPORTANCE_INVALID);
    }
}

const gchar *
rc_package_importance_to_string (RCPackageImportance importance)
{
    switch (importance) {
    case RC_IMPORTANCE_NECESSARY:
        return ("necessary");
        break;
    case RC_IMPORTANCE_URGENT:
        return ("urgent");
        break;
    case RC_IMPORTANCE_SUGGESTED:
        return ("suggested");
        break;
    case RC_IMPORTANCE_FEATURE:
        return ("feature");
        break;
    case RC_IMPORTANCE_MINOR:
        return ("minor");
        break;
    case RC_IMPORTANCE_NEW:
        return ("new");
        break;
    default:
        return ("(invalid)");
    }
}

xmlNode *
rc_package_update_to_xml_node (RCPackageUpdate *update)
{
    xmlNode *update_node;
    gchar *tmp_string;

    update_node = xmlNewNode (NULL, "update");

    xmlNewTextChild (update_node, NULL, "version", update->spec.version);

    xmlNewTextChild (update_node, NULL, "release", update->spec.release);

    xmlNewTextChild (update_node, NULL, "filename",
                     g_basename (update->package_url));

    tmp_string = g_strdup_printf ("%d", update->package_size);
    xmlNewTextChild (update_node, NULL, "filesize", tmp_string);
    g_free (tmp_string);

    tmp_string = g_strdup_printf ("%d", update->installed_size);
    xmlNewTextChild (update_node, NULL, "installedsize", tmp_string);
    g_free (tmp_string);

    if (update->signature_url) {
        xmlNewTextChild (update_node, NULL, "signaturename",
                         update->signature_url);

        tmp_string = g_strdup_printf ("%d", update->signature_size);
        xmlNewTextChild (update_node, NULL, "signaturesize", tmp_string);
        g_free (tmp_string);
    }

    if (update->md5sum) {
        xmlNewTextChild (update_node, NULL, "md5sum", update->md5sum);
    }

    xmlNewTextChild (update_node, NULL, "importance",
                     rc_package_importance_to_string (update->importance));

    xmlNewTextChild (update_node, NULL, "description", update->description);

    return (update_node);
}

RCPackageUpdate *
rc_xml_node_to_package_update (const xmlNode *node, const RCPackage *package)
{
    RCPackageUpdate *update;
    const xmlNode *iter;
    const gchar *url_prefix = NULL;

    /* Make sure this is an update node */
    if (g_strcasecmp (node->name, "update")) {
        return (NULL);
    }

    update = rc_package_update_new ();

    update->package = package;

    update->spec.name = g_strdup (package->spec.name);

    if (package->subchannel && package->subchannel->channel &&
        package->subchannel->channel->file_path)
    {
        url_prefix = package->subchannel->channel->file_path;
    }

#if LIBXML_VERSION < 20000
    iter = node->childs;
#else
    iter = node->children;
#endif

    while (iter) {
        if (!g_strcasecmp (iter->name, "epoch")) {
            update->spec.epoch =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "version")) {
            update->spec.version = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "release")) {
            update->spec.release = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "filename")) {
            if (url_prefix) {
                gchar *tmp = xml_get_content (iter);
                update->package_url =
                    g_strconcat (url_prefix, "/", tmp, NULL);
                g_free (tmp);
            } else {
                update->package_url = xml_get_content (iter);
            }
        } else if (!g_strcasecmp (iter->name, "filesize")) {
            update->package_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "installed_size")) {
            update->installed_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "signaturename")) {
            if (url_prefix) {
                gchar *tmp = xml_get_content (iter);
                update->signature_url =
                    g_strconcat (url_prefix, "/", tmp, NULL);
                g_free (tmp);
            } else {
                update->signature_url = xml_get_content (iter);
            }
        } else if (!g_strcasecmp (iter->name, "signaturesize")) {
            update->signature_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "md5sum")) {
            update->md5sum = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "importance")) {
            gchar *tmp = xml_get_content (iter);
            update->importance =
                rc_string_to_package_importance (tmp);
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "description")) {
            update->description = xml_get_content (iter);
        } else {
            /* FIXME: should we complain to the user at this point?  This
               should really never happen. */
        }

        iter = iter->next;
    }

    return (update);
}
