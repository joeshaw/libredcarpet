/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-update.c
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

#include "package-update.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackageUpdate *update;
} PyPackageUpdate;

static PyTypeObject PyPackageUpdate_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"PackageUpdate",
	sizeof (PyPackageUpdate),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyMethodDef PyPackageUpdate_methods[] = {
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackageUpdate_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyPackageUpdate *py_update = (PyPackageUpdate *) self;

	py_update->update = rc_package_update_new ();

	if (py_update->update == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create PackageUpdate");
		return -1;
	}

	return 0;
}

static PyObject *
PyPackageUpdate_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackageUpdate *py_update;

	py_update = (PyPackageUpdate *) type->tp_alloc (type, 0);
	py_update->update = NULL;

	return (PyObject *) py_update;
}

static void
PyPackageUpdate_tp_dealloc (PyObject *self)
{
	PyPackageUpdate *py_update = (PyPackageUpdate *) self;

	if (py_update->update)
		rc_package_update_free (py_update->update);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackageUpdate_register (PyObject *dict)
{
	PyPackageUpdate_type_info.tp_init    = PyPackageUpdate_init;
	PyPackageUpdate_type_info.tp_new     = PyPackageUpdate_tp_new;
	PyPackageUpdate_type_info.tp_dealloc = PyPackageUpdate_tp_dealloc;
	PyPackageUpdate_type_info.tp_methods = PyPackageUpdate_methods;

	pyutil_register_type (dict, &PyPackageUpdate_type_info);
}

int
PyPackageUpdate_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackageUpdate_type_info);
}

PyObject *
PyPackageUpdate_new (RCPackageUpdate *update)
{
	PyObject *py_update = PyPackageUpdate_tp_new (&PyPackageUpdate_type_info,
						      NULL, NULL);
	((PyPackageUpdate *) py_update)->update = rc_package_update_copy (update);

	return py_update;
}

RCPackageUpdate *
PyPackageUpdate_get_package (PyObject *obj)
{
	if (! PyPackageUpdate_check (obj))
		return NULL;

	return ((PyPackageUpdate *) obj)->update;
}
