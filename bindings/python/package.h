/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package.h
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

#ifndef __PACKAGE_H__
#define __PACKAGE_H__

#include <Python.h>
#include <libredcarpet.h>

void       PyPackage_register    (PyObject *dict);

int        PyPackage_check       (PyObject *obj);

PyObject  *PyPackage_new         (RCPackage *package);
RCPackage *PyPackage_get_package (PyObject *obj);

RCPackageSList *PyList_to_rc_package_slist (PyObject *py_list);
PyObject       *rc_package_slist_to_PyList (RCPackageSList *slist);

#endif /* __PACKAGE_H__ */
