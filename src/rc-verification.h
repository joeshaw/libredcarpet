/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-verification.h
 * Copyright (C) 2000-2002 Ximian, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _RC_VERIFICATION_H_
#define _RC_VERIFICATION_H_

#include <glib.h>

typedef enum {
    RC_VERIFICATION_TYPE_ALL  = ~0,
    RC_VERIFICATION_TYPE_SIZE = 1 << 1,
    RC_VERIFICATION_TYPE_MD5  = 1 << 2,
    RC_VERIFICATION_TYPE_GPG  = 1 << 3,
} RCVerificationType;

typedef enum {
    /* The file is known to be bad */
    RC_VERIFICATION_STATUS_FAIL,
    /* The status of the file cannot be determined */
    RC_VERIFICATION_STATUS_UNDEF,
    /* The file is verifiably ok */
    RC_VERIFICATION_STATUS_PASS,
} RCVerificationStatus;

typedef struct _RCVerification RCVerification;

struct _RCVerification {
    RCVerificationType type;
    RCVerificationStatus status;
    gchar *info;
};

typedef GSList RCVerificationSList;

RCVerification *rc_verification_new (void);

void rc_verification_free (RCVerification *verification);

void rc_verification_slist_free (RCVerificationSList *verification_list);

void rc_verification_set_keyring (const gchar *keyring);

#endif
