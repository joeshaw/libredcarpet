/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-file.h
 *
 * Copyright (C) 2003 The Free Software Foundation, Inc.
 *
 * Developed by James Willcox <james@ximian.com>
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

#ifndef __PACKAGE_FILE_H__
#define __PACKAGE_FILE_H__

#include <Python.h>
#include <libredcarpet.h>
#include <libxml/tree.h>

void       PyPackageFile_register    (PyObject *dict);
int        PyPackageFile_check       (PyObject *obj);
PyObject  *PyPackageFile_new         (RCPackageFile *file);

RCPackageFile *PyPackageFile_get_package_file (PyObject *obj);
PyObject      *rc_package_file_slist_to_PyList (RCPackageFileSList *list);

#endif /* __PACKAGE_FILE_H__ */
