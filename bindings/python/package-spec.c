/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-spec.c
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

#include "package-spec.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackageSpec *spec;
} PyPackageSpec;

static PyTypeObject PyPackageSpec_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"PackageSpec",
	sizeof (PyPackageSpec),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyMethodDef PyPackageSpec_methods[] = {
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackageSpec_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyPackageSpec *py_spec = (PyPackageSpec *) self;
	const char *name, *version, *release;
	gboolean has_epoch;
	int epoch;

	if (! PyArg_ParseTuple (args, "siiss", &name, &has_epoch, &epoch,
					    &version, &release))
		return -1;

	py_spec->spec = g_new0 (RCPackageSpec, 1);
	rc_package_spec_init (py_spec->spec, name, has_epoch, epoch, version, release);

	return 0;
}

static PyObject *
PyPackageSpec_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackageSpec *py_spec;

	py_spec = (PyPackageSpec *) type->tp_alloc (type, 0);
	py_spec->spec = NULL;

	return (PyObject *) py_spec;
}

static void
PyPackageSpec_tp_dealloc (PyObject *self)
{
	PyPackageSpec *py_spec = (PyPackageSpec *) self;

	if (py_spec->spec)
		rc_package_spec_free_members (py_spec->spec);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

static PyObject *
PyPackageSpec_tp_str (PyObject *self)
{
	PyPackageSpec *py_spec = (PyPackageSpec *) self;
	RCPackageSpec *spec = py_spec->spec;
	const char *str;

	if (spec == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	str = rc_package_spec_to_str_static (spec);
	if (str == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (str);
}

static int
PyPackageSpec_tp_cmp (PyObject *obj_x, PyObject *obj_y)
{
	RCPackageSpec *spec_x = ((PyPackageSpec *) obj_x)->spec;
	RCPackageSpec *spec_y = ((PyPackageSpec *) obj_y)->spec;

	return rc_package_spec_equal (spec_x, spec_y);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackageSpec_register (PyObject *dict)
{
	PyPackageSpec_type_info.tp_init    = PyPackageSpec_init;
	PyPackageSpec_type_info.tp_new     = PyPackageSpec_tp_new;
	PyPackageSpec_type_info.tp_dealloc = PyPackageSpec_tp_dealloc;
	PyPackageSpec_type_info.tp_methods = PyPackageSpec_methods;
	PyPackageSpec_type_info.tp_str     = PyPackageSpec_tp_str;
	PyPackageSpec_type_info.tp_compare = PyPackageSpec_tp_cmp;

	pyutil_register_type (dict, &PyPackageSpec_type_info);
}

int
PyPackageSpec_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackageSpec_type_info);
}

PyObject *
PyPackageSpec_new (RCPackageSpec *spec)
{
	PyObject *py_spec = PyPackageSpec_tp_new (&PyPackageSpec_type_info,
									  NULL, NULL);
	((PyPackageSpec *) py_spec)->spec = spec;

	return py_spec;
}

RCPackageSpec *
PyPackageSpec_get_package_spec (PyObject *obj)
{
	if (! PyPackageSpec_check (obj))
		return NULL;

	return ((PyPackageSpec *) obj)->spec;
}
