# if ENABLE_GTK_DOC
# DOC_DIR=doc
# else
DOC_DIR=
# endif

SUBDIRS = src dependency-tests tools $(DOC_DIR)

gpgdir = $(datadir)/@PRIVATE_NAMESPACE@
gpg_DATA = red-carpet.gpg

EXTRA_DIST = $(gpg_DATA) libredcarpet-2.0.pc.in

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libredcarpet-2.0.pc
