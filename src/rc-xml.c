/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-xml.c: XML routines
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include "rc-xml.h"
#include "rc-debug.h"
#include "rc-dep-or.h"
#include "rc-util.h"

/* SAX Parser */

typedef enum   _RCPackageSAXContextState RCPackageSAXContextState;

enum _RCPackageSAXContextState {
    PARSER_TOPLEVEL = 0,
    PARSER_PACKAGE,
    PARSER_HISTORY,
    PARSER_UPDATE,
    PARSER_DEP,
};

struct _RCPackageSAXContext {
    RCChannel *channel;
    gboolean processing;
    xmlParserCtxt *xml_context;
    RCPackageSAXContextState state;

    RCPackageSList *package_list;

    /* Temporary state */
    RCPackage *current_package;
    RCPackageUpdate *current_update;
    RCPackageDepSList **toplevel_dep_list;
    RCPackageDepSList **current_dep_list;

    char *text_buffer;
};

static void
sax_start_document(void *data)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;

    g_return_if_fail(!ctx->processing);

    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* Start document\n");

    ctx->processing = TRUE;
} /* sax_start_document */

static void
sax_end_document(void *data)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;

    g_return_if_fail(ctx->processing);

    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* End document\n");

    ctx->processing = FALSE;

    g_free (ctx->text_buffer);
    ctx->text_buffer = NULL;

} /* sax_end_document */

static void
parser_toplevel_start(RCPackageSAXContext *ctx,
                     const xmlChar *name,
                     const xmlChar **attrs)
{
    if (!strcmp(name, "package")) {
        g_assert(ctx->current_package == NULL);

        ctx->state = PARSER_PACKAGE;

        ctx->current_package = rc_package_new();
        ctx->current_package->channel = ctx->channel;
    }
    else
        rc_debug(RC_DEBUG_LEVEL_DEBUG, "! Not handling %s\n", name);
} /* parser_toplevel_start */

static void
parser_package_start(RCPackageSAXContext *ctx,
                    const xmlChar *name,
                    const xmlChar **attrs)
{
    g_assert(ctx->current_package != NULL);

    /* Only care about the containers here */
    if (!strcmp(name, "history")) {
        ctx->state = PARSER_HISTORY;
    }
    else if (!strcmp(name, "requires")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_package->requires;
    }
    else if (!strcmp(name, "recommends")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list = 
            &ctx->current_package->recommends;
    }
    else if (!strcmp(name, "suggests")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list = 
            &ctx->current_package->suggests;
    }
    else if (!strcmp(name, "conflicts")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_package->conflicts;
    }
    else if (!strcmp(name, "obsoletes")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_package->obsoletes;
    }
    else if (!strcmp(name, "provides")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_package->provides;
    }
    else
        rc_debug(RC_DEBUG_LEVEL_DEBUG, "! Not handling %s\n", name);
} /* parser_package_start */

static void
parser_history_start(RCPackageSAXContext *ctx,
                    const xmlChar *name,
                    const xmlChar **attrs)
{
    g_assert(ctx->current_package != NULL);

    if (!strcmp(name, "update")) {
        g_assert(ctx->current_update == NULL);

        ctx->current_update = rc_package_update_new();
        ctx->current_update->spec.name = g_strdup(
            ctx->current_package->spec.name);
        
        ctx->state = PARSER_UPDATE;
    }
    else
        rc_debug(RC_DEBUG_LEVEL_DEBUG, "! Not handling %s\n", name);
}

static void
parse_dep_attrs(RCPackageDep *dep, const xmlChar **attrs)
{
    int i;
    gboolean op_present = FALSE;
    /* Temporary variables dependent upon the presense of an 'op' attribute */
    guint32 tmp_epoch = 0;
    char *tmp_version = NULL;
    char *tmp_release = NULL;

    for (i = 0; attrs[i]; i++) {
        char *attr = g_strdup(attrs[i++]);
        const char *value = attrs[i];

        g_strdown(attr);

        if (!strcmp(attr, "name"))
            dep->spec.name = g_strdup(value);
        else if (!strcmp(attr, "op")) {
            op_present = TRUE;
            dep->relation = rc_string_to_package_relation(value);
        }
        else if (!strcmp(attr, "epoch"))
            tmp_epoch = rc_string_to_guint32_with_default(value, 0);
        else if (!strcmp(attr, "version"))
            tmp_version = g_strdup(value);
        else if (!strcmp(attr, "release"))
            tmp_release = g_strdup(value);
        else
            rc_debug(RC_DEBUG_LEVEL_DEBUG, "! Unknown attribute: %s = %s", attr, value);

        g_free(attr);
    }

    if (op_present) {
        dep->spec.epoch = tmp_epoch;
        dep->spec.version = tmp_version;
        dep->spec.release = tmp_release;
    }
    else {
        dep->relation = RC_RELATION_ANY;
        g_free(tmp_version);
        g_free(tmp_release);
    }
} /* parse_dep_attrs */

