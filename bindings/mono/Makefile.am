SUBDIRS = . glue

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

update-generated: $(API) RC.metadata RC.symbols
	$(GAPI_FIXUP) --api=$(API) --metadata=RC.metadata --symbols=RC.symbols && \
	$(GAPI_CODEGEN) --generate $(API) --outdir=generated \
	--customdir=. --assembly-name=libredcarpet-sharp \
	--gluelib-name=libredcarpetsharpglue-0 --glue-filename=glue/generated.c \
	--glue-includes=libredcarpet.h && \
	touch generated-stamp

$(ASSEMBLY): generated-stamp
	$(MCS) -unsafe -target:library -pkg:gtk-sharp-2.0 -out:$(ASSEMBLY) -recurse:'*.cs'

update-docs:
	$(DOC_UPDATER) $(ASSEMBLY) -o en -f

libredcarpet-sharp-docs.zip libredcarpet-sharp-docs.tree: $(srcdir)/en/*/*.xml
	$(DOC_ASSEMBLER) --ecma en -o libredcarpet-sharp-docs

merge-docs:
	@MONODOC@ --merge-changes ~/.config/monodoc/changeset.xml `pwd`

clean-docs:
	rm -f libredcarpet-sharp-docs.zip
	rm -f libredcarpet-sharp-docs.tree

clean-all: clean clean-docs
	rm -f generated-stamp
	rm -rf generated

prefix=@prefix@
apidir=$(prefix)/share/gapi
libdir=$(prefix)/lib

SHELL = @SHELL@
top_srcdir = @top_srcdir@
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs

apidatadir = $(apidir)
apidata_DATA = $(API)

docdir = `monodoc --get-sourcesdir`
doc_DATA =				\
	libredcarpet-sharp-docs.zip	\
	libredcarpet-sharp-docs.tree	\
	libredcarpet-sharp-docs.source

EXTRA_DIST =				\
	AssemblyInfo.cs			\
	libredcarpet-sharp.snk		\
	libredcarpet-sharp.pc.in	\
	libredcarpet-sources.xml 	\
	RC.metadata			\
	RC.symbols			\
	libredcarpet-sharp-docs.source	\
	Channel.custom			\
	Debug.custom			\
	Extract.custom			\
	Global.custom			\
	World.custom			\
	WorldService.custom		\
	Distro.custom			\
	PackageSpec.custom		\
	WorldMulti.custom		\
	ServiceMountException.cs	\
	PackageMatch.custom		\
	Package.custom			\
	PackageUpdate.custom		\
	PackageDep.custom		\
	Packman.custom			\
    WorldStore.custom       \
	generated/*.cs

dist-hook:
	mkdir -p $(distdir)/en/RC
	cp -r $(srcdir)/en/RC/*.xml $(distdir)/en/RC

install-data-hook:
	$(GACUTIL) /i $(ASSEMBLY) /package libredcarpet-sharp /gacdir $(prefix)/lib /root $(DESTDIR)$(prefix)/lib

CLEANFILES = $(ASSEMBLY)
