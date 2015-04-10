# This file is part of the FreeType project.
#
# Process this file with autoconf to produce a configure script.
#
# Copyright 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010 by
# David Turner, Robert Wilhelm, and Werner Lemberg.
#
# This file is part of the FreeType project, and may only be used, modified,
# and distributed under the terms of the FreeType project license,
# LICENSE.TXT.  By continuing to use, modify, or distribute this file you
# indicate that you have read the license and understand and accept it
# fully.

AC_INIT([FreeType], [2.3.12], [freetype@nongnu.org], [freetype])
AC_CONFIG_SRCDIR([ftconfig.in])


# Don't forget to update docs/VERSION.DLL!

version_info='10:0:4'
AC_SUBST([version_info])
ft_version=`echo $version_info | tr : .`
AC_SUBST([ft_version])


# checks for system type

AC_CANONICAL_HOST


# checks for programs

AC_PROG_CC
AC_PROG_CPP
AC_SUBST(EXEEXT)


# checks for native programs to generate building tool

if test ${cross_compiling} = yes; then
  AC_CHECK_PROG(CC_BUILD, ${build}-gcc, ${build}-gcc)
  test -z "${CC_BUILD}" && AC_CHECK_PROG(CC_BUILD, gcc, gcc)
  test -z "${CC_BUILD}" && AC_CHECK_PROG(CC_BUILD, cc, cc, , , /usr/ucb/cc)
  test -z "${CC_BUILD}" && AC_MSG_ERROR([cannot find native C compiler])

  AC_MSG_CHECKING([for suffix of native executables])
  rm -f a.* b.* a_out.exe conftest.*
  echo > conftest.c "int main() { return 0;}"
  ${CC_BUILD} conftest.c || AC_MSG_ERROR([native C compiler is not working])
  rm -f conftest.c
  if test -x a.out -o -x b.out -o -x conftest; then
    EXEEXT_BUILD=""
  elif test -x a_out.exe -o -x conftest.exe; then
    EXEEXT_BUILD=".exe"
  elif test -x conftest.* ; then
    EXEEXT_BUILD=`echo conftest.* | sed -n '1s/^.*\././'`
  fi
  rm -f a.* b.* a_out.exe conftest.*
  AC_MSG_RESULT($EXEEXT_BUILD)
else
  CC_BUILD=${CC}
  EXEEXT_BUILD=${EXEEXT}
fi

AC_SUBST(CC_BUILD)
AC_SUBST(EXEEXT_BUILD)



# get compiler flags right

if test "x$GCC" = xyes; then
  XX_CFLAGS="-Wall"
  XX_ANSIFLAGS="-pedantic -ansi"
else
  case "$host" in
  *-dec-osf*)
    CFLAGS=
    XX_CFLAGS="-std1 -g3"
    XX_ANSIFLAGS=
    ;;
  *)
    XX_CFLAGS=
    XX_ANSIFLAGS=
    ;;
  esac
fi
AC_SUBST([XX_CFLAGS])
AC_SUBST([XX_ANSIFLAGS])


# auxiliary programs

AC_CHECK_PROG([RMF], [rm], [rm -f])
AC_CHECK_PROG([RMDIR], [rmdir], [rmdir])


# Since this file will be finally moved to another directory we make
# the path of the install script absolute.  This small code snippet has
# been taken from automake's `ylwrap' script.

AC_PROG_INSTALL
case "$INSTALL" in
/*)
  ;;
*/*)
  INSTALL="`pwd`/$INSTALL" ;;
esac


# checks for header files

AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h unistd.h])


# checks for typedefs, structures, and compiler characteristics

AC_C_CONST
AC_CHECK_SIZEOF([int])
AC_CHECK_SIZEOF([long])


# check whether cpp computation of size of int and long in ftconfig.in works

AC_MSG_CHECKING([cpp computation of bit length in ftconfig.in works])
orig_CPPFLAGS="${CPPFLAGS}"
CPPFLAGS="-I${srcdir} -I. ${CPPFLAGS}"
ac_clean_files="ft2build.h ftoption.h ftstdlib.h"
touch ft2build.h ftoption.h ftstdlib.h

