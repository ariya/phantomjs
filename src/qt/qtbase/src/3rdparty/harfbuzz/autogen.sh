#!/bin/sh
# Run this to generate all the initial makefiles, etc.

set -e

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir
PROJECT=harfbuzz
TEST_TYPE=-f
FILE=src/harfbuzz.h
ACLOCAL=${ACLOCAL-aclocal}
LIBTOOLIZE=${LIBTOOLIZE-libtoolize}
AUTOMAKE=${AUTOMAKE-automake}
AUTOHEADER=${AUTOHEADER-autoheader}
AUTOCONF=${AUTOCONF-autoconf}
LIBTOOLIZE_FLAGS="--copy --force"

DIE=0

have_libtool=false
if $LIBTOOLIZE --version < /dev/null > /dev/null 2>&1 ; then
	libtool_version=`$LIBTOOLIZE --version | sed 's/^[^0-9]*\([0-9].[0-9.]*\).*/\1/'`
	case $libtool_version in
	    1.4*|1.5*|1.6*|1.7*|2*)
		have_libtool=true
		;;
	esac
fi
if $have_libtool ; then : ; else
	echo
	echo "You must have libtool 1.4 installed to compile $PROJECT."
	echo "Install the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
fi

($AUTOCONF --version) < /dev/null > /dev/null 2>&1 || {
	echo
	echo "You must have autoconf installed to compile $PROJECT."
	echo "libtool the appropriate package for your distribution,"
	echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
	DIE=1
}

have_automake=false
need_libtoolize=true
if $AUTOMAKE --version < /dev/null > /dev/null 2>&1 ; then
	automake_version=`$AUTOMAKE --version | grep 'automake (GNU automake)' | sed 's/^[^0-9]*\(.*\)/\1/'`
	case $automake_version in
	   1.2*|1.3*|1.4) 
		;;
	   1.4*)
	   	have_automake=true
	        need_libtoolize=false
		;;
	   *)
		have_automake=true
		;;
	esac
fi
if $have_automake ; then : ; else
	echo
	echo "You must have automake 1.4-p1 installed to compile $PROJECT."
	echo "Get ftp://ftp.gnu.org/pub/gnu/automake/automake-1.4-p1.tar.gz"
	echo "(or a newer version if it is available)"
	DIE=1
fi

if test "$DIE" -eq 1; then
	exit 1
fi

test $TEST_TYPE $FILE || {
	echo "You must run this script in the top-level $PROJECT directory"
	exit 1
}

if test -z "$AUTOGEN_SUBDIR_MODE"; then
        if test -z "$*"; then
                echo "I am going to run ./configure with no arguments - if you wish "
                echo "to pass any to it, please specify them on the $0 command line."
        fi
fi

echo Running $ACLOCAL $ACLOCAL_FLAGS
$ACLOCAL $ACLOCAL_FLAGS

# optionally run autoheader
if $AUTOHEADER --version  < /dev/null > /dev/null 2>&1; then
	echo Running $AUTOHEADER
	$AUTOHEADER
fi

case $need_libtoolize in
   true)
   	echo Running $LIBTOOLIZE $LIBTOOLIZE_FLAGS
   	$LIBTOOLIZE $LIBTOOLIZE_FLAGS
	;;
esac

echo Running $AUTOMAKE -a $am_opt
$AUTOMAKE -a $am_opt
echo Running $AUTOCONF
$AUTOCONF
cd $ORIGDIR

if test -z "$AUTOGEN_SUBDIR_MODE"; then
	echo Running $srcdir/configure "$@"
        $srcdir/configure "$@"

        echo 
        echo "Now type 'make' to compile $PROJECT."
fi
