INCLUDES = \
	-I$(top_srcdir)			\
	-I../src		\
	$(GLIB_CFLAGS) $(GTK_CFLAGS)	\
	$(XML_CFLAGS)			\
	-DG_LOG_DOMAIN=\"librc-tools\"	\
	-Wall


noinst_PROGRAMS = cur_to_xml

cur_to_xml_SOURCES = cur_to_xml.c
cur_to_xml_LDADD =	\
	../src/libredcarpet.a	\
	$(GLIB_LIBS) $(GTK_LIBS) $(XML_LIBS)