## Process this file with automake to produce Makefile.in
##
## This file was copied from the one shipped with gtk+'s reference docs.

# This is a hack for builddir != srcdir.  Use it instead of top_builddir.
abs_top_builddir=$(shell cd $(top_builddir) && pwd)

# How to find our own local libtool.
LOCAL_LIBTOOL=$(SHELL) $(abs_top_builddir)/libtool

# The scanner to use.
OBJ_SCANNER=gtkdoc-scanobj

# The name of the module.
DOC_MODULE=libredcarpet

# The top-level SGML file.
DOC_MAIN_SGML_FILE=libredcarpet.sgml

# The directory containing the source code
DOC_SOURCE_DIR=						\
	--source-dir=$(top_srcdir)/src

# Extra options to supply to gtkdoc-fixref
FIXXREF_OPTIONS=

# Header files to ignore when scanning
IGNORE_HFILES=			\
	libredcarpet

GTKDOC_CFLAGS = 					\
	@CFLAGS@					\
	-I$(top_srcdir) 				\
	-I$(top_srcdir)/src				\
	$(RPMLIB_CFLAGS)				\
	$(GLIB_CFLAGS)					\
	$(GTK_CFLAGS)					\
	$(XML_CFLAGS)



GTKDOC_LIBS =			 		\
	$(abs_top_builddir)/src/libredcarpet.a 	\
	$(GLIB_LIBS) $(GTK_LIBS) $(XML_LIBS)

content_files = 


##################################################
# Everything below here is generic.              #
# Only change it if you know what you are doing. #
##################################################

GTKDOC_CC=$(LOCAL_LIBTOOL) --mode=compile $(CC) -static
GTKDOC_LD=$(LOCAL_LIBTOOL) --mode=link $(CC)

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

EXTRA_DIST = 				\
	$(DOC_MODULE).types		\
	$(DOC_MAIN_SGML_FILE)		\
	$(content_files)

# These were in EXTRA_DIST... We don't have them.
#	$(DOC_MODULE)-sections.txt	\
#	$(DOC_MODULE)-overrides.txt

if ENABLE_GTK_DOC
all-local: html/index.html

$(DOC_MODULE)-decl.txt: 
	$(MAKE) scan

$(tmpl_sources): $(DOC_MODULE)-decl.txt $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt
	$(MAKE) templates

sgml/$(DOC_MODULE)-doc.bottom: $(tmpl_sources) $(DOC_MODULE)-decl.txt
	$(MAKE) sgml

html/index.html: sgml/$(DOC_MODULE)-doc.bottom $(DOC_MAIN_SGML_FILE) $(content_files)
	$(MAKE) html
endif

scan:
	(cd $(srcdir) \
	&& CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)" CFLAGS="$(GTKDOC_CFLAGS)" LDFLAGS="$(GTKDOC_LIBS)" $(OBJ_SCANNER) --module=$(DOC_MODULE) \
	&& gtkdoc-scan --module=$(DOC_MODULE) $(DOC_SOURCE_DIR) --ignore-headers="$(IGNORE_HFILES)" $(EXTRA_HFILES))

templates: 
	cd $(srcdir) && gtkdoc-mktmpl --module=$(DOC_MODULE)

tmpl.stamp: templates

sgml: tmpl.stamp
	cd $(srcdir) \
	&& gtkdoc-mkdb --module=$(DOC_MODULE) $(DOC_SOURCE_DIR)

sgml.stamp: sgml

html: sgml.stamp
	test -d $(srcdir)/html || mkdir $(srcdir)/html
	-cd $(srcdir)/html && gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE)
	@echo '-- Fixing Crossreferences' 
	-cd $(srcdir) && gtkdoc-fixxref --module-dir=html --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)

clean-local:
	rm -f *~ *.bak *.signals *-unused.txt

maintainer-clean-local: clean
	cd $(srcdir) && rm -rf sgml html $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

install-data-local:
	$(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)
	(installfiles=`echo $(srcdir)/html/*.html`; \
	if test "$$installfiles" = '$(srcdir)/html/*.html'; \
	then echo '-- Nothing to install' ; \
	else \
	  for i in $$installfiles; do \
	    echo '-- Installing '$$i ; \
	    $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR); \
	  done; \
	  echo '-- Installing $(srcdir)/html/index.sgml' ; \
	  $(INSTALL_DATA) $(srcdir)/html/index.sgml $(DESTDIR)$(TARGET_DIR); \
	fi)

#
# Require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc must be installed and enabled in order to make dist"
	@false
endif

CLEANFILES = html.stamp sgml.stamp tmpl.stamp

dist-hook: dist-check-gtkdoc
	mkdir $(distdir)/html
	mkdir $(distdir)/sgml
	mkdir $(distdir)/tmpl
	-cp $(srcdir)/html/*.html $(srcdir)/html/*.css $(srcdir)/html/index.sgml $(distdir)/html
	-cp $(srcdir)/tmpl/*.sgml $(distdir)/tmpl
	-cp $(srcdir)/sgml/*.sgml $(distdir)/sgml
	-cp $(srcdir)/sgml/*.bottom $(srcdir)/sgml/*.top $(distdir)/sgml

.PHONY : html sgml templates scan
