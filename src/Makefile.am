INCLUDES =				\
	-I$(top_srcdir)			\
	-I$(includedir)			\
	$(RPMLIB_CFLAGS)		\
	$(GTK_CFLAGS)			\
	$(XML_CFLAGS)			\
	-DG_LOG_DOMAIN=\"libredcarpet\" \
	-DSHAREDIR=\""$(datadir)/redcarpet"\"

noinst_LIBRARIES = libredcarpet.a

rpm_sources = 		\
	rc-rpmman.c	\
	rc-rpmman.h

if HAVE_LIBRPM
rpm_sources2 = $(rpm_sources)
endif

libredcarpet_a_SOURCES =		\
	libredcarpet.h			\
	rc-package-spec.c		\
	rc-package-spec.h		\
	rc-package-dep.c		\
	rc-package-dep.h		\
	rc-package-update.c		\
	rc-package-update.h		\
	rc-package-set.c		\
	rc-package-set.h		\
	rc-package.c			\
	rc-package.h			\
	rc-verification.c		\
	rc-verification.h		\
	rc-channel.h			\
	rc-channel.c			\
	rc-util.c			\
	rc-util.h			\
	rc-distro.c			\
	rc-distro.h			\
        deps.c                          \
        deps.h                          \
        deps-private.h                  \
        pkginfo.c                       \
        pkginfo.h                       \
	rc-common.h			\
        rc-distman.c                    \
        rc-distman.h                    \
        rc-packman.c                    \
        rc-packman.h                    \
        rc-packman-private.h            \
        rc-string.c			\
        rc-string.h			\
        rc-debman.c                     \
        rc-debman.h                     \
        rc-md5.c                        \
        rc-md5.h                        \
        rc-md5-private.h                \
        xml-util.c                      \
        xml-util.h                      \
	rc-line-buf.c			\
	rc-line-buf.h			\
        $(rpm_sources2)

#	rc-crypto.c			\
#	rc-crypto.h			\

EXTRA_DIST = $(rpm_sources)
