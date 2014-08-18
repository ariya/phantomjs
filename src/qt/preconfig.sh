#!/usr/bin/env bash

set -e

if [[ $1 == "system" ]] || [[ $1 == "bundled" ]]; then
    QTDEPLIBS=$1
    shift
else
    QTDEPLIBS=bundled
fi

QT_CFG=''
QT_CFG+=' -opensource'          # Use the open-source license
QT_CFG+=' -confirm-license'     # Silently acknowledge the license confirmation
QT_CFG+=' -v'                   # Reveal what header dependencies are missing
QT_CFG+=' -static'              # No shared libraries
QT_CFG+=' -qpa phantom'         # Default to our custom QPA platform
QT_CFG+=' -release'             # Build only for release (no debugging support)
QT_CFG+=' -nomake examples'     # Don't build any examples
QT_CFG+=' -no-compile-examples' # Seriously, don't build any examples
QT_CFG+=' -nomake tools'        # Don't build the tools

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
QT_CFG+=' -no-qml-debug'

# Unnecessary Unix-specific features
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
QT_CFG+=' -no-sm'
QT_CFG+=' -no-xcb'
QT_CFG+=' -no-xcb-xlib'
QT_CFG+=' -no-xinerama'
QT_CFG+=' -no-xinput2'
QT_CFG+=' -no-xkb'
QT_CFG+=' -no-xrender'

# These are also unnecessary, but it's not possible to turn them off.
#QT_CFG+=' -no-pulseaudio'
#QT_CFG+=' -no-xlib'

# Explicitly compile with support for OpenSSL enabled, so the build
# will fail if headers are missing.
QT_CFG+=' -openssl'

# ICU support in QtBase is reported to be unnecessary for Darwin.
if [[ $OSTYPE != darwin* ]]; then
    QT_CFG+=' -icu'
fi

# PCRE cannot be disabled, even though WebKit has its own regex
# engine.  The Qt probe for system PCRE is hardwired to look for
# -lpcre16, which is ancient and not present on current Linux
# installs.
QT_CFG+=' -qt-pcre'

if [[ $QTDEPLIBS == bundled ]]; then
    # Use the bundled libraries.
    # Note: as best I can tell, webkitcore has a hard dependency on sqlite.
    QT_CFG+=' -no-pkg-config'
    QT_CFG+=' -qt-freetype'
    QT_CFG+=' -qt-harfbuzz'
    QT_CFG+=' -qt-libjpeg'
    QT_CFG+=' -qt-libpng'
    QT_CFG+=' -qt-sql-sqlite'
    QT_CFG+=' -qt-zlib'

    # There is no bundled copy of fontconfig, and if activated it will
    # pull in the system copies of several of the above libraries.
    QT_CFG+=' -no-fontconfig'

else
    # Use system-provided libraries.
    QT_CFG+=' -system-freetype'
    QT_CFG+=' -system-harfbuzz'
    QT_CFG+=' -system-libjpeg'
    QT_CFG+=' -system-libpng'
    QT_CFG+=' -system-sqlite -sql-sqlite'
    QT_CFG+=' -system-zlib'

    # Fontconfig is reported to not work correctly on Darwin.
    if [[ $OSTYPE != darwin* ]]; then
        QT_CFG+=' -fontconfig'
    fi
fi

# Useless styles
QT_CFG+=' -D QT_NO_STYLESHEET'
QT_CFG+=' -D QT_NO_STYLE_CDE'
QT_CFG+=' -D QT_NO_STYLE_CLEANLOOKS'
QT_CFG+=' -D QT_NO_STYLE_MOTIF'
QT_CFG+=' -D QT_NO_STYLE_PLASTIQUE'

# Qt's configure's idea of "silent" is still quite noisy.
case "$*" in
    (*-silent*) exec >& /dev/null ;;
esac

cd qtbase
exec ./configure -prefix $PWD $QT_CFG "$@"
