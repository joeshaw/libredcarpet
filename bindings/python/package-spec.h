/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * package-spec.h
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

#ifndef __PACKAGE_SPEC_H__
#define __PACKAGE_SPEC_H__

#include <Python.h>
#include <libredcarpet.h>

void       PyPackageSpec_register    (PyObject *dict);
int        PyPackageSpec_check       (PyObject *obj);
PyObject  *PyPackageSpec_new         (RCPackageSpec *spec);

RCPackageSpec *PyPackageSpec_get_package_spec (PyObject *obj);

#endif /* __PACKAGE_SPEC_H__ */
