INCLUDES = \
	-I$(top_srcdir)/src		\
	$(LIBREDCARPET_CFLAGS)		\
	-DG_LOG_DOMAIN=\"librc-tools\"	\
	-Wall

if ENABLE_PACKMAN_TEST
  packman_test = packman_test
else
  packman_test =
endif

bin_PROGRAMS = pkg_to_xml pkg-vercmp cur_to_xml dep_lint

dep_lint_SOURCES = dep_lint.c
dep_lint_LDADD = 				\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

cur_to_xml_SOURCES = cur_to_xml.c
cur_to_xml_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

pkg_to_xml_SOURCES = pkg_to_xml.c
pkg_to_xml_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

pkg_vercmp_SOURCES = pkg-vercmp.c
pkg_vercmp_LDADD =	\
	$(top_builddir)/src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)
