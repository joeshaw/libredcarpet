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

#include "channel.h"
#include "package.h"
#include "package-dep.h"
#include "package-match.h"
#include "package-spec.h"
#include "pyutil.h"

PyTypeObject PyWorld_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"World",
	sizeof (PyWorld),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

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
PyWorld_add_lock (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCPackageMatch *lock;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	lock = PyPackageMatch_get_package_match (obj);
	if (lock == NULL)
		return NULL;

	rc_world_add_lock (world, lock);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_remove_lock (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCPackageMatch *lock;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	lock = PyPackageMatch_get_package_match (obj);
	if (lock == NULL)
		return NULL;

	rc_world_remove_lock (world, lock);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyWorld_clear_locks (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);

	rc_world_clear_locks (world);

	Py_INCREF (Py_None);
	return Py_None;
}

static gboolean
package_match_cb (RCPackageMatch *match, gpointer user_data)
{
	PyObject *list = user_data;

	PyList_Append (list, PyPackageMatch_new (match));

	return TRUE;
}

static PyObject *
PyWorld_get_all_locks (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *py_list;

	py_list = PyList_New (0);
	rc_world_foreach_lock (world, package_match_cb, py_list);

	return py_list;
}

static PyObject *
PyWorld_pkg_is_locked (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj;
	RCPackage *pkg;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	pkg = PyPackage_get_package (obj);
	if (pkg == NULL)
		return NULL;

	return Py_BuildValue ("i", rc_world_package_is_locked (world, pkg));
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

	/* Channel or channel wildcard */
	channel = PyChannel_get_channel (obj);

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

	/* Channel or channel wildcard */
	channel = PyChannel_get_channel (obj);

	py_list = PyList_New (0);
	rc_world_foreach_package_by_name (world,
					  name,
					  channel,
					  (RCPackageFn) get_all_pkgs_cb,
					  py_list);
	return py_list;
}

static PyObject *
PyWorld_get_all_pkgs_by_match (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj, *py_list;
	RCPackageMatch *match;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	match = PyPackageMatch_get_package_match (obj);
	if (match == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_package_by_match (world, match,
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

	/* Channel or channel wildcard */
	channel = PyChannel_get_channel (py_channel);

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

static void
get_all_system_upgrades_cb (RCPackage *old, RCPackage *new, gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *tuple;

	tuple = PyTuple_New(2);
	if (tuple == NULL)
		return;

	PyTuple_SetItem (tuple, 0, PyPackage_new (old));
	PyTuple_SetItem (tuple, 1, PyPackage_new (new));

	PyList_Append (list, tuple);
}

static PyObject *
PyWorld_get_all_system_upgrades (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *py_list;
	gboolean subscribed_only;

	if (! PyArg_ParseTuple (args, "i", &subscribed_only))
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_system_upgrade (world, subscribed_only,
					 get_all_system_upgrades_cb,
					 py_list);
	return py_list;
}

static gboolean
get_all_providing_pkgs_cb (RCPackage *pkg, RCPackageSpec *spec, gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *tuple;

	tuple = PyTuple_New (2);
	PyTuple_SetItem (tuple, 0, PyPackage_new (pkg));
	PyTuple_SetItem (tuple, 1, PyPackageSpec_new (spec));

	PyList_Append (list, tuple);

	return TRUE;
}

static PyObject *
PyWorld_get_all_providing_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj, *py_list;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_providing_package (world, dep,
					    get_all_providing_pkgs_cb,
					    py_list);
	return py_list;
}

static gboolean
get_pkg_and_dep_cb (RCPackage *pkg, RCPackageDep *dep, gpointer user_data)
{
	PyObject *list = user_data;
	PyObject *tuple;

	tuple = PyTuple_New (2);
	PyTuple_SetItem (tuple, 0, PyPackage_new (pkg));
	PyTuple_SetItem (tuple, 1, PyPackageDep_new (dep));

	PyList_Append (list, tuple);

	return TRUE;
}

static PyObject *
PyWorld_get_all_requiring_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj, *py_list;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_requiring_package (world, dep,
					    get_pkg_and_dep_cb,
					    py_list);
	return py_list;
}

static PyObject *
PyWorld_get_all_parent_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj, *py_list;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_parent_package (world, dep,
					 get_pkg_and_dep_cb,
					 py_list);
	return py_list;
}

static PyObject *
PyWorld_get_all_conflicting_pkgs (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	PyObject *obj, *py_list;
	RCPackageDep *dep;

	if (! PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	dep = PyPackageDep_get_package_dep (obj);
	if (dep == NULL)
		return NULL;

	py_list = PyList_New (0);
	rc_world_foreach_conflicting_package (world, dep,
					      get_pkg_and_dep_cb,
					      py_list);
	return py_list;
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

/* Methods from rc-world-import.c */

#if 0
static PyObject *
PyWorld_add_channel_from_dir (PyObject *self, PyObject *args)
{
	RCWorld *world = PyWorld_get_world (self);
	RCChannel *channel;
	const char *channel_name, *alias, *directory;

	if (! PyArg_ParseTuple (args, "sss", &channel_name, &alias, &directory))
		return NULL;

	channel = rc_world_add_channel_from_directory (world,
						       channel_name,
						       alias,
						       directory);

	return PyChannel_new (channel);
}
#endif

static PyMethodDef PyWorld_methods[] = {
	{ "get_package_sequence_number",        PyWorld_get_package_seq_num,   METH_NOARGS  },
	{ "get_channel_sequence_number",        PyWorld_get_channel_seq_num,   METH_NOARGS  },
	{ "get_subscriprion_sequence_number",   PyWorld_get_sub_seq_num,       METH_NOARGS  },
	{ "touch_package_sequence_number",      PyWorld_touch_package_seq_num, METH_NOARGS  },
	{ "touch_channel_sequence_number",      PyWorld_touch_channel_seq_num, METH_NOARGS  },
	{ "touch_subscription_sequence_number", PyWorld_touch_sub_seq_num,     METH_NOARGS  },
	{ "set_subscription",                   PyWorld_set_subscription,      METH_VARARGS },
	{ "is_subscribed",                      PyWorld_is_subscribed,         METH_VARARGS },
	{ "get_best_upgrade",                   PyWorld_get_best_upgrade,      METH_VARARGS },
	/* rc_world_foreach_channel */
	{ "get_all_channels",                   PyWorld_get_all_channels,      METH_NOARGS  },
	{ "get_channel_by_name",                PyWorld_get_channel_by_name,   METH_VARARGS },
	{ "get_channel_by_alias",               PyWorld_get_channel_by_alias,  METH_VARARGS },
	{ "get_channel_by_id",                  PyWorld_get_channel_by_id,     METH_VARARGS },
	{ "add_lock",                           PyWorld_add_lock,              METH_VARARGS },
	{ "remove_lock",                        PyWorld_remove_lock,           METH_VARARGS },
	{ "clear_locks",                        PyWorld_clear_locks,           METH_NOARGS  },
	{ "get_all_locks",                      PyWorld_get_all_locks,         METH_NOARGS  },
	{ "package_is_locked",                  PyWorld_pkg_is_locked,         METH_VARARGS },
	{ "find_installed_version",             PyWorld_find_installed_version,METH_VARARGS },
	{ "get_package",                        PyWorld_get_package,           METH_VARARGS },
	{ "guess_package_channel",              PyWorld_guess_package_channel, METH_VARARGS },
	/* rc_world_foreach_package */
	{ "get_all_packages",                   PyWorld_get_all_pkgs,          METH_VARARGS },
	/* rc_world_foreach_package_by_name */
	{ "get_all_packages_by_name",           PyWorld_get_all_pkgs_by_name,  METH_VARARGS },
	/* rc_world_foreach_package_by_match */
	{ "get_all_packages_by_match",          PyWorld_get_all_pkgs_by_match, METH_VARARGS },
	/* rc_world_foreach_upgrade */
	{ "get_all_upgrades",                   PyWorld_get_all_upgrades,      METH_VARARGS },
	{ "get_best_upgrade",                   PyWorld_get_best_upgrade,      METH_VARARGS },
	/* rc_world_foreach_system_upgrade */
	{ "get_all_system_upgrades",            PyWorld_get_all_system_upgrades, METH_VARARGS },
	/* rc_world_foreach_providing_package */
	{ "get_all_providing_packages",         PyWorld_get_all_providing_pkgs,  METH_VARARGS },
	/* rc_world_foreach_requiring_package */
	{ "get_all_requiring_packages",         PyWorld_get_all_requiring_pkgs,  METH_VARARGS },
	/* rc_world_foreach_parent_package */
	{ "get_all_parent_packages",            PyWorld_get_all_parent_pkgs,   METH_VARARGS },
	/* rc_world_foreach_conflicting_package */
	{ "get_all_conflicting_pkgs",           PyWorld_get_all_conflicting_pkgs,METH_VARARGS },

	{ "transact",                           PyWorld_transact,              METH_VARARGS },

	/* Methods from rc-world-import.c */

#if 0
	{ "add_channel_from_directory",         PyWorld_add_channel_from_dir,  METH_VARARGS },
#endif

	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyWorld *global_pyworld = NULL;

static PyObject *
PyWorld_get_global_world (PyObject *self, PyObject *args)
{
	RCWorld *world;

	world = rc_get_world ();

	if (!world) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return (PyObject *) global_pyworld;
}

static PyObject *
PyWorld_set_global_world (PyObject *self, PyObject *args)
{
	PyObject *obj;
	RCWorld *world;

	if (!PyArg_ParseTuple (args, "O", &obj))
		return NULL;

	world = PyWorld_get_world (obj);
	if (world == NULL)
		return NULL;

	rc_set_world (world);

	if (global_pyworld) {
		Py_DECREF (global_pyworld);
	}

	global_pyworld = (PyWorld *) obj;
	Py_INCREF (global_pyworld);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef PyWorld_global_methods[] = {
	{ "get_world", PyWorld_get_global_world, METH_NOARGS },
	{ "set_world", PyWorld_set_global_world, METH_VARARGS },
	{ NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static void
PyWorld_tp_dealloc (PyObject *self)
{
	PyWorld *py_world = (PyWorld *) self;

	if (py_world->world)
		g_object_unref (py_world->world);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void PyWorld_register (PyObject *dict)
{
	PyWorld_type_info.tp_dealloc = PyWorld_tp_dealloc;
	PyWorld_type_info.tp_methods = PyWorld_methods;

	pyutil_register_type (dict, &PyWorld_type_info);

	pyutil_register_methods (dict, PyWorld_global_methods);
}

int
PyWorld_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyWorld_type_info);
}

RCWorld *
PyWorld_get_world (PyObject *obj)
{
	if (! PyWorld_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a World");
		return NULL;
	}

	return ((PyWorld *) obj)->world;
}
