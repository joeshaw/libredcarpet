/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */

#include "rc-packman-private.h"

#include "rc-rpmman.h"

#include <fcntl.h>
#include <string.h>
#include <rpm/rpmurl.h>
#include <rpm/rpmmacro.h>
#include <rpm/misc.h>
#include <rpm/header.h>

static void rc_rpmman_class_init (RCRpmmanClass *klass);
static void rc_rpmman_init       (RCRpmman *obj);

int rc_rpmInstall(const char * rootdir, const char ** fileArgv, int transFlags, 
                  int interfaceFlags, int probFilter, 
                  rpmRelocation * relocations);

static gchar *rpmroot;

guint
rc_rpmman_get_type (void)
{
    static guint type = 0;

    if (!type) {
        GtkTypeInfo type_info = {
            "RCRpmman",
            sizeof (RCRpmman),
            sizeof (RCRpmmanClass),
            (GtkClassInitFunc) rc_rpmman_class_init,
            (GtkObjectInitFunc) rc_rpmman_init,
            (GtkArgSetFunc) NULL,
            (GtkArgGetFunc) NULL,
        };

        type = gtk_type_unique (rc_packman_get_type (), &type_info);
    }

    return type;
} /* rc_rpmman_get_type */

/* Ugly ugly ugly, need this global to point to the rpmman object so that the
   rpmCallback has access to the object to emit signals.  Anyone want to fix
   this? */

static RCPackman *ghp;

/* an rpmlib callback, gutted of the hash mark printing feature, and replaced
   with code to emit pkg_installed signals for RCPackman */

static void * showProgress(const Header h, const rpmCallbackType what,
                           const unsigned long amount,
                           const unsigned long total,
                           const void * pkgKey, void * data) {
    const char * filename = pkgKey;
    static FD_t fd;

    switch (what) {
    case RPMCALLBACK_INST_OPEN_FILE:
        fd = fdOpen(filename, O_RDONLY, 0);
        return fd;

    case RPMCALLBACK_INST_CLOSE_FILE:
        fdClose(fd);
        break;

    case RPMCALLBACK_INST_PROGRESS:
        /* The exact behavior here is debatable. I have changed this case so
           that a pkg_progress signal is emitted whenever there is a change.
           This includes when the package as finished. The progress signal
           is always emitted first. Then, if the package has finished
           installing, a pkg_installed signal is also emitted secondly. */
        rc_packman_package_progress (ghp, pkgKey, amount, total);
        if (amount == total) {
            RC_RPMMAN (ghp)->package_count++;
            rc_packman_package_installed(
                ghp, pkgKey, RC_RPMMAN (ghp)->package_count,
                RC_RPMMAN (ghp)->package_total);
        }
        break;

    case RPMCALLBACK_TRANS_PROGRESS:
        rc_packman_config_progress (ghp, amount, total);
        break;

    case RPMCALLBACK_INST_START:
    case RPMCALLBACK_UNINST_START:
    case RPMCALLBACK_UNINST_STOP:
    case RPMCALLBACK_UNINST_PROGRESS:
    case RPMCALLBACK_TRANS_START:
    case RPMCALLBACK_TRANS_STOP:
        /* ignore */
        break;
    }

    return NULL;
} /* showProgress */

/* This can probably go at some point(?) */

#ifndef _
#define _
#endif

/* straight copy of rpmInstall from rpmlib 3.0.4, we need this so that we can
   redefine the callback used by rpmRunTransactions to emit packman signals.
   It's ugly, I know. */

