PKG_PROG_PKG_CONFIG

# Check for libjpeg the way Gtk does it.
AC_CHECK_LIB(jpeg, jpeg_destroy_decompress, jpeg_ok=yes, jpeg_ok=no AC_MSG_ERROR([JPEG library (libjpeg) not found]))
if test "$jpeg_ok" = yes; then
    AC_MSG_CHECKING([for jpeglib])
    AC_TRY_CPP([
#include <stdio.h>
#undef PACKAGE
#undef VERSION
#undef HAVE_STDLIB_H
#include <jpeglib.h>
], jpeg_ok=yes, jpeg_ok=no)
    AC_MSG_RESULT($jpeg_ok)
    if test "$jpeg_ok" = yes; then
        JPEG_LIBS="-ljpeg"
        # Should we check for progressive JPEG like GTK+ as well?
    else
        AC_MSG_ERROR([JPEG library (libjpeg) not found])
    fi
fi
AC_SUBST([JPEG_LIBS])

# Check for libpng the way Gtk+ does it.
for l in libpng libpng14 libpng12; do
    AC_MSG_CHECKING(for $l)
    if $PKG_CONFIG --exists $l ; then
        AC_MSG_RESULT(yes)
        PNG_LIBS=`$PKG_CONFIG --libs $l`
        png_ok=yes
        break
    else
        AC_MSG_RESULT(no)
        png_ok=no
    fi
done
if test "$png_ok" != yes; then
    AC_CHECK_LIB(png, png_read_info, [AC_CHECK_HEADER(png.h, png_ok=yes, png_ok=no)],
        AC_MSG_ERROR([PNG library (libpng) not found]), -lz -lm)
   if test "$png_ok" = yes; then
        AC_MSG_CHECKING([for png_structp in png.h])
        AC_TRY_COMPILE([
#include <png.h>
], [png_structp pp; png_infop info; png_colorp cmap; png_create_read_struct;], png_ok=yes, png_ok=no)
        AC_MSG_RESULT($png_ok)
        if test "$png_ok" = yes; then
            PNG_LIBS='-lpng -lz'
        else
            AC_MSG_ERROR([PNG library (libpng) not found])
        fi
    else
        AC_MSG_ERROR([PNG library (libpng) not found])
    fi
fi
AC_SUBST([PNG_LIBS])

# Check for WebP Image support.
AC_CHECK_HEADERS([webp/decode.h], [WEBP_LIBS='-lwebp'], [AC_MSG_ERROR([WebP library (libwebp) not found])])
AC_SUBST([WEBP_LIBS])

if test "$os_win32" = "yes"; then
    WINMM_LIBS=-lwinmm
    SHLWAPI_LIBS=-lshlwapi
    OLE32_LIBS=-lole32
fi
AC_SUBST([WINMM_LIBS])
AC_SUBST([SHLWAPI_LIBS])
AC_SUBST([OLE32_LIBS])

case "$with_gtk" in
    2.0) GTK_REQUIRED_VERSION=gtk2_required_version
        GTK_API_VERSION=2.0
        WEBKITGTK_API_MAJOR_VERSION=1
        WEBKITGTK_API_MINOR_VERSION=0
        WEBKITGTK_API_VERSION=1.0
        WEBKITGTK_PC_NAME=webkit
        GAIL_PC_NAME=gail
        GAIL_REQUIRED_VERSION=gail2_required_version
        ;;
    3.0) GTK_REQUIRED_VERSION=gtk3_required_version
        GTK_API_VERSION=3.0
        WEBKITGTK_API_MAJOR_VERSION=3
        WEBKITGTK_API_MINOR_VERSION=0
        WEBKITGTK_API_VERSION=3.0
        WEBKITGTK_PC_NAME=webkitgtk
        GAIL_PC_NAME=gail-3.0
        GAIL_REQUIRED_VERSION=gail3_required_version
        ;;
