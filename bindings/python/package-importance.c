/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-importance.c
 *
 * Copyright (C) 2003 The Free Software Foundation, Inc.
 *
 * Developed by Tambet Ingo <tambet@ximian.com>
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

#include "package-importance.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackageImportance importance;
} PyPackageImportance;

static PyTypeObject PyPackageImportance_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"PackageImportance",
	sizeof (PyPackageImportance),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackageImportance_tp_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyPackageImportance *py_importance = (PyPackageImportance *) self;
	RCPackageImportance importance;

	if (! PyArg_ParseTuple (args, "i", &importance))
		return -1;

	if (importance < RC_IMPORTANCE_INVALID ||
	    importance >= RC_IMPORTANCE_LAST) {
		PyErr_SetString (PyExc_RuntimeError, "Invalid parameter");
		return -1;
	}

	py_importance->importance = importance;

	return 0;
}

static PyObject *
PyPackageImportance_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackageImportance *py_importance;

	py_importance = (PyPackageImportance *) type->tp_alloc (type, 0);
	py_importance->importance = RC_IMPORTANCE_INVALID;

	return (PyObject *) py_importance;
}

static PyObject *
PyPackageImportance_tp_str (PyObject *self)
{
	PyPackageImportance *py_importance = (PyPackageImportance *) self;
	return Py_BuildValue ("s",
					  rc_package_importance_to_string (py_importance->importance));
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackageImportance_register (PyObject *dict)
{
	PyPackageImportance_type_info.tp_init = PyPackageImportance_tp_init;
	PyPackageImportance_type_info.tp_new  = PyPackageImportance_tp_new;
	PyPackageImportance_type_info.tp_str  = PyPackageImportance_tp_str;

	pyutil_register_type (dict, &PyPackageImportance_type_info);

	pyutil_register_int_constant (dict, "IMPORTANCE_INVALID",
							RC_IMPORTANCE_INVALID);
	pyutil_register_int_constant (dict, "IMPORTANCE_NECESSARY",
							RC_IMPORTANCE_NECESSARY);
	pyutil_register_int_constant (dict, "IMPORTANCE_URGENT",
							RC_IMPORTANCE_URGENT);
	pyutil_register_int_constant (dict, "IMPORTANCE_SUGGESTED",
							RC_IMPORTANCE_SUGGESTED);
	pyutil_register_int_constant (dict, "IMPORTANCE_FEATURE",
							RC_IMPORTANCE_FEATURE);
	pyutil_register_int_constant (dict, "IMPORTANCE_MINOR",
							RC_IMPORTANCE_MINOR);
}

int
PyPackageImportance_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackageImportance_type_info);
}

PyObject *
PyPackageImportance_new (RCPackageImportance importance)
{
	PyObject *py_importance = PyPackageImportance_tp_new (&PyPackageImportance_type_info,
						    NULL, NULL);
	((PyPackageImportance *) importance)->importance = importance;

	return py_importance;
}

RCPackageImportance
PyPackageImportance_get_package_importance (PyObject *obj)
{
	if (! PyPackageImportance_check (obj))
		return RC_IMPORTANCE_INVALID;

	return ((PyPackageImportance *) obj)->importance;
}