int rc_rpmInstall(const char * rootdir, const char ** fileArgv, int transFlags, 
               int interfaceFlags, int probFilter, 
               rpmRelocation * relocations)
{
    rpmdb db = NULL;
    FD_t fd;
    int i;
    int mode, rc, major;
    const char ** pkgURL = NULL;
    const char ** tmppkgURL = NULL;
    const char ** fileURL;
    int numPkgs;
    int numTmpPkgs = 0, numRPMS = 0, numSRPMS = 0;
    int numFailed = 0;
    Header h;
    int isSource;
    rpmTransactionSet rpmdep = NULL;
    int numConflicts;
    int stopInstall = 0;
    int notifyFlags = interfaceFlags | (rpmIsVerbose() ? INSTALL_LABEL : 0 );
    int dbIsOpen = 0;
    const char ** sourceURL;
    rpmRelocation * defaultReloc;

    if (transFlags & RPMTRANS_FLAG_TEST) 
	mode = O_RDONLY;
    else
	mode = O_RDWR | O_CREAT;

    for (defaultReloc = relocations; defaultReloc && defaultReloc->oldPath;
	 defaultReloc++);
    if (defaultReloc && !defaultReloc->newPath) defaultReloc = NULL;

    rpmMessage(RPMMESS_DEBUG, _("counting packages to install\n"));
    for (fileURL = fileArgv, numPkgs = 0; *fileURL; fileURL++, numPkgs++)
	;

    rpmMessage(RPMMESS_DEBUG, _("found %d packages\n"), numPkgs);

    pkgURL = calloc( (numPkgs + 1), sizeof(*pkgURL) );
    tmppkgURL = calloc( (numPkgs + 1), sizeof(*tmppkgURL) );

    rpmMessage(RPMMESS_DEBUG, _("looking for packages to download\n"));
    for (fileURL = fileArgv, i = 0; *fileURL; fileURL++) {

	switch (urlIsURL(*fileURL)) {
	case URL_IS_FTP:
	case URL_IS_HTTP:
	{   int myrc;
        int j;
        const char *tfn;
        const char ** argv;
        int argc;

        myrc = rpmGlob(*fileURL, &argc, &argv);
        if (myrc) {
            rpmMessage(RPMMESS_ERROR, 
                       _("skipping %s - rpmGlob failed(%d)\n"),
                       *fileURL, myrc);
            numFailed++;
            pkgURL[i] = NULL;
            break;
        }
        if (argc > 1) {
            numPkgs += argc - 1;
            pkgURL = realloc(pkgURL, (numPkgs + 1) * sizeof(*pkgURL));
            tmppkgURL = realloc(tmppkgURL, (numPkgs + 1) * sizeof(*tmppkgURL));
        }

        for (j = 0; j < argc; j++) {

            if (rpmIsVerbose())
                fprintf(stdout, _("Retrieving %s\n"), argv[j]);

            {   char tfnbuf[64];
            strcpy(tfnbuf, "rpm-xfer.XXXXXX");
            /*@-unrecog@*/ mktemp(tfnbuf) /*@=unrecog@*/;
            tfn = rpmGenPath(rootdir, "%{_tmppath}/", tfnbuf);
            }

            /* XXX undefined %{name}/%{version}/%{release} here */
            /* XXX %{_tmpdir} does not exist */
            rpmMessage(RPMMESS_DEBUG, _(" ... as %s\n"), tfn);
            myrc = urlGetFile(argv[j], tfn);
            if (myrc < 0) {
                rpmMessage(RPMMESS_ERROR, 
                           _("skipping %s - transfer failed - %s\n"), 
                           argv[j], ftpStrerror(myrc));
                numFailed++;
                pkgURL[i] = NULL;
                free((char *)tfn);
            } else {
                tmppkgURL[numTmpPkgs++] = pkgURL[i++] = tfn;
            }
        }
        if (argv) {
            for (j = 0; j < argc; j++)
                free((char *)argv[j]);
            free(argv);
            argv = NULL;
        }
	}   break;
	case URL_IS_PATH:
	default:
	    pkgURL[i++] = *fileURL;
	    break;
	}
    }
    pkgURL[i] = NULL;
    tmppkgURL[numTmpPkgs] = NULL;

    sourceURL = alloca(sizeof(*sourceURL) * i);

    rpmMessage(RPMMESS_DEBUG, _("retrieved %d packages\n"), numTmpPkgs);

    /* Build up the transaction set. As a special case, v1 source packages
       are installed right here, only because they don't have headers and
       would create all sorts of confusion later. */

    for (fileURL = pkgURL; *fileURL; fileURL++) {
	const char * fileName;
	(void) urlPath(*fileURL, &fileName);
	fd = Fopen(*fileURL, "r.ufdio");
	if (fd == NULL || Ferror(fd)) {
	    rpmMessage(RPMMESS_ERROR, _("cannot open file %s: %s\n"), *fileURL,
                       Fstrerror(fd));
	    if (fd) Fclose(fd);
	    numFailed++;
	    pkgURL[i] = NULL;
	    continue;
	}

	rc = rpmReadPackageHeader(fd, &h, &isSource, &major, NULL);

	switch (rc) {
	case 1:
	    Fclose(fd);
	    rpmMessage(RPMMESS_ERROR, 
                       _("%s does not appear to be a RPM package\n"), 
                       *fileURL);
	    break;
	default:
	    rpmMessage(RPMMESS_ERROR, _("%s cannot be installed\n"), *fileURL);
	    numFailed++;
	    pkgURL[i] = NULL;
	    break;
	case 0:
	    if (isSource) {
		sourceURL[numSRPMS++] = fileName;
		Fclose(fd);
	    } else {
		if (!dbIsOpen) {
		    if (rpmdbOpen(rootdir, &db, mode, 0644)) {
			const char *dn;
			dn = rpmGetPath( (rootdir ? rootdir : ""), 
                                         "%{_dbpath}", NULL);
			rpmMessage(RPMMESS_ERROR, 
                                   _("cannot open %s/packages.rpm\n"), dn);
			free((char *)dn);
			exit(EXIT_FAILURE);
		    }
		    rpmdep = rpmtransCreateSet(db, rootdir);
		    dbIsOpen = 1;
		}

		if (defaultReloc) {
		    const char ** paths;
		    int c;

		    if (headerGetEntry(h, RPMTAG_PREFIXES, NULL,
				       (void **) &paths, &c) && (c == 1)) {
			defaultReloc->oldPath = strdup(paths[0]);
			free(paths);
		    } else {
			const char * name;
			headerNVR(h, &name, NULL, NULL);
			rpmMessage(RPMMESS_ERROR, 
                                   _("package %s is not relocateable\n"), name);

			goto errxit;
			/*@notreached@*/
		    }
		}

		rc = rpmtransAddPackage(rpmdep, h, NULL, fileName,
                                        (interfaceFlags & INSTALL_UPGRADE) != 0,
                                        relocations);

		headerFree(h);	/* XXX reference held by transaction set */
		Fclose(fd);

		switch(rc) {
		case 0:
                    break;
		case 1:
                    rpmMessage(RPMMESS_ERROR, 
                               _("error reading from file %s\n"), *fileURL);
                    goto errxit;
                    /*@notreached@*/ break;
		case 2:
                    rpmMessage(RPMMESS_ERROR, 
                               _("file %s requires a newer version of RPM\n"),
                               *fileURL);
                    goto errxit;
                    /*@notreached@*/ break;
		}

		if (defaultReloc) {
		    free((char *)(defaultReloc->oldPath));
		    defaultReloc->oldPath = NULL;
		}

		numRPMS++;
	    }
	    break;
	}
    }

    rpmMessage(RPMMESS_DEBUG, _("found %d source and %d binary packages\n"), 
               numSRPMS, numRPMS);

    if (numRPMS && !(interfaceFlags & INSTALL_NODEPS)) {
	struct rpmDependencyConflict * conflicts;
	if (rpmdepCheck(rpmdep, &conflicts, &numConflicts)) {
	    numFailed = numPkgs;
	    stopInstall = 1;
	}

	if (!stopInstall && conflicts) {
	    rpmMessage(RPMMESS_ERROR, _("failed dependencies:\n"));
	    printDepProblems(stderr, conflicts, numConflicts);
	    rpmdepFreeConflicts(conflicts, numConflicts);
	    numFailed = numPkgs;
	    stopInstall = 1;
	}
    }

    if (numRPMS && !(interfaceFlags & INSTALL_NOORDER)) {
	if (rpmdepOrder(rpmdep)) {
	    numFailed = numPkgs;
	    stopInstall = 1;
	}
    }

    if (numRPMS && !stopInstall) {
	rpmProblemSet probs = NULL;
	rpmMessage(RPMMESS_DEBUG, _("installing binary packages\n"));
	rc = rpmRunTransactions(rpmdep, showProgress, (void *) notifyFlags, 
                                NULL, &probs, transFlags, probFilter);

	if (rc < 0) {
	    numFailed += numRPMS;
	} else if (rc > 0) {
	    numFailed += rc;
	    rpmProblemSetPrint(stderr, probs);
	}

	if (probs) rpmProblemSetFree(probs);
    }

    if (numRPMS) rpmtransFree(rpmdep);

    if (numSRPMS && !stopInstall) {
	for (i = 0; i < numSRPMS; i++) {
	    fd = Fopen(sourceURL[i], "r.ufdio");
	    if (fd == NULL || Ferror(fd)) {
		rpmMessage(RPMMESS_ERROR, _("cannot open file %s: %s\n"), 
			   sourceURL[i], Fstrerror(fd));
		if (fd) Fclose(fd);
		continue;
	    }

	    if (!(transFlags & RPMTRANS_FLAG_TEST))
		numFailed += rpmInstallSourcePackage(rootdir, fd, NULL,
                                                     showProgress,
                                                     (void *) notifyFlags,
                                                     NULL);

	    Fclose(fd);
	}
    }

    for (i = 0; i < numTmpPkgs; i++) {
	Unlink(tmppkgURL[i]);
	free((char *)tmppkgURL[i]);
    }
    free(tmppkgURL);
    free(pkgURL);

    /* FIXME how do we close our various fd's? */

    if (dbIsOpen) rpmdbClose(db);

    return numFailed;

errxit:
    if (tmppkgURL) {
	for (i = 0; i < numTmpPkgs; i++)
	    free((char *)tmppkgURL[i]);
	free(tmppkgURL);
    }
    if (pkgURL)
	free(pkgURL);
    if (dbIsOpen) rpmdbClose(db);
    return numPkgs;
} /* rpmInstall */

