/*
 *    Copyright (C) 2000 Helix Code Inc.
 *
 *    Authors: Ian Peters <itp@helixcode.com>
 *
 *    This program is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public License
 *    as published by the Free Software Foundation; either version 2 of
 *    the License, or (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.    See the
 *    GNU Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public
 *    License along with this program; if not, write to the Free Software
 *    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "rc-packman-private.h"

#include "rc-rpmman.h"

#include <fcntl.h>
#include <string.h>
#include <rpm/rpmurl.h>
#include <rpm/rpmmacro.h>
#include <rpm/misc.h>
#include <rpm/header.h>

#if 1
#    define RPM_ROOTDIR "/cvs/redhat"
#else
#    define RPM_ROOTDIR "/"
#endif

static void rc_rpmman_class_init (RCRpmmanClass *klass);
static void rc_rpmman_init       (RCRpmman *obj);

/* static RCPackmanClass *rc_packman_parent; */

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
                ghp, pkgKey, RC_RPMMAN (ghp)->package_count);
        }
        break;

    case RPMCALLBACK_INST_START:
    case RPMCALLBACK_UNINST_START:
    case RPMCALLBACK_UNINST_STOP:
    case RPMCALLBACK_TRANS_PROGRESS:
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

int rpmInstall(const char * rootdir, const char ** fileArgv, int transFlags, 
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
   filenames, and passes to rpmInstallHelix.  Emits the install_done signal at
   the end. */

static void
rc_rpmman_install (RCPackman *p, GSList *pkgs)
{
    GSList *iter = pkgs;
    gchar *ptr;
    guint length;
    gchar **pkgv;
    int i;
    int ret;

    length = g_slist_length (pkgs);

    pkgv = g_new0 (char *, length + 1);

    for (i = 0; i < length; i++) {
        ptr = (gchar *)(iter->data);
        /* Gotta give a filename */
        g_assert (ptr);
        pkgv[i] = g_strdup (ptr);
        iter = iter->next;
    }

    ghp = p;

    RC_RPMMAN (p)->package_count = 0;

    ret = rpmInstall (RPM_ROOTDIR, (const char **)pkgv, 0,
                      INSTALL_NOORDER | INSTALL_UPGRADE, 0, NULL);

    g_strfreev (pkgv);

    /* I have no idea what return codes are possible */

    if (ret) {
        g_free (p->reason);
        p->reason = g_strdup ("RPM installation failed.");
        rc_packman_install_done (p, RC_PACKMAN_FAIL);
    } else {
        rc_packman_install_done (p, RC_PACKMAN_COMPLETE);
    }
} /* rc_rpmman_install */

/* Much like _install, once again FIXME check the return code */

static void
rc_rpmman_remove (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter = pkgs;
    RCPackage *ptr;
    guint length;
    gchar **pkgv;
    int i;
    int ret;

    length = g_slist_length (pkgs);

    pkgv = g_new0 (gchar *, length + 1);

    for (i = 0; i < length; i++) {
        ptr = (RCPackage *)(iter->data);

        /* FIXME: what the hell does RPM do with the epoch?! */

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

    ret = rpmErase (RPM_ROOTDIR, (const char **)pkgv, 0, 0);

    g_strfreev (pkgv);

    /* I have no idea what error codes are possible... */

    if (ret) {
        g_free (p->reason);
        p->reason = g_strdup ("RPM removal failed");
        rc_packman_remove_done (p, RC_PACKMAN_FAIL);
    } else {
        rc_packman_remove_done (p, RC_PACKMAN_COMPLETE);
    }
} /* rc_rpmman_remove */

static RCPackage *
rc_rpmman_query (RCPackman *p, RCPackage *pkg)
{
    rpmdb db;
    Header hdr;
    int_32 count;
    dbiIndexSet matches;
    guint i;

    if (rpmdbOpen (RPM_ROOTDIR, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                              "Unable to open RPM database");
        return (pkg);
    }

    if (rpmdbFindPackage (db, pkg->spec.name, &matches)) {
        rpmdbClose (db);
        rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                              "Unable to perform RPM database search");
        return (pkg);
    }

    for (i = 0; i < matches.count; i++) {
        gchar *version = NULL, *release = NULL;
        guint32 size = 0, epoch = 0;

        if (!(hdr = rpmdbGetRecord (db, matches.recs[i].recOffset))) {
            rpmdbClose (db);
            rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                                  "Unable to fetch RPM header from database");
            return (pkg);
        }

        headerGetEntry (hdr, RPMTAG_EPOCH, NULL, (void **)&epoch, &count);
        headerGetEntry (hdr, RPMTAG_VERSION, NULL, (void **)&version, &count);
        headerGetEntry (hdr, RPMTAG_RELEASE, NULL, (void **)&release, &count);
        headerGetEntry (hdr, RPMTAG_SIZE, NULL, (void **)&size, &count);

        /* FIXME: this could potentially be a big problem if an rpm can have
           just an epoch (serial), and no version/release.  I'm choosing to
           ignore that for now, because I need to get this stuff done and it
           doesn't seem very likely, but... */

        if ((!pkg->spec.version || !strcmp (pkg->spec.version, version)) &&
            (!pkg->spec.release || !strcmp (pkg->spec.release, release))) {

            g_free (pkg->spec.version);
            g_free (pkg->spec.release);

            pkg->spec.epoch = epoch;
            pkg->spec.version = g_strdup (version);
            pkg->spec.release = g_strdup (release);
            pkg->spec.installed = TRUE;
            pkg->spec.installed_size = size;

            headerFree (hdr);
            dbiFreeIndexRecord (matches);
            rpmdbClose (db);

            return (pkg);
        }

        headerFree (hdr);
    }

    pkg->spec.installed = FALSE;
    pkg->spec.installed_size = 0;

    dbiFreeIndexRecord (matches);
    rpmdbClose (db);

    return (pkg);
} /* rc_packman_query */

