/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * world-store.h
 *
 * Copyright (C) 2003 Ximian, Inc.
 *
 */

/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
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

#ifndef __WORLD_STORE_H__
#define __WORLD_STORE_H__

#include <Python.h>
#include <libredcarpet.h>

typedef struct _PyWorldStore PyWorldStore;

extern PyTypeObject PyWorldStore_type_info;

void          PyWorldStore_register    (PyObject *dict);

int           PyWorldStore_check       (PyObject *obj);

PyObject     *PyWorldStore_new         (RCWorldStore *store);
RCWorldStore *PyWorldStore_get_store   (PyObject *obj);

#endif /* __WORLD_STORE_H__ */