/* Gets a GSList of RCPackmanPackage, converts to a NULL terminated array of
   filenames, and passes to rpmInstall.  Emits the install_done signal at
   the end. */

static void
rc_rpmman_install (RCPackman *p, GSList *pkgs)
{
    GSList *iter = pkgs;
    guint length;
    gchar **pkgv;
    int i;
    int ret;

    length = g_slist_length (pkgs);

    RC_RPMMAN (p)->package_total = length;

    pkgv = g_new0 (char *, length + 1);

    for (i = 0; i < length; i++) {
        gchar *ptr = (gchar *)(iter->data);

        /* Gotta give a filename */
        g_assert (ptr);

        pkgv[i] = g_strdup (ptr);

        iter = iter->next;
    }

    /* Set the gross hack global pointer to the current object for callback
       purposes */

    ghp = p;

    /* Zero the package count */

    RC_RPMMAN (p)->package_count = 0;

    ret = rc_rpmInstall (rpmroot, (const char **)pkgv, 0,
                      INSTALL_NOORDER | INSTALL_UPGRADE | INSTALL_NODEPS, 0,
                      NULL);

    /* Although in this case, it should be safe to argue that no elements of
       pkgv are NULL (except for the terminating NULL), I'm trying to move away
       from g_strfreev in general, so I'll do it by hand here */

    for (i = 0; i < length; i++) {
        g_free (pkgv[i]);
    }

    g_free (pkgv);

    /* I have no idea what return codes are possible, so I hope I'm
       interpreting how rpmlib works correctly.  Given my track record,
       probably not, but... */

    if (ret) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "RPM installation failed.");
        rc_packman_install_done (p);
    } else {
        rc_packman_install_done (p);
    }
} /* rc_rpmman_install */