static RCPackage *
rc_rpmman_query_file (RCPackman *p, gchar *filename)
{
    FD_t fd;
    Header hdr;
    gchar *name = NULL, *version = NULL, *release = NULL;
    guint32 size = 0, epoch = 0;
    int count;
    RCPackage *pkg = rc_package_new ();

    fd = fdOpen (filename, O_RDONLY, 0444);

    if (rpmReadPackageHeader (fd, &hdr, NULL, NULL, NULL)) {
        rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                              "Unable to read package header");
        rc_package_free (pkg);
        return (NULL);
    }

    headerGetEntry (hdr, RPMTAG_NAME, NULL, (void **)&name, &count);
    headerGetEntry (hdr, RPMTAG_EPOCH, NULL, (void **)&epoch, &count);
    headerGetEntry (hdr, RPMTAG_VERSION, NULL, (void **)&version, &count);
    headerGetEntry (hdr, RPMTAG_RELEASE, NULL, (void **)&release, &count);
    headerGetEntry (hdr, RPMTAG_SIZE, NULL, (void **)&size, &count);

    pkg->spec.name = g_strdup (name);
    pkg->spec.epoch = epoch;
    pkg->spec.version = g_strdup (version);
    pkg->spec.release = g_strdup (release);
    pkg->spec.installed_size = size;

    headerFree (hdr);
    fdClose (fd);

    return (pkg);
} /* rc_packman_query_file */

