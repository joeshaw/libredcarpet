/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package.c
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

#include "package.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackage *package;
} PyPackage;

static PyTypeObject PyPackage_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Package",
	sizeof (PyPackage),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackage_is_installed (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	return Py_BuildValue ("i", rc_package_is_installed (package));
}

static PyObject *
PyPackage_is_package_set (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	return Py_BuildValue ("i", rc_package_is_package_set (package));
}

static PyObject *
PyPackage_is_synthetic (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	return Py_BuildValue ("i", rc_package_is_synthetic (package));
}

static PyObject *
PyPackage_get_best_upgrade (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	RCPackage *upgrade;
	gboolean subscribed_only;

	if (! PyArg_ParseTuple (args, "i", &subscribed_only))
		return NULL;

	upgrade = rc_package_get_best_upgrade (package, subscribed_only);
	if (upgrade == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (upgrade);
}

static PyMethodDef PyPackage_methods[] = {
	{ "is_installed",       PyPackage_is_installed,       METH_NOARGS  },
	{ "is_package_set",     PyPackage_is_package_set,     METH_NOARGS  },
	{ "is_synthetic",       PyPackage_is_synthetic,       METH_NOARGS  },
	{ "get_best_upgrade",   PyPackage_get_best_upgrade,   METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackage_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyPackage *py_package = (PyPackage *) self;

	py_package->package = rc_package_new ();

	if (py_package->package == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create Package");
		return -1;
	}

	return 0;
}

static PyObject *
PyPackage_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackage *py_package;

	py_package = (PyPackage *) type->tp_alloc (type, 0);
	py_package->package = NULL;

	return (PyObject *) py_package;
}

static PyObject *
PyPackage_tp_str (PyObject *self)
{
	PyPackage *py_package = (PyPackage *) self;
	RCPackage *package = py_package->package;
	const char *str;

	if (package == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	str = rc_package_to_str_static (package);
	if (str == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (str);
}

static void
PyPackage_tp_dealloc (PyObject *self)
{
	PyPackage *py_package = (PyPackage *) self;

	if (py_package->package)
		rc_package_unref (py_package->package);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackage_register (PyObject *dict)
{
	PyPackage_type_info.tp_init    = PyPackage_init;
	PyPackage_type_info.tp_new     = PyPackage_tp_new;
	PyPackage_type_info.tp_dealloc = PyPackage_tp_dealloc;
	PyPackage_type_info.tp_methods = PyPackage_methods;
	PyPackage_type_info.tp_str     = PyPackage_tp_str;

	pyutil_register_type (dict, &PyPackage_type_info);
}

int
PyPackage_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackage_type_info);
}

PyObject *
PyPackage_new (RCPackage *package)
{
	PyObject *py_package = PyPackage_tp_new (&PyPackage_type_info,
						 NULL, NULL);
	((PyPackage *) py_package)->package = rc_package_ref (package);

	return py_package;
}

RCPackage *
PyPackage_get_package (PyObject *obj)
{
	if (! PyPackage_check (obj))
		return NULL;

	return ((PyPackage *) obj)->package;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */
/* Helpers */

RCPackageSList *
PyList_to_rc_package_slist (PyObject *py_list)
{
	RCPackageSList *slist;
	int i;

	g_return_val_if_fail (PyList_Check (py_list) == 1, NULL);

	slist = NULL;
	for (i = 0; i < PyList_Size (py_list); i++) {
		RCPackage *package;

		package = PyPackage_get_package (PyList_GetItem (py_list, i));
		if (package != NULL)
			slist = g_slist_append (slist, rc_package_ref (package));
	}

	return slist;
}

static void
add_to_pylist_cb (gpointer data, gpointer user_data)
{
	RCPackage *package = (RCPackage *) data;
	PyObject *list = (PyObject *) user_data;
	PyObject *py_package;

	py_package = PyPackage_new (package);
	PyList_Append(list, py_package);
}

PyObject *
rc_package_slist_to_PyList (RCPackageSList *slist)
{
	PyObject *py_list;

	py_list = PyList_New (0);

	if (slist == NULL)
		return py_list;

	g_slist_foreach (slist, add_to_pylist_cb, (gpointer) py_list);

	return py_list;
}
