/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * resolver-queue.c
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

#include "resolver-queue.h"
#include "resolver-context.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCResolverQueue *queue;
} PyResolverQueue;

static PyTypeObject PyResolverQueue_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"ResolverQueue",
	sizeof (PyResolverQueue),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyResolverQueue_get_context (PyObject *self, PyObject *args)
{
	RCResolverQueue *q = PyResolverQueue_get_resolver_queue (self);

	return PyResolverContext_new (q->context);
}

static PyMethodDef PyResolverQueue_methods[] = {
	{ "get_context",  PyResolverQueue_get_context,  METH_NOARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyResolverQueue_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyResolverQueue *py_queue = (PyResolverQueue *) self;

	py_queue->queue = rc_resolver_queue_new ();

	if (py_queue->queue == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create Resolver Queue");
		return -1;
	}

	return 0;
}

static PyObject *
PyResolverQueue_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyResolverQueue *py_queue;

	py_queue = (PyResolverQueue *) type->tp_alloc (type, 0);
	py_queue->queue = NULL;

	return (PyObject *) py_queue;
}

static void
PyResolverQueue_tp_dealloc (PyObject *self)
{
	PyResolverQueue *py_queue = (PyResolverQueue *) self;

	if (py_queue->queue)
		rc_resolver_queue_free (py_queue->queue);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyResolverQueue_register (PyObject *dict)
{
	PyResolverQueue_type_info.tp_init    = PyResolverQueue_init;
	PyResolverQueue_type_info.tp_new     = PyResolverQueue_tp_new;
	PyResolverQueue_type_info.tp_dealloc = PyResolverQueue_tp_dealloc;
	PyResolverQueue_type_info.tp_methods = PyResolverQueue_methods;

	pyutil_register_type (dict, &PyResolverQueue_type_info);
}

int
PyResolverQueue_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyResolverQueue_type_info);
}

PyObject *
PyResolverQueue_new (RCResolverQueue *queue)
{
	PyObject *py_queue = PyResolverQueue_tp_new (&PyResolverQueue_type_info,
										NULL, NULL);
	((PyResolverQueue *) py_queue)->queue = queue;

	return py_queue;
}

RCResolverQueue *
PyResolverQueue_get_resolver_queue (PyObject *obj)
{
	if (! PyResolverQueue_check (obj))
		return NULL;

	return ((PyResolverQueue *) obj)->queue;
}
