INCLUDES = 						\
	-I$(top_srcdir) -Wall		 		\
	$(RPMLIB_CFLAGS) 				\
	$(LIBREDCARPET_CFLAGS)				\
	-DG_LOG_DOMAIN=\"libredcarpet\"

noinst_LIBRARIES = libredcarpettmp.a

rpm_sources =			\
	rc-rpmman.c		\
	rc-rpmman.h		\
	rc-rpmman-types.h	\
	rpm-rpmlead.h		\
	rpm-signature.h

rpm_built_sources =		\
	rpm-stubs.h

dpkg_sources =			\
	rc-debman.c		\
	rc-debman.h		\
	rc-debman-private.h

dpkg_built_sources =		\
	dpkg-helper.h

if ENABLE_RPM
rpm_sources2 = $(rpm_sources)
if STATIC_RPM
else
rpm_built_sources2 = $(rpm_built_sources)
endif
endif

if ENABLE_DPKG
dpkg_sources2 = $(dpkg_sources)
dpkg_built_sources2 = $(dpkg_built_sources)
endif

BUILT_SOURCES =				\
	distributions-xml.h		\
	rc-marshal.c			\
	rc-marshal.h			\
	$(rpm_built_sources2)		\
	$(dpkg_built_sources2)

libredcarpet_public_headers = 		\
	libredcarpet.h			\
	rc-arch.h			\
	rc-channel.h			\
	rc-debman-general.h		\
	rc-debug.h			\
	rc-dep-or.h			\
        rc-distman.h                    \
	rc-distro.h			\
	rc-line-buf.h			\
        rc-md5.h                        \
	rc-package.h			\
	rc-package-dep.h		\
	rc-package-importance.h		\
	rc-package-set.h		\
	rc-package-spec.h		\
	rc-package-update.h		\
	rc-package-section.h		\
        rc-packman.h                    \
	rc-pretty-name.h		\
	rc-queue-item.h			\
	rc-resolver.h			\
	rc-resolver-compare.h		\
	rc-resolver-context.h		\
	rc-resolver-info.h		\
	rc-resolver-queue.h		\
	rc-util.h			\
	rc-verification.h		\
	rc-world.h			\
	rc-world-dump.h			\
	rc-world-import.h		\
	rc-world-subscriptions.h	\
	rc-xml.h			\
        xml-util.h

libredcarpettmp_a_SOURCES =		\
	distributions-xml.h		\
	rc-arch.c			\
	rc-channel.c			\
	rc-channel-private.c		\
	rc-channel-private.h		\
	rc-debman-general.c		\
	rc-debug.c			\
	rc-dep-or.c			\
        rc-distman.c                    \
	rc-distro.c			\
	rc-line-buf.c			\
	rc-line-buf-private.h		\
	rc-marshal.c			\
	rc-marshal.h			\
        rc-md5.c                        \
        rc-md5-private.h                \
	rc-package.c			\
	rc-package-dep.c		\
	rc-package-importance.c		\
	rc-package-set.c		\
	rc-package-spec.c		\
	rc-package-update.c		\
	rc-package-section.c		\
        rc-packman.c                    \
        rc-packman-private.h            \
	rc-pretty-name.c		\
	rc-queue-item.c			\
	rc-resolver.c			\
	rc-resolver-compare.c		\
	rc-resolver-context.c		\
	rc-resolver-info.c		\
	rc-resolver-queue.c		\
	rc-util.c			\
	rc-verification.c		\
	rc-verification-private.h	\
	rc-world.c			\
	rc-world-dump.c			\
	rc-world-import.c		\
	rc-world-subscriptions.c	\
	rc-xml.c			\
        xml-util.c                      \
	$(libredcarpet_public_headers)	\
	$(dpkg_sources2)		\
	$(rpm_sources2)			\
	$(rpm_built_sources2)		\
	$(dpkg_built_sources2)

