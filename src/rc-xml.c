/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-xml.c: XML routines
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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
#include "rc-channel-private.h"

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

    RCPackageSList *all_packages;

    /* Temporary state */
    RCPackage *current_package;
    RCPackageDepSList *current_requires;
    RCPackageDepSList *current_provides;
    RCPackageDepSList *current_conflicts;
    RCPackageDepSList *current_children;
    RCPackageDepSList *current_recommends;
    RCPackageDepSList *current_suggests;
    RCPackageDepSList *current_obsoletes;
    RCPackageUpdate *current_update;
    RCPackageDepSList **toplevel_dep_list;
    RCPackageDepSList **current_dep_list;

    char *text_buffer;
};

/* Like g_strstrip(), only returns NULL on an empty string */
static char *
rc_xml_strip (char *str)
{
    char *s;

    if (str == NULL)
        return NULL;

    s = g_strstrip (str);

    if (s && *s)
        return s;
    else
        return NULL;
}

static void
sax_start_document(void *data)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;

    g_return_if_fail(!ctx->processing);

    if (getenv ("RC_SPEW_XML"))
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "* Start document");

    ctx->processing = TRUE;
} /* sax_start_document */

static void
sax_end_document(void *data)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;

    g_return_if_fail(ctx->processing);

    if (getenv ("RC_SPEW_XML"))
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "* End document");

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
        ctx->current_requires = NULL;
        ctx->current_provides = NULL;
        ctx->current_conflicts = NULL;
        ctx->current_children = NULL;
        ctx->current_recommends = NULL;
        ctx->current_suggests = NULL;
        ctx->current_obsoletes = NULL;

        ctx->current_package->channel = ctx->channel;
        rc_channel_ref (ctx->channel);
    }
    else {
        if (getenv ("RC_SPEW_XML"))
            rc_debug (RC_DEBUG_LEVEL_ALWAYS, "! Not handling %s", name);
    }
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
    else if (!strcmp (name, "deps")) {
        /*
         * We can get a <deps> tag surrounding the actual package
         * dependency sections (requires, provides, conflicts, etc).
         * In this case, we'll just ignore this tag quietly.
         */
    }
    else if (!strcmp(name, "requires")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_requires;
    }
    else if (!strcmp(name, "recommends")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list = 
            &ctx->current_recommends;
    }
    else if (!strcmp(name, "suggests")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list = 
            &ctx->current_suggests;
    }
    else if (!strcmp(name, "conflicts")) {
        gboolean is_obsolete = FALSE;
        int i;

        ctx->state = PARSER_DEP;

        for (i = 0; attrs && attrs[i] && !is_obsolete; i += 2) {

            if (!g_strcasecmp (attrs[i], "obsoletes"))
                is_obsolete = TRUE;
        }

        if (is_obsolete)
            ctx->current_dep_list = ctx->toplevel_dep_list =
                &ctx->current_obsoletes;
        else {
            ctx->current_dep_list = ctx->toplevel_dep_list =
                &ctx->current_conflicts;
        }
    }
    else if (!strcmp(name, "obsoletes")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_obsoletes;
    }
    else if (!strcmp(name, "provides")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_provides;
    }
    else if (!strcmp(name, "children")) {
        ctx->state = PARSER_DEP;
        ctx->current_dep_list = ctx->toplevel_dep_list =
            &ctx->current_children;
    } 
    else {
        if (getenv ("RC_SPEW_XML"))
            rc_debug (RC_DEBUG_LEVEL_ALWAYS, "! Not handling %s", name);
    }
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
        ctx->current_update->spec.nameq = ctx->current_package->spec.nameq;
        
        ctx->state = PARSER_UPDATE;
    }
    else {
        if (getenv ("RC_SPEW_XML"))
            rc_debug (RC_DEBUG_LEVEL_ALWAYS, "! Not handling %s", name);
    }
}