static RCPackageSList *
rc_rpmman_query_all (RCPackman *p)
{
    rpmdb db;
    RCPackageSList *list = NULL;
    guint recno;

    if (rpmdbOpen (RPM_ROOTDIR, &db, O_RDONLY, 0444)) {
        rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                              "Unable to open RPM database");
        return (NULL);
    }

    if (!(recno = rpmdbFirstRecNum (db))) {
        rpmdbClose (db);
        rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                              "Unable to access RPM database");
        return (NULL);
    }

    for (; recno; recno = rpmdbNextRecNum (db, recno)) {
        Header hdr;
        int_32 count;
        gchar *name = NULL, *version = NULL, *release = NULL;
        guint32 size = 0, epoch = 0;
        
        if (!(hdr = rpmdbGetRecord (db, recno))) {
            rpmdbClose (db);
            rc_package_slist_free (list);
            rc_packman_set_error (p, RC_PACKMAN_OPERATION_FAILED,
                                  "Unable to read RPM database entry");
            return (NULL);
        }
        
        headerGetEntry (hdr, RPMTAG_NAME, NULL, (void **)&name, &count);
        headerGetEntry (hdr, RPMTAG_EPOCH, NULL, (void **)&epoch, &count);
        headerGetEntry (hdr, RPMTAG_VERSION, NULL, (void **)&version, &count);
        headerGetEntry (hdr, RPMTAG_RELEASE, NULL, (void **)&release, &count);
        headerGetEntry (hdr, RPMTAG_SIZE, NULL, (void **)&size, &count);

        /* FIXME: the epoch I'm getting back from rpmlib is all fucked up */

        list = rc_package_slist_add_package (list, name, epoch, version,
                                             release, TRUE, size);

        headerFree(hdr);
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

#if 0

/* Return -1 if the package is too old or a problem makes it uninstallable
   Return  0 if the package is the same version and release as installed
   Return  1 if the package is newer than the currently installed one
*/

/* Alternatively, return 1 if the first package is greater than the second,
   0 if the packages are the same, and -1 if the first package is smaller than
   the second */

static gint
rc_rpmman_version_compare (RCPackman *p, gchar *v1, gchar *r1,
                           gchar *v2, gchar *r2)
{
    int stat;

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
} /* rc_rpmman_version_compare */

#endif

/* This function takes an array of strings of the form <version>-<release>, and
   breaks them into two arrays of strings of <version> and <release>. */

/* FIXME: this function isn't handling the epoch at all, and needs to.
   FUCKING rpmlib. */

static void
parse_versions (gchar **inputs, gchar ***versions, gchar ***releases,
                gint count)
{
    int i = 0, j;

    *versions = g_new0 (gchar *, count);
    *releases = g_new0 (gchar *, count);

    for (i = 0; i < count; i++) {
        j = 0;

        while (inputs[i][j] && inputs[i][j] != '-') {
            j++;
        }

        if (inputs[i][j]) {
            (*versions)[i] = g_strndup (inputs[i], j);
            (*releases)[i] = g_strdup (inputs[i] + j + 1);
        } else {
            (*versions)[i] = g_strdup (inputs[i]);
            (*releases)[i] = NULL;
        }
    }
}

static void
rc_rpmman_depends_fill (RCPackage *pkg, Header hdr)
{
    gchar **names, **verrels, **versions, **releases;
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

    parse_versions (verrels, &versions, &releases, count);

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

        if (!versions[i][0]) {
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

            dep = rc_package_dep_item_new (names[i], 0, versions[i],
                                           releases[i], relations[i]);

            depl = g_slist_append (NULL, dep);

            pkg->requires = g_slist_append (pkg->requires, depl);
        }
    }

    free (names);
    free (verrels);

    names = NULL;

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
    headerGetEntry (hdr, RPMTAG_CONFLICTVERSION, NULL, (void **)&versions,
                    &count);

    parse_versions (versions, &versions, &releases, count);

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

        if (!versions[i][0]) {
            versions[i] = NULL;
        }

        if (releases[i] && !releases[i][0]) {
            releases[i] = NULL;
        }

        dep = rc_package_dep_item_new (names[i], 0, versions[i], releases[i],
                                       relation);

        depl = g_slist_append (NULL, dep);

        pkg->conflicts = g_slist_append (pkg->conflicts, depl);
    }

    /* Only have to free the char** ones */

    free (names);
    free (versions);

} /* rc_rpmman_depends_fill */

/* Given a GSList of RCPackmanPackage, fills in the three dependency lists
   (requires, provides, conflicts) in place for all of the packages.  rpmlib
   is really really ugly.  FIXME: correctly handle multiple installs of the
   same package (could have different dependencies...) */

