SOURCES=libredcarpet-sources.xml
API=libredcarpet-api.xml
MCS=mcs
GAPI_PARSER=@GAPI_PARSER@
GAPI_FIXUP=@GAPI_FIXUP@
GAPI_CODEGEN=@GAPI_CODEGEN@
ASSEMBLY=libredcarpet-sharp.dll
DOC_UPDATER=@MONODOC@ --update
DOC_ASSEMBLER=@MONODOC@ --assemble

all: $(ASSEMBLY) docs-stamp

$(API):
	$(GAPI_PARSER) $(SOURCES)

generated-stamp: $(API) RC.metadata
	$(GAPI_FIXUP) --api=$(API) --metadata=RC.metadata && \
	$(GAPI_CODEGEN) --generate $(API) --outdir=generated \
	--customdir=. --assembly-name=libredcarpet-sharp && touch generated-stamp

$(ASSEMBLY): generated-stamp
	$(MCS) --unsafe -target:library -pkg:gtk-sharp -o $(ASSEMBLY) --recurse '*.cs'

docs-stamp:
	$(DOC_UPDATER) $(ASSEMBLY) -o en -f
	$(DOC_ASSEMBLER) --ecma en -o libredcarpet-sharp-docs && touch docs-stamp

merge-docs:
	@MONODOC@ --merge-changes ~/.config/monodoc/changeset.xml `pwd`

clean-docs:
	rm -f docs-stamp
	rm -f libredcarpet-sharp-docs.zip
	rm -f libredcarpet-sharp-docs.tree
	rm -rf en

clean:
	rm -f $(ASSEMBLY)

clean-all: clean clean-docs
	rm -f generated-stamp
	rm -rf generated

prefix=@prefix@
apidir=$(prefix)/share/gapi
libdir=$(prefix)/lib

EXTRA_DIST = libredcarpet-sharp.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libredcarpet-sharp.pc

SHELL = @SHELL@
top_srcdir = @top_srcdir@
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs

install: all
	$(mkinstalldirs) $(libdir) &&		\
	$(mkinstalldirs) $(apidir) &&		\
	$(mkinstalldirs) $(pkgconfigdir) &&\
	cp $(API) $(apidir) &&			\
	cp $(ASSEMBLY) $(libdir) &&		\
	cp $(pkgconfig_DATA) $(pkgconfigdir) &&	\
	install -m 644 libredcarpet-sharp-docs.zip libredcarpet-sharp-docs.source libredcarpet-sharp-docs.tree `monodoc --get-sourcesdir`
