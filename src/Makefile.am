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
	rc-package-info.c		\
	rc-package-info.h		\
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
        rc-deps.c                       \
        rc-deps.h                       \
	rc-common.h			\
        rc-distman.c                    \
        rc-distman.h                    \
        rc-packman.c                    \
        rc-packman.h                    \
        rc-packman-private.h            \
        rc-debman.c                     \
        rc-debman.h                     \
        rc-md5.c                        \
        rc-md5.h                        \
        rc-md5-private.h                \
	rc-debug-misc.c			\
	rc-debug-misc.h			\
        xml-util.c                      \
        xml-util.h                      \
	rc-line-buf.c			\
	rc-line-buf.h			\
	rpmlead-4-0-x.h			\
	rpmlead-3-0-x.h			\
	signature-4-0-x.h		\
	signature-3-0-x.h		\
        $(rpm_sources2)

#	rc-crypto.c			\
#	rc-crypto.h			\

EXTRA_DIST = $(rpm_sources) dpkg-helper.c

helperdir = $(datadir)/redcarpet
helper_DATA = \
	dpkg-helper.so

all-local: dpkg-helper.so

dpkg-helper.so: dpkg-helper.c
	gcc -fPIC -c -o dpkg-helper.o dpkg-helper.c
	ld -soname dpkg-helper -shared -o dpkg-helper.so dpkg-helper.o

clean-local:
	rm -f dpkg-helper.so