esac
AC_SUBST([WEBKITGTK_API_MAJOR_VERSION])
AC_SUBST([WEBKITGTK_API_MINOR_VERSION])
AC_SUBST([WEBKITGTK_API_VERSION])
AC_SUBST([WEBKITGTK_PC_NAME])
AC_SUBST([GTK_API_VERSION])


# Check for glib and required utilities. This macro is named as if it interacts
# with automake, but it doesn't. Thus it doesn't need to be in the automake section.
AM_PATH_GLIB_2_0(glib_required_version, :, :, gmodule gobject gthread gio)
if test -z "$GLIB_GENMARSHAL" || test -z "$GLIB_MKENUMS"; then
    AC_MSG_ERROR([You need the GLib dev tools in your path])
fi

# GResources
GLIB_COMPILE_RESOURCES=`$PKG_CONFIG --variable glib_compile_resources gio-2.0`
AC_SUBST(GLIB_COMPILE_RESOURCES)
GLIB_GSETTINGS

# TODO: use pkg-config (after CFLAGS in their .pc files are cleaned up).
case "$host" in
    *-*-darwin*)
        UNICODE_CFLAGS="-I$srcdir/Source/JavaScriptCore/icu -I$srcdir/Source/WebCore/icu"
        UNICODE_LIBS="-licucore"
        ;;
    *-*-mingw*)
        UNICODE_CFLAGS=""
        UNICODE_LIBS="-licui18n -licuuc"
        AC_CHECK_HEADERS([unicode/uchar.h], [], [AC_MSG_ERROR([Could not find ICU headers.])])
        ;;
    *)
        AC_PATH_PROG(icu_config, icu-config, no)
        if test "$icu_config" = "no"; then
            AC_MSG_ERROR([Cannot find icu-config. The ICU library is needed.])
        fi

        # We don't use --cflags as this gives us a lot of things that we don't necessarily want,
        # like debugging and optimization flags. See man (1) icu-config for more info.
        UNICODE_CFLAGS=`$icu_config --cppflags`
        UNICODE_LIBS=`$icu_config --ldflags-libsonly`
        ;;
esac

AC_SUBST([UNICODE_CFLAGS])
AC_SUBST([UNICODE_LIBS])

PKG_CHECK_MODULES([ZLIB], [zlib])
AC_SUBST([ZLIB_CFLAGS])
AC_SUBST([ZLIB_LIBS])

GETTEXT_PACKAGE=WebKitGTK-$GTK_API_VERSION
AC_SUBST(GETTEXT_PACKAGE)

PKG_CHECK_MODULES(LIBXML, libxml-2.0 >= libxml_required_version)
AC_SUBST(LIBXML_CFLAGS)
AC_SUBST(LIBXML_LIBS)

PKG_CHECK_MODULES(PANGO, [pango >= pango_required_version pangoft2])
AC_SUBST(PANGO_CFLAGS)
AC_SUBST(PANGO_LIBS)

if test "$enable_spellcheck" = "yes"; then
    PKG_CHECK_MODULES(ENCHANT, enchant >= enchant_required_version, [], [enable_spellcheck="no"])
    AC_SUBST(ENCHANT_CFLAGS)
    AC_SUBST(ENCHANT_LIBS)
fi

PKG_CHECK_MODULES(GAIL, $GAIL_PC_NAME >= $GAIL_REQUIRED_VERSION)
AC_SUBST(GAIL_CFLAGS)
AC_SUBST(GAIL_LIBS)

# Check for target-specific dependencies.
if test "$with_target" = "directfb"; then
    PKG_CHECK_MODULES(CAIRO, cairo-directfb >= cairo_required_version)
    PKG_CHECK_MODULES(GTK, gtk+-directfb-2.0 >= $GTK_REQUIRED_VERSION)