/* Given a list of packages to remove, with names and optionally versions and
   releases, remove them from the system.  The epoch isn't taken into
   consideration, because rpmlib can't, it seems.  Doesn't matter, because
   vlad tells me you can't have two packages installed which differ only in
   epoch.  So oh well. */

static void
rc_rpmman_remove (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter = pkgs;
    guint length;
    gchar **pkgv;
    int i;
    int ret;

    length = g_slist_length (pkgs);

    pkgv = g_new0 (gchar *, length + 1);

    for (i = 0; i < length; i++) {
        RCPackage *ptr = (RCPackage *)(iter->data);

        /* Must at least name the package */
        g_assert (ptr->spec.name);

        /* If you've got a release, you've gotta have a version */
        g_assert (!ptr->spec.release || ptr->spec.version);

        /* <name>-<version>-<release>, or <name>-<version>, or <name> */
        if (ptr->spec.version && ptr->spec.release) {
            pkgv[i] = g_strconcat (ptr->spec.name, "-", ptr->spec.version, "-",
                                   ptr->spec.release, NULL);
        } else if (ptr->spec.version) {
            pkgv[i] = g_strconcat (ptr->spec.name, "-", ptr->spec.version,
                                   NULL);
        } else {
            pkgv[i] = g_strdup (ptr->spec.name);
        }

        iter = iter->next;
    }

    ret = rpmErase (rpmroot, (const char **)pkgv, 0, 0);

    /* Once again, duck around the g_strfreev for anality */

    for (i = 0; i < length; i++) {
        g_free (pkgv[i]);
    }

    g_free (pkgv);

    /* I have no idea what error codes are possible... */

    if (ret) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "RPM removal failed");
        rc_packman_remove_done (p);
    } else {
        rc_packman_remove_done (p);
    }
} /* rc_rpmman_remove */

/* Helper function to read values out of an rpm header safely.  If any of the
   paramaters are NULL, they're ignored.  Remember, you don't need to free any
   of the elements, since they are just pointers into the rpm header structure.
   Just remember to free the header later! */