static gboolean
parse_dep_attrs(RCPackageDep **dep, const xmlChar **attrs)
{
    int i;
    gboolean op_present = FALSE;
    /* Temporary variables dependent upon the presense of an 'op' attribute */
    guint32 tmp_epoch = 0;
    gboolean has_epoch = 0;
    const char *tmp_version = NULL;
    const char *tmp_release = NULL;
    gboolean is_obsolete = FALSE;
    RCPackageRelation relation = RC_RELATION_ANY;
    const char *tmp_name = NULL;

    for (i = 0; attrs[i]; i++) {
        const char *attr = attrs[i++];
        const char *value = attrs[i];

        if (!g_strcasecmp(attr, "name"))
            tmp_name = value;
        else if (!g_strcasecmp(attr, "op")) {
            op_present = TRUE;
            relation = rc_package_relation_from_string (value);
        }
        else if (!g_strcasecmp(attr, "epoch")) {
            tmp_epoch = rc_string_to_guint32_with_default(value, 0);
            has_epoch = 1;
        } else if (!g_strcasecmp(attr, "version"))
            tmp_version = value;
        else if (!g_strcasecmp(attr, "release"))
            tmp_release = value;
        else if (!g_strcasecmp (attr, "obsoletes"))
            is_obsolete = TRUE;
        else {
            if (getenv ("RC_SPEW_XML"))
                rc_debug (RC_DEBUG_LEVEL_ALWAYS, "! Unknown attribute: %s = %s", attr, value);
        }

    }

    /* FIXME: should get channel from XML */
    *dep = rc_package_dep_new (tmp_name, has_epoch, tmp_epoch, tmp_version,
                               tmp_release, relation, 
                               RC_CHANNEL_ANY, FALSE, FALSE);

    return is_obsolete;
} /* parse_dep_attrs */

static void
parser_dep_start(RCPackageSAXContext *ctx,
                 const xmlChar *name,
                 const xmlChar **attrs)
{
    RCPackageDep *dep;

    if (!strcmp(name, "dep")) {
        gboolean is_obsolete;

        is_obsolete = parse_dep_attrs(&dep, attrs);
        
        if (is_obsolete)
            ctx->current_obsoletes =
                g_slist_append (ctx->current_obsoletes, dep);
        else {
            *ctx->current_dep_list = g_slist_append (
                *ctx->current_dep_list, dep);
        }
    }
    else if (!strcmp(name, "or"))
        ctx->current_dep_list = g_new0(RCPackageDepSList *, 1);
    else {
        if (getenv ("RC_SPEW_XML"))
            rc_debug (RC_DEBUG_LEVEL_ALWAYS, "! Not handling %s", name);
    }
} /* parser_dep_start */

static void
sax_start_element(void *data, const xmlChar *name, const xmlChar **attrs)
{
    RCPackageSAXContext *ctx = (RCPackageSAXContext *) data;
    int i;

    if (getenv ("RC_SPEW_XML"))
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "* Start element (%s)", name);

    if (attrs) {
        for (i = 0; attrs[i]; i += 2) {
            if (getenv ("RC_SPEW_XML"))
                rc_debug (RC_DEBUG_LEVEL_ALWAYS, "   - Attribute (%s=%s)", attrs[i], attrs[i+1]);
        }
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
            ctx->current_package->spec.has_epoch = update->spec.has_epoch;
            ctx->current_package->spec.version = g_strdup (update->spec.version);
            ctx->current_package->spec.release = g_strdup (update->spec.release);
            ctx->current_package->file_size = update->package_size;
            ctx->current_package->installed_size = update->installed_size;
        }
        else {
            RCPackageDepSList *iter;

            for (iter = ctx->current_provides;
                 iter; iter = iter->next) {
                RCPackageDep *dep = iter->data;
                
                if (rc_package_dep_get_relation (dep) == RC_RELATION_EQUAL &&
                    (RC_PACKAGE_SPEC (dep)->nameq ==
                     ctx->current_package->spec.nameq))
                {
                    ctx->current_package->spec.epoch =
                        RC_PACKAGE_SPEC (dep)->epoch;
                    ctx->current_package->spec.has_epoch =
                        RC_PACKAGE_SPEC (dep)->has_epoch;
                    ctx->current_package->spec.version =
                        g_strdup (RC_PACKAGE_SPEC (dep)->version);
                    ctx->current_package->spec.release =
                        g_strdup (RC_PACKAGE_SPEC (dep)->release);
                    break;
                }
            }
        }

