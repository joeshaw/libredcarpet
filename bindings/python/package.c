/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package.c
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

#include "package.h"
#include "package-update.h"
#include "package-spec.h"
#include "package-dep.h"
#include "channel.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCPackage *package;
} PyPackage;

static PyTypeObject PyPackage_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Package",
	sizeof (PyPackage),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackage_is_installed (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	return Py_BuildValue ("i", rc_package_is_installed (package));
}

static PyObject *
PyPackage_is_package_set (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	return Py_BuildValue ("i", rc_package_is_package_set (package));
}

static PyObject *
PyPackage_is_synthetic (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	return Py_BuildValue ("i", rc_package_is_synthetic (package));
}

static PyObject *
PyPackage_get_best_upgrade (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	RCPackage *upgrade;
	gboolean subscribed_only;

	if (! PyArg_ParseTuple (args, "i", &subscribed_only))
		return NULL;

	upgrade = rc_package_get_best_upgrade (package, subscribed_only);
	if (upgrade == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (upgrade);
}

static PyObject *
PyPackage_get_latest_update (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	RCPackageUpdate *update;

	update = rc_package_get_latest_update (package);
	if (update == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackageUpdate_new (update);
}

static PyObject *
PyPackage_add_dummy_update (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	char *filename;
	long filesize;

	if (! PyArg_ParseTuple (args, "sl", &filename, &filesize))
		return NULL;

	rc_package_add_dummy_update (package, filename, filesize);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyPackage_to_xml (PyObject *self, PyObject *args)
{
	RCPackage *package = PyPackage_get_package (self);
	PyObject *obj;
	xmlNode *node;
	xmlBuffer *buf;

	buf = xmlBufferCreate();

	node = rc_package_to_xml_node (package);
	xmlNodeDump (buf, NULL, node, 0, 0);
	xmlFreeNode (node);
	obj = Py_BuildValue ("s", buf->content);
	xmlBufferFree (buf);
	return obj;
}

static PyMethodDef PyPackage_methods[] = {
	{ "is_installed",       PyPackage_is_installed,       METH_NOARGS  },
	{ "is_package_set",     PyPackage_is_package_set,     METH_NOARGS  },
	{ "is_synthetic",       PyPackage_is_synthetic,       METH_NOARGS  },

#if BINDINGS_NOT_TOTALLY_BROKEN
	{ "get_best_upgrade",   PyPackage_get_best_upgrade,   METH_VARARGS },
	{ "get_latest_update",  PyPackage_get_latest_update,  METH_NOARGS  },
#endif
	{ "add_dummy_update",   PyPackage_add_dummy_update,   METH_VARARGS },

	/* From rc-xml.h */
	{ "to_xml",             PyPackage_to_xml,             METH_NOARGS  },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackage_get_spec (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return PyPackageSpec_new (&pkg->spec);
}

static PyObject *
PyPackage_get_file_size (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return PyInt_FromLong (pkg->file_size);
}

static PyObject *
PyPackage_get_installed_size (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return PyInt_FromLong (pkg->installed_size);
}

static PyObject *
PyPackage_get_channel (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (pkg->channel);
}

static PyObject *
PyPackage_get_requires (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->requires_a);
}

static PyObject *
PyPackage_get_provides (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->provides_a);
}

static PyObject *
PyPackage_get_conflicts (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->conflicts_a);
}

static PyObject *
PyPackage_get_obsoletes (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->obsoletes_a);
}

static PyObject *
PyPackage_get_children (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->children_a);
}

static PyObject *
PyPackage_get_suggests (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->suggests_a);
}

static PyObject *
PyPackage_get_recommends (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	return rc_package_dep_array_to_PyList (pkg->recommends_a);
}

static PyObject *
PyPackage_get_pretty_name (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->pretty_name == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (pkg->pretty_name);
}

static PyObject *
PyPackage_get_summary (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->summary == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (pkg->summary);
}

static PyObject *
PyPackage_get_description (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->description == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (pkg->description);
}

static PyObject *
PyPackage_get_history (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->history == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return rc_package_update_slist_to_PyList (pkg->history);
}

static PyObject *
PyPackage_get_package_filename (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->package_filename == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (pkg->package_filename);
}

static int
PyPackage_set_package_filename (PyObject *self, PyObject *value, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	gchar *val;

	val = PyString_AsString (value);
	if (PyErr_Occurred ())
		return -1;

	if (pkg->package_filename)
		g_free (pkg->package_filename);

	pkg->package_filename = g_strdup (val);

	return 0;
}

static PyObject *
PyPackage_get_signature_filename (PyObject *self, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);

	if (pkg->signature_filename == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (pkg->signature_filename);
}

static int
PyPackage_set_signature_filename (PyObject *self, PyObject *value, void *closure)
{
	RCPackage *pkg = PyPackage_get_package (self);
	gchar *val;

	val = PyString_AsString (value);
	if (PyErr_Occurred ())
		return -1;

	if (pkg->signature_filename)
		g_free (pkg->signature_filename);

	pkg->signature_filename = g_strdup (val);

	return 0;
}

