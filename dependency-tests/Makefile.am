INCLUDES = 					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/src			\
	$(LIBREDCARPET_CFLAGS)			\
	$(XML_CFLAGS)				\
	-DG_LOG_DOMAIN=\"dependency-tests\"	\
	-Wall

# We're not ready for this yet.
#check_PROGRAMS = deptestomatic
noinst_PROGRAMS = deptestomatic

deptestomatic_SOURCES = \
	deptestomatic.c

deptestomatic_LDADD = \
	../src/.libs/libredcarpet.so \
	$(LIBREDCARPET_LIBS)

#TESTS_ENVIRONMENT = \
#	TEST_DIRECTORY="$(srcdir)/basic-exercises"

#TESTS = $(srcdir)/run-tests.sh 