#if 0
        ctx->current_package->obsoletes = g_slist_concat (
            ctx->current_package->obsoletes, ctx->obsoletes);
#endif

        ctx->current_package->requires_a =
            rc_package_dep_array_from_slist (
                &ctx->current_requires);
        ctx->current_package->provides_a =
            rc_package_dep_array_from_slist (
                &ctx->current_provides);
        ctx->current_package->conflicts_a =
            rc_package_dep_array_from_slist (
                &ctx->current_conflicts);
        ctx->current_package->obsoletes_a =
            rc_package_dep_array_from_slist (
                &ctx->current_obsoletes);

        ctx->current_package->children_a =
            rc_package_dep_array_from_slist (
                &ctx->current_children);

        ctx->current_package->suggests_a =
            rc_package_dep_array_from_slist (
                &ctx->current_suggests);
        ctx->current_package->recommends_a =
            rc_package_dep_array_from_slist (
                &ctx->current_recommends);

        /* Hack for the old XML */
        if (ctx->current_package->arch == RC_ARCH_UNKNOWN)
            ctx->current_package->arch = rc_arch_get_system_arch ();

        ctx->all_packages = g_slist_prepend (ctx->all_packages,
                                             ctx->current_package);
        
        ctx->current_package = NULL;
        ctx->state = PARSER_TOPLEVEL;
    }
    else if (!strcmp(name, "name")) {
        ctx->current_package->spec.nameq =
            g_quark_from_string (rc_xml_strip (ctx->text_buffer));
        g_free (ctx->text_buffer);
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "pretty_name")) {
        ctx->current_package->pretty_name = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "summary")) {
        ctx->current_package->summary = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "description")) {
        ctx->current_package->description = ctx->text_buffer;
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "section")) {
        ctx->current_package->section =
            rc_string_to_package_section (rc_xml_strip (ctx->text_buffer));
    } else if (!strcmp(name, "arch")) {
        ctx->current_package->arch =
            rc_arch_from_string (rc_xml_strip (ctx->text_buffer));
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
        ctx->current_update->spec.has_epoch = 1;
    }
    else if (!strcmp(name, "version")) {
        ctx->current_update->spec.version = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "release")) {
        ctx->current_update->spec.release = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "filename")) {
        rc_xml_strip (ctx->text_buffer);
        if (url_prefix) {
            ctx->current_update->package_url =
                rc_maybe_merge_paths(url_prefix, ctx->text_buffer);
        }
        else {
            ctx->current_update->package_url = ctx->text_buffer;
            ctx->text_buffer = NULL;
        }
    }
    else if (!strcmp(name, "filesize")) {
        ctx->current_update->package_size =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "installedsize")) {
        ctx->current_update->installed_size =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "signaturename")) {
        rc_xml_strip (ctx->text_buffer);
        if (url_prefix) {
            ctx->current_update->signature_url =
                rc_maybe_merge_paths(url_prefix, ctx->text_buffer);
        }
        else {
            ctx->current_update->signature_url = ctx->text_buffer;
            ctx->text_buffer = NULL;
        }
    }
    else if (!strcmp(name, "signaturesize")) {
        ctx->current_update->signature_size =
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp(name, "md5sum")) {
        ctx->current_update->md5sum = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
    } else if (!strcmp(name, "importance")) {
        ctx->current_update->importance =
            rc_string_to_package_importance (rc_xml_strip (ctx->text_buffer));
    }
    else if (!strcmp(name, "description")) {
        ctx->current_update->description = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
    }
    else if (!strcmp(name, "hid")) {
        ctx->current_update->hid = 
            rc_string_to_guint32_with_default(ctx->text_buffer, 0);
    }
    else if (!strcmp (name, "license")) {
        ctx->current_update->license = rc_xml_strip (ctx->text_buffer);
        ctx->text_buffer = NULL;
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

    if (getenv ("RC_SPEW_XML"))
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "* End element (%s)", name);
    
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

    if (getenv ("RC_SPEW_XML"))
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "* Characters: \"%s\"", ctx->text_buffer);
} /* sax_characters */