static PyGetSetDef PyPackage_getsets[] = {
	{ "spec",           (getter) PyPackage_get_spec,           (setter) 0 },
	{ "file_size",      (getter) PyPackage_get_file_size,      (setter) 0 },
	{ "installed_size", (getter) PyPackage_get_installed_size, (setter) 0 },
	{ "channel",        (getter) PyPackage_get_channel,        (setter) 0 },
	{ "requires",       (getter) PyPackage_get_requires,       (setter) 0 },
	{ "provides",       (getter) PyPackage_get_provides,       (setter) 0 },
	{ "conflicts",      (getter) PyPackage_get_conflicts,      (setter) 0 },
	{ "obsoletes",      (getter) PyPackage_get_obsoletes,      (setter) 0 },
	{ "children",       (getter) PyPackage_get_children,       (setter) 0 },
	{ "suggests",       (getter) PyPackage_get_suggests,       (setter) 0 },
	{ "recommends",     (getter) PyPackage_get_recommends,     (setter) 0 },
	{ "pretty_name",    (getter) PyPackage_get_pretty_name,    (setter) 0 },
	{ "summary",        (getter) PyPackage_get_summary,        (setter) 0 },
	{ "description",    (getter) PyPackage_get_description,    (setter) 0 },
	{ "history",        (getter) PyPackage_get_history,        (setter) 0 },
	{ "package_filename", (getter) PyPackage_get_package_filename,
	  (setter) PyPackage_set_package_filename },
	{ "signature_filename", (getter) PyPackage_get_signature_filename,
	  (setter) PyPackage_set_signature_filename },
	
	
	{ NULL, (getter)0, (setter)0 },
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackage_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	char *xml = NULL;
	static char *kwlist[] = {"xml", "channel", NULL};
	PyPackage *py_package = (PyPackage *) self;
	PyObject *py_channel = NULL;

	if (! PyArg_ParseTupleAndKeywords (args, kwds, "|sO", kwlist, &xml, &py_channel)) {
		PyErr_SetString (PyExc_RuntimeError, "Can't parse arguments");
		return -1;
	}

	if (!xml) {
		py_package->package = rc_package_new ();
	} else {
		xmlNode *node;
		xmlDoc *doc;
		RCChannel *channel;

		doc = rc_parse_xml_from_buffer (xml, strlen (xml));

		node = xmlDocGetRootElement (doc);

		if (py_channel != NULL) {
			channel = PyChannel_get_channel(py_channel);
		} else {
			channel = NULL;
		}
		
		py_package->package = rc_xml_node_to_package (node, channel);

		xmlFreeDoc (doc);
	}

	if (py_package->package == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create Package");
		return -1;
	}

	return 0;
}

static PyObject *
PyPackage_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyPackage *py_package;

	py_package = (PyPackage *) type->tp_alloc (type, 0);
	py_package->package = NULL;

	return (PyObject *) py_package;
}

static PyObject *
PyPackage_tp_str (PyObject *self)
{
	PyPackage *py_package = (PyPackage *) self;
	RCPackage *package = py_package->package;
	const char *str;

	if (package == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	str = rc_package_to_str_static (package);
	if (str == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyString_FromString (str);
}

static void
PyPackage_tp_dealloc (PyObject *self)
{
	PyPackage *py_package = (PyPackage *) self;

	if (py_package->package)
		rc_package_unref (py_package->package);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackage_register (PyObject *dict)
{
	PyPackage_type_info.tp_init    = PyPackage_init;
	PyPackage_type_info.tp_new     = PyPackage_tp_new;
	PyPackage_type_info.tp_dealloc = PyPackage_tp_dealloc;
	PyPackage_type_info.tp_methods = PyPackage_methods;
	PyPackage_type_info.tp_getset  = PyPackage_getsets;
	PyPackage_type_info.tp_str     = PyPackage_tp_str;

	pyutil_register_type (dict, &PyPackage_type_info);
}

int
PyPackage_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyPackage_type_info);
}

PyObject *
PyPackage_new (RCPackage *package)
{
	PyObject *py_package = PyPackage_tp_new (&PyPackage_type_info,
						 NULL, NULL);
	((PyPackage *) py_package)->package = rc_package_ref (package);

	return py_package;
}

RCPackage *
PyPackage_get_package (PyObject *obj)
{
	if (! PyPackage_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a package");
		return NULL;
	}

	return ((PyPackage *) obj)->package;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

RCPackageSList *
PyList_to_rc_package_slist (PyObject *py_list)
{
	RCPackageSList *slist;
	int i;

	g_return_val_if_fail (PyList_Check (py_list) == 1, NULL);

	slist = NULL;
	for (i = 0; i < PyList_Size (py_list); i++) {
		RCPackage *package;

		package = PyPackage_get_package (PyList_GetItem (py_list, i));
		if (package != NULL)
			slist = g_slist_append (slist, rc_package_ref (package));
	}

	return slist;
}

static void
add_to_pylist_cb (gpointer data, gpointer user_data)
{
	RCPackage *package = (RCPackage *) data;
	PyObject *list = (PyObject *) user_data;
	PyObject *py_package;

	py_package = PyPackage_new (package);
	PyList_Append(list, py_package);
}

PyObject *
rc_package_slist_to_PyList (RCPackageSList *slist)
{
	PyObject *py_list;

	py_list = PyList_New (0);

	if (slist == NULL)
		return py_list;

	g_slist_foreach (slist, add_to_pylist_cb, (gpointer) py_list);

	return py_list;
}
