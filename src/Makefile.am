INCLUDES = -I$(top_srcdir) -I$(includedir) $(RPMLIB_CFLAGS) $(GLIB_CFLAGS) $(GTK_CFLAGS) $(XML_CFLAGS) -DG_LOG_DOMAIN=\"libredcarpet\" -DSHAREDIR=\""$(datadir)/redcarpet"\" -DLIBDIR=\""$(libdir)/redcarpet"\"

noinst_LIBRARIES = libredcarpettmp.a

rpm_sources =			\
	rc-rpmman.c		\
	rc-rpmman.h		\
	rc-rpmman-types.h	\
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

libredcarpettmp_a_SOURCES =		\
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
	rc-deps-util.c			\
	rc-deps-util.h			\
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

EXTRA_DIST = \
	$(rpm_sources) $(dpkg_sources) rc-dpkg-helper.c \
	rc-rpm-helper-with-rpmio.so \
	rc-rpm-helper.so

if ENABLE_RPM
rpmhelperdir = $(libdir)/redcarpet
rpmhelper_DATA = \
	rc-rpm-helper-with-rpmio.so	\
	rc-rpm-helper.so
endif

if ENABLE_DPKG
dpkghelperdir = $(libdir)/redcarpet
dpkghelper_DATA = \
	rc-dpkg-helper.so
endif

all-local: rc-dpkg-helper.so libredcarpet.a

rc-dpkg-helper.so: $(srcdir)/rc-dpkg-helper.c
if ENABLE_DPKG
	gcc -fPIC -c -o rc-dpkg-helper.o $(srcdir)/rc-dpkg-helper.c
	ld -soname rc-dpkg-helper -shared -o rc-dpkg-helper.so rc-dpkg-helper.o
else
	touch rc-dpkg-helper.so
endif

libredcarpet.a: libredcarpettmp.a
if STATIC_RPM
	rm -f libredcarpetbuild.a; \
	cp libredcarpettmp.a libredcarpetbuild.a; \
	for lib in $(RPM_STATIC_LIBS) ; do \
		dir=`basename $$lib`; \
		rm -rf $$dir; \
		mkdir $$dir; \
		cd $$dir; \
		ar x $$lib; \
		for object in `ls .` ; do \
			mv $$object $$dir-$$object; \
		done; \
		cd ..; \
		ar cru libredcarpetbuild.a $$dir/*.o; \
		rm -rf $$dir; \
	done; \
	ranlib libredcarpetbuild.a; \
	mv libredcarpetbuild.a libredcarpet.a; \
	rm -f libredcarpettmp.a
else
	cp libredcarpettmp.a libredcarpet.a
endif

clean-local:
	rm -f rc-dpkg-helper.so	\
	rm -f libredcarpet.a
