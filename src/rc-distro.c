/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*
 * rc-distro.c
 *
 * Copyright (C) 2000-2002 Ximian, Inc.
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

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <unistd.h>

#include "rc-debug.h"
#include "rc-distro.h"
#include "rc-line-buf.h"
#include "rc-util.h"

#include "distributions-xml.h"

/* A DistroCheck is a predicate that determines if we are on the given
 * distribution */

typedef enum {
    DISTRO_CHECK_TYPE_FILE,
    DISTRO_CHECK_TYPE_OS_NAME,
    DISTRO_CHECK_TYPE_OS_RELEASE,
} DistroCheckType;

typedef struct {
    DistroCheckType  type;
    char            *source;
    char            *substring;
} DistroCheck;

static DistroCheck *
distro_check_new (void)
{
    return g_new0 (DistroCheck, 1);
}

static void
distro_check_free (DistroCheck *check)
{
    g_free (check->source);
    g_free (check->substring);

    g_free (check);
}

/* Look for a given string on a file descriptor */

typedef struct {
    GMainLoop  *loop;
    const char *target;
    gboolean    found;
} DistroCheckEvalFDInfo;

static void
distro_check_eval_fd_read_line_cb (RCLineBuf *line_buf, const char *line,
                       gpointer user_data)
{
    DistroCheckEvalFDInfo *info = user_data;

    if (strstr (line, info->target)) {
        info->found = TRUE;
        g_main_quit (info->loop);
    }
}

static void
distro_check_eval_fd_read_done_cb (RCLineBuf *line_buf, RCLineBufStatus status,
                       gpointer user_data)
{
    DistroCheckEvalFDInfo *info = user_data;

    g_main_quit (info->loop);
}

static gboolean
distro_check_eval_fd (DistroCheck *check, int fd)
{
    RCLineBuf *line_buf;
    GMainLoop *loop;
    DistroCheckEvalFDInfo info;

    line_buf = rc_line_buf_new ();
    rc_line_buf_set_fd (line_buf, fd);

    loop = g_main_new (FALSE);
    info.loop = loop;

    info.target = check->substring;
    info.found = FALSE;

    g_signal_connect (line_buf, "read_line",
                      (GCallback) distro_check_eval_fd_read_line_cb, &info);
    g_signal_connect (line_buf, "read_done",
                      (GCallback) distro_check_eval_fd_read_done_cb, &info);

    g_main_run (loop);

    g_object_unref (line_buf);

    g_main_destroy (loop);

    return info.found;
}

/* Look in a file for a string */

static gboolean
distro_check_file_eval (DistroCheck *check)
{
    int fd;
    gboolean ret;

    if (!check->source) {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "incomplete distro check tag: no source for file check");
        return FALSE;
    }

    if ((fd = open (check->source, O_RDONLY)) == -1)
        return FALSE;

    ret = distro_check_eval_fd (check, fd);

    /* FIXME: rc_close */
    close (fd);

    return ret;
}

static gboolean
distro_check_os_info (DistroCheck *check)
{
    struct utsname uname_buf;
    char *buf_to_check;

    if (uname (&uname_buf) < 0) {
        g_warning ("Unable to get uname info");
        return FALSE;
    }

    switch (check->type) {
    case DISTRO_CHECK_TYPE_OS_NAME:
        buf_to_check = uname_buf.sysname;
        break;
    case DISTRO_CHECK_TYPE_OS_RELEASE:
        buf_to_check = uname_buf.release;
        break;
    default:
        g_assert_not_reached ();
        return FALSE;
    }

    return (strstr (buf_to_check, check->substring) != NULL);
}

/* Run a check for a distribution */

static gboolean
distro_check_eval (DistroCheck *check)
{
    switch (check->type) {
    case DISTRO_CHECK_TYPE_FILE:
        return distro_check_file_eval (check);
    case DISTRO_CHECK_TYPE_OS_NAME:
    case DISTRO_CHECK_TYPE_OS_RELEASE:
        return distro_check_os_info (check);
    }

    return FALSE;
}

/* Run a list of checks (results are ANDed together) */

static gboolean
distro_check_eval_list (GSList *checks)
{
    GSList *iter;
    gboolean ret = TRUE;

    for (iter = checks; iter; iter = iter->next) {
        DistroCheck *check = iter->data;

        ret = ret && distro_check_eval (check);
    }

    return ret;
}

/* A distribution record */

struct _RCDistro {
    char                *name;
    char                *version;
    RCArch               arch;
    RCDistroPackageType  type;
    char                *target;
    RCDistroStatus       status;
    time_t               death_date;
};

