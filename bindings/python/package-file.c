/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-file.c
 *
 * Copyright (C) 2003 The Free Software Foundation, Inc.
 *
 * Developed by James Willcox <james@ximian.com>
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

#include "package-file.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackageFile *file;
} PyPackageFile;

static PyTypeObject PyPackageFile_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"PackageFile",
	sizeof (PyPackageFile),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackageFile_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	return 0;
}

static PyObject *
PyPackageFile_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackageFile *py_file;

	py_file = (PyPackageFile *) type->tp_alloc (type, 0);
	py_file->file = NULL;

	return (PyObject *) py_file;
}

static PyObject *
PyPackageFile_tp_str (PyObject *self)
{
	return PyString_FromString ("FIXME");
}

static void
PyPackageFile_tp_dealloc (PyObject *self)
{
	PyPackageFile *py_file = (PyPackageFile *) self;

	if (py_file->file)
		rc_package_file_free (py_file->file);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

static PyObject *
PyPackageFile_get_filename (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyString_FromString (file->filename);
}
	
static PyObject *
PyPackageFile_get_size (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyInt_FromLong (file->size);
}

static PyObject *
PyPackageFile_get_md5sum (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyString_FromString (file->md5sum);
}

static PyObject *
PyPackageFile_get_uid (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyInt_FromLong (file->uid);
}

static PyObject *
PyPackageFile_get_gid (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyInt_FromLong (file->gid);
}

static PyObject *
PyPackageFile_get_mode (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyInt_FromLong (file->mode);
}

static PyObject *
PyPackageFile_get_mtime (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyInt_FromLong (file->mtime);
}

static PyObject *
PyPackageFile_get_ghost (PyObject *self, void *closure)
{
	RCPackageFile *file = PyPackageFile_get_package_file (self);
	return PyInt_FromLong (file->ghost);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyGetSetDef PyPackageFile_getsets[] = {
	{ "filename",           (getter) PyPackageFile_get_filename,   (setter) 0 },
	{ "size",               (getter) PyPackageFile_get_size,       (setter) 0 },
	{ "md5sum",             (getter) PyPackageFile_get_md5sum,     (setter) 0 },
	{ "uid",                (getter) PyPackageFile_get_uid,        (setter) 0 },
	{ "gid",                (getter) PyPackageFile_get_gid,        (setter) 0 },
	{ "mode",               (getter) PyPackageFile_get_mode,       (setter) 0 },
	{ "mtime",              (getter) PyPackageFile_get_mtime,      (setter) 0 },
	{ "ghost",              (getter) PyPackageFile_get_ghost,      (setter) 0 },
	{ NULL, (getter)0, (setter)0 },
};

void
PyPackageFile_register (PyObject *dict)
{
	PyPackageFile_type_info.tp_init    = PyPackageFile_init;
	PyPackageFile_type_info.tp_new     = PyPackageFile_tp_new;
	PyPackageFile_type_info.tp_dealloc = PyPackageFile_tp_dealloc;
	PyPackageFile_type_info.tp_getset  = PyPackageFile_getsets;
	PyPackageFile_type_info.tp_str     = PyPackageFile_tp_str;

	pyutil_register_type (dict, &PyPackageFile_type_info);
}


int
PyPackageFile_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackageFile_type_info);
}

PyObject *
PyPackageFile_new (RCPackageFile *file)
{
	PyObject *py_file = PyPackageFile_tp_new (&PyPackageFile_type_info,
						NULL, NULL);
	((PyPackageFile *) py_file)->file = file;

	return py_file;
}

RCPackageFile *
PyPackageFile_get_package_file (PyObject *obj)
{
	if (! PyPackageFile_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a PackageFile");
		return NULL;
	}

	return ((PyPackageFile *) obj)->file;
}

PyObject *
rc_package_file_slist_to_PyList (RCPackageFileSList *list)
{
	PyObject *py_list;
	GSList *l;

	py_list = PyList_New (0);

	for (l = list; l; l=l->next) {
		RCPackageFile *file = l->data;
		PyList_Append (py_list, PyPackageFile_new (file));
	}

	return py_list;
}