static void
parser_dep_start(RCPackageSAXContext *ctx,
                 const xmlChar *name,
                 const xmlChar **attrs)
{
    RCPackageDep *dep;

    if (!strcmp(name, "dep")) {
        dep = g_new0(RCPackageDep, 1);

        parse_dep_attrs(dep, attrs);

        *ctx->current_dep_list = g_slist_append(*ctx->current_dep_list, dep);
    }
    else if (!strcmp(name, "or"))
        ctx->current_dep_list = g_new0(RCPackageDepSList *, 1);
    else
        rc_debug(RC_DEBUG_LEVEL_DEBUG, "! Not handling %s\n", name);
} /* parser_dep_start */

static void
sax_start_element(void *data, const xmlChar *name, const xmlChar **attrs)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;
    int i;

    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* Start element (%s)\n", name);

    if (attrs) {
        for (i = 0; attrs[i]; i += 2)
            rc_debug(RC_DEBUG_LEVEL_DEBUG, "   - Attribute (%s=%s)\n", attrs[i], attrs[i+1]);
    }

    if (!strcmp(name, "channel") || !strcmp(name, "subchannel")) {
        /* Unneeded container tags.  Ignore */
        return;
    }

    switch (ctx->state) {
    case PARSER_TOPLEVEL:
        parser_toplevel_start(ctx, name, attrs);
        break;
    case PARSER_PACKAGE:
        parser_package_start(ctx, name, attrs);
        break;
    case PARSER_HISTORY:
        parser_history_start(ctx, name, attrs);
        break;
    case PARSER_DEP:
        parser_dep_start(ctx, name, attrs);
        break;
    default:
        break;
    }
} /* sax_start_element */

static void
parser_package_end(RCPackageSAXContext *ctx, const xmlChar *name)
{
    g_assert(ctx->current_package != NULL);

    if (!strcmp(name, "package")) {
        RCPackageUpdate *update;

        /* If possible, grab the version info from the most recent update.
         * Otherwise, try to find where the package provides itself and use
         * that version info.
         */
        update = rc_package_get_latest_update(ctx->current_package);
        if (update) {
            ctx->current_package->spec.epoch   = update->spec.epoch;
            ctx->current_package->spec.version = g_strdup (update->spec.version);
            ctx->current_package->spec.release = g_strdup (update->spec.release);
        }
        else {
            RCPackageDepSList *iter;

            for (iter = ctx->current_package->provides;
                 iter; iter = iter->next) {
                RCPackageDep *dep = iter->data;
                
                if (dep->relation == RC_RELATION_EQUAL &&
                    !strcmp(dep->spec.name, ctx->current_package->spec.name)) {
                    ctx->current_package->spec.epoch   = dep->spec.epoch;
                    ctx->current_package->spec.version = g_strdup (dep->spec.version);
                    ctx->current_package->spec.release = g_strdup (dep->spec.release);
                    break;
                }
            }
        }

        ctx->package_list = g_slist_append(
            ctx->package_list, ctx->current_package);
        
        ctx->current_package = NULL;
        ctx->state = PARSER_TOPLEVEL;
    }
    else if (!strcmp(name, "name")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_package->spec.name = g_strdup (stripped);
    } else if (!strcmp(name, "summary")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_package->summary = g_strdup (stripped);
    } else if (!strcmp(name, "description")) {
        ctx->current_package->description = g_strdup(ctx->text_buffer);
    } else if (!strcmp(name, "section")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_package->section = rc_string_to_package_section (stripped);
    }
    else if (!strcmp(name, "filesize")) {
        ctx->current_package->file_size = 
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "installedsize")) {
        ctx->current_package->installed_size = 
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
} /* parser_package_end */

static void
parser_history_end(RCPackageSAXContext *ctx, const xmlChar *name)
{
    g_assert(ctx->current_package != NULL);

    if (!strcmp(name, "history")) {
        g_assert(ctx->current_update == NULL);

        ctx->state = PARSER_PACKAGE;
    }
} /* parser_history_end */

