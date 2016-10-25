#!/usr/bin/env bash

set -e

SILENT=

# Defaults for choices between bundled/system libraries.
if [[ $OSTYPE == darwin* ]]; then
    # Fontconfig is not required on Darwin (we use Core Text for
    # font enumeration) and is reported to not work correctly.
    C_FONTCONFIG=' -no-fontconfig'

    # Use mostly bundled libraries for Darwin.
    C_FREETYPE=' -qt-freetype'
    C_LIBPNG=' -qt-libpng'
    C_LIBJPEG=' -qt-libjpeg'
    C_ZLIB=' -qt-zlib'
else
    # Fontconfig is essential on non-Darwin Unix.  It is not bundled
    # and links with the system freetype, so it is useless to avoid
    # the system freetype or its dependencies.  It is also expected
    # to be safe to use the system libjpeg on non-Darwin.
    C_FONTCONFIG=' -fontconfig'
    C_FREETYPE=' -system-freetype'
    C_LIBPNG=' -system-libpng'
    C_LIBJPEG=' -system-libjpeg'
    C_ZLIB=' -system-zlib'
fi

# These libraries are somewhat more unstable and/or inconsistently
# available; default to the bundled copies on all platforms.
C_PCRE=' -qt-pcre'
C_HARFBUZZ=' -qt-harfbuzz'
C_SQLITE=' -qt-sql-sqlite'

QT_CFG=''
while [[ -n "$1" && "$1" == --* ]]; do
    arg="$1"
    shift
    case "$arg" in
        (--)
            break
            ;;

        (--silent)
            SILENT=' -silent'
            ;;

        (--qt-config)
            QT_CFG+=" $1"
            shift
            ;;

        (--qtdeps=system)
            # Enable use of as many system libraries as possible.
            C_FREETYPE=' -system-freetype'
            C_LIBPNG=' -system-libpng'
            C_LIBJPEG=' -system-libjpeg'
            C_ZLIB=' -system-zlib'
            C_HARFBUZZ=' -system-harfbuzz'
            C_SQLITE=' -system-sqlite -sql-sqlite'

            # Qt requires the char16_t PCRE library, -lpcre16, which is not
            # present on some Linux distributions, so don't force it.
            C_PCRE=''
            ;;

        (--qtdeps=bundled)
            # Force use of as many bundled libraries as possible.
            C_FREETYPE=' -qt-freetype'
            C_LIBPNG=' -qt-libpng'
            C_LIBJPEG=' -qt-libjpeg'
            C_ZLIB=' -qt-zlib'
            C_HARFBUZZ=' -qt-harfbuzz'
            C_SQLITE=' -qt-sql-sqlite'
            C_PCRE=' -qt-pcre'
            ;;

        (--freetype=system)  C_FREETYPE=' -system-freetype' ;;
        (--freetype=bundled) C_FREETYPE=' -qt-freetype' ;;
        (--libpng=system)    C_LIBPNG=' -system-libpng' ;;
        (--libpng=bundled)   C_LIBPNG=' -qt-libpng' ;;
        (--libjpeg=system)   C_LIBJPEG=' -system-libjpeg' ;;
        (--libjpeg=bundled)  C_LIBJPEG=' -qt-libjpeg' ;;
        (--zlib=system)      C_ZLIB=' -system-zlib' ;;
        (--zlib=bundled)     C_ZLIB=' -qt-zlib' ;;
        (--harfbuzz=system)  C_HARFBUZZ=' -system-harfbuzz' ;;
        (--harfbuzz=bundled) C_HARFBUZZ=' -qt-harfbuzz' ;;
        (--sqlite=system)    C_SQLITE=' -system-sqlite -sql-sqlite' ;;
        (--sqlite=bundled)   C_SQLITE=' -qt-sql-sqlite' ;;
        (--pcre=system)      C_PCRE=' -system-pcre' ;;
        (--pcre=bundled)     C_PCRE=' -qt-pcre' ;;

        (*)
            printf 'preconfig.sh: unrecognized option: %s\n' "$arg" >&2
            exit 2 ;;
    esac