static void
sax_warning(void *data, const char *msg, ...)
{
    va_list args;
    char *tmp;

    va_start(args, msg);

    tmp = g_strdup_vprintf(msg, args);
    rc_debug (RC_DEBUG_LEVEL_WARNING, "* SAX Warning: %s", tmp);
    g_free(tmp);

    va_end(args);
}

static void
sax_error(void *data, const char *msg, ...)
{
    va_list args;
    char *tmp;

    va_start(args, msg);

    tmp = g_strdup_vprintf(msg, args);
    rc_debug (RC_DEBUG_LEVEL_ERROR, "* SAX Error: %s", tmp);
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
    sax_error,      /* error */
    sax_error,      /* fatalError */
};

void
rc_package_sax_context_parse_chunk(RCPackageSAXContext *ctx,
                                   const char *xmlbuf, int size)
{
    gboolean terminate = FALSE;

    xmlSubstituteEntitiesDefault(TRUE);
    
    if (!ctx->xml_context) {
        ctx->xml_context = xmlCreatePushParserCtxt(
            &sax_handler, ctx, NULL, 0, NULL);
    }

    if (xmlbuf[size] == '\0')
        terminate = TRUE;

    xmlParseChunk(ctx->xml_context, xmlbuf, size, terminate);
}

RCPackageSList *
rc_package_sax_context_done(RCPackageSAXContext *ctx)
{
    RCPackageSList *all_packages = NULL;

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

    all_packages = ctx->all_packages;

    g_free(ctx);

    return all_packages;
} /* rc_package_sax_context_done */

RCPackageSAXContext *
rc_package_sax_context_new(RCChannel *channel)
{
    RCPackageSAXContext *ctx;

    ctx = g_new0(RCPackageSAXContext, 1);
    ctx->channel = channel;

    if (getenv ("RC_SPEW_XML"))
        rc_debug (RC_DEBUG_LEVEL_ALWAYS, "* Context created (%p)", ctx);

    return ctx;
}

/* ------ */

struct DepTable {
    RCPackageDepSList *requires;
    RCPackageDepSList *provides;
    RCPackageDepSList *conflicts;
    RCPackageDepSList *obsoletes;
    RCPackageDepSList *children;
    RCPackageDepSList *suggests;
    RCPackageDepSList *recommends;
};

