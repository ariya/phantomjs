#!/usr/bin/env bash

set -e

QT_CFG=''
QT_CFG+=' -opensource'          # Use the open-source license
QT_CFG+=' -confirm-license'     # Silently acknowledge the license confirmation
QT_CFG+=' -v'                   # Makes it easier to see what header dependencies are missing
QT_CFG+=' -static'              # No shared libraries
QT_CFG+=' -qpa phantom'         # Default to our custom QPA platform
QT_CFG+=' -release'             # Build only for release (no debugging support)
QT_CFG+=' -nomake examples'     # Don't build any examples
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
QT_CFG+=' -no-pkg-config'
QT_CFG+=' -no-sm'
QT_CFG+=' -no-xcb'
QT_CFG+=' -no-xcb-xlib'
QT_CFG+=' -no-xinerama'
QT_CFG+=' -no-xkb'

# Use the bundled libraries, vs system-installed
# Note: pcre cannot be disabled, even though webkit has its own regex engine
# Note: as best I can tell, webkitcore has a hard dependency on sqlite
QT_CFG+=' -qt-harfbuzz'
QT_CFG+=' -qt-libjpeg'
QT_CFG+=' -qt-libpng'
QT_CFG+=' -qt-pcre'
QT_CFG+=' -qt-sql-sqlite'
QT_CFG+=' -qt-zlib'

# Explicitly compile with support for certain optional features enabled,
# so the build will fail if headers are missing.
QT_CFG+=' -openssl'

if [[ $OSTYPE != darwin* ]]; then
    # These are reported to be unnecessary and/or not work correctly
    # on Darwin.
    QT_CFG+=' -icu'
    QT_CFG+=' -fontconfig'
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