static void
parser_update_end(RCPackageSAXContext *ctx, const xmlChar *name)
{
    char *url_prefix = NULL;

    g_assert(ctx->current_package != NULL);
    g_assert(ctx->current_update != NULL);

    if (ctx->current_package->channel &&
        ctx->current_package->channel->file_path)
        url_prefix = ctx->current_package->channel->file_path;

    if (!strcmp(name, "update")) {
        rc_package_add_update (ctx->current_package,
                               ctx->current_update);

        ctx->current_update = NULL;
        ctx->state = PARSER_HISTORY;
    }
    else if (!strcmp(name, "epoch")) {
        ctx->current_update->spec.epoch =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "version")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_update->spec.version = g_strdup (stripped);
    } else if (!strcmp(name, "release")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_update->spec.release = g_strdup (stripped);
    } else if (!strcmp(name, "filename")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        if (url_prefix) {
            ctx->current_update->package_url =
                rc_maybe_merge_paths(url_prefix, stripped);
        }
        else
            ctx->current_update->package_url = g_strdup(stripped);
    }
    else if (!strcmp(name, "filesize")) {
        ctx->current_update->package_size =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "installedsize")) {
        ctx->current_update->package_size =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "signaturename")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        if (url_prefix) {
            ctx->current_update->signature_url =
                rc_maybe_merge_paths(url_prefix, stripped);
        }
        else
            ctx->current_update->signature_url = g_strdup(stripped);
    }
    else if (!strcmp(name, "signaturesize")) {
        ctx->current_update->signature_size =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "md5sum")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_update->md5sum = g_strdup(stripped);
    } else if (!strcmp(name, "importance")) {
        char *stripped = g_strstrip (ctx->text_buffer);
        ctx->current_update->importance =
            rc_string_to_package_importance(stripped);
    }
    else if (!strcmp(name, "hid")) {
        ctx->current_update->hid = 
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
} /* parser_update_end */

static void
parser_dep_end(RCPackageSAXContext *ctx, const xmlChar *name)
{
    if (!strcmp(name, "or")) {
        RCDepOr *or = rc_dep_or_new(*ctx->current_dep_list);
        RCPackageDep *dep = rc_dep_or_new_provide(or);

        rc_package_dep_slist_free(*ctx->current_dep_list);
        /* The current_dep_list was allocated with g_new0 */
        g_free(ctx->current_dep_list);
        
        *ctx->toplevel_dep_list = g_slist_append(
            *ctx->toplevel_dep_list, dep);
        ctx->current_dep_list = ctx->toplevel_dep_list;
    }
    else if (!strcmp(name, "dep")) {
        /* We handled everything we needed for dep in start */
    }
    else {
        /* All of the dep lists (requires, provides, etc.) */
        ctx->toplevel_dep_list = NULL;
        ctx->current_dep_list = NULL;
        ctx->state = PARSER_PACKAGE;
    }
} /* parser_deps_end */

static void
sax_end_element(void *data, const xmlChar *name)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;

    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* End element (%s)\n", name);
    
    if (!strcmp(name, "channel") || !strcmp(name, "subchannel")) {
        /* Unneeded container tags.  Ignore */
        goto DONE;
    }

    switch (ctx->state) {
    case PARSER_PACKAGE:
        parser_package_end(ctx, name);
        break;
    case PARSER_HISTORY:
        parser_history_end(ctx, name);
        break;
    case PARSER_UPDATE:
        parser_update_end(ctx, name);
        break;
    case PARSER_DEP:
        parser_dep_end(ctx, name);
        break;
    default:
        break;
    }

 DONE:
    g_free(ctx->text_buffer);
    ctx->text_buffer = NULL;
}

static void
sax_characters(void *data, const xmlChar *ch, int len)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;

    if (ctx->text_buffer) {
        int current_len = strlen (ctx->text_buffer);
        char *buf = g_malloc0 (current_len + len + 1);
        strcpy (buf, ctx->text_buffer);
        strncpy (buf + current_len, ch, len);
        g_free (ctx->text_buffer);
        ctx->text_buffer = buf;
    } else {
        ctx->text_buffer = g_strndup(ch, len);
    }

    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* Characters: \"%s\"\n", ctx->text_buffer);
} /* sax_characters */

static void
sax_warning(void *data, const char *msg, ...)
{
    va_list args;
    char *tmp;

    va_start(args, msg);

    tmp = g_strdup_vprintf(msg, args);
    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* Warning: %s\n", tmp);
    g_free(tmp);

    va_end(args);
}

