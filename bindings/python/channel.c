/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * channel.c
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

#include "channel.h"
#include "package.h"
#include "pyutil.h"

typedef struct {
	PyObject_HEAD;
	RCChannel *channel;
} PyChannel;

static PyTypeObject PyChannel_type_info = {
	PyObject_HEAD_INIT(&PyType_Type)
	0,
	"Channel",
	sizeof (PyChannel),
	0
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

static PyObject *
PyChannel_get_id (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_id (channel));
}

static PyObject *
PyChannel_get_name (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_name (channel));
}

static PyObject *
PyChannel_get_alias (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_alias (channel));
}

static PyObject *
PyChannel_get_description (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_description (channel));
}

static PyObject *
PyChannel_get_priority (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	gboolean subscribed;

	if (PyArg_ParseTuple (args, "i", &subscribed))
		return NULL;

	return Py_BuildValue ("i", rc_channel_get_priority (channel,
							    subscribed));
}

static PyObject *
PyChannel_get_type (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_get_type (channel));
}

static PyObject *
PyChannel_get_pkginfo_file (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_pkginfo_file (channel));
}

static PyObject *
PyChannel_get_pkginfo_compressed (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_get_pkginfo_compressed (channel));
}

static PyObject *
PyChannel_get_last_update (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_get_last_update (channel));
}

static PyObject *
PyChannel_get_path (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_path (channel));
}

static PyObject *
PyChannel_get_icon_file (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("s", rc_channel_get_icon_file (channel));
}

static PyObject *
PyChannel_subscribed (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_subscribed (channel));
}

static PyObject *
PyChannel_set_subscription (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	gboolean subscribed;

	if (! PyArg_ParseTuple (args, "i", &subscribed))
		return NULL;

	rc_channel_set_subscription (channel, subscribed);

	Py_INCREF (Py_None);
	return Py_None;
}

static void
get_all_packages_cb (RCPackage *package, PyObject *list)
{
	PyList_Append (list, PyPackage_new (package));
}

static PyObject *
PyChannel_get_all_packages (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	PyObject *py_list;

	py_list = PyList_New (0);
	rc_channel_foreach_package (channel,
				    (RCPackageFn) get_all_packages_cb,
				    py_list);
	return py_list;
}

static PyObject *
PyChannel_has_refresh_magic (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_has_refresh_magic (channel));
}

static PyObject *
PyChannel_use_refresh_magic (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_use_refresh_magic (channel));
}

static PyObject *
PyChannel_get_transient (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_get_transient (channel));
}

static PyObject *
PyChannel_get_silent (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_get_silent (channel));
}

static PyObject *
PyChannel_is_wildcard (PyObject *self, PyObject *args)
{
	RCChannel *channel = PyChannel_get_channel (self);
	return Py_BuildValue ("i", rc_channel_is_wildcard (channel));
}

static PyMethodDef PyChannel_methods[] = {
	{ "get_id",                 PyChannel_get_id,                 METH_NOARGS  },
	{ "get_name",               PyChannel_get_name,               METH_NOARGS  },
	{ "get_alias",              PyChannel_get_alias,              METH_NOARGS  },
	{ "get_description",        PyChannel_get_description,        METH_NOARGS  },
	{ "get_priority",           PyChannel_get_priority,           METH_VARARGS },
	{ "get_type",               PyChannel_get_type,               METH_NOARGS  },
	{ "get_pkginfo_file",       PyChannel_get_pkginfo_file,       METH_NOARGS  },
	{ "get_pkginfo_compressed", PyChannel_get_pkginfo_compressed, METH_NOARGS  },
	{ "get_last_update",        PyChannel_get_last_update,        METH_NOARGS  },
	{ "get_path",               PyChannel_get_path,               METH_NOARGS  },
	{ "get_icon_file",          PyChannel_get_icon_file,          METH_NOARGS  },
	{ "subscribed",             PyChannel_subscribed,             METH_NOARGS  },
	{ "set_subscription",       PyChannel_set_subscription,       METH_VARARGS },
	{ "get_all_packages",       PyChannel_get_all_packages,       METH_NOARGS  },
	{ "has_refresh_magic",      PyChannel_has_refresh_magic,      METH_NOARGS  },
	{ "use_refresh_magic",      PyChannel_use_refresh_magic,      METH_NOARGS  },
	{ "get_transient",          PyChannel_get_transient,          METH_NOARGS  },
	{ "get_silent",             PyChannel_get_silent,             METH_NOARGS  },

	{ "is_wildcard",            PyChannel_is_wildcard,            METH_NOARGS  },
	{ NULL, NULL }
};

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */
#if 0
static int
PyChannel_init (PyObject *self, PyObject *args, PyObject *kwds)
{
	PyChannel *py_channel = (PyChannel *) self;

	py_channel->channel = ;

	return 0;
}
#endif

static PyObject *
PyChannel_tp_new (PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	PyChannel *py_channel;

	py_channel = (PyChannel *) type->tp_alloc (type, 0);
	py_channel->channel = NULL;

	return (PyObject *) py_channel;
}

static void
PyChannel_tp_dealloc (PyObject *self)
{
	PyChannel *py_channel = (PyChannel *) self;

	if (py_channel->channel)
		rc_channel_unref (py_channel->channel);

	if (self->ob_type->tp_free)
		self->ob_type->tp_free (self);
	else
		PyObject_Del (self);
}

/* ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** ** */

void PyChannel_register (PyObject *dict)
{
	/* PyChannel_type_info.tp_init = PyChannel_init; */
	PyChannel_type_info.tp_new  = PyChannel_tp_new;
	PyChannel_type_info.tp_dealloc = PyChannel_tp_dealloc;
	PyChannel_type_info.tp_methods = PyChannel_methods;

	pyutil_register_type (dict, &PyChannel_type_info);

	/* RCChannelType */
	pyutil_register_int_constant (dict, "CHANNEL_TYPE_HELIX",
				      RC_CHANNEL_TYPE_HELIX);
	pyutil_register_int_constant (dict, "CHANNEL_TYPE_DEBIAN",
				      RC_CHANNEL_TYPE_DEBIAN);
	pyutil_register_int_constant (dict, "CHANNEL_TYPE_REDHAT",
				      RC_CHANNEL_TYPE_REDHAT);
	pyutil_register_int_constant (dict, "CHANNEL_TYPE_UNKNOWN",
				      RC_CHANNEL_TYPE_UNKNOWN);

	pyutil_register_int_constant (dict, "CHANNEL_SYSTEM",
				      RC_CHANNEL_SYSTEM);
	pyutil_register_int_constant (dict, "CHANNEL_ANY",
				      RC_CHANNEL_ANY);
	pyutil_register_int_constant (dict, "CHANNEL_NON_SYSTEM",
				      RC_CHANNEL_NON_SYSTEM);
}

int
PyChannel_check (PyObject *obj)
{
	return PyObject_TypeCheck (obj, &PyChannel_type_info);
}

PyObject *
PyChannel_new (RCChannel *channel)
{
	PyObject *py_channel = PyChannel_tp_new (&PyChannel_type_info,
						 NULL, NULL);
	((PyChannel *) py_channel)->channel = rc_channel_ref (channel);

	return py_channel;
}

RCChannel *
PyChannel_get_channel (PyObject *obj)
{
	if (! PyChannel_check (obj))
		return NULL;

	return ((PyChannel *) obj)->channel;
}
