#!/usr/bin/env bash

COMPILE_JOBS=4

QT_CFG=''
QT_CFG+=' -opensource'          # Use the open-source license
QT_CFG+=' -confirm-license'     # Silently acknowledge the license confirmation
QT_CFG+=' -v'                   # Makes it easier to see what header dependencies are missing

if [[ $OSTYPE = darwin* ]]; then
    QT_CFG+=' -static'          # Static build on Mac OS X only
    QT_CFG+=' -arch x86'
    QT_CFG+=' -cocoa'           # Cocoa only, ignore Carbon
    QT_CFG+=' -no-dwarf2'
else
    QT_CFG+=' -system-freetype' # Freetype for text rendering
    QT_CFG+=' -fontconfig'      # Fontconfig for better font matching
    QT_CFG+=' -qpa'             # X11-less with QPA (aka Lighthouse)
fi

QT_CFG+=' -release'             # Build only for release (no debugging support)
QT_CFG+=' -fast'                # Accelerate Makefiles generation
QT_CFG+=' -nomake demos'        # Don't build with the demos
QT_CFG+=' -nomake docs'         # Don't generate the documentatio
QT_CFG+=' -nomake examples'     # Don't build any examples
QT_CFG+=' -nomake translations' # Ignore the translations
QT_CFG+=' -nomake tools'        # Don't built the tools

QT_CFG+=' -no-exceptions'       # Don't use C++ exception
QT_CFG+=' -no-stl'              # No need for STL compatibility

# Irrelevant Qt features
QT_CFG+=' -no-libmng'
QT_CFG+=' -no-libtiff'
QT_CFG+=' -no-icu'

# Unnecessary Qt modules
QT_CFG+=' -no-declarative'
QT_CFG+=' -no-multimedia'
QT_CFG+=' -no-opengl'
QT_CFG+=' -no-openvg'
QT_CFG+=' -no-phonon'
QT_CFG+=' -no-qt3support'
QT_CFG+=' -no-script'
QT_CFG+=' -no-scripttools'
QT_CFG+=' -no-svg'
QT_CFG+=' -no-xmlpatterns'

# Unnecessary Qt features
QT_CFG+=' -D QT_NO_GRAPHICSVIEW'
QT_CFG+=' -D QT_NO_GRAPHICSEFFECT'

# Sets the default graphics system to the raster engine
QT_CFG+=' -graphicssystem raster'

# Unix
QT_CFG+=' -no-dbus'             # Disable D-Bus feature
QT_CFG+=' -no-glib'             # No need for Glib integration
QT_CFG+=' -no-gstreamer'        # Turn off GStreamer support
QT_CFG+=' -no-gtkstyle'         # Disable theming integration with Gtk+
QT_CFG+=' -no-cups'             # Disable CUPs support
QT_CFG+=' -no-sm'
QT_CFG+=' -no-xinerama'
QT_CFG+=' -no-xkb'

# Use the bundled libraries, vs system-installed
QT_CFG+=' -qt-libjpeg'
QT_CFG+=' -qt-libpng'
QT_CFG+=' -qt-zlib'

# Explicitly compile with SSL support, so build will fail if headers are missing
QT_CFG+=' -openssl'

# Useless styles
QT_CFG+=' -D QT_NO_STYLESHEET'
QT_CFG+=' -D QT_NO_STYLE_CDE'
QT_CFG+=' -D QT_NO_STYLE_CLEANLOOKS'
QT_CFG+=' -D QT_NO_STYLE_MOTIF'
QT_CFG+=' -D QT_NO_STYLE_PLASTIQUE'

until [ -z "$1" ]; do
    case $1 in
        "--qt-config")
            shift
            QT_CFG+=" $1"
            shift;;
        "--jobs")
            shift
            COMPILE_JOBS=$1
            shift;;
        "--help")
            echo "Usage: $0 [--qt-config CONFIG] [--jobs NUM]"
            echo
            echo "  --qt-config CONFIG          Specify extra config options to be used when configuring Qt"
            echo "  --jobs NUM                  How many parallel compile jobs to use. Defaults to 4."
            echo
            exit 0
            ;;
        *)
            echo "Unrecognised option: $1"
            exit 1;;
    esac
done


# For parallelizing the bootstrapping process, e.g. qmake and friends.
export MAKEFLAGS=-j$COMPILE_JOBS

./configure -prefix $PWD $QT_CFG
make -j$COMPILE_JOBS

# Extra step to ensure the static libraries are found
cp -rp src/3rdparty/webkit/Source/JavaScriptCore/release/* lib/
cp -rp src/3rdparty/webkit/Source/WebCore/release/* lib/
