/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-dep.c
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

#include "package-dep.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackageDep *dep;
} PyPackageDep;

static PyTypeObject PyPackageDep_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"PackageDep",
	sizeof (PyPackageDep),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackageDep_get_relation (PyObject *self, PyObject *args)
{
	RCPackageDep *dep = PyPackageDep_get_package_dep (self);
	return Py_BuildValue ("i", rc_package_dep_get_relation (dep));
}

static PyMethodDef PyPackageDep_methods[] = {
	{ "get_relation", PyPackageDep_get_relation, METH_NOARGS  },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
package_relation_from_string (PyObject *self, PyObject *args)
{
	const gchar *buf;

	if (! PyArg_ParseTuple (args, "s", &buf))
		return NULL;

	return Py_BuildValue ("i", rc_package_relation_from_string (buf));
}

static PyObject *
package_relation_to_string (PyObject *self, PyObject *args)
{
	RCPackageRelation relation;
	gint words;

	if (! PyArg_ParseTuple (args, "ii", &relation, &words))
		return NULL;

	return Py_BuildValue ("s", rc_package_relation_to_string (relation, words));
}

static PyMethodDef PyPackageDep_unbound_methods[] = {
	{ "package_relation_from_string", package_relation_from_string, METH_VARARGS },
	{ "package_relation_to_string",   package_relation_to_string,   METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackageDep_init (PyObject *self, PyObject *args, PyObject *kwds)
{
#if 0
	/* FIXME */
	PyPackageDep *py_dep = (PyPackageDep *) self;

	py_dep->dep = rc_package_dep_new_from_spec ();

	if (py_dep->dep == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create PackageDep");
		return -1;
	}
#endif
	return 0;
}

static PyObject *
PyPackageDep_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackageDep *py_dep;

	py_dep = (PyPackageDep *) type->tp_alloc (type, 0);
	py_dep->dep = NULL;

	return (PyObject *) py_dep;
}

static PyObject *
PyPackageDep_tp_str (PyObject *self)
{
	PyPackageDep *py_dep = (PyPackageDep *) self;
	RCPackageDep *dep = py_dep->dep;
	const char *str;

	if (dep == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	str = rc_package_dep_to_string_static (dep);
	if (str == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (str);
}

static void
PyPackageDep_tp_dealloc (PyObject *self)
{
	PyPackageDep *py_dep = (PyPackageDep *) self;

	if (py_dep->dep)
		rc_package_dep_unref (py_dep->dep);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackageDep_register (PyObject *dict)
{
	PyPackageDep_type_info.tp_init    = PyPackageDep_init;
	PyPackageDep_type_info.tp_new     = PyPackageDep_tp_new;
	PyPackageDep_type_info.tp_dealloc = PyPackageDep_tp_dealloc;
	PyPackageDep_type_info.tp_methods = PyPackageDep_methods;
	PyPackageDep_type_info.tp_str     = PyPackageDep_tp_str;

	pyutil_register_type (dict, &PyPackageDep_type_info);
	pyutil_register_methods (dict, PyPackageDep_unbound_methods);

	pyutil_register_int_constant (dict, "RELATION_INVALID",
							RC_RELATION_INVALID);
	pyutil_register_int_constant (dict, "RELATION_ANY",
							RC_RELATION_ANY);
	pyutil_register_int_constant (dict, "RELATION_EQUAL",
							RC_RELATION_EQUAL);
	pyutil_register_int_constant (dict, "RELATION_LESS",
							RC_RELATION_LESS);
	pyutil_register_int_constant (dict, "RELATION_LESS_EQUAL",
							RC_RELATION_LESS_EQUAL);
	pyutil_register_int_constant (dict, "RELATION_GREATER",
							RC_RELATION_GREATER);
	pyutil_register_int_constant (dict, "RELATION_GREATER_EQUAL",
							RC_RELATION_GREATER_EQUAL);
	pyutil_register_int_constant (dict, "RELATION_NOT_EQUAL",
							RC_RELATION_NOT_EQUAL);
	pyutil_register_int_constant (dict, "RELATION_NONE",
							RC_RELATION_NONE);
}

int
PyPackageDep_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackageDep_type_info);
}

PyObject *
PyPackageDep_new (RCPackageDep *dep)
{
	PyObject *py_dep = PyPackageDep_tp_new (&PyPackageDep_type_info,
						NULL, NULL);
	((PyPackageDep *) py_dep)->dep = rc_package_dep_ref (dep);

	return py_dep;
}

RCPackageDep *
PyPackageDep_get_package_dep (PyObject *obj)
{
	if (! PyPackageDep_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a PackageDep");
		return NULL;
	}

	return ((PyPackageDep *) obj)->dep;
}

PyObject *
rc_package_dep_array_to_PyList (RCPackageDepArray *array)
{
	PyObject *py_list;
	int i;
	int len = 0;

	if (array != NULL)
		len = array->len;

	py_list = PyList_New (len);

	for (i = 0; i < len; i++) {
		RCPackageDep *dep = array->data[i];
		PyList_SetItem (py_list, i, PyPackageDep_new (dep));
	}

	return py_list;
}
