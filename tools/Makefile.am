INCLUDES = \
	-I$(top_srcdir)			\
	-I../src			\
	$(LIBREDCARPET_CFLAGS)		\
	-DG_LOG_DOMAIN=\"librc-tools\"	\
	-Wall


noinst_PROGRAMS = cur_to_xml

cur_to_xml_SOURCES = cur_to_xml.c
cur_to_xml_LDADD =	\
	../src/libredcarpet.a	\
	$(LIBREDCARPET_LIBS)
