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

#ifndef __VERIFICATION_H__
#define __VERIFICATION_H__

#include <Python.h>
#include <libredcarpet.h>

void        PyVerification_register     (PyObject *dict);
int         PyVerification_check        (PyObject *obj);

PyObject   *PyVerification_new          (RCVerification *verification);
RCVerification *PyVerification_get_verification (PyObject *obj);

/* STEALS OWNERSHIP, DESTROYS slist!!!! */
PyObject *RCVerificationSList_to_py_list (RCVerificationSList *slist);


#endif /* __VERIFICATION_H__ */
