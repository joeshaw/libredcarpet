/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * verification.c
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

#include "verification.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCVerification *verification;
	gboolean borrowed;
} PyVerification;

static PyTypeObject PyVerification_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Verification",
	sizeof (PyVerification),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
verification_set_keyring (PyObject *self, PyObject *args)
{
	const char *keyring;

	if (! PyArg_ParseTuple (args, "s", &keyring))
		return NULL;

	rc_verification_set_keyring (keyring);

	Py_INCREF (Py_None);
	return Py_None;
}

static PyObject *
verification_type_to_string (PyObject *self, PyObject *args)
{
	RCVerificationType type;

	if (! PyArg_ParseTuple (args, "i", &type))
		return NULL;

	return Py_BuildValue ("s", rc_verification_type_to_string (type));
}

static PyMethodDef PyVerification_unbound_methods[] = {
	{ "verification_set_keyring",    verification_set_keyring,    METH_VARARGS },
	{ "verification_type_to_string", verification_type_to_string, METH_VARARGS },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static int
PyVerification_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyVerification *py_verification = (PyVerification *) self;

	py_verification->verification = rc_verification_new ();

	if (py_verification->verification == NULL) {
		PyErr_SetString (PyExc_RuntimeError, "Can't create verification");
		return -1;
	}

	return 0;
}

static PyObject *
PyVerification_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyVerification *py_verification;

	py_verification = (PyVerification *) type->tp_alloc (type, 0);
	py_verification->verification = NULL;
	py_verification->borrowed = FALSE;

	return (PyObject *) py_verification;
}

static void
PyVerification_tp_dealloc (PyObject *self)
{
	PyVerification *py_verification = (PyVerification *) self;

	if (py_verification->verification && py_verification->borrowed == FALSE)
		rc_verification_free (py_verification->verification);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void
PyVerification_register (PyObject *dict)
{
	PyVerification_type_info.tp_init    = PyVerification_init;
	PyVerification_type_info.tp_new     = PyVerification_tp_new;
	PyVerification_type_info.tp_dealloc = PyVerification_tp_dealloc;

	pyutil_register_type (dict, &PyVerification_type_info);
	pyutil_register_methods (dict, PyVerification_unbound_methods);

	pyutil_register_int_constant (dict, "VERIFICATION_TYPE_ALL",
							RC_VERIFICATION_TYPE_ALL);
	pyutil_register_int_constant (dict, "VERIFICATION_TYPE_SANITY",
							RC_VERIFICATION_TYPE_SANITY);
	pyutil_register_int_constant (dict, "VERIFICATION_TYPE_SIZE",
							RC_VERIFICATION_TYPE_SIZE);
	pyutil_register_int_constant (dict, "VERIFICATION_TYPE_MD5",
							RC_VERIFICATION_TYPE_MD5);
	pyutil_register_int_constant (dict, "VERIFICATION_TYPE_GPG",
							RC_VERIFICATION_TYPE_GPG);
}

int
PyVerification_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyVerification_type_info);
}

PyObject *
PyVerification_new (RCVerification *verification)
{
	PyObject *py_verification = PyVerification_tp_new (&PyVerification_type_info,
						   NULL, NULL);
	((PyVerification *) py_verification)->verification = verification;
	((PyVerification *) py_verification)->borrowed = TRUE;

	return py_verification;
}

RCVerification *
PyVerification_get_verification (PyObject *obj)
{
	if (! PyVerification_check (obj)) {
		PyErr_SetString (PyExc_TypeError, "Given object is not a verification");
		return NULL;
	}

	return ((PyVerification *) obj)->verification;
}

/* STEALS OWNERSHIP, DESTROYS slist!!!! */
PyObject *
RCVerificationSList_to_py_list (RCVerificationSList *slist)
{
	PyObject *py_list;
	GSList *iter;

	py_list = PyList_New (0);
	for (iter = slist; iter != NULL; iter = iter->next) {
		PyObject *py_ver;
		RCVerification *ver = iter->data;

		py_ver = PyVerification_new (ver);
		((PyVerification *) py_ver)->borrowed = FALSE;

		PyList_Append (py_list, py_ver);
	}

	g_slist_free (slist);

	return py_list;
}