static xmlSAXHandler sax_handler = {
    NULL,      /* internalSubset */
    NULL,      /* isStandalone */
    NULL,      /* hasInternalSubset */
    NULL,      /* hasExternalSubset */
    NULL,      /* resolveEntity */
    NULL,      /* getEntity */
    NULL,      /* entityDecl */
    NULL,      /* notationDecl */
    NULL,      /* attributeDecl */
    NULL,      /* elementDecl */
    NULL,      /* unparsedEntityDecl */
    NULL,      /* setDocumentLocator */
    sax_start_document,      /* startDocument */
    sax_end_document,        /* endDocument */
    sax_start_element,       /* startElement */
    sax_end_element,         /* endElement */
    NULL,      /* reference */
    sax_characters,          /* characters */
    NULL,      /* ignorableWhitespace */
    NULL,      /* processingInstruction */
    NULL,      /* comment */
    sax_warning,      /* warning */
    sax_warning,      /* error */
    sax_warning,      /* fatalError */
};

void
rc_package_sax_context_parse_chunk(RCPackageSAXContext *ctx,
                                   char *xmlbuf, int size)
{
    xmlSubstituteEntitiesDefault(TRUE);
    
    if (!ctx->xml_context) {
        ctx->xml_context = xmlCreatePushParserCtxt(
            &sax_handler, ctx, NULL, 0, NULL);
    }

    xmlParseChunk(ctx->xml_context, xmlbuf, size, 0);
}

RCPackageSList *
rc_package_sax_context_done(RCPackageSAXContext *ctx)
{
    RCPackageSList *packages;

    if (ctx->processing)
        xmlParseChunk(ctx->xml_context, "\0", 1, 1);

    if (ctx->xml_context)
        xmlFreeParserCtxt(ctx->xml_context);
    
    if (ctx->current_package) {
        g_warning("Incomplete package lost");
        rc_package_unref (ctx->current_package);
    }

    if (ctx->current_update) {
        g_warning("Incomplete update lost");
        rc_package_update_free (ctx->current_update);
    }

    g_free (ctx->text_buffer);

    packages = ctx->package_list;

    g_free(ctx);

    return packages;
} /* rc_package_sax_context_done */

RCPackageSAXContext *
rc_package_sax_context_new(RCChannel *channel)
{
    RCPackageSAXContext *ctx;

    ctx = g_new0(RCPackageSAXContext, 1);
    ctx->channel = channel;

    rc_debug(RC_DEBUG_LEVEL_DEBUG, "* Context created (%p)\n", ctx);

    return ctx;
}

/* ------ */

