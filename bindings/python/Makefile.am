
redcarpetdir = $(pythondir)

redcarpet_LTLIBRARIES = redcarpet.la
redcarpet_la_CFLAGS = $(LIBREDCARPET_CFLAGS) $(PYTHON_INCLUDES) -I../../src
redcarpet_la_LDFLAGS = -avoid-version -module -L../../src -lredcarpet -lbz2
redcarpet_la_LIBADD = $(LIBREDCARPET_LIBS)
redcarpet_la_SOURCES =		\
	pyutil.c		\
	pyutil.h		\
	distro.c		\
	distro.h		\
	verification.c		\
	verification.h		\
	package-importance.c	\
	package-importance.h	\
	package-spec.c		\
	package-spec.h		\
	package-dep.c		\
	package-dep.h		\
	package-match.c		\
	package-match.h		\
	package-update.c	\
	package-update.h	\
	package.c		\
	package.h		\
	package-file.c		\
	package-file.h		\
	packman.c		\
	packman.h		\
	channel.c		\
	channel.h		\
	world.c			\
	world.h			\
	world-store.c		\
	world-store.h		\
	resolver-info.c		\
	resolver-info.h		\
	resolver-context.c	\
	resolver-context.h	\
	resolver-queue.c	\
	resolver-queue.h	\
	resolver.c		\
	resolver.h		\
	redcarpet.c

