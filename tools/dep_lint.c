/* This is -*- C -*- */
/* vim: set sw=2: */
/* $Id$ */

/*
 * dep_lint.c
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
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

#include "libredcarpet.h"

static gboolean
try_to_install_cb (RCPackage *pkg, gpointer user_data)
{
  RCWorld *world = rc_get_world ();
  RCResolver *resolver;
  RCPackage *installed = rc_world_find_installed_version (world, pkg);

  /* If this exact version of this package is already installed,
     do nothing. */
  if (installed != NULL
      && rc_package_spec_equal (RC_PACKAGE_SPEC (pkg),
				RC_PACKAGE_SPEC (installed)))
    return TRUE;

  g_print ("-----------------------------------------------\n");
  g_print ("%s\n", rc_package_to_str_static (pkg));
  g_print ("\n");

  /* Set up our resolver */
  resolver = rc_resolver_new ();
  rc_resolver_set_world (resolver, world);
  rc_resolver_set_timeout (resolver, 60);
  rc_resolver_add_package_to_install (resolver, pkg);
  
  rc_resolver_resolve_dependencies (resolver);

  if (resolver->timed_out) {
    g_print ("FAILED: timed out\n\n");
  } else if (resolver->best_context == NULL) {
    RCResolverContext *context;
    g_print ("FAILED: dependency errors\n\n");
    context = ((RCResolverQueue *)resolver->invalid_queues->data)->context;
    rc_resolver_context_spew_info (context);
  } else {
    g_print ("SUCCEEDED\n\n");
    rc_resolver_context_spew (resolver->best_context);
  }

  g_print ("\n");

  rc_resolver_free (resolver);

  return TRUE;
}

void
try_to_install_everything (void)
{
  RCWorld *world = rc_get_world ();

  rc_world_foreach_package (world,
			    RC_CHANNEL_NON_SYSTEM,
			    try_to_install_cb,
			    NULL);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

int
main (int argc, char *argv[])
{
  RCPackman *packman;
  RCWorld *world;

  if (argc != 2) {
    fprintf (stderr, "Usage: %s <rc dump file>\n", argv[0]);
    exit (-1);
  }

  g_type_init ();
  rc_distro_parse_xml (NULL, 0);

  packman = rc_distman_new ();

  fprintf (stderr, "Undumping %s...\n", argv[1]);

  world = rc_world_undump_new (argv[1]);
  rc_set_world (world);

  try_to_install_everything ();

  return 0;
}