else
    PKG_CHECK_MODULES(CAIRO, cairo >= cairo_required_version)
    PKG_CHECK_MODULES(GTK, gtk+-$GTK_API_VERSION >= $GTK_REQUIRED_VERSION)

    if test "$with_target" = "x11" && test "$os_win32" = "no"; then
        PKG_CHECK_MODULES([XT], [xt], [xt_has_pkg_config=yes], [xt_has_pkg_config=no])

        # Some old versions of Xt do not provide xt.pc, so try to link against Xt
        # and if it's installed fall back to just adding -lXt.
        if test "$xt_has_pkg_config" = "no"; then
            # Using AC_CHECK_LIB instead of AC_SEARCH_LIB is fine in this case as we don't care
            # about the XtOpenDisplay symbol but only about the existence of libXt.
            AC_CHECK_LIB([Xt], [XtOpenDisplay], [XT_CFLAGS=""; XT_LIBS="-lXt"],
                [AC_MSG_ERROR([X Toolkit Intrinsics library (libXt) not found])])
        fi

        AC_SUBST([XT_CFLAGS])
        AC_SUBST([XT_LIBS])
   fi
fi
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
AC_SUBST(CAIRO_CFLAGS)
AC_SUBST(CAIRO_LIBS)


found_opengl=no
have_glx=no
AC_CHECK_HEADERS([GL/glx.h], [have_glx="yes"], [have_glx="no"])
AC_MSG_CHECKING([whether to enable GLX support])
if test "$enable_glx" != "no"; then
    if test "$have_glx" = "no"; then
        if test "$enable_glx" = "yes"; then
            AC_MSG_ERROR([--enable-glx specified, but not available])
        else
            enable_glx=no
        fi
    elif test "$enable_gles2" != "yes"; then
        enable_glx=yes
    fi
fi
AC_MSG_RESULT([$enable_glx])

have_egl=no
AC_CHECK_HEADERS([EGL/egl.h], [have_egl="yes"], [have_egl="no"])
AC_MSG_CHECKING([whether to enable EGL support])
if test "$enable_egl" != "no"; then
    if test "$have_egl" = "no"; then
        if test "$enable_egl" = "yes"; then
            AC_MSG_ERROR([--enable-egl specified, but not available])
        else
            enable_egl=no
        fi
    else
        enable_egl=yes
    fi
fi
AC_MSG_RESULT([$enable_egl])

have_gles2=no
AC_CHECK_HEADERS([GLES2/gl2.h], [have_gles2="yes"], [have_gles2="no"])
AC_MSG_CHECKING([whether to use OpenGL ES 2 support])
if test "$enable_glx" = "yes"; then
    if test "$enable_gles2" = "yes"; then
        AC_MSG_ERROR([Cannot enable OpenGL ES 2 support with GLX])
    else
        enable_gles2=no
    fi
fi
if test "$enable_egl" = "no"; then
    if test "$enable_gles2" = "yes"; then
        AC_MSG_ERROR([Cannot enable OpenGL ES 2 support without EGL])
    else
        enable_gles2=no
    fi
fi
if test "$enable_gles2" != "no"; then
    if test "$have_gles2" = "no"; then
        if test "$enable_gles2" = "yes"; then
            AC_MSG_ERROR([--enable-gles2 specified, but not available])
        else
            enable_gles2=no
        fi
   else
        enable_gles2=yes
        found_opengl=yes
   fi
fi
AC_MSG_RESULT([$enable_gles2])

if test "$enable_gles2" != "yes"; then
    AC_CHECK_HEADERS([GL/gl.h], [found_opengl="yes"], [])
fi

if test "$found_opengl" = "yes"; then
    PKG_CHECK_MODULES([XCOMPOSITE], [xcomposite]);
    PKG_CHECK_MODULES([XDAMAGE], [xdamage]);
    AC_SUBST(XCOMPOSITE_CFLAGS)
    AC_SUBST(XCOMPOSITE_LIBS)
    AC_SUBST(XDAMAGE_CFLAGS)
    AC_SUBST(XDAMAGE_LIBS)
fi

# OpenGL is turned on by default if available (along with WebGL and accelerated compositing).
if test "$enable_webgl" = "auto"; then
    if test "$found_opengl" = yes; then
        enable_webgl="yes";
    else
        enable_webgl="no";
    fi