static void
rc_rpmman_read_header (Header hdr, gchar **name, guint32 *epoch,
                       gchar **version, gchar **release,
                       RCPackageSection *section, guint32 *size,
                       gchar **summary, gchar **description)
{
    int_32 type, count;

    if (name) {
        gchar *tmpname;

        g_free (*name);
        *name = NULL;

        headerGetEntry (hdr, RPMTAG_NAME, &type, (void **)&tmpname, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpname && tmpname[0]) {
            *name = g_strdup (tmpname);
        }
    }

    if (epoch) {
        guint32 *tmpepoch;

        *epoch = 0;

        headerGetEntry (hdr, RPMTAG_EPOCH, &type, (void **)&tmpepoch, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *epoch = *tmpepoch;
        }
    }

    if (version) {
        gchar *tmpver;

        g_free (*version);
        *version = NULL;

        headerGetEntry (hdr, RPMTAG_VERSION, &type, (void **)&tmpver, &count);

        if (count && (type == RPM_STRING_TYPE) && tmpver && tmpver[0]) {
            *version = g_strdup (tmpver);
        }
    }

    if (release) {
        gchar *tmprel;

        g_free (*release);
        *release = NULL;

        headerGetEntry (hdr, RPMTAG_RELEASE, &type, (void **)&tmprel, &count);

        if (count && (type == RPM_STRING_TYPE) && tmprel && tmprel[0]) {
            *release = g_strdup (tmprel);
        }
    }

    if (section) {
        gchar *tmpsection;

        *section = SECTION_MISC;

        headerGetEntry (hdr, RPMTAG_GROUP, &type, (void **)&tmpsection,
                        &count);

        if (count && (type == RPM_STRING_TYPE) && tmpsection &&
            tmpsection[0]) {
            if (!g_strcasecmp (tmpsection, "Amusements/Games")) {
                *section = SECTION_GAME;
            } else if (!g_strcasecmp (tmpsection, "Amusements/Graphics")) {
                *section = SECTION_IMAGING;
            } else if (!g_strcasecmp (tmpsection, "Applications/Archiving")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection,
                                      "Applications/Communications")) {
                *section = SECTION_INTERNET;
            } else if (!g_strcasecmp (tmpsection, "Applications/Databases")) {
                *section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (tmpsection, "Applications/Editors")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection, "Applications/Emulators")) {
                *section = SECTION_GAME;
            } else if (!g_strcasecmp (tmpsection,
                                      "Applications/Engineering")) {
                *section = SECTION_MISC;
            } else if (!g_strcasecmp (tmpsection, "Applications/File")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection, "Applications/Internet")) {
                *section = SECTION_INTERNET;
            } else if (!g_strcasecmp (tmpsection, "Applications/Multimedia")) {
                *section = SECTION_MULTIMEDIA;
            } else if (!g_strcasecmp (tmpsection,
                                      "Applications/Productivity")) {
                *section = SECTION_PIM;
            } else if (!g_strcasecmp (tmpsection, "Applications/Publishing")) {
                *section = SECTION_MISC;
            } else if (!g_strcasecmp (tmpsection, "Applications/System")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection, "Applications/Text")) {
                *section = SECTION_UTIL;
            } else if (!g_strcasecmp (tmpsection, "Development/Debuggers")) {
                *section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (tmpsection, "Development/Languages")) {
                *section = SECTION_DEVEL;
            } else if (!g_strcasecmp (tmpsection, "Development/Libraries")) {
                *section = SECTION_LIBRARY;
            } else if (!g_strcasecmp (tmpsection, "Development/System")) {
                *section = SECTION_DEVEL;
            } else if (!g_strcasecmp (tmpsection, "Development/Tools")) {
                *section = SECTION_DEVELUTIL;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Base")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Daemons")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Kernel")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Libraries")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "System Environment/Shells")) {
                *section = SECTION_SYSTEM;
            } else if (!g_strcasecmp (tmpsection,
                                      "User Interface/Desktops")) {
                *section = SECTION_XAPP;
            } else if (!g_strcasecmp (tmpsection,
                                      "User Interface/X")) {
                *section = SECTION_XAPP;
            } else if (!g_strcasecmp (tmpsection,
                                      "User Interface/X Hardware Support")) {
                *section = SECTION_XAPP;
            } else {
                *section = SECTION_MISC;
            }
        } else {
            *section = SECTION_MISC;
        }
    }

    if (size) {
        guint32 *tmpsize;

        *size = 0;

        headerGetEntry (hdr, RPMTAG_SIZE, &type, (void **)&tmpsize, &count);

        if (count && (type == RPM_INT32_TYPE)) {
            *size = *tmpsize;
        }
    }

    if (summary) {
        gchar *tmpsummary;

        g_free (*summary);
        *summary = NULL;

        headerGetEntry (hdr, RPMTAG_SUMMARY, &type, (void **)&tmpsummary,
                        &count);

        if (count && (type == RPM_STRING_TYPE)) {
            *summary = g_strdup (tmpsummary);
        }
    }

    if (description) {
        gchar *tmpdescription;

        g_free (*description);
        *description = NULL;

        headerGetEntry (hdr, RPMTAG_DESCRIPTION, &type,
                        (void **)&tmpdescription, &count);

        if (count && (type == RPM_STRING_TYPE)) {
            *description = g_strdup (tmpdescription);
        }
    }
}

/* Takes an array of strings, which may be of the form <version>, or
   <epoch>:<version>, or <version>-<release>, or <epoch>:<version>-<release>,
   and does the right thing, breaking them into the right parts and returning
   the parts in the arrays epochs, versions, and releases. */

static void
parse_versions (gchar **inputs, guint32 **epochs, gchar ***versions,
                gchar ***releases, guint count)
{
    guint i;

    *versions = g_new0 (gchar *, count + 1);
    *releases = g_new0 (gchar *, count + 1);
    *epochs = g_new0 (guint32, count);

    for (i = 0; i < count; i++) {
        gchar **t1, **t2;

        if (!inputs[i] || !inputs[i][0]) {
            break;
        }

        t1 = g_strsplit (inputs[i], ":", 1);

        if (t1[1]) {
            (*epochs)[i] = strtoul (t1[0], NULL, 10);
            t2 = g_strsplit (t1[1], "-", 1);
        } else {
            t2 = g_strsplit (t1[0], "-", 1);
        }

        (*versions)[i] = g_strdup (t2[0]);
        (*releases)[i] = g_strdup (t2[1]);

        g_strfreev (t1);
        g_strfreev (t2);
    }
}

