/* rc-verification.h
 * Copyright (C) 2000 Helix Code, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifndef _RC_VERIFICATION_H_
#define _RC_VERIFICATION_H_

#include <glib.h>

typedef enum _RCVerificationType RCVerificationType;

enum _RCVerificationType {
    RC_VERIFICATION_TYPE_SIZE,
    RC_VERIFICATION_TYPE_MD5,
    RC_VERIFICATION_TYPE_GPG,
};

typedef enum _RCVerificationStatus RCVerificationStatus;

enum _RCVerificationStatus {
    /* The file is known to be bad */
    RC_VERIFICATION_STATUS_FAIL,
    /* The status of the file cannot be determined */
    RC_VERIFICATION_STATUS_UNDEF,
    /* The file is verifiably ok */
    RC_VERIFICATION_STATUS_PASS,
};

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

#endif
