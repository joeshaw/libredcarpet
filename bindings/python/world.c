/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * world.c
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

#include "world.h"
#include "packman.h"
#include "package.h"
#include "channel.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCWorld *world;
} PyWorld;

static PyTypeObject PyWorld_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"World",
	sizeof (PyWorld),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyWorld_set (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_set_world (world);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_get_package_seq_num (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	return Py_BuildValue ("i", rc_world_get_package_sequence_number (world));
}

static PyObject *
PyWorld_get_channel_seq_num (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	return Py_BuildValue ("i", rc_world_get_channel_sequence_number (world));
}

static PyObject *
PyWorld_get_sub_seq_num (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	return Py_BuildValue ("i", rc_world_get_subscription_sequence_number (world));
}

static PyObject *
PyWorld_touch_package_seq_num (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_world_touch_package_sequence_number (world);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_touch_channel_seq_num (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_world_touch_channel_sequence_number (world);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_touch_sub_seq_num (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_world_touch_subscription_sequence_number (world);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_get_packman (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCPackman *packman;

	packman = rc_world_get_packman (world);
	if (packman == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackman_new (packman);
}

static PyObject *
PyWorld_get_system_packages (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	return Py_BuildValue ("i", rc_world_get_system_packages (world));
}

static PyObject *
PyWorld_add_channel (PyObject *self, PyObject *args, PyObject *kwds)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	char *channel_name, *alias, *channel_id;
	RCChannelType type;
	gboolean hidden = FALSE;
	int subd_priority = -1;
	int unsubd_priority = -1;
	static char *kwlist[] = { "hidden", "subd_priority", "unsubd_priority", NULL };

	if (! PyArg_ParseTupleAndKeywords (args, kwds, "sssi|iii", kwlist,
								&channel_name, &alias,
								&channel_id, &type,
								&hidden, &subd_priority,
								&unsubd_priority))
		return NULL;

	channel = rc_world_add_channel_with_priorities (world,
										   channel_name,
										   alias,
										   channel_id,
										   hidden,
										   type,
										   subd_priority,
										   unsubd_priority);
	if (channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (channel);
}

static PyObject *
PyWorld_set_subscription (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCChannel *channel;
	gboolean subscribed;

	if (! PyArg_ParseTuple (args, "Oi", &obj, &subscribed))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_world_set_subscription (world, channel, subscribed);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_is_subscribed (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	return Py_BuildValue ("i", rc_world_is_subscribed (world, channel));
}

static PyObject *
PyWorld_migrate_channel (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_world_migrate_channel (world, channel);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_remove_channel (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_world_remove_channel (world, channel);
	Py_INCREF (Py_None);
	return Py_None;
}

static void
get_all_channels_cb (RCChannel *channel, PyObject *list)
{
	PyList_Append (list, PyChannel_new (channel));
}

static PyObject *
PyWorld_get_all_channels (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *py_list;

	py_list = PyList_New (0);
	rc_world_foreach_channel (world,
				  (RCChannelFn) get_all_channels_cb,
				  py_list);
	return py_list;
}

static PyObject *
PyWorld_get_channel_by_name (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	char *str;

	if (! PyArg_ParseTuple (args, "s", &str))
		return NULL;

	channel = rc_world_get_channel_by_name (world, str);
	if (channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (channel);
}

static PyObject *
PyWorld_get_channel_by_alias (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	char *str;

	if (! PyArg_ParseTuple (args, "s", &str))
		return NULL;

	channel = rc_world_get_channel_by_alias (world, str);
	if (channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (channel);
}

static PyObject *
PyWorld_get_channel_by_id (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	char *str;

	if (! PyArg_ParseTuple (args, "s", &str))
		return NULL;

	channel = rc_world_get_channel_by_id (world, str);
	if (channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (channel);
}

static PyObject *
PyWorld_freeze (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_world_freeze (world);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_thaw (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_world_thaw (world);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_add_package (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCPackage *pkg;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	pkg = PyPackage_get_package (obj);
	if (pkg == NULL)
		return NULL;

	return Py_BuildValue ("i", rc_world_add_package (world, pkg));
}

static PyObject *
PyWorld_remove_package (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCPackage *pkg;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	pkg = PyPackage_get_package (obj);
	if (pkg == NULL)
		return NULL;

	rc_world_remove_package (world, pkg);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_remove_packages (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCChannel *channel;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	rc_world_remove_packages (world, channel);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_set_synthetic_pkg_db (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	char *filename;

	if (! PyArg_ParseTuple (args, "s", &filename))
		return NULL;

	rc_world_set_synthetic_package_db (world, filename);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_load_synthetic_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	return Py_BuildValue ("i", rc_world_load_synthetic_packages (world));
}

static PyObject *
PyWorld_save_synthetic_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	return Py_BuildValue ("i", rc_world_save_synthetic_packages (world));
}

static PyObject *
PyWorld_find_installed_version (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCPackage *pkg, *dest_pkg;
	PyObject *obj;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	pkg = PyPackage_get_package (obj);
	if (pkg == NULL)
		return NULL;

	dest_pkg = rc_world_find_installed_version (world, pkg);
	if (dest_pkg == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (dest_pkg);
}

static PyObject *
PyWorld_get_package (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	RCPackage *pkg;
	PyObject *obj;
	char *name;

	if (! PyArg_ParseTuple (args, "Os", &obj, &name))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	pkg = rc_world_get_package (world, channel, name);
	if (pkg == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (pkg);
}

static PyObject *
PyWorld_guess_package_channel (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCPackage *pkg;
	RCChannel *channel;
	PyObject *obj;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	pkg = PyPackage_get_package (obj);
	if (pkg == NULL)
		return NULL;

	channel = rc_world_guess_package_channel (world, pkg);
	if (channel == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyChannel_new (channel);
}

static void
get_all_pkgs_cb (RCPackage *pkg, PyObject *list)
{
	PyList_Append (list, PyPackage_new (pkg));
}

static PyObject *
PyWorld_get_all_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	PyObject *obj, *py_list;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_package (world, channel,
				  (RCPackageFn) get_all_pkgs_cb,
				  py_list);
	return py_list;
}

static PyObject *
PyWorld_get_all_pkgs_by_name (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	PyObject *obj, *py_list;
	char *name;

	if (! PyArg_ParseTuple (args, "sO", &name, &obj))
		return NULL;

	channel = PyChannel_get_channel (obj);
	if (channel == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_package_by_name (world,
					  name,
					  channel,
					  (RCPackageFn) get_all_pkgs_cb,
					  py_list);
	return py_list;
}

static PyObject *
PyWorld_get_all_upgrades (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCPackage *pkg;
	RCChannel *channel;
	PyObject *py_pkg, *py_channel;
	PyObject *py_list;

	if (! PyArg_ParseTuple (args, "OO", &py_pkg, &py_channel))
		return NULL;

	pkg = PyPackage_get_package (py_pkg);
	if (pkg == NULL)
		return NULL;

	channel = PyChannel_get_channel (py_channel);
	if (channel == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_upgrade (world, pkg, channel,
				  (RCPackageFn) get_all_pkgs_cb,
				  py_list);
	return py_list;
}

static PyObject *
PyWorld_get_best_upgrade (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCPackage *pkg, *dest_pkg;
	PyObject *py_pkg;
	gboolean subscribed_only;

	if (! PyArg_ParseTuple (args, "Oi", &py_pkg, &subscribed_only))
		return NULL;

	pkg = PyPackage_get_package (py_pkg);
	if (pkg == NULL)
		return NULL;

	dest_pkg = rc_world_get_best_upgrade (world, pkg, subscribed_only);
	if (dest_pkg == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (dest_pkg);
}

static PyObject *
PyWorld_transact (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCPackageSList *install_packages;
	RCPackageSList *remove_packages;
	PyObject *inst, *rem;
	int flags;

	if (! PyArg_ParseTuple (args, "O!O!i",
					    &PyList_Type, &inst,
					    &PyList_Type, &rem,
					    &flags))
		return NULL;

	install_packages = PyList_to_rc_package_slist (inst);
	remove_packages = PyList_to_rc_package_slist (rem);

	rc_world_transact (world, install_packages, remove_packages, flags);

	rc_package_slist_unref (install_packages);
	rc_package_slist_unref (remove_packages);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef PyWorld_methods[] = {
	{ "set",                                PyWorld_set,                   METH_NOARGS  },
	{ "get_package_sequence_number",        PyWorld_get_package_seq_num,   METH_NOARGS  },
	{ "get_channel_sequence_number",        PyWorld_get_channel_seq_num,   METH_NOARGS  },
	{ "get_subscriprion_sequence_number",   PyWorld_get_sub_seq_num,       METH_NOARGS  },
	{ "touch_package_sequence_number",      PyWorld_touch_package_seq_num, METH_NOARGS  },
	{ "touch_channel_sequence_number",      PyWorld_touch_channel_seq_num, METH_NOARGS  },
	{ "touch_subscription_sequence_number", PyWorld_touch_sub_seq_num,     METH_NOARGS  },
	{ "get_packman",                        PyWorld_get_packman,           METH_NOARGS  },
	{ "get_system_packages",                PyWorld_get_system_packages,   METH_NOARGS  },
	{ "add_channel",          (PyCFunction) PyWorld_add_channel,           METH_VARARGS|METH_KEYWORDS },
	{ "set_subscription",                   PyWorld_set_subscription,      METH_VARARGS },
	{ "is_subscribed",                      PyWorld_is_subscribed,         METH_VARARGS },
	{ "migrate_channel",                    PyWorld_migrate_channel,       METH_VARARGS },
	{ "remove_channel",                     PyWorld_remove_channel,        METH_VARARGS },
	/* rc_world_foreach_channel */
	{ "get_all_channels",                   PyWorld_get_all_channels,      METH_NOARGS  },
	{ "get_channel_by_name",                PyWorld_get_channel_by_name,   METH_VARARGS },
	{ "get_channel_by_alias",               PyWorld_get_channel_by_alias,  METH_VARARGS },
	{ "get_channel_by_id",                  PyWorld_get_channel_by_id,     METH_VARARGS },

	{ "freeze",                             PyWorld_freeze,                METH_NOARGS  },
	{ "thaw",                               PyWorld_thaw,                  METH_NOARGS  },
	{ "add_package",                        PyWorld_add_package,           METH_VARARGS },
	{ "remove_package",                     PyWorld_remove_package,        METH_VARARGS },
	{ "remove_packages",                    PyWorld_remove_packages,       METH_VARARGS },
	{ "set_synthetic_package_db",           PyWorld_set_synthetic_pkg_db,  METH_VARARGS },
	{ "load_synthetic_packages",            PyWorld_load_synthetic_pkgs,   METH_NOARGS  },
	{ "save_synthetic_packages",            PyWorld_save_synthetic_pkgs,   METH_NOARGS  },
	{ "find_installed_version",             PyWorld_find_installed_version,METH_VARARGS },
	{ "get_package",                        PyWorld_get_package,           METH_VARARGS },
	{ "guess_package_channel",              PyWorld_guess_package_channel, METH_VARARGS },

	/* rc_world_foreach_package */
	{ "get_all_packages",                   PyWorld_get_all_pkgs,          METH_VARARGS },
	/* rc_world_foreach_package_by_name */
	{ "get_all_packages_by_name",           PyWorld_get_all_pkgs_by_name,  METH_VARARGS },
	/* rc_world_foreach_upgrade */
	{ "get_all_upgrades",                   PyWorld_get_all_upgrades,      METH_VARARGS },
	{ "get_best_upgrade",                   PyWorld_get_best_upgrade,      METH_VARARGS },

	{ "transact",                           PyWorld_transact,              METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyWorld_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyWorld *py_world = (PyWorld *) self;
	PyObject *obj;

	if (! PyArg_ParseTuple (args, "O", &obj)) {
		py_world->world = rc_get_world();
	} else {
		RCPackman *packman;

		packman = PyPackman_get_packman (obj);
		if (packman == NULL)
			return NULL;

		py_world->world = rc_world_new (packman);
	}

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyWorld *py_world;

	py_world = (PyWorld *) type->tp_alloc (type, 0);
	py_world->world = NULL;

	return (PyObject *) py_world;
}

static void
PyWorld_tp_dealloc (PyObject *self)
{
	PyWorld *py_world = (PyWorld *) self;

	if (py_world->world)
		rc_world_free (py_world->world);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void PyWorld_register (PyObject *dict)
{
	PyWorld_type_info.tp_init = (initproc) PyWorld_init;
	PyWorld_type_info.tp_new  = PyWorld_tp_new;
	PyWorld_type_info.tp_dealloc = PyWorld_tp_dealloc;
	PyWorld_type_info.tp_methods = PyWorld_methods;

	pyutil_register_type (dict, &PyWorld_type_info);
}

int
PyWorld_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyWorld_type_info);
}

PyObject *
PyWorld_new (RCWorld *world)
{
	PyObject *py_world = PyWorld_tp_new (&PyWorld_type_info,
					     NULL, NULL);
	((PyWorld *) py_world)->world = world;

	return py_world;
}

RCWorld *
PyWorld_get_world (PyObject *obj)
{
	if (! PyWorld_check (obj))
		return NULL;

	return ((PyWorld *) obj)->world;
}
