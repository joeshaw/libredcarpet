/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-distro.c
 * Copyright (C) 2000, 2001 Ximian, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <stdio.h>

#include "rc-distro.h"

#ifdef RC_DISTRO_NO_GLIB
typedef int gboolean;
typedef int gint;
typedef void * gpointer;
typedef char gchar;

#define g_malloc malloc
#define g_free free
#define g_realloc realloc
#define g_warning(x...) fprintf(stderr, x)
#define g_error(x...) do { fprintf(stderr, x); exit(-1); } while (0)
#ifndef FALSE
#define FALSE 0
#define TRUE 1
#endif

#endif

typedef gboolean (*distro_check_function) (gpointer param1, gpointer param2, gpointer param3);

#define CHECK_OP_NONE	0
#define CHECK_OP_AND	1
#define CHECK_OP_NOT	2
#define CHECK_OP_OR	3

typedef struct _RCDistroChunk {
    char *unique_name;
    RCDistroArch arch;
    distro_check_function func1;
    gpointer param11;
    gpointer param12;
    gpointer param13;
    short op;
    distro_check_function func2;
    gpointer param21;
    gpointer param22;
    gpointer param23;
} RCDistroChunk;

/* Various comparison functions */
static gboolean func_string_in_file (gpointer arg1,  /* filename */
                                     gpointer arg2,  /* string */
                                     gpointer arg3); /* exact_match */
static gboolean func_nth_string_in_file (gpointer arg1,  /* filename */
                                         gpointer arg2,  /* string */
                                         gpointer arg3); /* n */
static gboolean func_sys (gpointer arg1,  /* prog to call, with args */
                          gpointer arg2,  /* string */
                          gpointer arg3); /* exact_match */


/*
 * Note: The unique name strings should be kept in sync with the build system
 */

RCDistroChunk distro_figurers[] = {
    /* scyld needs to come before redhat, since they include a redhat-release */
    { "scyld-20-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/scyld-release", "Scyld Beowulf release 2", NULL, 0 },

    { "linuxppc-2000-ppc", RC_ARCH_PPC,
      func_string_in_file, "/etc/redhat-release", "LinuxPPC 2000", NULL, 0 },
    { "linuxppc-2000q4-ppc", RC_ARCH_PPC,
      func_string_in_file, "/etc/redhat-release", "Linux/PPC 2000 Q4", NULL, 0 },

    { "yellowdog-12-ppc", RC_ARCH_PPC,
      func_string_in_file, "/etc/yellowdog-release", "Champion Server release 1.2",
      NULL, 0 },
    { "yellowdog-20-ppc", RC_ARCH_PPC,
      func_string_in_file, "/etc/yellowdog-release", "Yellow Dog Linux release 2.0 (Pomona)",
      NULL, 0 },

    { "mandrake-70-i586", RC_ARCH_IA32,
      func_string_in_file, "/etc/mandrake-release", "7.0", NULL, 0 },
    { "mandrake-71-i586", RC_ARCH_IA32,
      func_string_in_file, "/etc/mandrake-release", "7.1", NULL, 0 },
    { "mandrake-72-i586", RC_ARCH_IA32,
      func_string_in_file, "/etc/mandrake-release", "7.2", NULL, 0 },
    { "mandrake-80-i586", RC_ARCH_IA32,
      func_string_in_file, "/etc/mandrake-release", "8.0", NULL, 0 },

    { "redhat-60-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/redhat-release", "6.0", NULL, 0 },
    { "redhat-61-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/redhat-release", "6.1", NULL, 0 },
    { "redhat-62-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/redhat-release", "6.2", NULL, 0 },
    { "redhat-70-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/redhat-release", "7.0", NULL, 0 },
    { "redhat-71-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/redhat-release", "7.1", NULL, 0 },

    { "turbolinux-60-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/turbolinux-release", "6.0", NULL, 0 },

    { "suse-63-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/SuSE-release", "6.3", NULL, 0 },
    { "suse-64-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/SuSE-release", "6.4", NULL, 0 },
    { "suse-70-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/SuSE-release", "7.0", NULL, 0 },
    { "suse-71-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/SuSE-release", "7.1", NULL, 0 },
    { "suse-72-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/SuSE-release", "7.2", NULL, 0 },

    { "debian-potato-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/debian_version", "2.2", NULL, 0},
    { "debian-woody-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/debian_version", "woody", NULL, 0},
    { "debian-sid-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/debian_version", "sid", NULL, 0},
    { "debian-woody-i386", RC_ARCH_IA32,
      func_string_in_file, "/etc/debian_version", "testing", NULL, 0},

    { "solaris-7-sun4", RC_ARCH_SPARC,
      func_sys, "uname -s", "SunOS", (gpointer) 0, CHECK_OP_AND,
      func_sys, "uname -r", "5.7", (gpointer) 0 },

    { "solaris-8-sun4", RC_ARCH_SPARC,
      func_sys, "uname -s", "SunOS", (gpointer) 0, CHECK_OP_AND,
      func_sys, "uname -r", "5.8", (gpointer) 0 },

    { NULL }
};

