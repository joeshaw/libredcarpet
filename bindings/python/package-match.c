/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-match.c
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

#include "package-match.h"
#include "package-dep.h"
#include "package.h"
#include "channel.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackageMatch *match;
} PyPackageMatch;

static PyTypeObject PyPackageMatch_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"PackageMatch",
	sizeof (PyPackageMatch),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackageMatch_set_channel (PyObject *self, PyObject *args)
{
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_package_match_set_channel (match, channel);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyPackageMatch_get_channel (PyObject *self, PyObject *args)
{
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	RCChannel *channel;

	channel = rc_package_match_get_channel (match);
	if (channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (channel);
}

static PyObject *
PyPackageMatch_set_dep (PyObject *self, PyObject *args)
{
	/* FIXME */
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	PyObject *obj;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	rc_package_match_set_dep (match, dep);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyPackageMatch_get_dep (PyObject *self, PyObject *args)
{
	/* FIXME */
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	RCPackageDep *dep;

	dep = rc_package_match_get_dep (match);
	if (dep == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackageDep_new (dep);
}

static PyObject *
PyPackageMatch_set_glob (PyObject *self, PyObject *args)
{
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	const char *glob_str;

	if (! PyArg_ParseTuple (args, "s", &glob_str))
		return NULL;

	rc_package_match_set_glob (match, glob_str);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyPackageMatch_get_glob (PyObject *self, PyObject *args)
{
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	const char *glob_str;

	glob_str = rc_package_match_get_glob (match);
	if (glob_str == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return Py_BuildValue ("s", glob_str);
}

static PyObject *
PyPackageMatch_set_importance (PyObject *self, PyObject *args)
{
	/* FIXME */
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyPackageMatch_get_importance (PyObject *self, PyObject *args)
{
	/* FIXME */
	RCPackageMatch *match = PyPackageMatch_get_package_match (self);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef PyPackageMatch_methods[] = {
	{ "set_channel",    PyPackageMatch_set_channel,    METH_VARARGS },
	{ "get_channel",    PyPackageMatch_get_channel,    METH_NOARGS  },
	{ "set_dep",        PyPackageMatch_set_dep,        METH_VARARGS },
	{ "get_dep",        PyPackageMatch_get_dep,        METH_NOARGS  },
	{ "set_glob",       PyPackageMatch_set_glob,       METH_VARARGS },
	{ "get_glob",       PyPackageMatch_get_glob,       METH_NOARGS  },
	{ "set_importance", PyPackageMatch_set_importance, METH_VARARGS },
	{ "get_importance", PyPackageMatch_get_importance, METH_NOARGS  },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackageMatch_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyPackageMatch *py_match = (PyPackageMatch *) self;

	py_match->match = rc_package_match_new ();

	if (py_match->match == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create PackageMatch");
		return -1;
	}

	return 0;
}

static PyObject *
PyPackageMatch_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackageMatch *py_match;

	py_match = (PyPackageMatch *) type->tp_alloc (type, 0);
	py_match->match = NULL;

	return (PyObject *) py_match;
}

static void
PyPackageMatch_tp_dealloc (PyObject *self)
{
	PyPackageMatch *py_match = (PyPackageMatch *) self;

	if (py_match->match)
		rc_package_match_free (py_match->match);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void PyPackageMatch_register (PyObject *dict)
{
	PyPackageMatch_type_info.tp_init    = PyPackageMatch_init;
	PyPackageMatch_type_info.tp_new     = PyPackageMatch_tp_new;
	PyPackageMatch_type_info.tp_dealloc = PyPackageMatch_tp_dealloc;
	PyPackageMatch_type_info.tp_methods = PyPackageMatch_methods;

	pyutil_register_type (dict, &PyPackageMatch_type_info);
}

int
PyPackageMatch_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackageMatch_type_info);
}

PyObject *
PyPackageMatch_new (RCPackageMatch *match)
{
	PyObject *py_match = PyPackageMatch_tp_new (&PyPackageMatch_type_info,
										NULL, NULL);
	((PyPackageMatch *) match)->match = match;

	return py_match;
}

RCPackageMatch *
PyPackageMatch_get_package_match (PyObject *obj)
{
	if (! PyPackageMatch_check (obj))
		return NULL;

	return ((PyPackageMatch *) obj)->match;
}
