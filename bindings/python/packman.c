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

static PyMethodDef PyPackman_methods[] = {
  { "parse_version",       PyPackman_parse_version,       METH_VARARGS },
  { "is_locked",           PyPackman_is_locked,           METH_NOARGS  },
  { "lock",                PyPackman_lock,                METH_NOARGS  },
  { "unlock",              PyPackman_unlock,              METH_NOARGS  },
  { "is_database_changed", PyPackman_is_database_changed, METH_NOARGS  },
  { "get_file_extension",  PyPackman_get_file_extension,  METH_NOARGS  },
  { "get_error",           PyPackman_get_error,           METH_NOARGS  },
  { "get_reason",          PyPackman_get_reason,          METH_NOARGS  },
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
  if (! PyPackman_check (obj))
    return NULL;

  return ((PyPackman *) obj)->packman;
}