RCDistroType distro_types[] = {
    { "redhat-60-i386", NULL, "Red Hat Linux", "6.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "redhat-61-i386", NULL, "Red Hat Linux", "6.1", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "redhat-62-i386", NULL, "Red Hat Linux", "6.2", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "redhat-70-i386", NULL, "Red Hat Linux", "7.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "redhat-71-i386", NULL, "Red Hat Linux", "7.1", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "scyld-20-i386", NULL, "Scyld Beowulf", "2.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "turbolinux-60-i386", NULL, "TurboLinux", "6.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "suse-63-i386", NULL, "SuSE", "6.3", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=3, non-gdm-runlevel=2" },
    { "suse-64-i386", NULL, "SuSE", "6.4", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=3, non-gdm-runlevel=2" },
    { "suse-70-i386", NULL, "SuSE", "7.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=3, non-gdm-runlevel=2" },
    { "suse-71-i386", NULL, "SuSE", "7.1", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "suse-72-i386", NULL, "SuSE", "7.2", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "mandrake-70-i586", NULL, "Linux Mandrake", "7.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "mandrake-71-i586", NULL, "Linux Mandrake", "7.1", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "mandrake-72-i586", NULL, "Linux Mandrake", "7.2", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "mandrake-80-i586", NULL, "Linux Mandrake", "8.0", RC_PKG_RPM, RC_ARCH_IA32, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "debian-sid-i386", NULL, "Debian GNU/Linux", "sid", RC_PKG_DPKG, RC_ARCH_IA32, NULL },
    { "debian-woody-i386", NULL, "Debian GNU/Linux", "woody", RC_PKG_DPKG, RC_ARCH_IA32, NULL },
    { "debian-potato-i386", NULL, "Debian GNU/Linux", "potato", RC_PKG_DPKG, RC_ARCH_IA32, NULL },

    { "linuxppc-2000-ppc", NULL, "LinuxPPC", "2000", RC_PKG_RPM, RC_ARCH_PPC, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "linuxppc-2000q4-ppc", NULL, "Linux/PPC", "2000 Q4", RC_PKG_RPM, RC_ARCH_PPC, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "yellowdog-12-ppc", NULL, "Yellow Dog Linux", "1.2", RC_PKG_RPM, RC_ARCH_PPC, "gdm-runlevel=5, non-gdm-runlevel=3" },
    { "yellowdog-20-ppc", NULL, "Yellow Dog Linux", "2.0", RC_PKG_RPM, RC_ARCH_PPC, "gdm-runlevel=5, non-gdm-runlevel=3" },

    { "solaris-7-sun4", NULL, "Sun Solaris 7", "7", RC_PKG_RPM, RC_ARCH_SPARC, "" },
    { "solaris-8-sun4", NULL, "Sun Solaris 8", "8", RC_PKG_RPM, RC_ARCH_SPARC, "" },

    { NULL }
};

static gint
suck_file (gchar *filename, gchar **out_buf)
{
    int fd;
    gchar *buf;
    gint buf_sz;
    struct stat st;

    if (stat (filename, &st)) {
        return -1;
    }

    buf_sz = st.st_size;

    if ((fd = open (filename, O_RDONLY)) < 0) {
        return -1;
    }

    buf = g_malloc (buf_sz + 1);

    if (read (fd, buf, buf_sz) != buf_sz) {
        perror ("read");
        g_warning ("read didn't complete on %s", filename);
        g_free (buf);
        close (fd);
        return -1;
    }
    buf[buf_sz] = 0;

    close (fd);

    *out_buf = buf;
    return buf_sz;
}

static gint
call_system (gchar *cmdline, gchar **out_buf)
{
    gchar *result = NULL;
    int res_size;
    FILE *fp;
    int ret;

    fp = popen (cmdline, "r");
    if (fp == NULL)
        return -1;

    result = g_malloc(1024);
    res_size = 1024;
    ret = fread (result, 1, 1023, fp);
    result[ret] = '\0';
    while (ret == 1023) {
        gchar more[1024];
        ret = fread (more, 1, 1024, fp);
        g_realloc (result, res_size + ret);
        strcpy (result + res_size - 1, more);
        res_size += ret; /* We have the extra byte already from above */
    }

    pclose (fp);
    *out_buf = result;

    return res_size;
}

static gboolean
check_string_in_data (gint (*data_func) (gchar *, gchar **),
                      gchar *data_arg,
                      gchar *str,
                      gboolean exact)
{
    gchar *data_out;
    gboolean ret;

    if (data_func (data_arg, &data_out) < 0)
        return FALSE;

    if (exact) {
        if (strcmp (data_out, str) == 0)
            ret = TRUE;
        else
            ret = FALSE;
    } else {
        if (strstr (data_out, str) != NULL)
            ret = TRUE;
        else
            ret = FALSE;
    }

    g_free (data_out);
    return ret;
}

static gboolean
func_string_in_file (gpointer arg1, gpointer arg2, gpointer arg3)
{
    gchar *fn = (gchar *) arg1;
    gchar *str = (gchar *) arg2;
    gboolean exact = (arg3 != NULL);

    return check_string_in_data (suck_file, fn, str, exact);
}

static gboolean
func_nth_string_in_file (gpointer arg1, gpointer arg2, gpointer arg3)
{
/*
    gchar *fn = (gchar *) arg1;
    gchar *str = (gchar *) arg2;
    gint n = (gint) arg3;
*/
    g_error ("Implement nth_string_in_file in rc-distro.c!");
    return FALSE;
}


static gboolean
func_sys (gpointer arg1, gpointer arg2, gpointer arg3)
{
    gchar *cmd = (gchar *) arg1;
    gchar *str = (gchar *) arg2;
    gboolean exact = (arg3 != NULL);

    return check_string_in_data (call_system, cmd, str, exact);
}

/* These are here to shut gcc up about unused functions */
void *foo_unused__func_nth_string_in_file = func_nth_string_in_file;
void *foo_unused__func_sys = func_sys;

/* We try to determine the architecture in many sneaky ways here */
static RCDistroArch
determine_arch ()
{
    gint bufsz;
    gchar *buf;
    RCDistroArch ret = 0;

    /* First try arch */
    bufsz = call_system ("arch", &buf);
    /* Try uname -m if this fails */
    if (bufsz < 0)
        bufsz = call_system ("uname -m", &buf);

    if (bufsz > 0) {
        if (!strncmp (buf, "i386", 4) ||
            !strncmp (buf, "i486", 4) ||
            !strncmp (buf, "i586", 4) ||
            !strncmp (buf, "i686", 4))
        {
            ret = RC_ARCH_IA32;
        }

        if (!strncmp (buf, "sun4", 4) ||
            !strncmp (buf, "sparc", 5))
        {
            ret = RC_ARCH_SPARC;
        }

        if (!strncmp (buf, "alpha", 5))
        {
            ret = RC_ARCH_ALPHA;
        }

        if (!strncmp (buf, "ppc", 3))
        {
            ret = RC_ARCH_PPC;
        }

        if (!strncmp (buf, "armv4l", 6) ||
            !strncmp (buf, "arm", 3))
        {
            ret = RC_ARCH_ARM;
        }

        /* Add more! */

        g_free (buf);
        if (ret) return ret;
    }

    return RC_ARCH_UNKNOWN;
}

const char *
rc_distro_option_lookup(RCDistroType *distro, const char *key)
{
#ifndef RC_DISTRO_NO_GLIB
    return g_hash_table_lookup(distro->extra_hash, key);
#else
    return NULL;
#endif
} /* rc_distro_option_lookup */

RCDistroType *
rc_figure_distro (void)
{
    int i = 0;
    RCDistroArch arch;
    gchar *distro_name = NULL;
    char **options;

    static RCDistroType *dtype = NULL;

    if (!dtype) {
        if (!getenv ("RC_DISTRO_NAME")) {
            /* Go through each chunk and figure out if it matches */
            arch = determine_arch ();
            if (arch == RC_ARCH_UNKNOWN) {
                g_warning("Unable to figure out what architecture you're on");
                return NULL;
            }
            
            while (distro_figurers[i].unique_name) {
                gboolean result;
                if (!distro_figurers[i].func1) {
                    g_error ("Malformed distro discovery array");
                }

                /* Check if this is the right architecture */
                if (distro_figurers[i].arch != arch) {
                    i++;
                    continue;
                }
                
                result = (*distro_figurers[i].func1) (distro_figurers[i].param11,
                                                      distro_figurers[i].param12,
                                                      distro_figurers[i].param13);
            
                if (distro_figurers[i].op) {
                    gboolean res2;
                    res2 = (*distro_figurers[i].func2) (distro_figurers[i].param21,
                                                        distro_figurers[i].param22,
                                                        distro_figurers[i].param23);
                    switch (distro_figurers[i].op) {
                        case CHECK_OP_AND:
                            result = result && res2;
                            break;
                        case CHECK_OP_OR:
                            result = result || res2;
                            break;
                        case CHECK_OP_NOT:
                            result = result && !res2;
                            break;
                        default:
                            g_warning ("Unkown check_op!\n");
                            break;
                    }
                }
                
                if (result) {
                    /* We'll take this one! */
                    distro_name = distro_figurers[i].unique_name;
                    break;
                }
                i++;
            }
        } else {
            distro_name = getenv ("RC_DISTRO_NAME");
        }

        if (distro_name) {
            /* It's a real one! */
            int j = 0;
            while (distro_types[j].unique_name &&
                   strcmp (distro_name, distro_types[j].unique_name) != 0)
                j++;
            
            if (distro_types[j].unique_name) {
                dtype = &distro_types[j];
            }
        }
    }

    if (dtype) {
#ifndef RC_DISTRO_NO_GLIB
        dtype->extra_hash = g_hash_table_new(g_str_hash, g_str_equal);
        if (dtype->extra_stuff) {
            options = g_strsplit(dtype->extra_stuff, ",", 0);
            for (i = 0; options[i]; i++) {
                char **parseit;
            
                parseit = g_strsplit(options[i], "=", 1);
                if (!parseit[0] || !parseit[1]) {
                    g_warning("Invalid distribution option: %s", options[i]);
                    continue;
                }
                g_hash_table_insert(
                    dtype->extra_hash, g_strstrip(parseit[0]), 
                    g_strstrip(parseit[1]));
            }
            g_strfreev(options);
        }
#else
        dtype->extra_hash = NULL;
#endif
    }

    return dtype;
}
