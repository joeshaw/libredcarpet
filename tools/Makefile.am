INCLUDES = \
	-I$(top_srcdir)/src		\
	$(LIBREDCARPET_CFLAGS)		\
	-DG_LOG_DOMAIN=\"librc-tools\"	\
	-Wall

bin_PROGRAMS = pkg_to_xml pkg-vercmp

noinst_PROGRAMS = cur_to_xml packman_test

cur_to_xml_SOURCES = cur_to_xml.c
cur_to_xml_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

pkg_to_xml_SOURCES = pkg_to_xml.c
pkg_to_xml_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

packman_test_SOURCES = packman_test.c
packman_test_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)	\
	-lreadline -ltermcap

pkg_vercmp_SOURCES = pkg-vercmp.c
pkg_vercmp_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)