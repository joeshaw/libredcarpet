/* This is -*- C -*- */
/* vim: set sw=2: */
/* $Id$ */

/*
 * packman.c
 *
 * Copyright (C) 2003 The Free Software Foundation, Inc.
 *
 * Developed by Jon Trowbridge <trow@gnu.org>
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

#include "packman.h"
#include "package-spec.h"
#include "package.h"
#include "verification.h"
#include "pyutil.h"

typedef struct {
  PyObject_HEAD;
  RCPackman *packman;
} PyPackman;

static PyTypeObject PyPackman_type_info = {
  PyObject_HEAD_INIT(&PyType_Type)
  0,
  "Packman",
  sizeof (PyPackman),
  0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyPackman_transact (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
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

	rc_packman_transact (packman, install_packages, remove_packages, flags);

	rc_package_slist_unref (install_packages);
	rc_package_slist_unref (remove_packages);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
PyPackman_query_file (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	RCPackage *package;
	char *filename;

	if (! PyArg_ParseTuple (args, "s", &filename))
		return NULL;

	package = rc_packman_query_file (packman, filename);
	if (package == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (package);
}

static PyObject *
PyPackman_query (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	PyObject *py_list;
	RCPackageSList *slist;
	char *name;

	if (! PyArg_ParseTuple (args, "s", &name))
		return NULL;

	slist = rc_packman_query (packman, name);
	if (slist == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	py_list = rc_package_slist_to_PyList (slist);
	rc_package_slist_unref (slist);

	return py_list;
}

static PyObject *
PyPackman_query_all (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	PyObject *py_list;
	RCPackageSList *slist;

	slist = rc_packman_query_all (packman);
	if (slist == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	py_list = rc_package_slist_to_PyList (slist);
	rc_package_slist_unref (slist);

	return py_list;
}

static PyObject *
PyPackman_version_compare (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  PyObject *obj1, *obj2;
  RCPackageSpec *spec1, *spec2;

  if (! PyArg_ParseTuple (args, "OO", &obj1, &obj2))
	  return NULL;

  spec1 = PyPackageSpec_get_package_spec (obj1);
  spec2 = PyPackageSpec_get_package_spec (obj2);

  if (spec1 == NULL || spec2 == NULL)
	  return NULL;

  return Py_BuildValue ("i",
				    rc_packman_version_compare (packman, spec1, spec2));
}

static PyObject *
PyPackman_parse_version (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  char *str;
  gboolean has_epoch;
  guint32 epoch;
  char *version, *release;
  PyObject *retval;

  if (! PyArg_ParseTuple (args, "s", &str))
    return NULL;

  if (! rc_packman_parse_version (packman, str,
				  &has_epoch, &epoch, &version, &release)) {
    Py_INCREF (Py_None);
    return Py_None;
  }

  if (! has_epoch)
    epoch = 0;

  retval = Py_BuildValue ("(iiss)", has_epoch, epoch, version, release);

  g_free (version);
  g_free (release);

  return retval;
}

static PyObject *
PyPackman_verify (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	PyObject *obj;
	RCPackage *package;
	RCVerificationType type;
	RCVerificationSList *slist;

	if (! PyArg_ParseTuple (args, "Oi", &obj, &type))
		return NULL;

	package = PyPackage_get_package (obj);
	if (package == NULL)
		return NULL;

	slist = rc_packman_verify (packman, package, type);

	return RCVerificationSList_to_py_list (slist);
}

static PyObject *
PyPackman_find_file (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	RCPackage *package;
	char *filename;

	if (! PyArg_ParseTuple (args, "s", &filename))
		return NULL;

	package = rc_packman_find_file (packman, filename);
	if (package == NULL) {
		Py_INCREF (Py_None);
		return Py_None;
	}

	return PyPackage_new (package);
}

static PyObject *
PyPackman_is_locked (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("i", rc_packman_is_locked (packman));
}

static PyObject *
PyPackman_lock (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("i", rc_packman_lock (packman));
}

static PyObject *
PyPackman_unlock (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  rc_packman_unlock (packman);
  Py_INCREF (Py_None);
  return Py_None;
}

static PyObject *
PyPackman_is_database_changed (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("i", rc_packman_is_database_changed (packman));
}

static PyObject *
PyPackman_get_file_extension (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("s", rc_packman_get_file_extension (packman));
}

static PyObject *
PyPackman_get_capabilities (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("i", rc_packman_get_capabilities (packman));
}

static PyObject *
PyPackman_get_error (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("i", rc_packman_get_error (packman));
}

static PyObject *
PyPackman_get_reason (PyObject *self, PyObject *args)
{
  RCPackman *packman = PyPackman_get_packman (self);
  return Py_BuildValue ("s", rc_packman_get_reason (packman));
}

static PyObject *
PyPackman_get_repackage_dir (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	return Py_BuildValue ("s", rc_packman_get_repackage_dir (packman));
}

static PyObject *
PyPackman_set_repackage_dir (PyObject *self, PyObject *args)
{
	RCPackman *packman = PyPackman_get_packman (self);
	char *str;

	if (! PyArg_ParseTuple (args, "s", &str))
		return NULL;

	rc_packman_set_repackage_dir (packman, str);
	Py_INCREF (Py_None);
	return Py_None;
}

static PyMethodDef PyPackman_methods[] = {
  { "transact",            PyPackman_transact,            METH_VARARGS },
  { "query_file",          PyPackman_query_file,          METH_VARARGS },
  { "query",               PyPackman_query,               METH_VARARGS },
  { "query_all",           PyPackman_query_all,           METH_NOARGS  },
  { "version_compare",     PyPackman_version_compare,     METH_VARARGS },
  { "parse_version",       PyPackman_parse_version,       METH_VARARGS },
  { "verify",              PyPackman_verify,              METH_VARARGS },
  { "find_file",           PyPackman_find_file,           METH_VARARGS },
  { "is_locked",           PyPackman_is_locked,           METH_NOARGS  },
  { "lock",                PyPackman_lock,                METH_NOARGS  },
  { "unlock",              PyPackman_unlock,              METH_NOARGS  },
  { "is_database_changed", PyPackman_is_database_changed, METH_NOARGS  },
  { "get_file_extension",  PyPackman_get_file_extension,  METH_NOARGS  },
  { "get_capabilities",    PyPackman_get_capabilities,    METH_NOARGS  },
  { "get_error",           PyPackman_get_error,           METH_NOARGS  },
  { "get_reason",          PyPackman_get_reason,          METH_NOARGS  },
  { "get_repackage_dir",   PyPackman_get_repackage_dir,   METH_NOARGS  },
  { "set_repackage_dir",   PyPackman_set_repackage_dir,   METH_VARARGS },
  { NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyPackman_init (PyObject *self, PyObject *args, PyObject *kwds)
{
  PyPackman *py_packman = (PyPackman *) self;

  py_packman->packman = rc_distman_new ();

  if (py_packman->packman == NULL) {
    PyErr_SetString (PyExc_RuntimeError, "Can't create packman");
    return -1;
  }

  return 0;
}

static PyObject *
PyPackman_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
  PyPackman *py_packman;

  py_packman = (PyPackman *) type->tp_alloc (type, 0);
  py_packman->packman = NULL;

  return (PyObject *) py_packman;
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyPackman_register (PyObject *dict)
{
  PyPackman_type_info.tp_init = PyPackman_init;
  PyPackman_type_info.tp_new  = PyPackman_tp_new;

  PyPackman_type_info.tp_methods = PyPackman_methods;

  pyutil_register_type (dict, &PyPackman_type_info);

  pyutil_register_int_constant (dict, "PACKMAN_CAP_NONE",
						  RC_PACKMAN_CAP_NONE);
  pyutil_register_int_constant (dict, "PACKMAN_CAP_PROVIDE_ALL_VERSIONS",
						  RC_PACKMAN_CAP_PROVIDE_ALL_VERSIONS);
  pyutil_register_int_constant (dict, "PACKMAN_CAP_IGNORE_ABSENT_EPOCHS",
						  RC_PACKMAN_CAP_IGNORE_ABSENT_EPOCHS);
  pyutil_register_int_constant (dict, "PACKMAN_CAP_ROLLBACK",
						  RC_PACKMAN_CAP_ROLLBACK);
  pyutil_register_int_constant (dict, "PACKMAN_CAP_ALWAYS_VERIFY_RELEASE",
						  RC_PACKMAN_CAP_ALWAYS_VERIFY_RELEASE);

  pyutil_register_int_constant (dict, "TRANSACT_FLAG_NONE",
						  RC_TRANSACT_FLAG_NONE);
  pyutil_register_int_constant (dict, "TRANSACT_FLAG_NO_ACT",
						  RC_TRANSACT_FLAG_NO_ACT);
  pyutil_register_int_constant (dict, "TRANSACT_FLAG_REPACKAGE",
						  RC_TRANSACT_FLAG_REPACKAGE);
}

int
PyPackman_check (PyObject *obj)
{
  return PyObject_TypeCheck (obj, &PyPackman_type_info);
}

PyObject *
PyPackman_new (RCPackman *packman)
{
  PyObject *py_packman = PyPackman_tp_new (&PyPackman_type_info,
					   NULL, NULL);
  ((PyPackman *) py_packman)->packman = packman;

  return py_packman;
}

RCPackman *
PyPackman_get_packman (PyObject *obj)
{
	if (! PyPackman_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a packman");
		return NULL;
	}

  return ((PyPackman *) obj)->packman;
}