static void
extract_dep_info (const xmlNode *iter, struct DepTable *dep_table)
{

    if (!g_strcasecmp (iter->name, "requires")) {
        const xmlNode *iter2;
        
        iter2 = iter->xmlChildrenNode;

        while (iter2) {
            if (iter2->type != XML_ELEMENT_NODE) {
                iter2 = iter2->next;
                continue;
            }

            dep_table->requires =
                g_slist_prepend (dep_table->requires,
                                 rc_xml_node_to_package_dep (iter2));
            iter2 = iter2->next;
        }

        dep_table->requires = g_slist_reverse (dep_table->requires);

    } else if (!g_strcasecmp (iter->name, "recommends")) {
        const xmlNode *iter2;

        iter2 = iter->xmlChildrenNode;

        while (iter2) {
            if (iter2->type != XML_ELEMENT_NODE) {
                iter2 = iter2->next;
                continue;
            }

            dep_table->recommends =
                g_slist_prepend (dep_table->recommends,
                                 rc_xml_node_to_package_dep (iter2));
            iter2 = iter2->next;
        }

        dep_table->recommends = g_slist_reverse (dep_table->recommends);

    } else if (!g_strcasecmp (iter->name, "suggests")) {
        const xmlNode *iter2;

        iter2 = iter->xmlChildrenNode;

        while (iter2) {
            if (iter2->type != XML_ELEMENT_NODE) {
                iter2 = iter2->next;
                continue;
            }

            dep_table->suggests =
                g_slist_prepend (dep_table->suggests,
                                 rc_xml_node_to_package_dep (iter2));
            iter2 = iter2->next;
        }

        dep_table->suggests = g_slist_reverse (dep_table->suggests);

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
                dep_table->obsoletes =
                    g_slist_prepend (dep_table->obsoletes, dep);
            } else {
                dep_table->conflicts =
                    g_slist_prepend (dep_table->conflicts, dep);
            }
                
            iter2 = iter2->next;
        }

        dep_table->conflicts = g_slist_reverse (dep_table->conflicts);

    } else if (!g_strcasecmp (iter->name, "obsoletes")) {
        const xmlNode *iter2;

        iter2 = iter->xmlChildrenNode;

        while (iter2) {
            if (iter2->type != XML_ELEMENT_NODE) {
                iter2 = iter2->next;
                continue;
            }

            dep_table->obsoletes = 
                g_slist_prepend (dep_table->obsoletes,
                                 rc_xml_node_to_package_dep (iter2));
            iter2 = iter2->next;
        }

        dep_table->obsoletes = g_slist_reverse (dep_table->obsoletes);

    } else if (!g_strcasecmp (iter->name, "provides")) {
        const xmlNode *iter2;

        iter2 = iter->xmlChildrenNode;

        while (iter2) {
            if (iter2->type != XML_ELEMENT_NODE) {
                iter2 = iter2->next;
                continue;
            }

            dep_table->provides =
                g_slist_prepend (dep_table->provides,
                                 rc_xml_node_to_package_dep (iter2));
            iter2 = iter2->next;
        }

        dep_table->provides = g_slist_reverse (dep_table->provides);

    } else if (!g_strcasecmp (iter->name, "children")) {
        const xmlNode *iter2;

        iter2 = iter->xmlChildrenNode;

        while (iter2) {
            if (iter2->type != XML_ELEMENT_NODE) {
                iter2 = iter2->next;
                continue;
            }

            dep_table->children = 
                g_slist_prepend (dep_table->children,
                                 rc_xml_node_to_package_dep (iter2));
            iter2 = iter2->next;
        }

        dep_table->children = g_slist_reverse (dep_table->children);
    }
}