static GSList *
rc_rpmman_depends (RCPackman *p, RCPackageSList *pkgs)
{
    RCPackageSList *iter;
    rpmdb db;

    if (rpmdbOpen (RPM_ROOTDIR, &db, O_RDONLY, 0444)) {
        g_free (p->reason);
        p->reason = g_strdup ("Unable to open RPM database");
        return (NULL);
    }

    /* Run through all of the packages for which dependency information is
       desired */
    for (iter = pkgs; iter; iter = iter->next) {
        dbiIndexSet matches;
        Header hdr = NULL;
        RCPackage *d = (RCPackage *)(iter->data);

        rpmdbFindPackage (db, d->spec.name, &matches);

        /* Is this really what I want? */
        g_assert (matches.count > 0);

        rc_packman_query (p, d);

        if (d->spec.installed) {
            char *version = NULL, *release = NULL;
            guint i = 0;
            int type, count;

            do {
                if (hdr)
                    headerFree(hdr);

                if (!(hdr = rpmdbGetRecord (db, matches.recs[i].recOffset))) {
                    rpmdbClose (db);
                    g_free (p->reason);
                    p->reason = g_strdup ("Unable to read RPM database entry");
                    return (NULL);
                }

                headerGetEntry (hdr, RPMTAG_VERSION, &type,
                                (void **)&version, &count);
                headerGetEntry (hdr, RPMTAG_RELEASE, &type,
                                (void **)&release, &count);

                i++;
            } while ((i < matches.count) &&
                     (strcmp (version, d->spec.version) ||
                      strcmp (release, d->spec.release)));

            if (!strcmp (version, d->spec.version) &&
                !strcmp (release, d->spec.release)) {
                /* This is the match we were looking for */

                rc_rpmman_depends_fill (d, hdr);
            }

            headerFree(hdr);
        }

        dbiFreeIndexRecord (matches);

    }

    return (pkgs);
} /* rc_rpmman_depends */

static GSList *
rc_rpmman_depends_files (RCPackman *p, GSList *files)
{
    GSList *iter;
    RCPackageSList *dlist = NULL;

    for (iter = files; iter; iter = iter->next) {
        gchar *filename = (gchar *)(iter->data);
        FD_t fd;
        Header hdr;
        int ret;
        RCPackage *d = rc_package_new ();
        int count;
        gchar *name, *version, *release;
        gint32 size, epoch;

        fd = fdOpen (filename, O_RDONLY, 0444);

        ret = rpmReadPackageHeader (fd, &hdr, NULL, NULL, NULL);

        headerGetEntry (hdr, RPMTAG_NAME, NULL, (void **)&name, &count);
        headerGetEntry (hdr, RPMTAG_EPOCH, NULL, (void **)&epoch, &count);
        headerGetEntry (hdr, RPMTAG_VERSION, NULL, (void **)&version, &count);
        headerGetEntry (hdr, RPMTAG_RELEASE, NULL, (void **)&release, &count);
        headerGetEntry (hdr, RPMTAG_SIZE, NULL, (void **)&size, &count);

        d->spec.name = g_strdup (name);
        d->spec.version = g_strdup (version);
        d->spec.release = g_strdup (release);

        d->spec.epoch = epoch;
        d->spec.installed_size = size;

        rc_rpmman_depends_fill (d, hdr);

        dlist = g_slist_append (dlist, d);

        fdClose (fd);
    }

    return (dlist);
}

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
    hpc->rc_packman_real_depends = rc_rpmman_depends;
    hpc->rc_packman_real_depends_files = rc_rpmman_depends_files;
//    hpc->rc_packman_real_verify = rc_rpmman_verify;
} /* rc_rpmman_class_init */

static void
rc_rpmman_init (RCRpmman *obj)
{
    /*
      RCPackman *hp = RC_PACKMAN (obj);
    */
    /*
      rpmdb db;
    */

    rpmReadConfigFiles( NULL, NULL );

    /* FIXME: is this really what we want to do? */

    /* Probably not, at least not this simplistically */

    /* 
       if (rpmdbOpen (RPM_ROOTDIR, &db, O_RDONLY, 0644)) {
       if (rpmdbInit (RPM_ROOTDIR, 0644)) {
       g_assert_not_reached ();
       }
       }
    */
    
} /* rc_rpmman_init */

RCRpmman *
rc_rpmman_new (void)
{
    RCRpmman *new =
        RC_RPMMAN (gtk_type_new (rc_rpmman_get_type ()));

    return new;
} /* rc_rpmman_new */
