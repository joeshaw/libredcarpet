#ifndef RC_DISTRO_H_
#define RC_DISTRO_H_

#include <glib.h>

#include <time.h>

#include "rc-arch.h"

typedef enum {
    RC_DISTRO_PACKAGE_TYPE_UNKNOWN,
    RC_DISTRO_PACKAGE_TYPE_RPM,
    RC_DISTRO_PACKAGE_TYPE_DPKG,
} RCDistroPackageType;

typedef enum {
    RC_DISTRO_STATUS_UNSUPPORTED,
    RC_DISTRO_STATUS_PRESUPPORTED,
    RC_DISTRO_STATUS_SUPPORTED,
    RC_DISTRO_STATUS_DEPRECATED,
    RC_DISTRO_STATUS_RETIRED,
} RCDistroStatus;

gboolean
rc_distro_parse_xml (const char *xml_buf,
                     guint compressed_length);

const char *
rc_distro_get_name (void);

const char *
rc_distro_get_version (void);

RCArch
rc_distro_get_arch (void);

RCDistroPackageType
rc_distro_get_package_type (void);

const char *
rc_distro_get_target (void);

RCDistroStatus
rc_distro_get_status (void);

time_t
rc_distro_get_death_date (void);

#endif