cat > conftest.c <<\_ACEOF
#include <limits.h>
#define FT_CONFIG_OPTIONS_H "ftoption.h"
#define FT_CONFIG_STANDARD_LIBRARY_H "ftstdlib.h"
#define FT_UINT_MAX  UINT_MAX
#define FT_ULONG_MAX ULONG_MAX
#include "ftconfig.in"
_ACEOF
echo >> conftest.c "#if FT_SIZEOF_INT == "${ac_cv_sizeof_int}
echo >> conftest.c "ac_cpp_ft_sizeof_int="${ac_cv_sizeof_int}
echo >> conftest.c "#endif"
echo >> conftest.c "#if FT_SIZEOF_LONG == "${ac_cv_sizeof_long}
echo >> conftest.c "ac_cpp_ft_sizeof_long="${ac_cv_sizeof_long}
echo >> conftest.c "#endif"

${CPP} ${CPPFLAGS} conftest.c | ${GREP} ac_cpp_ft > conftest.sh
eval `cat conftest.sh`
${RMF} conftest.c conftest.sh confft2build.h ftoption.h ftstdlib.h

if test x != "x${ac_cpp_ft_sizeof_int}" \
   -a x != x"${ac_cpp_ft_sizeof_long}"; then
  unset ft_use_autoconf_sizeof_types
else
  ft_use_autoconf_sizeof_types=yes
fi

AC_ARG_ENABLE(biarch-config,
[  --enable-biarch-config  install biarch ftconfig.h to support multiple
                          architectures by single file], [], [])

case :${ft_use_autoconf_sizeof_types}:${enable_biarch_config}: in
  :yes:yes:)
    AC_MSG_RESULT([broken but use it])
    unset ft_use_autoconf_sizeof_types
    ;;
  ::no:)
    AC_MSG_RESULT([works but ignore it])
    ft_use_autoconf_sizeof_types=yes
    ;;
  ::yes: | :::)
    AC_MSG_RESULT([yes])
    unset ft_use_autoconf_sizeof_types
    ;;
  *)
    AC_MSG_RESULT([no])
    ft_use_autoconf_sizeof_types=yes
    ;;
esac

if test x"${ft_use_autoconf_sizeof_types}" = xyes; then
  AC_DEFINE([FT_USE_AUTOCONF_SIZEOF_TYPES])
fi

CPPFLAGS="${orig_CPPFLAGS}"


# checks for library functions

# Here we check whether we can use our mmap file component.

AC_FUNC_MMAP
if test "$ac_cv_func_mmap_fixed_mapped" != yes; then
  FTSYS_SRC='$(BASE_DIR)/ftsystem.c'
else
  FTSYS_SRC='$(BUILD_DIR)/ftsystem.c'

  AC_CHECK_DECLS([munmap],
    [],
    [],
    [

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <sys/mman.h>

    ])

  FT_MUNMAP_PARAM
fi
AC_SUBST([FTSYS_SRC])

AC_CHECK_FUNCS([memcpy memmove])


# check for system zlib

# don't quote AS_HELP_STRING!
AC_ARG_WITH([zlib],
  AS_HELP_STRING([--without-zlib],
                 [use internal zlib instead of system-wide]))
if test x$with_zlib != xno && test -z "$LIBZ"; then
  AC_CHECK_LIB([z], [gzsetparams], [AC_CHECK_HEADER([zlib.h], [LIBZ='-lz'])])
fi
if test x$with_zlib != xno && test -n "$LIBZ"; then
  CFLAGS="$CFLAGS -DFT_CONFIG_OPTION_SYSTEM_ZLIB"
  LDFLAGS="$LDFLAGS $LIBZ"
  SYSTEM_ZLIB=yes
fi


# Some options handling SDKs/archs in CFLAGS should be copied
# to LDFLAGS. Apple TechNote 2137 recommends to include these
# options in CFLAGS but not in LDFLAGS.

save_config_args=$*
set dummy ${CFLAGS}
i=1
while test $i -lt $#
do
  c=$1

  case "${c}" in
  -isysroot|-arch) # options taking 1 argument
    a=$2
    AC_MSG_CHECKING([whether CFLAGS and LDFLAGS share ${c} ${a}])
    if expr " ${LDFLAGS} " : ".* ${c} *${a}.*" > /dev/null
    then
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no, copy to LDFLAGS])
      LDFLAGS="${LDFLAGS} ${c} ${a}"
    fi
    shift 1
    ;;
  -m32|-m64) # options taking no argument
    AC_MSG_RESULT([whether CFLAGS and LDFLAGS share ${c}])
    if expr " ${LDFLAGS} " : ".* ${c} *${a}.*" > /dev/null
    then
      AC_MSG_RESULT([yes])
    else
      AC_MSG_RESULT([no, copy to LDFLAGS])
      LDFLAGS="${LDFLAGS} ${c}"
    fi
    ;;
  # *)
  #   AC_MSG_RESULT([${c} is not copied to LDFLAGS])
  #   ;;
  esac

  shift 1
