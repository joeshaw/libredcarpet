/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* rc-dpkg-helper.c
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

/*
 * Ugly little thing.
 */

#define NULL ((void *) 0)

extern void printf (const char *, ...);
extern int __libc_read (int ifd, void *vbf, long nb);
extern char *getenv (const char *);
extern int atoi (const char *);
extern void *dlopen (const char *, int);
extern void *dlsym (void *, const char *);
extern int kill (int pid, int sig);
extern int isatty (int fd);
extern int write (int, char *, int);

#define SIGUSR1 10              /* ick */
#define SIGUSR2 12              /* ick */

#define SIGNAL_NAME SIGUSR2

int read (int ifd, void *vbf, long nb)
{
    char *pidstr;
    static int pid = 0;

    if (ifd == 0 && pid != -1) { /* stdin */
        if (!pid) {
            pidstr = getenv ("RC_READ_NOTIFY_PID");
            if (pidstr) {
                pid = atoi(pidstr);
            } else {
                pid = -1;
            }
        }
        if (pid != -1 && isatty (ifd)) {
            kill (pid, SIGNAL_NAME);
        }
    }

    return __libc_read (ifd, vbf, nb);
}

int __read (int ifd, void *vbf, long nb)
{
    char *pidstr;
    static int pid = 0;

    if (ifd == 0 && pid != -1) { /* stdin */
        if (!pid) {
            pidstr = getenv ("RC_READ_NOTIFY_PID");
            if (pidstr) {
                pid = atoi(pidstr);
            } else {
                pid = -1;
            }
        }
        if (pid != -1 && isatty (ifd)) {
            kill (pid, SIGNAL_NAME);
        }
    }

    return __libc_read (ifd, vbf, nb);
}