static RCDistro *
rc_distro_new (void)
{
    return g_new0 (RCDistro, 1);
}

void
rc_distro_free (RCDistro *distro)
{
    g_free (distro->name);
    g_free (distro->version);
    g_free (distro->target);

    g_free (distro);
}

/* The SAX parser for the distributions.xml file */

typedef enum {
    PARSER_DISTROS,
    PARSER_DISTRO,
    PARSER_NAME,
    PARSER_VERSION,
    PARSER_ARCH,
    PARSER_TYPE,
    PARSER_TARGET,
    PARSER_STATUS,
    PARSER_ENDDATE,
    PARSER_DETECT,
    PARSER_FILE,
    PARSER_OS_NAME,
    PARSER_OS_RELEASE,
    PARSER_UNKNOWN,
} ParserState;

typedef struct {
    /* A stack of the various states we've walked through */
    GSList        *state;

    /* Our current potential distribution, and the checks for it */
    RCDistro      *cur_distro;
    GSList        *cur_checks;

    /* Character data accumulator */
    GString       *text_buffer;

    /* Once we find a matching distro, it gets returned here */
    RCDistro      *our_distro;

    /* The compatability list for our system architecture */ 
    GSList        *compat_arch_list;

    /* A backpointer to our parser context */
    xmlParserCtxt *ctxt;
} DistroParseState;

static void
parser_push_state (DistroParseState *state, ParserState new_state)
{
    state->state = g_slist_prepend (state->state,
                                    GINT_TO_POINTER (new_state));
}

static ParserState
parser_pop_state (DistroParseState *state)
{
    ParserState ret;

    ret = GPOINTER_TO_INT (state->state->data);
    state->state = g_slist_delete_link (state->state, state->state);

    return ret;
}

static ParserState
parser_get_state (DistroParseState *state)
{
    return GPOINTER_TO_INT (state->state->data);
}

static void
sax_start_document (void *data)
{
    DistroParseState *state = data;

    state->state = NULL;
    state->cur_distro = NULL;
    state->cur_checks = NULL;
    state->our_distro = NULL;
    state->compat_arch_list =
        rc_arch_get_compat_list (rc_arch_get_system_arch ());
    state->text_buffer = g_string_new ("");
}

static void
sax_end_document (void *data)
{
    DistroParseState *state = data;

    g_slist_free (state->compat_arch_list);
    g_string_free (state->text_buffer, TRUE);
}

static void
sax_start_element (void *data, const xmlChar *name, const xmlChar **attrs)
{
    DistroParseState *state = data;
    DistroCheck *check = NULL;
    int i;

    state->text_buffer = g_string_truncate (state->text_buffer, 0);

    if (!strcmp (name, "distros")) {
        parser_push_state (state, PARSER_DISTROS);
        return;
    }

    if (!strcmp (name, "distro")) {
        if (parser_get_state (state) == PARSER_DISTROS) {
            parser_push_state (state, PARSER_DISTRO);
            state->cur_distro = rc_distro_new ();
        } else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "name")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_NAME);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "version")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_VERSION);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "arch")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_ARCH);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "type")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_TYPE);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "target")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_TARGET);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "status")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_STATUS);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "enddate")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_ENDDATE);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "detect")) {
        if (parser_get_state (state) == PARSER_DISTRO)
            parser_push_state (state, PARSER_DETECT);
        else
            parser_push_state (state, PARSER_UNKNOWN);
        return;
    }

    if (!strcmp (name, "file")) {
        if (parser_get_state (state) == PARSER_DETECT) {
            parser_push_state (state, PARSER_FILE);
            check = distro_check_new ();
            check->type = DISTRO_CHECK_TYPE_FILE;
        } else
            parser_push_state (state, PARSER_UNKNOWN);
        goto CHECK;
    }

    if (!strcmp (name, "os_name")) {
        if (parser_get_state (state) == PARSER_DETECT) {
            parser_push_state (state, PARSER_OS_NAME);
            check = distro_check_new ();
            check->type = DISTRO_CHECK_TYPE_OS_NAME;
        } else
            parser_push_state (state, PARSER_UNKNOWN);
        goto CHECK;
    }

    if (!strcmp (name, "os_release")) {
        if (parser_get_state (state) == PARSER_DETECT) {
            parser_push_state (state, PARSER_OS_RELEASE);
            check = distro_check_new ();
            check->type = DISTRO_CHECK_TYPE_OS_RELEASE;
        } else
            parser_push_state (state, PARSER_UNKNOWN);
        goto CHECK;
    }
        
    parser_push_state (state, PARSER_UNKNOWN);
    return;

  CHECK:
    for (i = 0; attrs[i]; i += 2) {
        if (!strcmp (attrs[i], "source"))
            check->source = g_strdup (attrs[i + 1]);
        else if (!strcmp (attrs[i], "substring"))
            check->substring = g_strdup (attrs[i + 1]);
    }

    if (check->substring)
        state->cur_checks = g_slist_prepend (state->cur_checks, check);
    else {
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "incomplete distro check tag: no substring");
        distro_check_free (check);
    }
}