done
set ${save_config_args}


# Whether to use Mac OS resource-based fonts.

ftmac_c="" # src/base/ftmac.c should not be included in makefiles by default

# don't quote AS_HELP_STRING!
AC_ARG_WITH([old-mac-fonts],
  AS_HELP_STRING([--with-old-mac-fonts],
                 [allow Mac resource-based fonts to be used]))
if test x$with_old_mac_fonts = xyes; then
  orig_LDFLAGS="${LDFLAGS}"
  AC_MSG_CHECKING([CoreServices & ApplicationServices of Mac OS X])
  FT2_EXTRA_LIBS="-Wl,-framework,CoreServices -Wl,-framework,ApplicationServices"
  LDFLAGS="$LDFLAGS $FT2_EXTRA_LIBS"
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
#endif

      ],
      [

        short res = 0;


        UseResFile( res );

      ])],
    [AC_MSG_RESULT([ok])
     ftmac_c='ftmac.c'
     AC_MSG_CHECKING([OS_INLINE macro is ANSI compatible])
     orig_CFLAGS="$CFLAGS"
     CFLAGS="$CFLAGS $XX_CFLAGS $XX_ANSIFLAGS"
     AC_COMPILE_IFELSE([
       AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
#endif

         ],
         [

           /* OSHostByteOrder() is typed as OS_INLINE */
           int32_t  os_byte_order = OSHostByteOrder();


           if ( OSBigEndian != os_byte_order )
             return 1;

         ])],
       [AC_MSG_RESULT([ok])
        CFLAGS="$orig_CFLAGS"
        CFLAGS="$CFLAGS -DHAVE_ANSI_OS_INLINE=1"
       ],
       [AC_MSG_RESULT([no, ANSI incompatible])
        CFLAGS="$orig_CFLAGS"
       ])
     AC_MSG_CHECKING([type ResourceIndex])
     orig_CFLAGS="$CFLAGS"
     CFLAGS="$CFLAGS $XX_CFLAGS $XX_ANSIFLAGS"
     AC_COMPILE_IFELSE([
       AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
# include <Resources.h>
#endif

         ],
         [

           ResourceIndex i = 0;
           return i;

         ])],
       [AC_MSG_RESULT([ok])
        CFLAGS="$orig_CFLAGS"
        CFLAGS="$CFLAGS -DHAVE_TYPE_RESOURCE_INDEX=1"
       ],
       [AC_MSG_RESULT([no])
        CFLAGS="$orig_CFLAGS"
        CFLAGS="$CFLAGS -DHAVE_TYPE_RESOURCE_INDEX=0"
       ])],
    [AC_MSG_RESULT([not found])
     FT2_EXTRA_LIBS=""
     LDFLAGS="${orig_LDFLAGS}"
     CFLAGS="$CFLAGS -DDARWIN_NO_CARBON"])
else
  case x$host_os in
  xdarwin*)
    dnl AC_MSG_WARN([host system is MacOS but configured to build without Carbon])
    CFLAGS="$CFLAGS -DDARWIN_NO_CARBON"
    ;;
  *) ;;
  esac
fi


# Whether to use FileManager which is deprecated since Mac OS X 10.4.

AC_ARG_WITH([fsspec],
  AS_HELP_STRING([--with-fsspec],
                 [use obsolete FSSpec API of MacOS, if available (default=yes)]))
if test x$with_fsspec = xno; then
  CFLAGS="$CFLAGS -DHAVE_FSSPEC=0"
elif test x$with_old_mac_fonts = xyes -a x$with_fsspec != x; then
  AC_MSG_CHECKING([FSSpec-based FileManager])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
#endif

      ],
      [

        FCBPBPtr          paramBlock;
        short             vRefNum;
        long              dirID;
        ConstStr255Param  fileName;
        FSSpec*           spec;


        /* FSSpec functions: deprecated since Mac OS X 10.4 */
        PBGetFCBInfoSync( paramBlock );
        FSMakeFSSpec( vRefNum, dirID, fileName, spec );

      ])],
    [AC_MSG_RESULT([ok])
     CFLAGS="$CFLAGS -DHAVE_FSSPEC=1"],
    [AC_MSG_RESULT([not found])
     CFLAGS="$CFLAGS -DHAVE_FSSPEC=0"])
