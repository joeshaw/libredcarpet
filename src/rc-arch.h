#ifndef RC_ARCH_H_
#define RC_ARCH_H_

#include <glib.h>

typedef enum {
    RC_ARCH_UNKNOWN = -1,
    RC_ARCH_NOARCH = 0,
    RC_ARCH_I386,
    RC_ARCH_I486,
    RC_ARCH_I586,
    RC_ARCH_I686,
    RC_ARCH_ATHLON,
    RC_ARCH_PPC,
    RC_ARCH_SPARC,
    RC_ARCH_SPARC64,
} RCArch;

RCArch
rc_arch_from_string (const char *arch_name);

const char *
rc_arch_to_string (RCArch arch);

RCArch
rc_arch_get_system_arch (void);

GSList *
rc_arch_get_compat_list (RCArch arch);

gint
rc_arch_get_compat_score (GSList *compat_arch_list, RCArch arch);

#endif
