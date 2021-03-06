/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * resolver-info.c
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

#include "resolver-info.h"
#include "package.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCResolverInfo *info;
} PyResolverInfo;

static PyTypeObject PyResolverInfo_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"ResolverInfo",
	sizeof (PyResolverInfo),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyResolverInfo_get_packages (PyObject *self, PyObject *args)
{
	RCResolverInfo *info = PyResolverInfo_get_resolver_info (self);
	PyObject *py_list;
	GSList *iter;

	py_list = PyList_New (0);
	for (iter = info->package_list; iter != NULL; iter = iter->next) {
		RCPackage *pkg = iter->data;
		PyList_Append (py_list, PyPackage_new (pkg));
	}

	return py_list;
}

static PyObject *
PyResolverInfo_type (PyObject *self, PyObject *args)
{
	RCResolverInfo *info = PyResolverInfo_get_resolver_info (self);
	return Py_BuildValue ("i", rc_resolver_info_type (info));
}

static PyObject *
PyResolverInfo_is_error (PyObject *self, PyObject *args)
{
	RCResolverInfo *info = PyResolverInfo_get_resolver_info (self);
	return Py_BuildValue ("i", rc_resolver_info_is_error (info));
}

static PyObject *
PyResolverInfo_is_important (PyObject *self, PyObject *args)
{
	RCResolverInfo *info = PyResolverInfo_get_resolver_info (self);
	return Py_BuildValue ("i", rc_resolver_info_is_important (info));
}

static PyMethodDef PyResolverInfo_methods[] = {
	{ "get_packages", PyResolverInfo_get_packages, METH_NOARGS },
	{ "type",         PyResolverInfo_type,         METH_NOARGS },
	{ "is_error",     PyResolverInfo_is_error,     METH_NOARGS },
	{ "is_important", PyResolverInfo_is_important, METH_NOARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
resolver_info_type_to_string (PyObject *self, PyObject *args)
{
	RCResolverInfoType type;

	if (! PyArg_ParseTuple (args, "i", &type))
		return NULL;

	return Py_BuildValue ("s", rc_resolver_info_type_to_string (type));
}

static PyMethodDef PyResolverInfo_unbound_methods[] = {
	{ "resolver_info_type_to_string", resolver_info_type_to_string, METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyResolverInfo_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyResolverInfo *py_info;

	py_info = (PyResolverInfo *) type->tp_alloc (type, 0);
	py_info->info = NULL;

	return (PyObject *) py_info;
}

static PyObject *
PyResolverInfo_tp_str (PyObject *self)
{
	PyResolverInfo *py_info = (PyResolverInfo *) self;
	RCResolverInfo *info = py_info->info;
	PyObject *obj;
	char *str;

	if (info == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	str = rc_resolver_info_to_string (info);
	if (str == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	obj = PyString_FromString (str);
	g_free (str);

	return obj;
}

static void
PyResolverInfo_tp_dealloc (PyObject *self)
{
	PyResolverInfo *py_info = (PyResolverInfo *) self;

	if (py_info->info)
		rc_resolver_info_free (py_info->info);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyResolverInfo_register (PyObject *dict)
{
	PyResolverInfo_type_info.tp_new     = PyResolverInfo_tp_new;
	PyResolverInfo_type_info.tp_dealloc = PyResolverInfo_tp_dealloc;
	PyResolverInfo_type_info.tp_methods = PyResolverInfo_methods;
	PyResolverInfo_type_info.tp_str     = PyResolverInfo_tp_str;

	pyutil_register_type (dict, &PyResolverInfo_type_info);
	pyutil_register_methods (dict, PyResolverInfo_unbound_methods);

	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_INVALID",
				      RC_RESOLVER_INFO_TYPE_INVALID);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_NEEDED_BY",
				      RC_RESOLVER_INFO_TYPE_NEEDED_BY);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_CONFLICTS_WITH",
				      RC_RESOLVER_INFO_TYPE_CONFLICTS_WITH);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_OBSOLETES",
				      RC_RESOLVER_INFO_TYPE_OBSOLETES);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_DEPENDS_ON",
				      RC_RESOLVER_INFO_TYPE_DEPENDS_ON);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_CHILD_OF",
				      RC_RESOLVER_INFO_TYPE_CHILD_OF);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_MISSING_REQ",
				      RC_RESOLVER_INFO_TYPE_MISSING_REQ);
	pyutil_register_int_constant (dict, "RESOLVER_INFO_TYPE_MISC",
				      RC_RESOLVER_INFO_TYPE_MISC);
	
}

int
PyResolverInfo_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyResolverInfo_type_info);
}

PyObject *
PyResolverInfo_new (RCResolverInfo *info)
{
	PyObject *py_info = PyResolverInfo_tp_new (&PyResolverInfo_type_info,
						   NULL, NULL);
	((PyResolverInfo *) py_info)->info = rc_resolver_info_copy (info);

	return py_info;
}

RCResolverInfo *
PyResolverInfo_get_resolver_info (PyObject *obj)
{
	if (! PyResolverInfo_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a ResolverInfo");
		return NULL;
	}

	return ((PyResolverInfo *) obj)->info;
}