static char *
parser_get_chars (DistroParseState *state)
{
    char *ret;

    ret = g_strdup (state->text_buffer->str);
    g_string_truncate (state->text_buffer, 0);

    return ret;
}

static void
sax_warning (void *data, const char *msg, ...)
{
    va_list args;
    char *tmp;

    va_start (args, msg);

    tmp = g_strdup_vprintf (msg, args);
    rc_debug (RC_DEBUG_LEVEL_WARNING, tmp);
    g_free (tmp);

    va_end (args);
}

static void sax_parser_disable (DistroParseState *state);

static void
sax_end_element (void *data, const xmlChar *name)
{
    DistroParseState *state = data;
    char *tmp;

    switch (parser_pop_state (state)) {
    case PARSER_NAME:
        state->cur_distro->name = parser_get_chars (state);
        break;
    case PARSER_VERSION:
        state->cur_distro->version = parser_get_chars (state);
        break;
    case PARSER_ARCH:
        tmp = parser_get_chars (state);
        state->cur_distro->arch = rc_arch_from_string (tmp);
        g_free (tmp);
        break;
    case PARSER_TYPE:
        tmp = parser_get_chars (state);
        if (!strcmp (tmp, "rpm"))
            state->cur_distro->type = RC_DISTRO_PACKAGE_TYPE_RPM;
        else if (!strcmp (tmp, "dpkg"))
            state->cur_distro->type = RC_DISTRO_PACKAGE_TYPE_DPKG;
        else
            state->cur_distro->type = RC_DISTRO_PACKAGE_TYPE_UNKNOWN;
        g_free (tmp);
        break;
    case PARSER_TARGET:
        state->cur_distro->target = parser_get_chars (state);
        break;
    case PARSER_STATUS:
        tmp = parser_get_chars (state);
        if (!strcmp (tmp, "unsupported"))
            state->cur_distro->status = RC_DISTRO_STATUS_UNSUPPORTED;
        else if (!strcmp (tmp, "presupported"))
            state->cur_distro->status = RC_DISTRO_STATUS_PRESUPPORTED;
        else if (!strcmp (tmp, "supported"))
            state->cur_distro->status = RC_DISTRO_STATUS_SUPPORTED;
        else if (!strcmp (tmp, "deprecated"))
            state->cur_distro->status = RC_DISTRO_STATUS_DEPRECATED;
        else if (!strcmp (tmp, "retired"))
            state->cur_distro->status = RC_DISTRO_STATUS_RETIRED;
        else
            state->cur_distro->status = RC_DISTRO_STATUS_UNSUPPORTED;
        g_free (tmp);
        break;
    case PARSER_ENDDATE:
        tmp = parser_get_chars (state);
        state->cur_distro->death_date = strtoul (tmp, NULL, 10);
        g_free (tmp);
        break;
    case PARSER_DETECT:
        /* We'll run the checks when we finish the distro tag, not the
         * detect tag */
    case PARSER_FILE:
    case PARSER_OS_NAME:
    case PARSER_OS_RELEASE:
        /* All handled by attributes */
        break;
    case PARSER_DISTRO:
        if (getenv ("RC_DISTRO_TARGET")) {
            if (strcmp (getenv ("RC_DISTRO_TARGET"),
                        state->cur_distro->target) == 0)
            {
                state->our_distro = state->cur_distro;
                sax_parser_disable (state);
            } else
                rc_distro_free (state->cur_distro);
        }
        else {
            if (rc_arch_get_compat_score (
                    state->compat_arch_list, state->cur_distro->arch) > -1 &&
                distro_check_eval_list (state->cur_checks))
            {
                state->our_distro = state->cur_distro;
                sax_parser_disable (state);
            } else
                rc_distro_free (state->cur_distro);
        }
        g_slist_foreach (state->cur_checks, (GFunc) distro_check_free, NULL);
        g_slist_free (state->cur_checks);
        state->cur_checks = NULL;
        break;
    case PARSER_DISTROS:
        break;
    case PARSER_UNKNOWN:
        break;
    }
}

