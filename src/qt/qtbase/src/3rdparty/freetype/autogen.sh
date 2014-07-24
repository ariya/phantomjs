#!/bin/sh

# Copyright 2005, 2006, 2007, 2008, 2009, 2010 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

run ()
{
  echo "running \`$*'"
  eval $*

  if test $? != 0 ; then
    echo "error while running \`$*'"
    exit 1
  fi
}

get_major_version ()
{
  echo $1 | sed -e 's/\([0-9][0-9]*\)\..*/\1/g'
}

get_minor_version ()
{
  echo $1 | sed -e 's/[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/g'
}

get_patch_version ()
{
  # tricky: some version numbers don't include a patch
  # separated with a point, but something like 1.4-p6
  patch=`echo $1 | sed -e 's/[0-9][0-9]*\.[0-9][0-9]*\.\([0-9][0-9]*\).*/\1/g'`
  if test "$patch" = "$1"; then
    patch=`echo $1 | sed -e 's/[0-9][0-9]*\.[0-9][0-9]*\-p\([0-9][0-9]*\).*/\1/g'`
    # if there isn't any patch number, default to 0
    if test "$patch" = "$1"; then
      patch=0
    fi
  fi
  echo $patch
}

# $1: version to check
# $2: minimum version

compare_to_minimum_version ()
{
  MAJOR1=`get_major_version $1`
  MAJOR2=`get_major_version $2`
  if test $MAJOR1 -lt $MAJOR2; then
    echo 0
    return
  else 
    if test $MAJOR1 -gt $MAJOR2; then
      echo 1
      return
    fi
  fi

  MINOR1=`get_minor_version $1`
  MINOR2=`get_minor_version $2`
  if test $MINOR1 -lt $MINOR2; then
    echo 0
    return
  else 
    if test $MINOR1 -gt $MINOR2; then
      echo 1
      return
    fi
  fi

  PATCH1=`get_patch_version $1`
  PATCH2=`get_patch_version $2`
  if test $PATCH1 -lt $PATCH2; then
    echo 0
  else
    echo 1
  fi
}

# check the version of a given tool against a minimum version number
#
# $1: tool path
# $2: tool usual name (e.g. `aclocal')
# $3: tool variable  (e.g. `ACLOCAL')
# $4: minimum version to check against
# $5: option field index used to extract the tool version from the
#     output of --version

check_tool_version ()
{
  field=$5
  # assume the output of "[TOOL] --version" is "toolname (GNU toolname foo bar) version"
  if test "$field"x = x; then
    field=3  # default to 3 for all GNU autotools, after filtering enclosed string
  fi
  version=`$1 --version | head -1 | sed 's/([^)]*)/()/g' | cut -d ' ' -f $field`
  version_check=`compare_to_minimum_version $version $4`
  if test "$version_check"x = 0x; then
    echo "ERROR: Your version of the \`$2' tool is too old."
    echo "       Minimum version $4 is required (yours is version $version)."
    echo "       Please upgrade or use the $3 variable to point to a more recent one."
    echo ""
    exit 1
  fi
}

if test ! -f ./builds/unix/configure.raw; then
  echo "You must be in the same directory as \`autogen.sh'."
  echo "Bootstrapping doesn't work if srcdir != builddir."
  exit 1
fi

# On MacOS X, the GNU libtool is named `glibtool'.
HOSTOS=`uname`
LIBTOOLIZE=libtoolize
if test "$HOSTOS"x = Darwinx; then
  LIBTOOLIZE=glibtoolize
fi

if test "$ACLOCAL"x = x; then
  ACLOCAL=aclocal
fi

if test "$AUTOCONF"x = x; then
  AUTOCONF=autoconf
fi

check_tool_version $ACLOCAL    aclocal    ACLOCAL    1.10.1
check_tool_version $LIBTOOLIZE libtoolize LIBTOOLIZE 2.2.4
check_tool_version $AUTOCONF   autoconf   AUTOCONF   2.62

# This sets freetype_major, freetype_minor, and freetype_patch.
eval `sed -nf version.sed include/freetype/freetype.h`

# We set freetype-patch to an empty value if it is zero.
if test "$freetype_patch" = ".0"; then
  freetype_patch=
fi

cd builds/unix

echo "generating \`configure.ac'"
sed -e "s;@VERSION@;$freetype_major$freetype_minor$freetype_patch;" \
  < configure.raw > configure.ac

run aclocal -I . --force
run $LIBTOOLIZE --force --copy --install
run autoconf --force

chmod +x mkinstalldirs
chmod +x install-sh

cd ../..

chmod +x ./configure

# EOF
