/* This is -*- C -*- */
/* vim: set sw=2: */
/* $Id$ */

/*
 * pyutil.c
 *
 * Copyright (C) 2003 Ximian, Inc.
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

#include "pyutil.h"

static void
py_default_dealloc (PyObject *self)
{
  if (self->ob_type->tp_free)
    self->ob_type->tp_free (self);
  else
    PyObject_Del (self);
}

static void
py_default_free (PyObject *self)
{
  PyObject_Del (self);
}

static PyObject *
py_default_alloc (PyTypeObject *type, int nitems)
{
  return PyObject_New (PyObject, type);
}

void
pyutil_register_type (PyObject *dict,
		      PyTypeObject *type)
{

  /* Set some useful default values in the type */

  if (type->tp_getattro == NULL)
    type->tp_getattro = PyObject_GenericGetAttr;

  if (type->tp_setattro == NULL)
    type->tp_setattro = PyObject_GenericSetAttr;

  if (type->tp_dealloc == NULL)
    type->tp_dealloc = py_default_dealloc;

  if (type->tp_free == NULL)
    type->tp_free = py_default_free;

  if (type->tp_alloc == NULL)
    type->tp_alloc = py_default_alloc;


  if (type->tp_flags == 0)
    type->tp_flags = Py_TPFLAGS_DEFAULT;

  PyType_Ready (type);
  PyDict_SetItemString (dict, type->tp_name, (PyObject *) type);
}

void
pyutil_register_methods (PyObject *dict, PyMethodDef *methods)
{
	PyObject *v;
	PyMethodDef *ml;

	for (ml = methods; ml->ml_name != NULL; ml++) {
		v = PyCFunction_New (ml, NULL);
		if (v == NULL)
			return;
		if (PyDict_SetItemString (dict, ml->ml_name, v) != 0) {
			Py_DECREF (v);
			return;
		}
		Py_DECREF (v);
	}
}

void
pyutil_register_int_constant (PyObject *dict, const char *name, int value)
{
	PyDict_SetItemString (dict, (char *) name, PyInt_FromLong(value));
}
