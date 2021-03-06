/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * resolver.c
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

#include "resolver.h"
#include "resolver-context.h"
#include "resolver-queue.h"
#include "package-dep.h"
#include "package.h"
#include "channel.h"
#include "world.h"
#include "world-store.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCResolver *resolver;
	gboolean borrowed;

	PyWorld *py_world;
} PyResolver;

static PyTypeObject PyResolver_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Resolver",
	sizeof (PyResolver),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyResolver_set_timeout (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	int seconds;

	if (! PyArg_ParseTuple (args, "i", &seconds))
		return NULL;

	rc_resolver_set_timeout (resolver, seconds);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_set_world (PyObject *self, PyObject *args)
{
	PyResolver *py_resolver = (PyResolver *) self;
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCWorldStore *store;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;
	store = PyWorldStore_get_store (obj);
	if (store == NULL)
		return NULL;

	rc_resolver_set_world (resolver, RC_WORLD(store));

	if (py_resolver->py_world) {
		Py_DECREF (py_resolver->py_world);
	}

	py_resolver->py_world = (PyWorld *) obj;
	Py_INCREF (py_resolver->py_world);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_get_world (PyObject *self, PyObject *args)
{
	PyResolver *py_resolver = (PyResolver *) self;

	return (PyObject *) py_resolver->py_world;
}

static PyObject *
PyResolver_set_current_channel (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;
	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_resolver_set_current_channel (resolver, channel);
	Py_INCREF (Py_None);
	return Py_None;
}

#if 0
static PyObject *
PyResolver_add_subscribed_channel (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;
	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_resolver_add_subscribed_channel (resolver, channel);
	Py_INCREF (Py_None);
	return Py_None;
}
#endif

static PyObject *
PyResolver_add_packages_to_install (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCPackageSList *pkg_list;

	if (! PyArg_ParseTuple (args, "O!", &PyList_Type, &obj))
		return NULL;

	pkg_list = PyList_to_rc_package_slist (obj);
	if (pkg_list == NULL)
		return NULL;

	rc_resolver_add_packages_to_install_from_slist (resolver, pkg_list);

	rc_package_slist_unref (pkg_list);
	g_slist_free (pkg_list);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_add_packages_to_remove (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCPackageSList *pkg_list;

	if (! PyArg_ParseTuple (args, "O!", &PyList_Type, &obj))
		return NULL;

	pkg_list = PyList_to_rc_package_slist (obj);
	if (pkg_list == NULL)
		return NULL;

	rc_resolver_add_packages_to_remove_from_slist (resolver, pkg_list);

	rc_package_slist_unref (pkg_list);
	g_slist_free (pkg_list);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_add_extra_dependency (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	rc_resolver_add_extra_dependency (resolver, dep);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_add_extra_conflict (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *obj;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	rc_resolver_add_extra_conflict (resolver, dep);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_verify_system (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);

	rc_resolver_verify_system (resolver);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyResolver_resolve_dependencies (PyObject *self, PyObject *args)
{
	RCResolver *resolver = PyResolver_get_resolver (self);

	rc_resolver_resolve_dependencies (resolver);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef PyResolver_methods[] = {
	{ "set_timeout",             PyResolver_set_timeout,             METH_VARARGS },
	{ "set_world",               PyResolver_set_world,               METH_VARARGS },
	{ "get_world",               PyResolver_get_world,               METH_NOARGS  },
	{ "set_current_channel",     PyResolver_set_current_channel,     METH_VARARGS },
/*	{ "add_subscribed_channel",  PyResolver_add_subscribed_channel,  METH_VARARGS }, */
	{ "add_packages_to_install", PyResolver_add_packages_to_install, METH_VARARGS },
	{ "add_packages_to_remove",  PyResolver_add_packages_to_remove,  METH_VARARGS },
	{ "add_extra_dependency",    PyResolver_add_extra_dependency,    METH_VARARGS },
	{ "add_extra_conflict",      PyResolver_add_extra_conflict,      METH_VARARGS },
	{ "verify_system",           PyResolver_verify_system,           METH_NOARGS  },
	{ "resolve_dependencies",    PyResolver_resolve_dependencies,    METH_NOARGS  },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyResolver_get_invalid_queues (PyObject *self, void *closure)
{
	RCResolver *resolver = PyResolver_get_resolver (self);
	PyObject *py_list;
	GSList *iter;

	py_list = PyList_New (0);

	for (iter = resolver->invalid_queues; iter != NULL; iter = iter->next) {
		RCResolverQueue *q = iter->data;
		PyList_Append (py_list, PyResolverQueue_new (q));
	}

	return py_list;
}

static PyObject *
PyResolver_get_best_context (PyObject *self, void *closure)
{
	RCResolver *resolver = PyResolver_get_resolver (self);

	if (resolver->best_context == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyResolverContext_new (resolver->best_context);
}

static PyGetSetDef PyResolver_getsets[] = {
	{ "invalid_queues", (getter) PyResolver_get_invalid_queues, (setter) 0 },
	{ "best_context",   (getter) PyResolver_get_best_context,   (setter) 0 },

	{ NULL, (getter) 0, (setter) 0 },
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyResolver_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyResolver *py_resolver = (PyResolver *) self;

	py_resolver->resolver = rc_resolver_new ();
	py_resolver->py_world = NULL;

	if (py_resolver->resolver == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create resolver");
		return -1;
	}

	return 0;
}

static PyObject *
PyResolver_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyResolver *py_resolver;

	py_resolver = (PyResolver *) type->tp_alloc (type, 0);
	py_resolver->resolver = NULL;
	py_resolver->borrowed = FALSE;

	return (PyObject *) py_resolver;
}

static void
PyResolver_tp_dealloc (PyObject *self)
{
	PyResolver *py_resolver = (PyResolver *) self;

	if (py_resolver->resolver && py_resolver->borrowed == FALSE)
		rc_resolver_free (py_resolver->resolver);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyResolver_register (PyObject *dict)
{
	PyResolver_type_info.tp_init    = PyResolver_init;
	PyResolver_type_info.tp_new     = PyResolver_tp_new;
	PyResolver_type_info.tp_dealloc = PyResolver_tp_dealloc;
	PyResolver_type_info.tp_methods = PyResolver_methods;
	PyResolver_type_info.tp_getset  = PyResolver_getsets;

	pyutil_register_type (dict, &PyResolver_type_info);
}

int
PyResolver_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyResolver_type_info);
}

PyObject *
PyResolver_new (RCResolver *resolver)
{
	PyObject *py_resolver = PyResolver_tp_new (&PyResolver_type_info,
						   NULL, NULL);
	((PyResolver *) py_resolver)->resolver = resolver;
	((PyResolver *) py_resolver)->borrowed = TRUE;

	return py_resolver;
}

RCResolver *
PyResolver_get_resolver (PyObject *obj)
{
	if (! PyResolver_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a resolver");
		return NULL;
	}

	return ((PyResolver *) obj)->resolver;
}