fi

if test "$enable_webgl" = "yes" && test "$found_opengl" != "yes"; then
    AC_MSG_ERROR([OpenGL must be active to use WebGL.])
fi;

if test "$enable_accelerated_compositing" = "yes" && test "$found_opengl" != "yes"; then
    AC_MSG_ERROR([OpenGL must be active to use accelerated compositing.])
fi

if test "$enable_accelerated_compositing" = "auto"; then
    if test "$found_opengl" = "yes"; then
        enable_accelerated_compositing="yes";
    else
        enable_accelerated_compositing="no";
    fi
fi

if test "$enable_gamepad" = "yes" && test "$os_linux" = no; then
    AC_MSG_WARN([Gamepad support is only available on Linux. Disabling Gamepad support.])
    enable_gamepad=no;
fi

# Turn off svg features if --disable-svg is requested.
if test "$enable_svg" = "no"; then
   enable_svg_fonts=no
fi

# Check for SVG features, enabling SVG if necessary.
if test "$enable_svg_fonts" = "yes" && test "$enable_svg" = "no"; then
    AC_MSG_WARN([SVG feature(s) requested but SVG is disabled.. Enabling SVG support])
    enable_svg=yes
fi

if test "$enable_opcode_stats" = "yes"; then
    if test "$enable_jit" = "yes"; then
        AC_MSG_ERROR([JIT must be disabled for Opcode stats to work.])
    fi
fi

case "$enable_jit" in
    yes) JSC_CPPFLAGS="-DENABLE_JIT=1" ;;
    no) JSC_CPPFLAGS="-DENABLE_JIT=0" ;;
    *) enable_jit="autodetect" ;;
esac
AC_SUBST(JSC_CPPFLAGS)


# Enable CSS Filters and Shaders if accelerated_compositing is turned on.
enable_css_filters=no;
enable_css_shaders=no;
AC_MSG_CHECKING([whether to enable CSS Filters and Shaders])
if test "$enable_accelerated_compositing" = "yes" && test "$found_opengl" = "yes"; then
    enable_css_filters=yes;
    enable_css_shaders=yes;
fi
AC_MSG_RESULT([$enable_css_filters])

G_IR_SCANNER=
G_IR_COMPILER=
G_IR_GENERATE=
GIRDIR=
GIRTYPELIBDIR=

if test "$enable_introspection" = "yes"; then
    PKG_CHECK_MODULES([INTROSPECTION],[gobject-introspection-1.0 >= gobject_introspection_required_version])

    G_IR_SCANNER="$($PKG_CONFIG --variable=g_ir_scanner gobject-introspection-1.0)"
    G_IR_COMPILER="$($PKG_CONFIG --variable=g_ir_compiler gobject-introspection-1.0)"
    G_IR_GENERATE="$($PKG_CONFIG --variable=g_ir_generate gobject-introspection-1.0)"
fi

AC_SUBST([G_IR_SCANNER])
AC_SUBST([G_IR_COMPILER])
AC_SUBST([G_IR_GENERATE])

PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4 >= libsoup_required_version])
AC_SUBST([LIBSOUP_CFLAGS])
AC_SUBST([LIBSOUP_LIBS])

if test "$enable_credential_storage" = "yes"; then
    PKG_CHECK_MODULES([LIBSECRET], [libsecret-1])
    AC_SUBST([LIBSECRET_CFLAGS])
    AC_SUBST([LIBSECRET_LIBS])
fi

# Check if FreeType/FontConfig are available.
if test "$with_target" = "directfb"; then
    PKG_CHECK_MODULES([FREETYPE],
        [fontconfig >= fontconfig_required_version freetype2 >= freetype2_required_version harfbuzz >= harfbuzz_required_version])
else
    PKG_CHECK_MODULES([FREETYPE],
        [cairo-ft fontconfig >= fontconfig_required_version freetype2 >= freetype2_required_version harfbuzz >= harfbuzz_required_version])