done

# Baseline Qt configuration.
QT_CFG+=' -opensource'          # Use the open-source license
QT_CFG+=' -confirm-license'     # Silently acknowledge the license confirmation
QT_CFG+=' -v'                   # Reveal what header dependencies are missing
QT_CFG+=' -static'              # No shared libraries
QT_CFG+=' -qpa phantom'         # Default to our custom QPA platform
QT_CFG+=' -nomake tools'        # Don't build the tools
QT_CFG+=' -nomake examples'     # Don't build any examples
QT_CFG+=' -no-compile-examples' # Seriously, don't build any examples

if [[ $OSTYPE == darwin* ]]; then
    QT_CFG+=' -no-c++11'        # Build fails on mac right now with C++11
fi

# Unnecessary Qt modules
QT_CFG+=' -no-opengl'
QT_CFG+=' -no-openvg'
QT_CFG+=' -no-egl'
QT_CFG+=' -no-eglfs'
QT_CFG+=' -no-sql-sqlite2'

# Unnecessary Qt features
QT_CFG+=' -D QT_NO_GRAPHICSVIEW'
QT_CFG+=' -D QT_NO_GRAPHICSEFFECT'
QT_CFG+=' -D QT_NO_STYLESHEET'
QT_CFG+=' -D QT_NO_STYLE_CDE'
QT_CFG+=' -D QT_NO_STYLE_CLEANLOOKS'
QT_CFG+=' -D QT_NO_STYLE_MOTIF'
QT_CFG+=' -D QT_NO_STYLE_PLASTIQUE'
QT_CFG+=' -no-qml-debug'

# Unnecessary Unix-specific features
QT_CFG+=' -no-alsa'
QT_CFG+=' -no-cups'
QT_CFG+=' -no-dbus'
QT_CFG+=' -no-directfb'
QT_CFG+=' -no-evdev'
QT_CFG+=' -no-glib'
QT_CFG+=' -no-gtkstyle'
QT_CFG+=' -no-kms'
QT_CFG+=' -no-libudev'
QT_CFG+=' -no-linuxfb'
QT_CFG+=' -no-mtdev'
QT_CFG+=' -no-nis'
QT_CFG+=' -no-pulseaudio'
QT_CFG+=' -no-sm'
QT_CFG+=' -no-xcb'
QT_CFG+=' -no-xcb-xlib'
QT_CFG+=' -no-xinerama'
QT_CFG+=' -no-xinput2'
QT_CFG+=' -no-xkb'
QT_CFG+=' -no-xrender'
QT_CFG+=' -no-feature-PRINTPREVIEWWIDGET'

# This is also unnecessary, but it's not possible to turn it off.
#QT_CFG+=' -no-xlib'

# QtWebkit can only detect that system sqlite is in use if pkg-config
# support is available in qmake.
if [[ $C_SQLITE == *qt* ]]; then
    QT_CFG+=' -no-pkg-config'
else
    QT_CFG+=' -pkg-config'
fi

# Explicitly compile with support for OpenSSL enabled, so the build
# will fail if headers are missing.
QT_CFG+=' -openssl -openssl-linked'

# ICU support in QtBase is reported to be unnecessary for Darwin.
if [[ $OSTYPE != darwin* ]]; then
    QT_CFG+=' -icu'
fi

# Configurable libraries.
QT_CFG+="$C_FONTCONFIG"
QT_CFG+="$C_FREETYPE"
QT_CFG+="$C_LIBPNG"
QT_CFG+="$C_LIBJPEG"
QT_CFG+="$C_ZLIB"
QT_CFG+="$C_PCRE"
QT_CFG+="$C_HARFBUZZ"
QT_CFG+="$C_SQLITE"

# Qt's configure's idea of "silent" is still quite noisy.
QT_CFG+="$SILENT"
if [[ -n "$SILENT" ]]; then
    exec >& /dev/null
fi

cd qtbase
exec ./configure -prefix $PWD $QT_CFG "$@"
