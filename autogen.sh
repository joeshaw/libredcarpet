#!/bin/sh
# Run this to generate all the initial makefiles, etc.

DIE=0

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

rm -f config.cache

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile libredcarpet."
	echo "Download the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

(libtool --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have libtool installed to compile libredcarpet."
	echo "Get ftp://alpha.gnu.org/gnu/libtool-1.2d.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have automake installed to compile libredcarpet."
	echo "Get ftp://ftp.cygnus.com/pub/home/tromey/automake-1.4.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
}

if test "$DIE" -eq 1; then
	exit 1
fi

THEDIR="`pwd`"
cd $srcdir
(test -f src/libredcarpet.h) || {
	echo "You must run this script in the top-level libredcarpet directory"
	exit 1
}

if test -z "$*"; then
	echo "I am going to run ./configure with no arguments - if you wish "
        echo "to pass any to it, please specify them on the $0 command line."
fi

for i in .
do
  echo processing $i
  (cd $i; \
    libtoolize --copy --force; \
    aclocal $ACLOCAL_FLAGS; autoheader; \
    automake --add-missing; \
    autoheader; \
    autoconf)
done

cd "$THEDIR"

echo "Running $srcdir/configure --enable-maintainer-mode" "$@"
$srcdir/configure --enable-maintainer-mode "$@"

echo
echo "Now type 'make' to compile libredcarpet."
