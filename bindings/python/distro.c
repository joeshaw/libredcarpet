/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * distro.c
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

#include "distro.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCDistro *distro;
} PyDistro;

static PyTypeObject PyDistro_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Distro",
	sizeof (PyDistro),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
distro_get_name (PyObject *self, PyObject *args)
{
	PyDistro *py_distro = (PyDistro *) self;

	return Py_BuildValue ("s", rc_distro_get_name (py_distro->distro));
}

static PyObject *
distro_get_version (PyObject *self, PyObject *args)
{
	PyDistro *py_distro = (PyDistro *) self;

	return Py_BuildValue ("s", rc_distro_get_version (py_distro->distro));
}

static PyObject *
distro_get_package_type (PyObject *self, PyObject *args)
{
	PyDistro *py_distro = (PyDistro *) self;

	return Py_BuildValue ("i", rc_distro_get_package_type (py_distro->distro));
}

static PyObject *
distro_get_target (PyObject *self, PyObject *args)
{
	PyDistro *py_distro = (PyDistro *) self;

	return Py_BuildValue ("s", rc_distro_get_target (py_distro->distro));
}

static PyObject *
distro_get_status (PyObject *self, PyObject *args)
{
	PyDistro *py_distro = (PyDistro *) self;

	return Py_BuildValue ("i", rc_distro_get_status (py_distro->distro));
}

static PyObject *
distro_get_death_date (PyObject *self, PyObject *args)
{
	PyDistro *py_distro = (PyDistro *) self;

	return Py_BuildValue ("i", rc_distro_get_death_date (py_distro->distro));
}

static PyMethodDef PyDistro_methods[] = {
	{ "get_name",         distro_get_name,         METH_VARARGS },
	{ "get_version",      distro_get_version,      METH_VARARGS },
	{ "get_package_type", distro_get_package_type, METH_VARARGS },
	{ "get_target",       distro_get_target,       METH_VARARGS },
	{ "get_status",       distro_get_status,       METH_VARARGS },
	{ "get_death_date",   distro_get_death_date,   METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyDistro_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	char *distro_data;
	int distro_len;
	static char *kwlist[] = { "data", NULL };
	RCDistro *distro;
	PyDistro *py_distro;

	if (!PyArg_ParseTupleAndKeywords (args, kwds, "s#", kwlist,
					  &distro_data, &distro_len))
		return NULL;

	distro = rc_distro_parse_xml (distro_data, distro_len);

	if (!distro) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	py_distro = (PyDistro *) type->tp_alloc (type, 0);
	py_distro->distro = distro;

	return (PyObject *) py_distro;
}

static void
PyDistro_tp_dealloc (PyObject *self)
{
	PyDistro *py_distro = (PyDistro *) self;

	if (py_distro->distro)
		rc_distro_free (py_distro->distro);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyDistro_register (PyObject *dict)
{
	PyDistro_type_info.tp_new  = PyDistro_tp_new;
	PyDistro_type_info.tp_dealloc = PyDistro_tp_dealloc;
	PyDistro_type_info.tp_methods = PyDistro_methods;

	pyutil_register_type (dict, &PyDistro_type_info);

	pyutil_register_int_constant (dict, "DISTRO_PACKAGE_TYPE_UNKNOWN",
				      RC_DISTRO_PACKAGE_TYPE_UNKNOWN);
	pyutil_register_int_constant (dict, "DISTRO_PACKAGE_TYPE_RPM",
				      RC_DISTRO_PACKAGE_TYPE_RPM);
	pyutil_register_int_constant (dict, "DISTRO_PACKAGE_TYPE_DPKG",
				      RC_DISTRO_PACKAGE_TYPE_DPKG);

	pyutil_register_int_constant (dict, "DISTRO_STATUS_UNSUPPORTED",
				      RC_DISTRO_STATUS_UNSUPPORTED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_PRESUPPORTED",
				      RC_DISTRO_STATUS_PRESUPPORTED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_SUPPORTED",
				      RC_DISTRO_STATUS_SUPPORTED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_DEPRECATED",
				      RC_DISTRO_STATUS_DEPRECATED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_RETIRED",
				      RC_DISTRO_STATUS_RETIRED);
}
