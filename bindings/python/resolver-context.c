/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * resolver-context.c
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

#include "resolver-context.h"
#include "resolver-info.h"
#include "package.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCResolverContext *context;
} PyResolverContext;

static PyTypeObject PyResolverContext_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"ResolverContext",
	sizeof (PyResolverContext),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
get_all_install_cb (RCPackage *pkg, RCPackageStatus status, gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *py_pkg;

	if (rc_package_status_is_to_be_installed (status)) {
		py_pkg = PyPackage_new (pkg);
		if (py_pkg)
			PyList_Append (list, py_pkg);
	}
}

static PyObject *
PyResolverContext_get_all_install (PyObject *self, PyObject *args)
{
	RCResolverContext *ctx = PyResolverContext_get_resolver_context (self);
	PyObject *py_list;

	py_list = PyList_New (0);
	rc_resolver_context_foreach_install (ctx, get_all_install_cb, py_list);

	return py_list;
}

static void
get_all_uninstall_cb (RCPackage *pkg, RCPackageStatus status, gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *py_pkg;

	if (rc_package_status_is_to_be_uninstalled (status)) {
		py_pkg = PyPackage_new (pkg);
		if (py_pkg)
			PyList_Append (list, py_pkg);
	}
}

static PyObject *
PyResolverContext_get_all_uninstall (PyObject *self, PyObject *args)
{
	RCResolverContext *ctx = PyResolverContext_get_resolver_context (self);
	PyObject *py_list;

	py_list = PyList_New (0);
	rc_resolver_context_foreach_uninstall (ctx, get_all_uninstall_cb, py_list);

	return py_list;
}

static void
get_all_upgrade_cb (RCPackage *pkg_new, RCPackageStatus status_new,
		    RCPackage *pkg_old, RCPackageStatus status_old,
		    gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *py_pkg;

	py_pkg = PyPackage_new (pkg_new);
	if (py_pkg)
		PyList_Append (list, py_pkg);
}

static PyObject *
PyResolverContext_get_all_upgrade (PyObject *self, PyObject *args)
{
	RCResolverContext *ctx = PyResolverContext_get_resolver_context (self);
	PyObject *py_list;

	py_list = PyList_New (0);
	rc_resolver_context_foreach_upgrade (ctx, get_all_upgrade_cb, py_list);

	return py_list;
}

static void
get_all_info_cb (RCResolverInfo *info, gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *py_info;

	py_info = PyResolverInfo_new (info);
	if (py_info != NULL)
		PyList_Append (list, py_info);
}

static PyObject *
PyResolverContext_get_all_info (PyObject *self, PyObject *args)
{
	RCResolverContext *ctx = PyResolverContext_get_resolver_context (self);
	PyObject *obj;
	RCPackage *pkg;
	PyObject *py_list;
	int priority;

	if (! PyArg_ParseTuple (args, "Oi", &obj, &priority))
		return NULL;

	if (obj == Py_None)
		pkg = NULL;
	else {
		pkg = PyPackage_get_package (obj);
		if (pkg == NULL)
			return NULL;
	}

	py_list = PyList_New (0);
	rc_resolver_context_foreach_info (ctx, pkg, priority,
					  get_all_info_cb, py_list);

	return py_list;
}

static PyMethodDef PyResolverContext_methods[] = {
	{ "get_all_install",   PyResolverContext_get_all_install,   METH_VARARGS },
	{ "get_all_uninstall", PyResolverContext_get_all_uninstall, METH_VARARGS },
	{ "get_all_upgrade",   PyResolverContext_get_all_upgrade,   METH_VARARGS },
	{ "get_all_info",      PyResolverContext_get_all_info,      METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyResolverContext_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyResolverContext *py_context = (PyResolverContext *) self;

	py_context->context = rc_resolver_context_new ();

	if (py_context->context == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create Resolver Context");
		return -1;
	}

	return 0;
}

static PyObject *
PyResolverContext_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyResolverContext *py_context;

	py_context = (PyResolverContext *) type->tp_alloc (type, 0);
	py_context->context = NULL;

	return (PyObject *) py_context;
}

static void
PyResolverContext_tp_dealloc (PyObject *self)
{
	PyResolverContext *py_context = (PyResolverContext *) self;

	if (py_context->context)
		rc_resolver_context_unref (py_context->context);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void PyResolverContext_register (PyObject *dict)
{
	PyResolverContext_type_info.tp_init    = PyResolverContext_init;
	PyResolverContext_type_info.tp_new     = PyResolverContext_tp_new;
	PyResolverContext_type_info.tp_dealloc = PyResolverContext_tp_dealloc;
	PyResolverContext_type_info.tp_methods = PyResolverContext_methods;

	pyutil_register_type (dict, &PyResolverContext_type_info);
}

int
PyResolverContext_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyResolverContext_type_info);
}

PyObject *
PyResolverContext_new (RCResolverContext *context)
{
	PyObject *py_context = PyResolverContext_tp_new (&PyResolverContext_type_info,
							 NULL, NULL);
	((PyResolverContext *) py_context)->context = rc_resolver_context_ref (context);

	return py_context;
}

RCResolverContext *
PyResolverContext_get_resolver_context (PyObject *obj)
{
	if (! PyResolverContext_check (obj))
		return NULL;

	return ((PyResolverContext *) obj)->context;
}
