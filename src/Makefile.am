INCLUDES = 						\
	-I$(top_srcdir) -Wall		 		\
	$(RPMLIB_CFLAGS) 				\
	$(LIBREDCARPET_CFLAGS)				\
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

BUILT_SOURCES =				\
	rc-marshal.h			\
	rc-marshal.c

libredcarpet_public_headers = 		\
	rc-debug.h			\
	rc-package-spec.h		\
	libredcarpet.h			\
	rc-package-dep.h		\
	rc-package-importance.h		\
	rc-package-update.h		\
	rc-package-set.h		\
	rc-package.h			\
	rc-package-section.h		\
	rc-world.h			\
	rc-world-import.h		\
	rc-world-subscriptions.h	\
	rc-resolver-info.h		\
	rc-resolver-context.h		\
	rc-resolver-compare.h		\
	rc-queue-item.h			\
	rc-resolver-queue.h		\
	rc-resolver.h			\
	rc-verification.h		\
	rc-channel.h			\
	rc-xml.h			\
	rc-util.h			\
	rc-distro.h			\
	rc-dep-or.h			\
        rc-distman.h                    \
        rc-packman.h                    \
        rc-md5.h                        \
	rc-debug-misc.h			\
	rc-pretty-name.h		\
        xml-util.h                      \
	rc-line-buf.h			\
	rc-debman-general.h

libredcarpettmp_a_SOURCES =		\
	rc-debug.c			\
	rc-package-spec.c		\
	rc-package-dep.c		\
	rc-package-importance.c		\
	rc-package-update.c		\
	rc-package-set.c		\
	rc-package.c			\
	rc-package-section.c		\
	rc-world.c			\
	rc-world-import.c		\
	rc-world-subscriptions.c	\
	rc-resolver-info.c		\
	rc-resolver-context.c		\
	rc-resolver-compare.c		\
	rc-queue-item.c			\
	rc-resolver-queue.c		\
	rc-resolver.c			\
	rc-verification.c		\
	rc-verification-private.h	\
	rc-channel.c			\
	rc-channel-private.h		\
	rc-channel-private.c		\
	rc-xml.c			\
	rc-util.c			\
	rc-distro.c			\
	rc-dep-or.c			\
        rc-distman.c                    \
        rc-packman.c                    \
        rc-packman-private.h            \
        rc-md5.c                        \
        rc-md5-private.h                \
	rc-debug-misc.c			\
	rc-pretty-name.c		\
        xml-util.c                      \
	rc-line-buf.c			\
	rc-line-buf-private.h		\
	rc-debman-general.c		\
	rc-marshal.c			\
	$(libredcarpet_public_headers)	\
	$(dpkg_sources2)		\
	$(rpm_sources2)

EXTRA_DIST =				\
	$(rpm_sources)			\
	$(dpkg_sources)			\
	rc-channel-private.h		\
	rc-dpkg-helper.c 		\
	make_stub.sh			\
	rc-marshal.list

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


lib_LIBRARIES = libredcarpet.a

libredcarpet_a_SOURCES = $(libredcarpet_public_headers)

libredcarpetincludedir = $(includedir)/libredcarpet-2.0

libredcarpetinclude_HEADERS = \
	$(libredcarpet_public_headers)

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

###
### Deal with auto-generating the signal marshallers
###

rc-marshal.h: stamp-rc-marshal.h
	@true   

rc-marshal.c: stamp-rc-marshal.c
	@true

stamp-rc-marshal.h: rc-marshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=rc_marshal rc-marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh rc-marshal.h || cp xgen-gmh rc-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

stamp-rc-marshal.c: rc-marshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=rc_marshal rc-marshal.list --body >> xgen-gmc \
	&& (cmp -s xgen-gmc rc-marshal.c || cp xgen-gmc rc-marshal.c) \
	&& rm -f xgen-gmc xgen-gmc~

clean-local:
	rm -f libredcarpet.a
