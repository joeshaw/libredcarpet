/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * rc-distman.h: glue between packman and distro
 *
 * Copyright 2000 Helix Code, Inc.
 *
 * Author: Jacob Berkman <jacob@Helixcode.com>
 *
 */

#ifndef __RC_DISTMAN_H
#define __RC_DISTMAN_H

#include "rc-packman.h"
#include "distro.h"

RCPackman *rc_distman_new (void);

#endif
