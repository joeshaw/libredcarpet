/* This is -*- C -*- */
/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* $Id$ */

/*
 * rc-package-match.c
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

#include <config.h>
#include "rc-package-match.h"

#include <glib.h>
#include "rc-world.h"
#include "rc-xml.h"

struct _RCPackageMatch {
  char *channel_id;

  RCPackageDep *dep;

  char *name_glob;
  GPatternSpec *pattern_spec;

  RCPackageImportance importance;
  gboolean importance_gteq;
};

RCPackageMatch *
rc_package_match_new (void)
{
  RCPackageMatch *match;

  match = g_new0 (RCPackageMatch, 1);

  match->importance = RC_IMPORTANCE_INVALID;

  return match;
}

void
rc_package_match_free (RCPackageMatch *match)
{
  if (match != NULL) {
    g_free (match->channel_id);
    rc_package_dep_unref (match->dep);
    g_free (match->name_glob);
    if (match->pattern_spec)
      g_pattern_spec_free (match->pattern_spec);
    g_free (match);
  }
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
rc_package_match_set_channel (RCPackageMatch *match,
			      RCChannel *channel)
{
  g_return_if_fail (match != NULL);
  g_free (match->channel_id);
  if (channel)
    rc_package_match_set_channel_id (match, rc_channel_get_id (channel));
  else
    rc_package_match_set_channel_id (match, rc_channel_get_id (channel));
}

void
rc_package_match_set_channel_id (RCPackageMatch *match,
				 const char     *cid)
{
  g_return_if_fail (match != NULL);
  g_free (match->channel_id);
  match->channel_id = g_strdup (cid);
}

const char *
rc_package_match_get_channel_id (RCPackageMatch *match)
{
  g_return_val_if_fail (match != NULL, NULL);
  return match->channel_id;
}
	
void
rc_package_match_set_dep (RCPackageMatch *match,
			  RCPackageDep   *dep)
{
  RCPackageDep *new_dep;
  g_return_if_fail (match != NULL);
  new_dep = rc_package_dep_ref (dep);
  rc_package_dep_unref (match->dep);
  match->dep = new_dep;
}

RCPackageDep *
rc_package_match_get_dep (RCPackageMatch *match)
{
  g_return_val_if_fail (match != NULL, NULL);
  return match->dep;
}

void
rc_package_match_set_glob (RCPackageMatch *match,
			   const char     *glob_str)
{
  g_return_if_fail (match != NULL);

  g_free (match->name_glob);
  if (match->pattern_spec) {
    g_pattern_spec_free (match->pattern_spec);
    match->pattern_spec = NULL;
  }

  match->name_glob = g_strdup (glob_str);
  if (match->name_glob) {
    match->pattern_spec = g_pattern_spec_new (match->name_glob);
  }
}

const char *
rc_package_match_get_glob (RCPackageMatch *match)
{
  g_return_val_if_fail (match != NULL, NULL);
  return match->name_glob;
}

void
rc_package_match_set_importance (RCPackageMatch *match,
				 RCPackageImportance importance,
				 gboolean match_gteq)
{
  g_return_if_fail (match != NULL);
  g_return_if_fail (RC_IMPORTANCE_INVALID < importance
		    && importance < RC_IMPORTANCE_LAST);

  match->importance = importance;
  match->importance_gteq = match_gteq;
}

RCPackageImportance
rc_package_match_get_importance (RCPackageMatch *match,
				 gboolean       *match_gteq)
{
  g_return_val_if_fail (match != NULL, RC_IMPORTANCE_INVALID);
  
  if (match_gteq)
    *match_gteq = match->importance_gteq;
  return match->importance;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_package_match_equal (RCPackageMatch *a,
			RCPackageMatch *b)
{
  g_return_val_if_fail (a != NULL, FALSE);
  g_return_val_if_fail (b != NULL, FALSE);

  /* Check the name glob */
  if ((a->name_glob == NULL) ^ (b->name_glob == NULL))
    return FALSE;
  if (a->name_glob && b->name_glob && strcmp (a->name_glob, b->name_glob))
    return FALSE;

  /* Check the channel */
  if ((a->channel_id == NULL) ^ (b->channel_id == NULL))
    return FALSE;
  if (a->channel_id && b->channel_id && strcmp (a->channel_id, b->channel_id))
    return FALSE;

  /* Check the importance */
  if (a->importance != b->importance
      || a->importance_gteq != b->importance_gteq)
    return FALSE;

  /* Check the dep */
  if ((a->dep == NULL) ^ (b->dep == NULL))
    return FALSE;
  if (a->dep && b->dep) {
    if (rc_package_spec_not_equal (RC_PACKAGE_SPEC (a->dep),
				   RC_PACKAGE_SPEC (b->dep))
	|| (rc_package_dep_get_relation (a->dep) 
	    != rc_package_dep_get_relation(b->dep)))
      return FALSE;
  }

  return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