RCPackage *
rc_xml_node_to_package (const xmlNode *node, const RCChannel *channel)
{
    RCPackage *package;
    const xmlNode *iter;

    if (g_strcasecmp (node->name, "package")) {
        return (NULL);
    }

    package = rc_package_new ();

    package->channel = channel;

    iter = node->xmlChildrenNode;

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
        } else if (!g_strcasecmp (iter->name, "filesize")) {
            gchar *tmp = xml_get_content (iter);
            package->file_size = tmp && *tmp ? atoi (tmp) : 0;
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "installedsize")) {
            gchar *tmp = xml_get_content (iter);
            package->installed_size = tmp && *tmp ? atoi (tmp) : 0;
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "history")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                RCPackageUpdate *update;

                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                update = rc_xml_node_to_package_update (iter2, package);

                rc_package_add_update (package, update);

                iter2 = iter2->next;
            }
        } else if (!g_strcasecmp (iter->name, "requires")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                package->requires =
                    g_slist_prepend (package->requires,
                                     rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }

            package->requires = g_slist_reverse (package->requires);

        } else if (!g_strcasecmp (iter->name, "recommends")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                package->recommends =
                    g_slist_prepend (package->recommends,
                                     rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }

            package->recommends = g_slist_reverse (package->recommends);

        } else if (!g_strcasecmp (iter->name, "suggests")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                package->suggests =
                    g_slist_prepend (package->suggests,
                                     rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }

            package->suggests = g_slist_reverse (package->suggests);

        } else if (!g_strcasecmp (iter->name, "conflicts")) {
            xmlNode *iter2;
            gboolean all_are_obs = FALSE, this_is_obs = FALSE;
            xmlChar *obs;

            iter2 = iter->xmlChildrenNode;

            obs = xmlGetProp ((xmlNode *) iter, "obsoletes"); /* cast out const */
            if (obs)
                all_are_obs = TRUE;
            xmlFree (obs);

            while (iter2) {
                RCPackageDep *dep;

                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                dep = rc_xml_node_to_package_dep (iter2);

                if (! all_are_obs) {
                    this_is_obs = FALSE;
                    obs = xmlGetProp (iter2, "obsoletes");
                    if (obs) 
                        this_is_obs = TRUE;
                    xmlFree (obs);
                }
                
                if (all_are_obs || this_is_obs) {
                    package->obsoletes =
                        g_slist_prepend (package->obsoletes, dep);
                } else {
                    package->conflicts =
                        g_slist_prepend (package->conflicts, dep);
                }
                
                iter2 = iter2->next;
            }

            package->conflicts = g_slist_reverse (package->conflicts);

        } else if (!g_strcasecmp (iter->name, "obsoletes")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                package->obsoletes = 
                    g_slist_prepend (package->obsoletes,
                                     rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }

            package->obsoletes = g_slist_reverse (package->obsoletes);

        } else if (!g_strcasecmp (iter->name, "provides")) {
            const xmlNode *iter2;

            iter2 = iter->xmlChildrenNode;

            while (iter2) {
                if (iter2->type != XML_ELEMENT_NODE) {
                    iter2 = iter2->next;
                    continue;
                }

                package->provides =
                    g_slist_prepend (package->provides,
                                     rc_xml_node_to_package_dep (iter2));
                iter2 = iter2->next;
            }

            package->provides = g_slist_reverse (package->provides);

        } else {
            /* FIXME: do we want to bitch to the user here? */
        }

        iter = iter->next;
    }

    if (package->history && package->history->data) {

        /* If possible, we grab the version info from the most
           recent update. */

        RCPackageUpdate *update = package->history->data;

        package->spec.epoch   = update->spec.epoch;
        package->spec.version = g_strdup (update->spec.version);
        package->spec.release = g_strdup (update->spec.release);

    } else {

        /* Otherwise, try to find where the package provides itself,
           and use that version info. */
        
        RCPackageDepSList *iter3;

        for (iter3 = package->provides; iter3 != NULL; iter3 = iter3->next) {
            RCPackageDep *dep = (RCPackageDep *) iter3->data;
            
            if (dep->relation == RC_RELATION_EQUAL
                && ! strcmp (dep->spec.name, package->spec.name)) {
                package->spec.epoch   = dep->spec.epoch;
                package->spec.version = g_strdup (dep->spec.version);
                package->spec.release = g_strdup (dep->spec.release);
                break;
            }
        }
    }

    return (package);
} /* rc_xml_node_to_package */

static RCPackageDep *
rc_xml_node_to_package_dep_internal (const xmlNode *node)
{
    RCPackageDep *dep_item;
    gchar *tmp;

    if (g_strcasecmp (node->name, "dep")) {
        return (NULL);
    }

    /* FIXME: this sucks */
    dep_item = g_new0 (RCPackageDep, 1);

    dep_item->spec.name = xml_get_prop (node, "name");
    tmp = xml_get_prop (node, "op");
    if (tmp) {
        dep_item->relation = rc_string_to_package_relation (tmp);
        dep_item->spec.epoch =
            xml_get_guint32_value_default (node, "epoch", 0);
        dep_item->spec.version =
            xml_get_prop (node, "version");
        dep_item->spec.release =
            xml_get_prop (node, "release");
        g_free (tmp);
    } else {
        dep_item->relation = RC_RELATION_ANY;
    }

    return (dep_item);
} /* rc_xml_node_to_package_dep_internal */

RCPackageDep *
rc_xml_node_to_package_dep (const xmlNode *node)
{
    RCPackageDep *dep = NULL;

    if (!g_strcasecmp (node->name, "dep")) {
        dep = rc_xml_node_to_package_dep_internal (node);
        return (dep);
    } else if (!g_strcasecmp (node->name, "or")) {
        RCPackageDepSList *or_dep_slist = NULL;
        RCDepOr *or;
        xmlNode *iter = node->xmlChildrenNode;

        while (iter) {
            if (iter->type == XML_ELEMENT_NODE) {
                or_dep_slist = g_slist_append(
                    or_dep_slist,
                    rc_xml_node_to_package_dep_internal (iter));
            }

            iter = iter->next;
        }

        or = rc_dep_or_new (or_dep_slist);
        dep = rc_dep_or_new_provide (or);
    }

    return (dep);
} /* rc_xml_node_to_package_dep */

