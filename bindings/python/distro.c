/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * distro.c
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

#include "distro.h"
#include "pyutil.h"

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
distro_get_name (PyObject *self, PyObject *args)
{
	return Py_BuildValue ("s", rc_distro_get_name ());
}

static PyObject *
distro_get_version (PyObject *self, PyObject *args)
{
	return Py_BuildValue ("s", rc_distro_get_version ());
}

static PyObject *
distro_get_package_type (PyObject *self, PyObject *args)
{
	return Py_BuildValue ("i", rc_distro_get_package_type ());
}

static PyObject *
distro_get_target (PyObject *self, PyObject *args)
{
	return Py_BuildValue ("s", rc_distro_get_target ());
}

static PyObject *
distro_get_status (PyObject *self, PyObject *args)
{
	return Py_BuildValue ("i", rc_distro_get_status ());
}

static PyObject *
distro_get_death_date (PyObject *self, PyObject *args)
{
	return Py_BuildValue ("i", rc_distro_get_death_date ());
}

static PyMethodDef PyDistro_methods[] = {
	{ "distro_get_name",         distro_get_name,         METH_NOARGS },
	{ "distro_get_version",      distro_get_version,      METH_NOARGS },
	{ "distro_get_package_type", distro_get_package_type, METH_NOARGS },
	{ "distro_get_target",       distro_get_target,       METH_NOARGS },
	{ "distro_get_status",       distro_get_status,       METH_NOARGS },
	{ "distro_get_death_date",   distro_get_death_date,   METH_NOARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyDistro_register (PyObject *dict)
{
	pyutil_register_methods (dict, PyDistro_methods);

	pyutil_register_int_constant (dict, "DISTRO_PACKAGE_TYPE_UNKNOWN",
							RC_DISTRO_PACKAGE_TYPE_UNKNOWN);
	pyutil_register_int_constant (dict, "DISTRO_PACKAGE_TYPE_RPM",
							RC_DISTRO_PACKAGE_TYPE_RPM);
	pyutil_register_int_constant (dict, "DISTRO_PACKAGE_TYPE_DPKG",
							RC_DISTRO_PACKAGE_TYPE_DPKG);

	pyutil_register_int_constant (dict, "DISTRO_STATUS_UNSUPPORTED",
							RC_DISTRO_STATUS_UNSUPPORTED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_PRESUPPORTED",
							RC_DISTRO_STATUS_PRESUPPORTED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_SUPPORTED",
							RC_DISTRO_STATUS_SUPPORTED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_DEPRECATED",
							RC_DISTRO_STATUS_DEPRECATED);
	pyutil_register_int_constant (dict, "DISTRO_STATUS_RETIRED",
							RC_DISTRO_STATUS_RETIRED);
}
