INCLUDES = 						\
	-I$(top_srcdir) -Wall		 		\
	$(RPMLIB_CFLAGS) 				\
	$(GLIB_CFLAGS) $(GTK_CFLAGS) 			\
	$(XML_CFLAGS) 					\
	-DG_LOG_DOMAIN=\"libredcarpet\" 		\
	-DSHAREDIR=\""$(datadir)/@PRIVATE_NAMESPACE@"\" \
	-DLIBDIR=\""$(libdir)/@PRIVATE_NAMESPACE@"\"

noinst_LIBRARIES = libredcarpettmp.a

rpm_sources =			\
	rc-rpmman.c		\
	rc-rpmman.h		\
	rc-rpmman-types.h	\
	rpm-rpmlead.h		\
	rpm-signature.h		

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
	rc-world.c			\
	rc-world.h			\
	rc-resolver-info.c		\
	rc-resolver-info.h		\
	rc-resolver-context.c		\
	rc-resolver-context.h		\
	rc-resolver-compare.c		\
	rc-resolver-compare.h		\
	rc-queue-item.c			\
	rc-queue-item.h			\
	rc-resolver-queue.c		\
	rc-resolver-queue.h		\
	rc-resolver.c			\
	rc-resolver.h			\
	rc-verification.c		\
	rc-verification.h		\
	rc-verification-private.h	\
	rc-channel.h			\
	rc-channel.c			\
	rc-util.c			\
	rc-util.h			\
	rc-distro.c			\
	rc-distro.h			\
	rc-dep-or.c			\
	rc-dep-or.h			\
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
	$(rpm_sources) $(dpkg_sources) rc-dpkg-helper.c make_stub.sh

if ENABLE_DPKG
dpkghelperdir = $(libdir)/@PRIVATE_NAMESPACE@
dpkghelper_DATA = \
	rc-dpkg-helper.so
CLEANFILES += rc-dpkg-helper.so
endif

rc-dpkg-helper.so: $(srcdir)/rc-dpkg-helper.c
	gcc -fPIC -c -o rc-dpkg-helper.o $(srcdir)/rc-dpkg-helper.c
	ld -soname rc-dpkg-helper -shared -o rc-dpkg-helper.so rc-dpkg-helper.o

if ENABLE_RPM
if STATIC_RPM
else
rpmhelperdir = $(libdir)/@PRIVATE_NAMESPACE@
rpmhelper_DATA = \
	rc-{rpm}.so.0 \
	rc-{rpm_rpmio}.so.0 \
	rc-{rpm_rpmio_rpmdb}-4.0.3.so \
	rc-{rpm_rpmio_rpmdb}-4.0.4.so \
	rc-{rpm_rpmio_rpmdb}.so
CLEANFILES += rc-{rpm}.so.0 rc-{rpm_rpmio}.so.0 rc-{rpm_rpmio_rpmdb}-4.0.3.so rc-{rpm_rpmio_rpmdb}-4.0.4.so rc-{rpm_rpmio_rpmdb}.so
endif
endif

rc-{rpm}.so.0:
	./make_stub.sh rc-{rpm}.so.0 librpm.so.0

rc-{rpm_rpmio}.so.0:
	./make_stub.sh rc-{rpm_rpmio}.so.0 librpm.so.0 librpmio.so.0

rc-{rpm_rpmio_rpmdb}-4.0.3.so:
	./make_stub.sh rc-{rpm_rpmio_rpmdb}-4.0.3.so librpm-4.0.3.so librpmio-4.0.3.so librpmdb-4.0.3.so

rc-{rpm_rpmio_rpmdb}-4.0.4.so:
	./make_stub.sh rc-{rpm_rpmio_rpmdb}-4.0.4.so librpm-4.0.4.so librpmio-4.0.4.so librpmdb-4.0.4.so

rc-{rpm_rpmio_rpmdb}.so:
	./make_stub.sh rc-{rpm_rpmio_rpmdb}.so librpm.so librpmio.so librpmdb.so

all-local: libredcarpet.a

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
		ar cru libredcarpetbuild.a $$dir/*; \
		rm -rf $$dir; \
	done; \
	ranlib libredcarpetbuild.a; \
	mv libredcarpetbuild.a libredcarpet.a;
else
	cp libredcarpettmp.a libredcarpet.a
endif

clean-local:
	rm -f libredcarpet.a
