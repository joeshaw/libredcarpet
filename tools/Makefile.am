INCLUDES = \
	-I$(top_srcdir)			\
	-I../src			\
	$(LIBREDCARPET_CFLAGS)		\
	-DG_LOG_DOMAIN=\"librc-tools\"	\
	-Wall


noinst_PROGRAMS = cur_to_xml packman_test

cur_to_xml_SOURCES = cur_to_xml.c
cur_to_xml_LDADD =	\
	../src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)

packman_test_SOURCES = packman_test.c

packman_test_LDADD =	\
	../src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)	\
	-lreadline -ltermcap