static void
rc_rpmman_depends_fill (Header hdr, RCPackage *pkg)
{
    gchar **names, **verrels, **versions, **releases;
    guint32 *epochs;
    gint *relations;
    gint32 count;
    guint i;
    RCPackageDepItem *dep;
    RCPackageDep *depl = NULL;

    /* Shouldn't ever ask for dependencies unless you know what you really
       want (name, version, release) */

    g_assert (pkg->spec.name);
    g_assert (pkg->spec.version);
    g_assert (pkg->spec.release);

    /* Better safe than sorry, eh? */

    rc_package_dep_slist_free (pkg->requires);
    pkg->requires = NULL;
    rc_package_dep_slist_free (pkg->conflicts);
    pkg->conflicts = NULL;
    rc_package_dep_slist_free (pkg->provides);
    pkg->provides = NULL;

    /* Query the RPM database for the packages required by this package, and
       all associated information */

    headerGetEntry (hdr, RPMTAG_REQUIREFLAGS, NULL, (void **)&relations,
                    &count);
    headerGetEntry (hdr, RPMTAG_REQUIRENAME, NULL, (void **)&names, &count);
    headerGetEntry (hdr, RPMTAG_REQUIREVERSION, NULL, (void **)&verrels,
                    &count);

    /* Break this useless information into version and release fields.
       FIXME: this needs to do the epoch too */

    /* Some packages, such as setup and termcap, store a NULL in the
       RPMTAG_REQUIREVERSION, yet set the count to 1. That's pretty damned
       broken, so if that's the case, we'll set the count to zero, as we
       should. *sigh* */

    if (verrels == NULL)
	count = 0;

    parse_versions (verrels, &epochs, &versions, &releases, count);

    for (i = 0; i < count; i++) {
        RCPackageRelation relation = 0;

//        printf ("%s %d %s %s\n", verrels[i], epochs[i], versions[i], releases[i]);
                
        if (relations[i] & RPMSENSE_LESS) {
            relation |= RC_RELATION_LESS;
        }
        if (relations[i] & RPMSENSE_GREATER) {
            relation |= RC_RELATION_GREATER;
        }
        if (relations[i] & RPMSENSE_EQUAL) {
            relation |= RC_RELATION_EQUAL;
        }

        if (versions[i] && !versions[i][0]) {
            g_free (versions[i]);
            versions[i] = NULL;
        }

        if (releases[i] && !releases[i][0]) {
            g_free (releases[i]);
            releases[i] = NULL;
        }

        if (names[i][0] == '/') {
            /* This is a broken RPM file dependency.  I hate it, I don't know
               how I want to support it, but I don't intend to drop to the
               filesystem and verify it, or ask vlad to do so.  For now I'm
               just going to ignore it. */
        } else {
            /* Add the dependency to the list of dependencies */

            dep = rc_package_dep_item_new (names[i], epochs[i], versions[i],
                                           releases[i], relation);

            depl = g_slist_append (NULL, dep);

            pkg->requires = g_slist_append (pkg->requires, depl);
        }

        g_free (versions[i]);
        g_free (releases[i]);
    }

    free (names);
    free (verrels);

    g_free (versions);
    g_free (releases);
    g_free (epochs);

    names = NULL;
    verrels = NULL;

    /* Provide myself */

    dep = rc_package_dep_item_new (pkg->spec.name, pkg->spec.epoch,
                                   pkg->spec.version, pkg->spec.release,
                                   RC_RELATION_EQUAL);

    depl = g_slist_append (NULL, dep);

    pkg->provides = g_slist_append (pkg->provides, depl);

    /* RPM doesn't do versioned provides (as of 3.0.4), so we only need to find
       out the name of things that this package provides */

    headerGetEntry (hdr, RPMTAG_PROVIDENAME, NULL, (void **)&names, &count);

    for (i = 0; i < count; i++) {
        dep = rc_package_dep_item_new (names[i], 0, NULL, NULL,
                                       RC_RELATION_ANY);

        depl = g_slist_append (NULL, dep);

        pkg->provides = g_slist_append (pkg->provides, depl);
    }

    /* Only have to free the char** ones */

    free (names);
    names = NULL;

    /* Find full information on anything that this package conflicts with */

    headerGetEntry (hdr, RPMTAG_CONFLICTFLAGS, NULL, (void **)&relations,
                    &count);
    headerGetEntry (hdr, RPMTAG_CONFLICTNAME, NULL, (void **)&names, &count);
    headerGetEntry (hdr, RPMTAG_CONFLICTVERSION, NULL, (void **)&verrels,
                    &count);

    parse_versions (verrels, &epochs, &versions, &releases, count);

    for (i = 0; i < count; i++) {
        RCPackageRelation relation = 0;

        if (relations[i] & RPMSENSE_LESS) {
            relation |= RC_RELATION_LESS;
        }
        if (relations[i] & RPMSENSE_GREATER) {
            relation |= RC_RELATION_GREATER;
        }
        if (relations[i] & RPMSENSE_EQUAL) {
            relation |= RC_RELATION_EQUAL;
        }

        if (versions[i] && !versions[i][0]) {
            versions[i] = NULL;
        }

        if (releases[i] && !releases[i][0]) {
            releases[i] = NULL;
        }

        dep = rc_package_dep_item_new (names[i], epochs[i], versions[i],
                                       releases[i], relation);

        depl = g_slist_append (NULL, dep);

        pkg->conflicts = g_slist_append (pkg->conflicts, depl);

        g_free (versions[i]);
        g_free (releases[i]);
    }

    /* Only have to free the char** ones */

    free (names);
    free (verrels);

    g_free (versions);
    g_free (releases);
    g_free (epochs);
} /* rc_rpmman_depends_fill */