static void
sax_characters (void *data, const xmlChar *chars, int len)
{
    DistroParseState *state = data;

    state->text_buffer = g_string_append_len (
        state->text_buffer, chars, len);
}

static void
sax_parser_enable (DistroParseState *state)
{
    state->ctxt->sax->startDocument = sax_start_document;
    state->ctxt->sax->endDocument = sax_end_document;
    state->ctxt->sax->startElement = sax_start_element;
    state->ctxt->sax->endElement = sax_end_element;
    state->ctxt->sax->characters = sax_characters;

    state->ctxt->sax->warning = sax_warning;
    state->ctxt->sax->error = sax_warning;
    state->ctxt->sax->fatalError = sax_warning;

    state->ctxt->sax->initialized = 1;
}

static void
sax_parser_disable (DistroParseState *state)
{
    state->ctxt->sax->startDocument = 0;
    state->ctxt->sax->endDocument = 0;
    state->ctxt->sax->startElement = 0;
    state->ctxt->sax->endElement = 0;
    state->ctxt->sax->characters = 0;
}

/* Public methods */

RCDistro *
rc_distro_parse_xml (const char *data, guint size)
{
    DistroParseState state;
    xmlParserCtxt *ctxt;
    GByteArray *byte_array = NULL;
    const char *buf;

    if (data == NULL) {
        char *distro_file = getenv ("RC_DISTRIBUTIONS_FILE");

        if (distro_file) {
            RCBuffer *buffer;
            RCDistro *distro;

            if (!g_file_test (distro_file, G_FILE_TEST_EXISTS)) {
                rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                          "RC_DISTRIBUTIONS_FILE %s does not exist",
                          distro_file);
                goto ERROR;
            }

            buffer = rc_buffer_map_file (distro_file);
            if (!buffer) {
                rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                          "Unable to map RC_DISTRIBUTIONS_FILE %s",
                          distro_file);
                goto ERROR;
            }

            distro = rc_distro_parse_xml (buffer->data, buffer->size);

            if (!distro) {
                rc_debug (RC_DEBUG_LEVEL_CRITICAL,
                          "Unable to parse RC_DISTRIBUTIONS_FILE %s",
                          distro_file);
                rc_buffer_unmap_file (buffer);
                goto ERROR;
            }

            rc_buffer_unmap_file (buffer);

            return distro;
        } else {
            data = distros_xml;
            size = distros_xml_len;
        }
    }

    if (rc_memory_looks_compressed (data, size)) {
        if (rc_uncompress_memory (data, size, &byte_array)) {
            rc_debug (RC_DEBUG_LEVEL_WARNING, "Uncompression failed");
            return NULL;
        }

        buf = byte_array->data;
        size = byte_array->len;
    }
    else
        buf = data;

    ctxt = xmlCreatePushParserCtxt (NULL, NULL, NULL, 0, NULL);
    if (!ctxt)
        goto ERROR;

    state.ctxt = ctxt;

    sax_parser_enable (&state);

    ctxt->userData = &state;

    xmlParseChunk(ctxt, buf, size, 1);

    if (!ctxt->wellFormed)
        rc_debug (RC_DEBUG_LEVEL_WARNING,
                  "distribution list not well formed");
    xmlFreeParserCtxt (ctxt);

    if (state.our_distro) {
        if (byte_array) {
            g_byte_array_free (byte_array, TRUE);
            byte_array = NULL;
        }

        if (state.our_distro->name &&
            state.our_distro->version &&
            state.our_distro->target)
            return state.our_distro;
        else
            rc_distro_free (state.our_distro);
    }

  ERROR:
    if (byte_array)
        g_byte_array_free (byte_array, TRUE);

    return NULL;
}

const char *
rc_distro_get_name (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, NULL);

    return distro->name;
}

const char *
rc_distro_get_version (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, NULL);

    return distro->version;
}

RCArch
rc_distro_get_arch (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, RC_ARCH_UNKNOWN);

    return distro->arch;
}

RCDistroPackageType
rc_distro_get_package_type (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, RC_DISTRO_PACKAGE_TYPE_UNKNOWN);

    return distro->type;
}

const char *
rc_distro_get_target (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, NULL);

    return distro->target;
}

RCDistroStatus
rc_distro_get_status (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, RC_DISTRO_STATUS_UNSUPPORTED);

    return distro->status;
}

time_t
rc_distro_get_death_date (RCDistro *distro)
{
    g_return_val_if_fail (distro != NULL, -1);

    return distro->death_date;
}
