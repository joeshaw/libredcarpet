
redcarpetdir = $(pythondir)

redcarpet_LTLIBRARIES = redcarpet.la
redcarpet_la_CFLAGS = $(LIBREDCARPET_CFLAGS) $(PYTHON_INCLUDES) -I../../src
redcarpet_la_LDFLAGS = -avoid-version -module -L../../src -lredcarpet -lbz2
redcarpet_la_LIBADD = $(LIBREDCARPET_LIBS)
redcarpet_la_SOURCES =		\
	pyutil.c		\
	distro.c		\
	verification.c		\
	package-importance.c	\
	package-spec.c		\
	package-dep.c		\
	package-match.c		\
	package-update.c	\
	package.c		\
	package-file.c		\
	packman.c		\
	channel.c		\
	world.c			\
	world-store.c		\
	resolver-info.c		\
	resolver-context.c	\
	resolver-queue.c	\
	resolver.c		\
	redcarpet.c

