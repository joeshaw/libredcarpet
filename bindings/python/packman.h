/* This is -*- C -*- */
/* vim: set sw=2: */
/* $Id$ */

/*
 * packman.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
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

#ifndef __PACKMAN_H__
#define __PACKMAN_H__

#include <Python.h>
#include <libredcarpet.h>

void       PyPackman_register    (PyObject *dict);

int        PyPackman_check       (PyObject *obj);

PyObject  *PyPackman_new         (RCPackman *packman);
RCPackman *PyPackman_get_packman (PyObject *obj);

#endif /* __PACKMAN_H__ */