gboolean
rc_package_match_test (RCPackageMatch *match,
		       RCPackage      *pkg,
		       RCWorld        *world)
{
  const char *pkg_name;

  g_return_val_if_fail (match != NULL, FALSE);
  g_return_val_if_fail (pkg != NULL, FALSE);
  
  if (pkg->channel && match->channel_id) {
    if (! rc_channel_equal_id (pkg->channel, match->channel_id))
      return FALSE;
  }

  pkg_name = g_quark_to_string (RC_PACKAGE_SPEC (pkg)->nameq);

  if (match->pattern_spec &&
      ! g_pattern_match_string (match->pattern_spec, pkg_name))
    return FALSE;

  if (match->importance != RC_IMPORTANCE_INVALID && ! rc_package_is_installed (pkg)) {
    RCPackageUpdate *up = rc_package_get_latest_update (pkg);
    if (up) {
      if (match->importance_gteq ? up->importance > match->importance
	  : up->importance < match->importance)
	return FALSE;
    }
  }

  if (match->dep) {
    RCPackman *packman;
    RCPackageDep *pkg_dep;
    gboolean check;

    packman = rc_packman_get_global ();
    g_assert (packman != NULL);

    pkg_dep = rc_package_dep_new_from_spec (RC_PACKAGE_SPEC (pkg),
					    RC_RELATION_EQUAL,
					    pkg->channel,
					    FALSE, FALSE);
    check = rc_package_dep_verify_relation (packman, match->dep, pkg_dep);
    rc_package_dep_unref (pkg_dep);
    return check;
  }

  return TRUE;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

xmlNode *
rc_package_match_to_xml_node (RCPackageMatch *match)
{
  xmlNode *node;

  g_return_val_if_fail (match != NULL, NULL);

  node = xmlNewNode (NULL, "match");

  if (match->channel_id) {
    xmlNewTextChild (node, NULL, "channel", match->channel_id);
  }

  if (match->dep) {
    xmlAddChild (node, rc_package_dep_to_xml_node (match->dep));
  }

  if (match->name_glob) {
    xmlNewTextChild (node, NULL, "glob", match->name_glob);
  }

  if (match->importance != RC_IMPORTANCE_INVALID) {
    xmlNode *imp_node;
    imp_node = xmlNewTextChild (node, NULL, "importance",
				rc_package_importance_to_string (match->importance));
    xmlSetProp (imp_node, "gteq", match->importance_gteq ? "1" : "0");
  }

  return node;
}

RCPackageMatch *
rc_package_match_from_xml_node (xmlNode *node,
				RCWorld *world)
{
  RCPackageMatch *match;

  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (world != NULL, NULL);

  if (strcasecmp (node->name, "match"))
    return NULL;

  match = rc_package_match_new ();

  node = node->xmlChildrenNode;
  while (node) {

    if (! g_strcasecmp (node->name, "channel")) {

      char *cid = xml_get_content (node);
      rc_package_match_set_channel_id (match, cid);
      g_free (cid);

    } else if (! g_strcasecmp (node->name, "dep")) {

      RCPackageDep *dep;
      dep = rc_xml_node_to_package_dep (node);
      rc_package_match_set_dep (match, dep);
      rc_package_dep_unref (dep);

    } else if (! g_strcasecmp (node->name, "glob")) {

      gchar *tmp = xml_get_content (node);
      rc_package_match_set_glob (match, tmp);
      g_free (tmp);

    } else if (! g_strcasecmp (node->name, "importance")) {

      gchar *imp_str = xml_get_content (node);
      gchar *gteq_str = xml_get_prop (node, "gteq");
      RCPackageImportance imp;

      imp = rc_string_to_package_importance (imp_str);
      /* default to true if the geteq property is missing */
      rc_package_match_set_importance (match, imp,
				       gteq_str ? atoi (gteq_str) : TRUE);

      g_free (imp_str);
      g_free (gteq_str);
    }
    
    node = node->next;
  }

  return match;
}

  