/* Query for information about a package (version, release, installed status,
   installed size...).  If you specify more than just a name, it tries to match
   those criteria. */

static RCPackage *
rc_rpmman_query (RCPackman *p, RCPackage *pkg)
{
    rpmdb db;
    dbiIndexSet matches;
    guint i;

    if (rpmdbOpen (rpmroot, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open RPM database");
        return (pkg);
    }

    if (rpmdbFindPackage (db, pkg->spec.name, &matches)) {
        rpmdbClose (db);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to perform RPM database search");
        return (pkg);
    }

    for (i = 0; i < matches.count; i++) {
        Header hdr;
        gchar *version = NULL, *release = NULL;
        gchar *summary = NULL, *description = NULL;
        guint32 size = 0, epoch = 0;
        RCPackageSection section = SECTION_MISC;

        if (!(hdr = rpmdbGetRecord (db, matches.recs[i].recOffset))) {
            rpmdbClose (db);
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to fetch RPM header from database");
            return (pkg);
        }

        rc_rpmman_read_header (hdr, NULL, &epoch, &version, &release, &section,
                               &size, &summary, &description);

        /* FIXME: this could potentially be a big problem if an rpm can have
           just an epoch (serial), and no version/release.  I'm choosing to
           ignore that for now, because I need to get this stuff done and it
           doesn't seem very likely, but... */

        if ((!pkg->spec.version || !strcmp (pkg->spec.version, version)) &&
            (!pkg->spec.release || !strcmp (pkg->spec.release, release)) &&
            (!pkg->spec.epoch || pkg->spec.epoch == epoch)) {

            g_free (pkg->spec.version);
            g_free (pkg->spec.release);

            pkg->spec.epoch = epoch;
            pkg->spec.version = version;
            pkg->spec.release = release;
            pkg->summary = summary;
            pkg->description = description;
            pkg->spec.installed = TRUE;
            pkg->spec.installed_size = size;

            rc_rpmman_depends_fill (hdr, pkg);

            headerFree (hdr);
            dbiFreeIndexRecord (matches);
            rpmdbClose (db);

            return (pkg);
        } else {
            g_free (version);
            g_free (release);
            g_free (summary);
            g_free (description);
        }

        headerFree (hdr);
    }

    pkg->spec.installed = FALSE;
    pkg->spec.installed_size = 0;

    dbiFreeIndexRecord (matches);
    rpmdbClose (db);

    return (pkg);
} /* rc_packman_query */

/* Query a file for rpm header information */

static RCPackage *
rc_rpmman_query_file (RCPackman *p, gchar *filename)
{
    FD_t fd;
    Header hdr;
    RCPackage *pkg = rc_package_new ();

    fd = fdOpen (filename, O_RDONLY, 0444);

    if (rpmReadPackageHeader (fd, &hdr, NULL, NULL, NULL)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to read package header");
        rc_package_free (pkg);
        return (NULL);
    }

    rc_rpmman_read_header (hdr, &pkg->spec.name, &pkg->spec.epoch,
                           &pkg->spec.version, &pkg->spec.release,
                           &pkg->spec.section,
                           &pkg->spec.installed_size, &pkg->summary,
                           &pkg->description);

    rc_rpmman_depends_fill (hdr, pkg);

    headerFree (hdr);
    fdClose (fd);

    return (pkg);
} /* rc_packman_query_file */

/* Query all of the packages on the system */

static RCPackageSList *
rc_rpmman_query_all (RCPackman *p)
{
    rpmdb db;
    RCPackageSList *list = NULL;
    guint recno;

    if (rpmdbOpen (rpmroot, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to open RPM database");
        return (NULL);
    }

    if (!(recno = rpmdbFirstRecNum (db))) {
        rpmdbClose (db);
        rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                              "Unable to access RPM database");
        return (NULL);
    }

    for (; recno; recno = rpmdbNextRecNum (db, recno)) {
        Header hdr;
        RCPackage *pkg = rc_package_new ();
        
        if (!(hdr = rpmdbGetRecord (db, recno))) {
            rpmdbClose (db);
            rc_package_slist_free (list);
            rc_packman_set_error (p, RC_PACKMAN_ERROR_FAIL,
                                  "Unable to read RPM database entry");
            return (NULL);
        }

        rc_rpmman_read_header (hdr, &pkg->spec.name, &pkg->spec.epoch,
                               &pkg->spec.version, &pkg->spec.release,
                               &pkg->spec.section,
                               &pkg->spec.installed_size, &pkg->summary,
                               &pkg->description);

        rc_rpmman_depends_fill (hdr, pkg);

        list = g_slist_append (list, pkg);
#if 1
	/* So, this line seems to have issues. It frees data that the package
	   needs, and doing so ends up corrupting the linked list. Ick. */
        /* Ok, I think I've fixed this.  I need to test it, but can't, because
           I don't have a convenient RPM-based machine.  Suck. */
        headerFree(hdr);
#endif
    }

    rpmdbClose (db);

    return (list);
} /* rc_rpmman_query_all */

static gint
rc_rpmman_version_compare (RCPackman *p, RCPackageSpec *s1, RCPackageSpec *s2)
{
    int stat;
    gchar *v1 = s1->version, *r1 = s1->release, *v2 = s2->version,
        *r2 = s2->release;

    g_return_val_if_fail(p, -1);

    /* Are we requiring that they provide strings for v1, r1, v2, and r2? If
       so, we can just remove the following code and put some
       g_return_val_if_fails instead. Otherwise... 
    */

    /* I don't think we should require that; this code looks correct.  Any
       non-NULL string should be interpreted as a "later" version than a NULL
       string. */

    if (v1 && !v2)
        return 1;
    else if (v2 && !v1)
        return -1;
    else if (!v1 && !v2)
        stat = 0;
    else
        stat = rpmvercmp(v1, v2);

    if (stat)
        return stat;
    
    if (r1 && !r2)
        return 1;
    else if (r2 && !r1)
        return -1;
    else if (!r1 && !r2)
        return 0;
    else
        return rpmvercmp(r1, r2);
}

/* FIXME FIXME FIXME: the strfreev is leaking all over the place, need to fix
   that shit nowish but I need sleep */

/* FIXME: need to write this ;) */
static gboolean
rc_rpmman_verify (RCPackman *p, RCPackage *d)
{
    return (TRUE);
} /* rc_rpmman_verify */

static void
rc_rpmman_class_init (RCRpmmanClass *klass)
{
/*    GtkObjectClass *object_class = (GtkObjectClass *) klass; */
    RCPackmanClass *hpc = (RCPackmanClass *) klass;

    hpc->rc_packman_real_install = rc_rpmman_install;
    hpc->rc_packman_real_remove = rc_rpmman_remove;
    hpc->rc_packman_real_query = rc_rpmman_query;
    hpc->rc_packman_real_query_all = rc_rpmman_query_all;
    hpc->rc_packman_real_query_file = rc_rpmman_query_file;
    hpc->rc_packman_real_version_compare = rc_rpmman_version_compare;
//    hpc->rc_packman_real_verify = rc_rpmman_verify;
} /* rc_rpmman_class_init */

static void
rc_rpmman_init (RCRpmman *obj)
{
    RCPackman *p = RC_PACKMAN (obj);
    /*
    rpmdb db;
    */

    gchar *tmp;

    rpmReadConfigFiles( NULL, NULL );

    tmp = getenv ("RC_RPM_ROOT");

    if (tmp) {
        rpmroot = g_strdup (tmp);
    } else {
        rpmroot = g_strdup ("/");
    }

    p->pre_config = TRUE;
    p->pkg_progress = TRUE;
    p->post_config = FALSE;

    /* FIXME: is this really what we want to do? */

    /* Probably not, at least not this simplistically */

    /*
    if (rpmdbOpen (rpmroot, &db, O_RDONLY, 0644)) {
        if (rpmdbInit (rpmroot, 0644)) {
            g_assert_not_reached ();
        }
    }

    rpmdbClose (db);
    */

} /* rc_rpmman_init */

RCRpmman *
rc_rpmman_new (void)
{
    RCRpmman *new =
        RC_RPMMAN (gtk_type_new (rc_rpmman_get_type ()));

    return new;
} /* rc_rpmman_new */
