INCLUDES = 						\
	-I$(top_srcdir) -Wall		 		\
	$(RPMLIB_CFLAGS) 				\
	$(LIBREDCARPET_CFLAGS)				\
	-DG_LOG_DOMAIN=\"libredcarpet\"

rpm_sources =			\
	rc-rpmman.c		\
	rc-rpmman.h		\
	rc-rpmman-types.h	\
	rpm-rpmlead.h		\
	rpm-signature.h		\
	rpm-ugid.h

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
	rc-extract-channels.h		\
	rc-extract-packages.h		\
	rc-line-buf.h			\
        rc-md5.h                        \
	rc-package.h			\
	rc-package-dep.h		\
	rc-package-file.h		\
	rc-package-importance.h		\
	rc-package-match.h		\
	rc-package-spec.h		\
	rc-package-update.h		\
	rc-package-section.h		\
        rc-packman.h                    \
	rc-pending.h			\
	rc-queue-item.h			\
	rc-resolver.h			\
	rc-resolver-compare.h		\
	rc-resolver-context.h		\
	rc-resolver-info.h		\
	rc-resolver-queue.h		\
	rc-rollback.h			\
	rc-subscription.h		\
	rc-util.h			\
	rc-verification.h		\
	rc-world.h			\
	rc-world-local-dir.h		\
	rc-world-multi.h		\
	rc-world-service.h		\
	rc-world-store.h		\
	rc-world-synthetic.h		\
	rc-world-system.h		\
	rc-world-undump.h		\
	rc-xml.h			\
        xml-util.h

libredcarpet_la_SOURCES =		\
	$(libredcarpet_public_headers)	\
	distributions-xml.h		\
	rc-arch.c			\
	rc-channel.c			\
	rc-debman-general.c		\
	rc-debug.c			\
	rc-dep-or.c			\
        rc-distman.c                    \
	rc-distro.c			\
	rc-extract-channels.c		\
	rc-extract-packages.c		\
	rc-line-buf.c			\
	rc-line-buf-private.h		\
	rc-marshal.c			\
	rc-marshal.h			\
        rc-md5.c                        \
        rc-md5-private.h                \
	rc-package.c			\
	rc-package-dep.c		\
	rc-package-file.c		\
	rc-package-importance.c		\
	rc-package-match.c		\
	rc-package-spec.c		\
	rc-package-update.c		\
	rc-package-section.c		\
        rc-packman.c                    \
        rc-packman-private.h            \
	rc-pending.c			\
	rc-queue-item.c			\
	rc-resolver.c			\
	rc-resolver-compare.c		\
	rc-resolver-context.c		\
	rc-resolver-info.c		\
	rc-resolver-queue.c		\
	rc-rollback.c			\
	rc-subscription.c		\
	rc-util.c			\
	rc-verification.c		\
	rc-verification-private.h	\
	rc-world.c			\
	rc-world-local-dir.c		\
	rc-world-multi.c		\
	rc-world-service.c		\
	rc-world-store.c		\
	rc-world-synthetic.c		\
	rc-world-system.c		\
	rc-world-undump.c		\
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
	rc-dpkg-helper.c 		\
	make_stub.sh			\
	distributions.xml		\
	packer.pl			\
	rc-marshal.list

dpkg-helper.h: rc-dpkg-helper.so
	gzip -9 -c rc-dpkg-helper.so > rc-dpkg-helper.so.gz && \
	$(srcdir)/packer.pl rc-dpkg-helper.so.gz rc-dpkg-helper.so > dpkg-helper.h && \
	rm rc-dpkg-helper.so.gz

rc-dpkg-helper.so: $(srcdir)/rc-dpkg-helper.c
	gcc -fPIC -c -o rc-dpkg-helper.o $(srcdir)/rc-dpkg-helper.c
	ld -soname rc-dpkg-helper -shared -o rc-dpkg-helper.so rc-dpkg-helper.o

CLEANFILES = dpkg-helper.h rc-dpkg-helper.so

RPM_STUBS = rc-{rpm}.so.0 rc-{rpm_rpmio}.so.0 rc-{rpm_rpmio_rpmdb}-4.0.3.so rc-{rpm_rpmio_rpmdb}-4.0.4.so rc-{rpm_rpmio_rpmdb}-4.1-popt.so.0.so rc-{rpm_rpmio_rpmdb}-4.1-popt.so.1.so rc-{rpm_rpmio_rpmdb}-4.2.so rc-{rpm_rpmio_rpmdb}-4.3.so

rpm-stubs.h: $(RPM_STUBS) packer.pl 
	rm -f rpm-stubs.h && \
	for stub in $(RPM_STUBS) ; do \
		gzip -c -9 $$stub > $$stub.gz && \
		$(srcdir)/packer.pl $$stub.gz $$stub >> rpm-stubs.h && \
		rm $$stub.gz; \
	done;

CLEANFILES += rpm-stubs.h $(RPM_STUBS)

rc-{rpm}.so.0:
	$(srcdir)/make_stub.sh rc-{rpm}.so.0 librpm.so.0 libpopt.so.0

rc-{rpm_rpmio}.so.0:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio}.so.0 librpm.so.0 librpmio.so.0 libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.0.3.so:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio_rpmdb}-4.0.3.so librpm-4.0.3.so librpmio-4.0.3.so librpmdb-4.0.3.so libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.0.4.so:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio_rpmdb}-4.0.4.so librpm-4.0.4.so librpmio-4.0.4.so librpmdb-4.0.4.so libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.1-popt.so.0.so:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio_rpmdb}-4.1-popt.so.0.so librpm-4.1.so librpmio-4.1.so librpmdb-4.1.so libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.1-popt.so.1.so:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio_rpmdb}-4.1-popt.so.1.so librpm-4.1.so librpmio-4.1.so librpmdb-4.1.so libpopt.so.1

rc-{rpm_rpmio_rpmdb}-4.2.so:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio_rpmdb}-4.2.so librpm-4.2.so librpmio-4.2.so librpmdb-4.2.so libpopt.so.0

rc-{rpm_rpmio_rpmdb}-4.3.so:
	$(srcdir)/make_stub.sh rc-{rpm_rpmio_rpmdb}-4.3.so librpm-4.3.so librpmio-4.3.so librpmdb-4.3.so libpopt.so.0

lib_LTLIBRARIES = libredcarpet.la
libredcarpet_la_LDFLAGS = $(LIBREDCARPET_LIBS)

libredcarpetincludedir = $(includedir)/libredcarpet

libredcarpetinclude_HEADERS = \
	$(libredcarpet_public_headers)

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