fi


# Whether to use FileManager in Carbon since MacOS 9.x.

AC_ARG_WITH([fsref],
  AS_HELP_STRING([--with-fsref],
                 [use Carbon FSRef API of MacOS, if available (default=yes)]))
if test x$with_fsref = xno; then
  AC_MSG_WARN([
*** WARNING
    FreeType2 built without FSRef API cannot load
    data-fork fonts on MacOS, except of XXX.dfont.
    ])
  CFLAGS="$CFLAGS -DHAVE_FSREF=0"
elif test x$with_old_mac_fonts = xyes -a x$with_fsref != x; then
  AC_MSG_CHECKING([FSRef-based FileManager])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
#endif

      ],
      [

        short                vRefNum;
        long                 dirID;
        ConstStr255Param     fileName;

        Boolean*             isDirectory;
        UInt8*               path;
        SInt16               desiredRefNum;
        SInt16*              iterator;
        SInt16*              actualRefNum;
        HFSUniStr255*        outForkName;
        FSVolumeRefNum       volume;
        FSCatalogInfoBitmap  whichInfo;
        FSCatalogInfo*       catalogInfo;
        FSForkInfo*          forkInfo;
        FSRef*               ref;

#if HAVE_FSSPEC
        FSSpec*              spec;
#endif

        /* FSRef functions: no need to check? */
        FSGetForkCBInfo( desiredRefNum, volume, iterator,
                         actualRefNum, forkInfo, ref,
                         outForkName );
        FSPathMakeRef( path, ref, isDirectory );

#if HAVE_FSSPEC
        FSpMakeFSRef ( spec, ref );
        FSGetCatalogInfo( ref, whichInfo, catalogInfo,
                          outForkName, spec, ref );
#endif
      ])],
    [AC_MSG_RESULT([ok])
     CFLAGS="$CFLAGS -DHAVE_FSREF=1"],
    [AC_MSG_RESULT([not found])
     CFLAGS="$CFLAGS -DHAVE_FSREF=0"])
fi


# Whether to use QuickDraw API in ToolBox which is deprecated since
# Mac OS X 10.4.

AC_ARG_WITH([quickdraw-toolbox],
  AS_HELP_STRING([--with-quickdraw-toolbox],
                 [use MacOS QuickDraw in ToolBox, if available (default=yes)]))
if test x$with_quickdraw_toolbox = xno; then
  CFLAGS="$CFLAGS -DHAVE_QUICKDRAW_TOOLBOX=0"
elif test x$with_old_mac_fonts = xyes -a x$with_quickdraw_toolbox != x; then
  AC_MSG_CHECKING([QuickDraw FontManager functions in ToolBox])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Fonts.h>
#endif

      ],
      [

        Str255     familyName;
        SInt16     familyID   = 0;
        FMInput*   fmIn       = NULL;
        FMOutput*  fmOut      = NULL;


        GetFontName( familyID, familyName );
        GetFNum( familyName, &familyID );
        fmOut = FMSwapFont( fmIn );

      ])],
    [AC_MSG_RESULT([ok])
     CFLAGS="$CFLAGS -DHAVE_QUICKDRAW_TOOLBOX=1"],
    [AC_MSG_RESULT([not found])
     CFLAGS="$CFLAGS -DHAVE_QUICKDRAW_TOOLBOX=0"])
fi


# Whether to use QuickDraw API in Carbon which is deprecated since
# Mac OS X 10.4.

AC_ARG_WITH([quickdraw-carbon],
  AS_HELP_STRING([--with-quickdraw-carbon],
                 [use MacOS QuickDraw in Carbon, if available (default=yes)]))
if test x$with_quickdraw_carbon = xno; then
  CFLAGS="$CFLAGS -DHAVE_QUICKDRAW_CARBON=0"
elif test x$with_old_mac_fonts = xyes -a x$with_quickdraw_carbon != x; then
  AC_MSG_CHECKING([QuickDraw FontManager functions in Carbon])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Fonts.h>
