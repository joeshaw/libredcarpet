SOURCES=libredcarpet-sources.xml
API=libredcarpet-api.xml
MCS=mcs
GAPI_PARSER=@GAPI_PARSER@
GAPI_FIXUP=@GAPI_FIXUP@
GAPI_CODEGEN=@GAPI_CODEGEN@
ASSEMBLY=libredcarpet-sharp.dll
DOC_UPDATER=@MONODOC@ --update
DOC_ASSEMBLER=@MONODOC@ --assemble

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libredcarpet-sharp.pc

all: $(ASSEMBLY)

$(API):
	$(GAPI_PARSER) $(SOURCES)

generated-stamp: $(API) RC.metadata
	$(GAPI_FIXUP) --api=$(API) --metadata=RC.metadata && \
	$(GAPI_CODEGEN) --generate $(API) --outdir=generated \
	--customdir=. --assembly-name=libredcarpet-sharp && touch generated-stamp

$(ASSEMBLY): generated-stamp
	$(MCS) --unsafe -target:library -pkg:gtk-sharp -o $(ASSEMBLY) --recurse '*.cs'

update-docs:
	$(DOC_UPDATER) $(ASSEMBLY) -o en -f

libredcarpet-sharp-docs.zip libredcarpet-sharp-docs.tree: $(srcdir)/en/*/*.xml
	$(DOC_ASSEMBLER) --ecma en -o libredcarpet-sharp-docs

merge-docs:
	@MONODOC@ --merge-changes ~/.config/monodoc/changeset.xml `pwd`

clean-docs:
	rm -f libredcarpet-sharp-docs.zip
	rm -f libredcarpet-sharp-docs.tree
	rm -rf en

clean-all: clean clean-docs
	rm -f generated-stamp
	rm -rf generated

prefix=@prefix@
apidir=$(prefix)/share/gapi
libdir=$(prefix)/lib

SHELL = @SHELL@
top_srcdir = @top_srcdir@
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs

assemblydir = $(libdir)
assembly_DATA = $(ASSEMBLY)

apidatadir = $(apidir)
apidata_DATA = $(API)

docdir = `monodoc --get-sourcesdir`
doc_DATA =				\
	libredcarpet-sharp-docs.zip	\
	libredcarpet-sharp-docs.tree	\
	libredcarpet-sharp-docs.source

EXTRA_DIST =				\
	libredcarpet-sharp.pc.in	\
	libredcarpet-sources.xml 	\
	RC.metadata			\
	libredcarpet-sharp-docs.source	\
	Channel.custom			\
	Global.custom			\
	World.custom			\
	WorldService.custom		\
	Distro.custom			\
	PackageSpec.custom		\
	WorldMulti.custom		\
	ServiceMountException.cs	\
	PackageDepArray.custom

dist-hook:
	mkdir -p $(distdir)/en/RC
	cp -r $(srcdir)/en/RC/*.xml $(distdir)/en/RC

CLEANFILES = $(ASSEMBLY)
