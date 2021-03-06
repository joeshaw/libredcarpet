/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-importance.h
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

#ifndef __PACKAGE_IMPORTANCE_H__
#define __PACKAGE_IMPORTANCE_H__

#include <Python.h>
#include <libredcarpet.h>

void      PyPackageImportance_register (PyObject *dict);
int       PyPackageImportance_check    (PyObject *obj);
PyObject *PyPackageImportance_new      (RCPackageImportance importance);

RCPackageImportance PyPackageImportance_get_package_importance (PyObject *obj);


#endif /* __PACKAGE_IMPORTANCE_H__ */