#endif

      ],
      [

        FMFontFamilyIterator          famIter;
        FMFontFamily                  family;
        Str255                        famNameStr;
        FMFontFamilyInstanceIterator  instIter;
        FMFontStyle                   style;
        FMFontSize                    size;
        FMFont                        font;
        FSSpec*                       pathSpec;


        FMCreateFontFamilyIterator( NULL, NULL, kFMUseGlobalScopeOption,
                                    &famIter );
        FMGetNextFontFamily( &famIter, &family );
        FMGetFontFamilyName( family, famNameStr );
        FMCreateFontFamilyInstanceIterator( family, &instIter );
        FMGetNextFontFamilyInstance( &instIter, &font, &style, &size );
        FMDisposeFontFamilyInstanceIterator( &instIter );
        FMDisposeFontFamilyIterator( &famIter );
        FMGetFontContainer( font, pathSpec );

      ])],
    [AC_MSG_RESULT([ok])
     CFLAGS="$CFLAGS -DHAVE_QUICKDRAW_CARBON=1"],
    [AC_MSG_RESULT([not found])
     CFLAGS="$CFLAGS -DHAVE_QUICKDRAW_CARBON=0"])
fi


# Whether to use AppleTypeService since Mac OS X.

# don't quote AS_HELP_STRING!
AC_ARG_WITH([ats],
  AS_HELP_STRING([--with-ats],
                 [use AppleTypeService, if available (default=yes)]))
if test x$with_ats = xno; then
  CFLAGS="$CFLAGS -DHAVE_ATS=0"
elif test x$with_old_mac_fonts = xyes -a x$with_ats != x ; then
  AC_MSG_CHECKING([AppleTypeService functions])
  AC_LINK_IFELSE([
    AC_LANG_PROGRAM([

#if defined(__GNUC__) && defined(__APPLE_CC__)
# include <CoreServices/CoreServices.h>
# include <ApplicationServices/ApplicationServices.h>
#else
# include <ConditionalMacros.h>
# include <Files.h>
#endif

      ],
      [

        FSSpec*  pathSpec;


        ATSFontFindFromName( NULL, kATSOptionFlagsUnRestrictedScope );
#if HAVE_FSSPEC
        ATSFontGetFileSpecification( 0, pathSpec );
#endif

      ])],
    [AC_MSG_RESULT([ok])
     CFLAGS="$CFLAGS -DHAVE_ATS=1"],
    [AC_MSG_RESULT([not found])
     CFLAGS="$CFLAGS -DHAVE_ATS=0"])
fi

case "$CFLAGS" in
  *HAVE_FSSPEC* | *HAVE_FSREF* | *HAVE_QUICKDRAW* | *HAVE_ATS* )
    AC_MSG_WARN([
*** WARNING
    FSSpec/FSRef/QuickDraw/ATS options are explicitly given,
    thus it is recommended to replace src/base/ftmac.c by builds/mac/ftmac.c.
    ])
    CFLAGS="$CFLAGS "'-I$(TOP_DIR)/builds/mac/'
    ;;
  *)
    ;;
esac


AC_SUBST([ftmac_c])
AC_SUBST([LIBZ])
AC_SUBST([CFLAGS])
AC_SUBST([LDFLAGS])
AC_SUBST([FT2_EXTRA_LIBS])
AC_SUBST([SYSTEM_ZLIB])


LT_INIT(win32-dll)

AC_SUBST([hardcode_libdir_flag_spec])
AC_SUBST([wl])
AC_SUBST([build_libtool_libs])


# configuration file -- stay in 8.3 limit
#
# since #undef doesn't survive in configuration header files we replace
# `/undef' with `#undef' after creating the output file

AC_CONFIG_HEADERS([ftconfig.h:ftconfig.in],
  [mv ftconfig.h ftconfig.tmp
   sed 's|/undef|#undef|' < ftconfig.tmp > ftconfig.h
   rm ftconfig.tmp])

# create the Unix-specific sub-Makefiles `builds/unix/unix-def.mk'
# and `builds/unix/unix-cc.mk' that will be used by the build system
#
AC_CONFIG_FILES([unix-cc.mk:unix-cc.in
                 unix-def.mk:unix-def.in
                 freetype-config
                 freetype2.pc:freetype2.in])

# re-generate the Jamfile to use libtool now
#
# AC_CONFIG_FILES([../../Jamfile:../../Jamfile.in])

AC_OUTPUT

# end of configure.raw
