INCLUDES = -I$(top_srcdir) -I$(includedir) $(RPMLIB_CFLAGS) $(GTK_CFLAGS) $(XML_CFLAGS) -DG_LOG_DOMAIN=\"libredcarpet\" -DSHAREDIR=\""$(datadir)/redcarpet"\"

noinst_LIBRARIES = libredcarpet.a

rpm_sources =			\
	rc-rpmman.c		\
	rc-rpmman.h		\
	rpmlead-3-0-x.h		\
	signature-3-0-x.h	\
	rpmlead-4-0-x.h		\
	signature-4-0-x.h

dpkg_sources =			\
	rc-debman.c		\
	rc-debman.h		\
	rc-debman-private.h

if ENABLE_RPM
rpm_sources2 = $(rpm_sources)
endif

if ENABLE_DPKG
dpkg_sources2 = $(dpkg_sources)
endif

libredcarpet_a_SOURCES =		\
	libredcarpet.h			\
	rc-debug.c			\
	rc-debug.h			\
	rc-package-spec.c		\
	rc-package-spec.h		\
	rc-package-dep.c		\
	rc-package-dep.h		\
	rc-package-info.c		\
	rc-package-info.h		\
	rc-package-importance.c		\
	rc-package-importance.h		\
	rc-package-update.c		\
	rc-package-update.h		\
	rc-package-set.c		\
	rc-package-set.h		\
	rc-package.c			\
	rc-package.h			\
	rc-package-section.c		\
	rc-package-section.h		\
	rc-verification.c		\
	rc-verification.h		\
	rc-verification-private.h	\
	rc-channel.h			\
	rc-channel.c			\
	rc-util.c			\
	rc-util.h			\
	rc-distro.c			\
	rc-distro.h			\
        rc-deps.c                       \
        rc-deps.h                       \
        rc-distman.c                    \
        rc-distman.h                    \
        rc-packman.c                    \
        rc-packman.h                    \
        rc-packman-private.h            \
        rc-md5.c                        \
        rc-md5.h                        \
        rc-md5-private.h                \
	rc-debug-misc.c			\
	rc-debug-misc.h			\
	rc-pretty-name.c		\
	rc-pretty-name.h		\
        xml-util.c                      \
        xml-util.h                      \
	rc-line-buf.c			\
	rc-line-buf.h			\
	rc-line-buf-private.h		\
	rc-debman-general.c		\
	rc-debman-general.h		\
	$(dpkg_sources2)		\
	$(rpm_sources2)

EXTRA_DIST = $(rpm_sources) $(dpkg_sources) rc-dpkg-helper.c

helperdir = $(datadir)/redcarpet
helper_DATA = \
	rc-dpkg-helper.so

all-local: rc-dpkg-helper.so

rc-dpkg-helper.so: rc-dpkg-helper.c
	gcc -fPIC -c -o rc-dpkg-helper.o rc-dpkg-helper.c
	ld -soname rc-dpkg-helper -shared -o rc-dpkg-helper.so rc-dpkg-helper.o

clean-local:
	rm -f rc-dpkg-helper.so
