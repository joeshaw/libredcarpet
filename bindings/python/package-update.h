/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-update.h
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

#ifndef __PACKAGE_UPDATE_H__
#define __PACKAGE_UPDATE_H__

#include <Python.h>
#include <libredcarpet.h>

void       PyPackageUpdate_register    (PyObject *dict);
int        PyPackageUpdate_check       (PyObject *obj);
PyObject  *PyPackageUpdate_new         (RCPackageUpdate *update);

RCPackageUpdate *PyPackageUpdate_get_package_update (PyObject *obj);

#endif /* __PACKAGE_UPDATE_H__ */
