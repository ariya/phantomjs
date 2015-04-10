AC_INIT(README)
AM_INIT_AUTOMAKE(harfbuzz, 0.1)
AC_PROG_CC
AC_PROG_CXX
AM_PROG_LIBTOOL
PKG_PROG_PKG_CONFIG
AM_CONFIG_HEADER(config.h)

if test "x$ac_compiler_gnu" = xyes; then
    CFLAGS="$CFLAGS -Wall -W -pedantic -ansi"
    CXXFLAGS="$CXXFLAGS -Wall -W"
fi

AC_PATH_PROG(ft_config,freetype-config,no)
if test "$ft_config" = "no"; then
    AC_MSG_ERROR([You must have freetype installed; see http://www.freetype.org/])
fi

FREETYPE_CFLAGS="`$ft_config --cflags`"
FREETYPE_LIBS="`$ft_config --libs`"

AC_SUBST(FREETYPE_LIBS)
AC_SUBST(FREETYPE_CFLAGS)

AC_ARG_ENABLE(qt, AS_HELP_STRING([--disable-qt], [Build Qt support (default: auto)]), [QT=$enableval], [QT=auto])

if test "x$QT" = xauto; then
    PKG_CHECK_MODULES(QT, [QtGui >= 4.3], [QT=yes], [QT=no])
fi
if test "x$QT" = xyes; then
    PKG_CHECK_MODULES(QT_GUI, [QtGui >= 4.3])
    PKG_CHECK_MODULES(QT_QTEST, [QtTest >= 4.3])

    _PKG_CONFIG(QT_INCDIR, [variable=includedir], [QtGui >= 4.3])
    QT_GUI_CFLAGS="$QT_GUI_CFLAGS -I$pkg_cv_QT_INCDIR/../Qt"

    AC_SUBST(QT_GUI_CFLAGS)
    AC_SUBST(QT_GUI_LIBS)
    AC_SUBST(QT_QTEST_CFLAGS)
    AC_SUBST(QT_QTEST_LIBS)

    _PKG_CONFIG(QT_MOC, [variable=moc_location], [QtGui >= 4.3])
    QT_MOC=$pkg_cv_QT_MOC
    AC_SUBST(QT_MOC)
fi
AM_CONDITIONAL(QT, [test "x$QT" = xyes])

AC_OUTPUT([
Makefile
src/Makefile
tests/Makefile
tests/linebreaking/Makefile
tests/shaping/Makefile
])
