INCLUDES = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	$(GLIB_CFLAGS) $(GTK_CFLAGS)		\
	$(XML_CFLAGS)				\
	-DG_LOG_DOMAIN=\"dependency-tests\"	\
	-Wall

noinst_PROGRAMS = deptestomatic

deptestomatic_SOURCES = \
	deptestomatic.c

deptestomatic_LDADD = \
	../src/libredcarpet.a \
	$(GLIB_LIBS) $(GTK_LIBS) $(XML_LIBS)

