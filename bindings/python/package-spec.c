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
	gboolean borrowed;
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

static PyObject *
PyPackageSpec_get_name (PyObject *self, void *closure)
{
	RCPackageSpec *spec = PyPackageSpec_get_package_spec (self);
	return Py_BuildValue ("s", g_quark_to_string (spec->nameq));
}

static PyObject *
PyPackageSpec_get_version (PyObject *self, void *closure)
{
	RCPackageSpec *spec = PyPackageSpec_get_package_spec (self);

	if (spec->version == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return Py_BuildValue ("s", spec->version);
}

static PyObject *
PyPackageSpec_get_release (PyObject *self, void *closure)
{
	RCPackageSpec *spec = PyPackageSpec_get_package_spec (self);

	if (spec->release == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return Py_BuildValue ("s", spec->release);
}

static PyObject *
PyPackageSpec_get_has_epoch (PyObject *self, void *closure)
{
	RCPackageSpec *spec = PyPackageSpec_get_package_spec (self);
	return Py_BuildValue ("i", spec->has_epoch);
}

static PyObject *
PyPackageSpec_get_epoch (PyObject *self, void *closure)
{
	RCPackageSpec *spec = PyPackageSpec_get_package_spec (self);
	return Py_BuildValue ("i", spec->epoch);
}

static PyGetSetDef PyPackageSpec_getsets[] = {
	{ "name",      (getter) PyPackageSpec_get_name,      (setter) 0 },
	{ "version",   (getter) PyPackageSpec_get_version,   (setter) 0 },
	{ "release",   (getter) PyPackageSpec_get_release,   (setter) 0 },
	{ "has_epoch", (getter) PyPackageSpec_get_has_epoch, (setter) 0 },
	{ "epoch",     (getter) PyPackageSpec_get_epoch,     (setter) 0 },
	{ NULL, (getter) 0, (setter) 0 },
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
	py_spec->borrowed = FALSE;

	return (PyObject *) py_spec;
}

static void
PyPackageSpec_tp_dealloc (PyObject *self)
{
	PyPackageSpec *py_spec = (PyPackageSpec *) self;

	if (py_spec->spec && py_spec->borrowed == FALSE) {
		rc_package_spec_free_members (py_spec->spec);
		g_free (py_spec->spec);
	}

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
	PyPackageSpec_type_info.tp_getset  = PyPackageSpec_getsets;
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
	((PyPackageSpec *) py_spec)->borrowed = TRUE;

	return py_spec;
}

RCPackageSpec *
PyPackageSpec_get_package_spec (PyObject *obj)
{
	if (! PyPackageSpec_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a PackageSpec");
		return NULL;
	}

	return ((PyPackageSpec *) obj)->spec;
}
