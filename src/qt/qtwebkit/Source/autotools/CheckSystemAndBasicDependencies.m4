AC_CANONICAL_HOST

os_win32=no
os_linux=no
os_freebsd=no
os_gnu=no

case "$host_os" in
    mingw*)
        os_win32=yes
        ;;
    freebsd*)
        os_freebsd=yes
        ;;
    linux*)
        os_linux=yes
        os_gnu=yes
        ;;
    darwin*)
        os_darwin=yes
        ;;
    gnu*|k*bsd*-gnu*)
        os_gnu=yes
        ;;
esac

AC_PATH_PROG(PERL, perl)
if test -z "$PERL"; then
    AC_MSG_ERROR([You need 'perl' to compile WebKit])
fi

AC_PATH_PROG(PYTHON, python)
if test -z "$PYTHON"; then
    AC_MSG_ERROR([You need 'python' to compile WebKit])
fi

AC_PATH_PROG(RUBY, ruby)
if test -z "$RUBY"; then
    AC_MSG_ERROR([You need 'ruby' to compile WebKit])
fi

AC_PATH_PROG(BISON, bison)
if test -z "$BISON"; then
    AC_MSG_ERROR([You need the 'bison' parser generator to compile WebKit])
fi

AC_PATH_PROG(MV, mv)
if test -z "$MV"; then
    AC_MSG_ERROR([You need 'mv' to compile WebKit])
fi

AC_PATH_PROG(GPERF, gperf)
if test -z "$GPERF"; then
    AC_MSG_ERROR([You need the 'gperf' hash function generator to compile WebKit])
fi

AC_PATH_PROG(FLEX, flex)
if test -z "$FLEX"; then
    AC_MSG_ERROR([You need the 'flex' lexer generator to compile WebKit])
else
    FLEX_VERSION=`$FLEX --version | sed 's,.*\ \([0-9]*\.[0-9]*\.[0-9]*\)$,\1,'`
    AX_COMPARE_VERSION([2.5.33],[gt],[$FLEX_VERSION],
        AC_MSG_WARN([You need at least version 2.5.33 of the 'flex' lexer generator to compile WebKit correctly]))
fi

# If CFLAGS and CXXFLAGS are unset, default to empty.
# This is to tell automake not to include '-g' if C{XX,}FLAGS is not set.
# For more info - http://www.gnu.org/software/automake/manual/autoconf.html#C_002b_002b-Compiler
if test -z "$CXXFLAGS"; then
    CXXFLAGS=""
fi
if test -z "$CFLAGS"; then
    CFLAGS=""
fi

AC_PROG_CC
AC_PROG_CXX
AC_PROG_INSTALL
AC_SYS_LARGEFILE

# Check that an appropriate C compiler is available.
AC_LANG_PUSH([C])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#if !(defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 7) \
    && !(defined(__clang__) && __clang_major__ >= 3 && __clang_minor__ >= 0)
#error Unsupported compiler
#endif
],[])],[],[AC_MSG_ERROR([Compiler GCC >= 4.7 or Clang >= 3.0 is required for C compilation])])
AC_LANG_POP([C])

# Check that an appropriate C++ compiler is available.
AC_LANG_PUSH([C++])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
#if !(defined(__GNUG__) && defined(__GNUC__) && !defined(__clang__) && !defined(__INTEL_COMPILER) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 7) \
    && !(defined(__clang__) && __clang_major__ >= 3 && __clang_minor__ >= 0)
#error Unsupported compiler
#endif
],[])],[],[AC_MSG_ERROR([Compiler GCC >= 4.7 or Clang >= 3.0 is required for C++ compilation])])
AC_LANG_POP([C++])

# C/C++ Language Features
AC_C_CONST
AC_C_INLINE
AC_C_VOLATILE

# C/C++ Headers
AC_HEADER_STDC
AC_HEADER_STDBOOL
