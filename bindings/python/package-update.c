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
#include "package-spec.h"
#include "package-importance.h"
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

static PyObject *
PyPackageUpdate_to_xml (PyObject *self, PyObject *args)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);
	PyObject *obj;
	xmlNode *node;
	xmlBuffer *buf;

	buf = xmlBufferCreate();

	node = rc_package_update_to_xml_node (update);
	xmlNodeDump (buf, NULL, node, 0, 0);
	xmlFreeNode (node);
	obj = Py_BuildValue ("s", buf->content);
	xmlBufferFree (buf);
	return obj;
}

static PyMethodDef PyPackageUpdate_methods[] = {
	{ "to_xml",  PyPackageUpdate_to_xml, METH_NOARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackageUpdate_get_spec (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);
	return PyPackageSpec_new (&update->spec);
}

static PyObject *
PyPackageUpdate_get_package_url (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);

	if (update->package_url == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (update->package_url);
}

static PyObject *
PyPackageUpdate_get_package_size (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);
	return PyInt_FromLong (update->package_size);
}

static PyObject *
PyPackageUpdate_get_signature_url (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);

	if (update->signature_url == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (update->signature_url);
}

static PyObject *
PyPackageUpdate_get_importance (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);
	return PyPackageImportance_new (update->importance);
}

static PyObject *
PyPackageUpdate_get_description (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);

	if (update->description == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (update->description);
}

static PyObject *
PyPackageUpdate_get_hid (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);
	return PyInt_FromLong (update->hid);
}

static PyObject *
PyPackageUpdate_get_license (PyObject *self, void *closure)
{
	RCPackageUpdate *update = PyPackageUpdate_get_package_update (self);

	if (update->license == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (update->license);
}

static PyGetSetDef PyPackageUpdate_getsets[] = {
	{ "spec",          (getter) PyPackageUpdate_get_spec,          (setter) 0 },
	{ "package_url",   (getter) PyPackageUpdate_get_package_url,   (setter) 0 },
	{ "package_size",  (getter) PyPackageUpdate_get_package_size,  (setter) 0 },
	{ "signature_url", (getter) PyPackageUpdate_get_signature_url, (setter) 0 },
	{ "importance",    (getter) PyPackageUpdate_get_importance,    (setter) 0 },
	{ "description",   (getter) PyPackageUpdate_get_description,   (setter) 0 },
	{ "hid",           (getter) PyPackageUpdate_get_hid,           (setter) 0 },
	{ "license",       (getter) PyPackageUpdate_get_license,       (setter) 0 },

	{ NULL, (getter)0, (setter)0 },
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
	PyPackageUpdate_type_info.tp_getset  = PyPackageUpdate_getsets;

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
PyPackageUpdate_get_package_update (PyObject *obj)
{
	if (! PyPackageUpdate_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a PackageUpdate");
		return NULL;
	}

	return ((PyPackageUpdate *) obj)->update;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCPackageUpdateSList *
PyList_to_rc_package_update_slist (PyObject *py_list)
{
	RCPackageUpdateSList *slist;
	int i;

	g_return_val_if_fail (PyList_Check (py_list) == 1, NULL);

	slist = NULL;
	for (i = 0; i < PyList_Size (py_list); i++) {
		RCPackageUpdate *update;

		update = PyPackageUpdate_get_package_update (PyList_GetItem (py_list, i));
		if (update != NULL)
			slist = g_slist_append (slist, rc_package_update_copy (update));
	}

	return slist;
}

static void
add_to_pylist_cb (gpointer data, gpointer user_data)
{
	RCPackageUpdate *update = (RCPackageUpdate *) data;
	PyObject *list = (PyObject *) user_data;
	PyObject *py_update;

	py_update = PyPackageUpdate_new (update);
	PyList_Append(list, py_update);
}

PyObject *
rc_package_update_slist_to_PyList (RCPackageUpdateSList *slist)
{
	PyObject *py_list;

	py_list = PyList_New (0);

	if (slist == NULL)
		return py_list;

	g_slist_foreach (slist, add_to_pylist_cb, (gpointer) py_list);

	return py_list;
}