RCPackage *
rc_xml_node_to_package (const xmlNode *node, const RCChannel *channel)
{
    RCPackage *package;
    const xmlNode *iter;
    char *epoch = NULL, *version = NULL, *release = NULL;
    struct DepTable dep_table;

    if (g_strcasecmp (node->name, "package")) {
        return (NULL);
    }

    package = rc_package_new ();

    dep_table.requires = NULL;
    dep_table.provides = NULL;
    dep_table.conflicts = NULL;
    dep_table.children = NULL;
    dep_table.obsoletes = NULL;
    dep_table.suggests = NULL;
    dep_table.recommends = NULL;

    package->channel = (RCChannel *) channel;
    rc_channel_ref ((RCChannel *) channel);

    iter = node->xmlChildrenNode;

    while (iter) {
        gboolean extracted_deps = FALSE;

        if (!g_strcasecmp (iter->name, "name")) {
            gchar *tmp = xml_get_content (iter);
            package->spec.nameq = g_quark_from_string (tmp);
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "epoch")) {
            epoch = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "version")) {
            version = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "release")) {
            release = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "summary")) {
            package->summary = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "description")) {
            package->description = xml_get_content (iter);
        } else if (!g_strcasecmp (iter->name, "section")) {
            gchar *tmp = xml_get_content (iter);
            package->section = rc_string_to_package_section (tmp);
            g_free (tmp);
        } else if (!g_strcasecmp (iter->name, "arch")) {
            gchar *tmp = xml_get_content (iter);
            package->arch = rc_arch_from_string (tmp);
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
        } else if (!g_strcasecmp (iter->name, "deps")) {
            const xmlNode *iter2;

            for (iter2 = iter->xmlChildrenNode; iter2; iter2 = iter2->next) {
                if (iter2->type != XML_ELEMENT_NODE)
                    continue;

                extract_dep_info (iter2, &dep_table);
            }

            extracted_deps = TRUE;
        }
        else {
            if (!extracted_deps)
                extract_dep_info (iter, &dep_table);
            else {
                /* FIXME: Bitch to the user here? */
            }
        }

        iter = iter->next;
    }

    package->requires_a =
        rc_package_dep_array_from_slist (&dep_table.requires);
    package->provides_a =
        rc_package_dep_array_from_slist (&dep_table.provides);
    package->conflicts_a =
        rc_package_dep_array_from_slist (&dep_table.conflicts);
    package->obsoletes_a =
        rc_package_dep_array_from_slist (&dep_table.obsoletes);
    package->children_a = 
        rc_package_dep_array_from_slist (&dep_table.children);
    package->suggests_a =
        rc_package_dep_array_from_slist (&dep_table.suggests);
    package->recommends_a =
        rc_package_dep_array_from_slist (&dep_table.recommends);

    if (version) {

        package->spec.has_epoch = (epoch != NULL);
        package->spec.epoch = epoch ? atoi (epoch) : 0;
        package->spec.version = version;
        package->spec.release = release;

        /* We set these to NULL so that they won't get freed when we
           clean up before returning. */
        version = release = NULL;

    } else if (package->history && package->history->data) {

        /* If possible, we grab the version info from the most
           recent update. */

        RCPackageUpdate *update = package->history->data;

        package->spec.epoch   = update->spec.epoch;
        package->spec.has_epoch = update->spec.has_epoch;
        package->spec.version = g_strdup (update->spec.version);
        package->spec.release = g_strdup (update->spec.release);

    } else {
        
        /* Otherwise, try to find where the package provides itself,
           and use that version info. */
        
        int i;

        if (package->provides_a)
            for (i = 0; i < package->provides_a->len; i++) {
                RCPackageDep *dep = package->provides_a->data[i];
            
                if (rc_package_dep_get_relation (dep) == RC_RELATION_EQUAL &&
                    (RC_PACKAGE_SPEC (dep)->nameq == package->spec.nameq))
                {
                    package->spec.epoch =
                        RC_PACKAGE_SPEC (dep)->epoch;
                    package->spec.has_epoch =
                        RC_PACKAGE_SPEC (dep)->has_epoch;
                    package->spec.version =
                        g_strdup (RC_PACKAGE_SPEC (dep)->version);
                    package->spec.release =
                        g_strdup (RC_PACKAGE_SPEC (dep)->release);
                    break;
                }
            }
    }

    /* clean-up */
    g_free (epoch);
    g_free (version);
    g_free (release);

    /* Hack for no archs in the XML yet */
    if (package->arch == RC_ARCH_UNKNOWN)
        package->arch = rc_arch_get_system_arch ();

    return (package);
} /* rc_xml_node_to_package */

static RCPackageDep *
rc_xml_node_to_package_dep_internal (const xmlNode *node)
{
    gchar *name = NULL, *version = NULL, *release = NULL;
    gboolean has_epoch = FALSE;
    guint32 epoch = 0;
    RCPackageRelation relation;
    RCPackageDep *dep;
    
    gchar *tmp;

    if (g_strcasecmp (node->name, "dep")) {
        return (NULL);
    }

    name = xml_get_prop (node, "name");
    tmp = xml_get_prop (node, "op");
    if (tmp) {
        relation = rc_package_relation_from_string (tmp);
        
        has_epoch = xml_get_guint32_value (node, "epoch", &epoch);
        
        version =
            xml_get_prop (node, "version");
        release =
            xml_get_prop (node, "release");
    } else {
        relation = RC_RELATION_ANY;
    }

    /* FIXME: should get channel from XML */
    dep = rc_package_dep_new (name, has_epoch, epoch, version, release,
                              relation, RC_CHANNEL_ANY, FALSE, FALSE);

    g_free (tmp);
    g_free (name);
    g_free (version);
    g_free (release);

    return dep;
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
    
    update->spec.nameq = package->spec.nameq;

    if (package->channel && package->channel->file_path)
    {
        url_prefix = package->channel->file_path;
    }

    iter = node->xmlChildrenNode;

    while (iter) {
        if (!g_strcasecmp (iter->name, "epoch")) {
            update->spec.epoch =
                xml_get_guint32_content_default (iter, 0);
            update->spec.has_epoch = 1;
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
        } else if (!g_strcasecmp (iter->name, "license")) {
            update->license = xml_get_content (iter);
        }

        iter = iter->next;
    }

    return (update);
}