RCPackageUpdate *
rc_xml_node_to_package_update (const xmlNode *node, const RCPackage *package)
{
    RCPackageUpdate *update;
    const xmlNode *iter;
    const gchar *url_prefix = NULL;

    g_return_val_if_fail (node, NULL);

    /* Make sure this is an update node */
    if (g_strcasecmp (node->name, "update")) {
        return (NULL);
    }

    update = rc_package_update_new ();

    update->package = package;
    
    update->spec.name = g_strdup (package->spec.name);

    if (package->channel && package->channel->file_path)
    {
        url_prefix = package->channel->file_path;
    }

    iter = node->xmlChildrenNode;

    while (iter) {
        if (!g_strcasecmp (iter->name, "epoch")) {
            update->spec.epoch =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "version")) {
            update->spec.version = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "release")) {
            update->spec.release = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "filename")) {
            gchar *tmp = xml_get_content (iter);
            if (url_prefix) {
                update->package_url =
                    rc_maybe_merge_paths (url_prefix, tmp);
                g_free (tmp);
            } else {
                update->package_url = tmp;
            }
        } else if (!g_strcasecmp (iter->name, "filesize")) {
            update->package_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "installedsize")) {
            update->installed_size =
                xml_get_guint32_content_default (iter, 0);
        } else if (!g_strcasecmp (iter->name, "signaturename")) {
            gchar *tmp = xml_get_content (iter);
            if (url_prefix) {
                update->signature_url =
                    rc_maybe_merge_paths (url_prefix, tmp);
                g_free (tmp);
            } else {
                update->signature_url = tmp;
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
        } else if (!g_strcasecmp (iter->name, "hid")) {
            update->hid =
                xml_get_guint32_content_default (iter, 0);
        } else {
            /* FIXME: should we complain to the user at this point?  This
               should really never happen. */
        }

        iter = iter->next;
    }

    return (update);
}

/* ------ */

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
} /* rc_package_to_xml_node */

xmlNode *
rc_package_dep_or_slist_to_xml_node (RCPackageDepSList *dep)
{
    xmlNode *or_node;
    const RCPackageDepSList *dep_iter;

    or_node = xmlNewNode (NULL, "or");

    dep_iter = dep;
    while (dep_iter) {
        RCPackageDep *dep_item = (RCPackageDep *)(dep_iter->data);
        xmlAddChild (or_node, rc_package_dep_to_xml_node (dep_item));
        dep_iter = dep_iter->next;
    }

    return or_node;
} /* rc_package_dep_or_slist_to_xml_node */

xmlNode *
rc_package_dep_to_xml_node (RCPackageDep *dep_item)
{
    xmlNode *dep_node;

    if (dep_item->is_or) {
        RCPackageDepSList *dep_or_slist;
        dep_or_slist = rc_dep_string_to_or_dep_slist (dep_item->spec.name);
        dep_node = rc_package_dep_or_slist_to_xml_node (dep_or_slist);
        rc_package_dep_slist_free (dep_or_slist);
        return dep_node;
    }

    dep_node = xmlNewNode (NULL, "dep");

    xmlSetProp (dep_node, "name", dep_item->spec.name);

    if (dep_item->relation != RC_RELATION_ANY) {
        xmlSetProp (dep_node, "op",
                    rc_package_relation_to_string (dep_item->relation, FALSE));

        if (dep_item->spec.epoch > 0) {
            gchar *tmp;

            tmp = g_strdup_printf ("%d", dep_item->spec.epoch);
            xmlSetProp (dep_node, "epoch", tmp);
            g_free (tmp);
        }

        if (dep_item->spec.version) {
            xmlSetProp (dep_node, "version", dep_item->spec.version);
        }

        if (dep_item->spec.release) {
            xmlSetProp (dep_node, "release", dep_item->spec.release);
        }
    }

    return (dep_node);
} /* rc_package_dep_to_xml_node */

xmlNode *
rc_package_update_to_xml_node (RCPackageUpdate *update)
{
    xmlNode *update_node;
    gchar *tmp_string;

    update_node = xmlNewNode (NULL, "update");

    if (update->spec.epoch) {
        tmp_string = g_strdup_printf("%d", update->spec.epoch);
        xmlNewTextChild (update_node, NULL, "epoch", tmp_string);
        g_free(tmp_string);
    }

    xmlNewTextChild (update_node, NULL, "version", update->spec.version);

    if (update->spec.release) {
        xmlNewTextChild (update_node, NULL, "release", update->spec.release);
    }

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