fi
# HarfBuzz 0.9.18 splits harbuzz-icu into a separate library.
# Since we support earlier HarfBuzz versions we keep this conditional for now.
if $PKG_CONFIG --atleast-version 0.9.18 harfbuzz; then
    PKG_CHECK_MODULES([HARFBUZZ_ICU], [harfbuzz-icu >= harfbuzz_required_version])
    FREETYPE_CFLAGS="$FREETYPE_CFLAGS $HARFBUZZ_ICU_CFLAGS"
    FREETYPE_LIBS="$FREETYPE_LIBS $HARFBUZZ_ICU_LIBS"
fi
AC_SUBST([FREETYPE_CFLAGS])
AC_SUBST([FREETYPE_LIBS])

# Check if SQLite3 is available. Error out only if one of the features hard-depending
# on it is enabled while SQLite3 is unavailable.
PKG_CHECK_MODULES([SQLITE3], [sqlite3 >= sqlite_required_version], [sqlite3_has_pkg_config=yes], [sqlite3_has_pkg_config=no])
if test "$sqlite3_has_pkg_config" = "no"; then
    AC_SEARCH_LIBS([sqlite3_open16], [sqlite3],
        [sqlite3_found=yes; SQLITE3_LIBS="$LIBS"; SQLITE3_CFLAGS="-I $srcdir/WebKitLibraries/WebCoreSQLite3"], [sqlite3_found=no])
fi
AC_SUBST([SQLITE3_CFLAGS])
AC_SUBST([SQLITE3_LIBS])

if (test "$sqlite3_found" = "no"); then
    AC_MSG_ERROR([SQLite3 is required for the Database related features])
fi

PKG_CHECK_MODULES([LIBXSLT],[libxslt >= libxslt_required_version])
AC_SUBST([LIBXSLT_CFLAGS])
AC_SUBST([LIBXSLT_LIBS])

# Check if geoclue is available.
if test "$enable_geolocation" = "yes"; then
    PKG_CHECK_MODULES([GEOCLUE], [geoclue])
    AC_SUBST([GEOCLUE_CFLAGS])
    AC_SUBST([GEOCLUE_LIBS])
fi

# Check for XRender under Linux/Unix. Some linkers require explicit linkage (like GNU Gold),
# so we cannot rely on GTK+ pulling XRender.
if test "$with_target" = "x11"; then
    PKG_CHECK_MODULES([XRENDER], [xrender])
    AC_SUBST([XRENDER_CFLAGS])
    AC_SUBST([XRENDER_LIBS])
fi

if test "$enable_video" = "yes" || test "$enable_web_audio" = "yes"; then
    PKG_CHECK_MODULES([GSTREAMER], [
        gstreamer-1.0 >= gstreamer_required_version
        gstreamer-plugins-base-1.0 >= gstreamer_plugins_base_required_version
        gstreamer-app-1.0
        gstreamer-audio-1.0,
        gstreamer-fft-1.0,
        gstreamer-base-1.0,
        gstreamer-pbutils-1.0,
        gstreamer-video-1.0])
    AC_SUBST([GSTREAMER_CFLAGS])
    AC_SUBST([GSTREAMER_LIBS])
fi

acceleration_description=
if test "$found_opengl" = "yes"; then
    acceleration_description="OpenGL"
    if test "$enable_gles2" = "yes"; then
        acceleration_description="$acceleration_description (gles2"
        OPENGL_LIBS="-lGLESv2"
    else
        acceleration_description="$acceleration_description (gl"
        OPENGL_LIBS="-lGL"
    fi
    if test "$enable_egl" = "yes"; then
        acceleration_description="$acceleration_description, egl"
        OPENGL_LIBS="$OPENGL_LIBS -lEGL"
    fi
    if test "$enable_glx" = "yes"; then
        acceleration_description="$acceleration_description, glx"
    fi

    # Check whether dlopen() is in the core libc like on FreeBSD, or in a separate
    # libdl like on GNU/Linux (in which case we want to link to libdl).
    AC_CHECK_FUNC([dlopen], [], [AC_CHECK_LIB([dl], [dlopen], [DLOPEN_LIBS="-ldl"])])
    AC_SUBST([DLOPEN_LIBS])

    OPENGL_LIBS="$OPENGL_LIBS $DLOPEN_LIBS"
    acceleration_description="$acceleration_description)"