EXTRA_DIST =				\
	$(rpm_sources)			\
	$(dpkg_sources)			\
	rc-channel-private.h		\
	rc-dpkg-helper.c 		\
	make_stub.sh			\
	distributions.xml		\
	packer.pl			\
	rc-marshal.list

dpkg-helper.h: rc-dpkg-helper.so
	cd $(srcdir) && \
	gzip -9 -c rc-dpkg-helper.so > rc-dpkg-helper.so.gz && \
	./packer.pl rc-dpkg-helper.so.gz rc-dpkg-helper.so > dpkg-helper.h && \
	rm rc-dpkg-helper.so.gz

rc-dpkg-helper.so: $(srcdir)/rc-dpkg-helper.c
	gcc -fPIC -c -o rc-dpkg-helper.o $(srcdir)/rc-dpkg-helper.c
	ld -soname rc-dpkg-helper -shared -o rc-dpkg-helper.so rc-dpkg-helper.o

CLEANFILES += dpkg-helper.h rc-dpkg-helper.so

RPM_STUBS = rc-{rpm}.so.0 rc-{rpm_rpmio}.so.0 rc-{rpm_rpmio_rpmdb}-4.0.3.so rc-{rpm_rpmio_rpmdb}-4.0.4.so

rpm-stubs.h: $(RPM_STUBS) packer.pl 
	cd $(srcdir) && \
	rm -f rpm-stubs.h && \
	for stub in $(RPM_STUBS) ; do \
		gzip -c -9 $$stub > $$stub.gz && \
		./packer.pl $$stub.gz $$stub >> rpm-stubs.h && \
		rm $$stub.gz; \
	done;

CLEANFILES += rpm-stubs.h $(RPM_STUBS)

rc-{rpm}.so.0:
	./make_stub.sh rc-{rpm}.so.0 librpm.so.0 libpopt.so.0

rc-{rpm_rpmio}.so.0:
	./make_stub.sh rc-{rpm_rpmio}.so.0 librpm.so.0 librpmio.so.0 libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.0.3.so:
	./make_stub.sh rc-{rpm_rpmio_rpmdb}-4.0.3.so librpm-4.0.3.so librpmio-4.0.3.so librpmdb-4.0.3.so libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.0.4.so:
	./make_stub.sh rc-{rpm_rpmio_rpmdb}-4.0.4.so librpm-4.0.4.so librpmio-4.0.4.so librpmdb-4.0.4.so libpopt.so.0

all-local: libredcarpet.a

lib_LIBRARIES = libredcarpet.a

libredcarpet_a_SOURCES = $(libredcarpet_public_headers)

libredcarpetincludedir = $(includedir)/libredcarpet

libredcarpetinclude_HEADERS = \
	$(libredcarpet_public_headers)

CLEANFILES += libredcarpet.a
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

CLEANFILES += distributions-xml.h
distributions-xml.h: distributions.xml packer.pl
	cd $(srcdir) \
	&& gzip -9 -c distributions.xml > distributions.xml.gz \
	&& ./packer.pl distributions.xml.gz distros_xml > distributions-xml.h \
	&& rm distributions.xml.gz

###
### Deal with auto-generating the signal marshallers
###

CLEANFILES += rc-marshal.c rc-marshal.h

rc-marshal.h: rc-marshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=rc_marshal rc-marshal.list --header >> xgen-gmh \
	&& (cmp -s xgen-gmh rc-marshal.h || cp xgen-gmh rc-marshal.h) \
	&& rm -f xgen-gmh xgen-gmh~

rc-marshal.c: rc-marshal.list
	cd $(srcdir) \
	&& glib-genmarshal --prefix=rc_marshal rc-marshal.list --body >> xgen-gmc \
	&& (cmp -s xgen-gmc rc-marshal.c || cp xgen-gmc rc-marshal.c) \
	&& rm -f xgen-gmc xgen-gmc~

CLEANFILES += $(MAYBECLEAN)
