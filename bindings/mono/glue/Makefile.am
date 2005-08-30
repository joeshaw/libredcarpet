lib_LTLIBRARIES = libredcarpetsharpglue-0.la

libredcarpetsharpglue_0_la_SOURCES =	\
	generated.c

# Adding a new glue file?
# Please remember to update makefile.win32

libredcarpetsharpglue_0_la_LDFLAGS = -module -avoid-version -no-undefined

libredcarpetsharpglue_0_la_LIBADD = $(LIBREDCARPET_LIBS)

INCLUDES = $(LIBREDCARPET_CFLAGS) -I$(top_srcdir)/src

libredcarpetsharpglue.dll: $(libredcarpetsharpglue_0_la_OBJECTS) libredcarpetsharpglue.rc libredcarpetsharpglue.def
	./build-dll libredcarpetsharpglue-2 $(VERSION)

CLEANFILES = lib*.a lib*.dll
