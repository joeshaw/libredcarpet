INCLUDES = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	$(LIBREDCARPET_CFLAGS)			\
	$(XML_CFLAGS)				\
	-DG_LOG_DOMAIN=\"dependency-tests\"	\
	-Wall

check_PROGRAMS = deptestomatic

deptestomatic_SOURCES = \
	deptestomatic.c

deptestomatic_LDADD = \
	../src/libredcarpet.a \
	$(LIBREDCARPET_LIBS)

TESTS_ENVIRONMENT = \
	TEST_DIRECTORY="$(srcdir)/basic-exercises"

TESTS = $(srcdir)/run-tests.sh 
