/* This is -*- C -*- */
/* vim: set sw=2: */
/* $Id$ */

/*
 * redcarpet.c
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

#include <Python.h>

#include "package-importance.h"
#include "package-spec.h"
#include "package-dep.h"
#include "package-match.h"
#include "package.h"
#include "packman.h"
#include "channel.h"
#include "world.h"
#include "resolver-info.h"
#include "resolver-context.h"
#include "resolver-queue.h"
#include "resolver.h"

static PyMethodDef redcarpet_methods[] = {

  { NULL, NULL, 0, NULL }
};

typedef void (*InitFns) (void);
static InitFns redcarpet_init_fns[] = {
  NULL
};

typedef void (*RegistrationFn) (PyObject *dict);
static RegistrationFn redcarpet_registration_fns[] = {
	PyPackageImportance_register,
	PyPackageSpec_register,
	PyPackageDep_register,
	PyPackageMatch_register,
	PyPackage_register,
	PyPackman_register,
	PyChannel_register,
	PyWorld_register,
	PyResolverInfo_register,
	PyResolverContext_register,
	PyResolverQueue_register,
	PyResolver_register,
	NULL
};

void
initxxx_redcarpet (void)
{
  PyObject *m, *d;
  int i;

  g_type_init ();
  rc_distro_parse_xml (NULL, 0);
  
  m = Py_InitModule ("xxx_redcarpet", redcarpet_methods);
  d = PyModule_GetDict (m);

  for (i = 0; redcarpet_init_fns[i]; ++i)
    redcarpet_init_fns[i] ();

  for (i = 0; redcarpet_registration_fns[i]; ++i)
    redcarpet_registration_fns[i] (d);
}

