INCLUDES = \
	-I$(top_srcdir)			\
	-I../src			\
	$(LIBREDCARPET_CFLAGS)		\
	-DG_LOG_DOMAIN=\"librc-tools\"	\
	-Wall


noinst_PROGRAMS = pkg_to_xml cur_to_xml packman_test

cur_to_xml_SOURCES = cur_to_xml.c
cur_to_xml_LDADD =	\
	../src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

pkg_to_xml_SOURCES = pkg_to_xml.c
pkg_to_xml_LDADD =	\
	../src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

packman_test_SOURCES = packman_test.c
packman_test_LDADD =	\
	../src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)	\
	-lreadline -ltermcap