/* ------ */

xmlNode *
rc_channel_to_xml_node (RCChannel *channel)
{
    xmlNode *node;
    char tmp[128];

    g_return_val_if_fail (channel != NULL, NULL);

    node = xmlNewNode (NULL, "channel");

    xmlNewProp (node, "id", rc_channel_get_id (channel));

    xmlNewProp (node, "name", rc_channel_get_name (channel));

    if (rc_channel_get_alias (channel))
        xmlNewProp (node, "alias", rc_channel_get_alias (channel));

    sprintf (tmp, "%d", rc_channel_subscribed (channel) ? 1 : 0);
    xmlNewProp (node, "subscribed", tmp);

    sprintf (tmp, "%d", rc_channel_get_priority (channel, TRUE));
    xmlNewProp (node, "priority_base", tmp);

    sprintf (tmp, "%d", rc_channel_get_priority (channel, FALSE));
    xmlNewProp (node, "priority_unsubd", tmp);

    return node;
}

/* This hack cleans 8-bit characters out of a string.  This is a very
   problematic "solution" to the problem of non-UTF-8 package info. */
static gchar *
sanitize_string (const char *str)
{
    gchar *dup = g_strdup (str);
    gchar *c;

    return dup;

    if (dup) {
        for (c = dup; *c; ++c) {
            if ((guint)*c > 0x7f)
                *c = '_';
        }
    }

    return dup;
}