fi
AC_SUBST([OPENGL_LIBS])

enable_accelerated_canvas=no
if test "$enable_accelerated_compositing" = "yes" && test "$found_opengl" = "yes"; then
    CAIRO_GL_LIBS="cairo-gl"
    if test "$enable_glx" = "yes"; then
        CAIRO_GL_LIBS="$CAIRO_GL_LIBS cairo-glx"
    fi
    if test "$enable_egl" = "yes"; then
        CAIRO_GL_LIBS="$CAIRO_GL_LIBS cairo-egl"
    fi

    # At the moment CairoGL does not add any extra cflags and libraries, so we can
    # safely ignore CAIRO_GL_LIBS and CAIRO_GL_CFLAGS for the moment.
    PKG_CHECK_MODULES(CAIRO_GL, $CAIRO_GL_LIBS, [enable_accelerated_canvas=yes], [enable_accelerated_canvas=no])
fi

if test "$enable_gamepad" = "yes"; then
    PKG_CHECK_MODULES([GAMEPAD], [gio-unix-2.0 gudev-1.0])

    AC_SUBST(GAMEPAD_CFLAGS)
    AC_SUBST(GAMEPAD_LIBS)
fi

if test "$enable_battery_status" = "yes"; then
    PKG_CHECK_MODULES([UPOWER_GLIB], [upower-glib])

    AC_SUBST(UPOWER_GLIB_CFLAGS)
    AC_SUBST(UPOWER_GLIB_LIBS)
fi

# Check whether to enable code coverage support.
if test "$enable_coverage" = "yes"; then
    COVERAGE_CFLAGS="-MD"
    COVERAGE_LDFLAGS="-ftest-coverage -fprofile-arcs"
    AC_SUBST([COVERAGE_CFLAGS])
    AC_SUBST([COVERAGE_LDFLAGS])
fi

if test "$enable_webkit2" = "yes"; then
    if test "$GTK_API_VERSION" = "2.0"; then
        AC_MSG_ERROR([WebKit2 requires GTK+ 3.x, use --with-gtk=3.0])
    fi

    # Make sure we have GTK+ 2.x to build the plugin process.
    PKG_CHECK_MODULES(GTK2, gtk+-2.0 >= gtk2_required_version gail >= gail2_required_version)
    AC_SUBST(GTK2_CFLAGS)
    AC_SUBST(GTK2_LIBS)

    # Check Unix printing
    PKG_CHECK_MODULES(GTK_UNIX_PRINTING, gtk+-unix-print-3.0, [have_gtk_unix_printing=yes], [have_gtk_unix_printing=no])
    AC_SUBST(GTK_UNIX_PRINTING_CFLAGS)
    AC_SUBST(GTK_UNIX_PRINTING_LIBS)

    # On some Linux/Unix platforms, shm_* may only be available if linking against librt
    if test "$os_win32" = "no"; then
        AC_SEARCH_LIBS([shm_open], [rt], [SHM_LIBS="-lrt"])
        AC_SUBST(SHM_LIBS)
    fi
fi

# We need atspi2 for Webkit2 unit tests.
if test "$enable_webkit2" = "yes"; then
   PKG_CHECK_MODULES([ATSPI2], [atspi-2 >= atspi2_required_version], [have_atspi2=yes], [have_atspi2=no])
   AC_SUBST([ATSPI2_CFLAGS])
   AC_SUBST([ATSPI2_LIBS])
fi

m4_ifdef([GTK_DOC_CHECK], [
GTK_DOC_CHECK([1.10])
],[
AM_CONDITIONAL([ENABLE_GTK_DOC], false)
])