xmlNode *
rc_package_to_xml_node (RCPackage *package)
{
    xmlNode *package_node;
    xmlNode *tmp_node;
    xmlNode *deps_node;
    RCPackageUpdateSList *history_iter;
    int i;
    char buffer[128];
    char *tmp_str;

    package_node = xmlNewNode (NULL, "package");

    xmlNewTextChild (package_node, NULL, "name",
                     g_quark_to_string (package->spec.nameq));

    if (package->spec.has_epoch) {
        g_snprintf (buffer, 128, "%d", package->spec.epoch);
        xmlNewTextChild (package_node, NULL, "epoch", buffer);
    }

    xmlNewTextChild (package_node, NULL, "version", package->spec.version);

    if (package->spec.release) {
        xmlNewTextChild (package_node, NULL, "release", package->spec.release);
    }

    tmp_str = sanitize_string (package->summary);
    xmlNewTextChild (package_node, NULL, "summary", tmp_str);
    g_free (tmp_str);

    tmp_str = sanitize_string (package->description);
    xmlNewTextChild (package_node, NULL, "description", tmp_str);
    g_free (tmp_str);

    xmlNewTextChild (package_node, NULL, "arch",
                     rc_arch_to_string (package->arch));

    xmlNewTextChild (package_node, NULL, "section",
                     rc_package_section_to_string (package->section));

    g_snprintf (buffer, 128, "%u", package->file_size);
    xmlNewTextChild (package_node, NULL, "filesize", buffer);

    g_snprintf (buffer, 128, "%u", package->installed_size);
    xmlNewTextChild (package_node, NULL, "installedsize", buffer);

    if (package->history) {
        tmp_node = xmlNewChild (package_node, NULL, "history", NULL);
        for (history_iter = package->history; history_iter;
             history_iter = history_iter->next)
        {
            RCPackageUpdate *update = (RCPackageUpdate *)(history_iter->data);
            xmlAddChild (tmp_node, rc_package_update_to_xml_node (update));
        }
    }

    deps_node = xmlNewChild (package_node, NULL, "deps", NULL);

    if (package->requires_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "requires", NULL);
        for (i = 0; i < package->requires_a->len; i++) {
            RCPackageDep *dep = package->requires_a->data[i];

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->recommends_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "recommends", NULL);
        for (i = 0; i < package->recommends_a->len; i++) {
            RCPackageDep *dep = package->recommends_a->data[i];

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->suggests_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "suggests", NULL);
        for (i = 0; i < package->suggests_a->len; i++) {
            RCPackageDep *dep = package->suggests_a->data[i];

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->conflicts_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "conflicts", NULL);
        for (i = 0; i < package->conflicts_a->len; i++) {
            RCPackageDep *dep = package->conflicts_a->data[i];

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->obsoletes_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "obsoletes", NULL);
        for (i = 0; i < package->obsoletes_a->len; i++) {
            RCPackageDep *dep = package->obsoletes_a->data[i];

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->children_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "children", NULL);
        for (i = 0; i < package->children_a->len; i++) {
            RCPackageDep *dep = package->children_a->data[i];

            xmlAddChild (tmp_node, rc_package_dep_to_xml_node (dep));
        }
    }

    if (package->provides_a) {
        tmp_node = xmlNewChild (deps_node, NULL, "provides", NULL);
        for (i = 0; i < package->provides_a->len; i++) {
            RCPackageDep *dep = package->provides_a->data[i];

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

    if (rc_package_dep_is_or (dep_item)) {
        RCPackageDepSList *dep_or_slist;
        dep_or_slist = rc_dep_string_to_or_dep_slist (
            g_quark_to_string (RC_PACKAGE_SPEC (dep_item)->nameq));
        dep_node = rc_package_dep_or_slist_to_xml_node (dep_or_slist);
        rc_package_dep_slist_free (dep_or_slist);
        return dep_node;
    }

    dep_node = xmlNewNode (NULL, "dep");

    xmlSetProp (dep_node, "name",
                g_quark_to_string (RC_PACKAGE_SPEC (dep_item)->nameq));

    if (rc_package_dep_get_relation (dep_item) != RC_RELATION_ANY) {
        xmlSetProp (dep_node, "op",
                    rc_package_relation_to_string (
                        rc_package_dep_get_relation (dep_item), FALSE));

        if (RC_PACKAGE_SPEC (dep_item)->has_epoch) {
            gchar *tmp;

            tmp = g_strdup_printf ("%d", RC_PACKAGE_SPEC (dep_item)->epoch);
            xmlSetProp (dep_node, "epoch", tmp);
            g_free (tmp);
        }

        if (RC_PACKAGE_SPEC (dep_item)->version) {
            xmlSetProp (dep_node, "version",
                        RC_PACKAGE_SPEC (dep_item)->version);
        }

        if (RC_PACKAGE_SPEC (dep_item)->release) {
            xmlSetProp (dep_node, "release",
                        RC_PACKAGE_SPEC (dep_item)->release);
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

    if (update->spec.has_epoch) {
        tmp_string = g_strdup_printf("%d", update->spec.epoch);
        xmlNewTextChild (update_node, NULL, "epoch", tmp_string);
        g_free(tmp_string);
    }

    xmlNewTextChild (update_node, NULL, "version", update->spec.version);

    if (update->spec.release) {
        xmlNewTextChild (update_node, NULL, "release", update->spec.release);
    }

    if (update->package_url && *update->package_url) {
        xmlNewTextChild (update_node, NULL, "filename",
                         g_basename (update->package_url));
    }

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

    if (update->hid) {
        tmp_string = g_strdup_printf ("%d", update->hid);
        xmlNewTextChild (update_node, NULL, "hid", tmp_string);
        g_free (tmp_string);
    }

    if (update->license)
        xmlNewTextChild (update_node, NULL, "license", update->license);

    return (update_node);
}
