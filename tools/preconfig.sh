#!/bin/bash

COMPILE_JOBS=4

export MAKEFLAGS=-j$COMPILE_JOBS

CFG=''

CFG+=' -opensource'          # Use the open-source license
CFG+=' -confirm-license'     # Silently acknowledge the license confirmation

CFG+=' -release'             # Build only for release (no debugging support)
CFG+=' -static'              # Compile for static libraries
CFG+=' -fast'                # Accelerate Makefiles generation
CFG+=' -nomake demos'        # Don't build with the demos
CFG+=' -nomake docs'         # Don't generate the documentatio
CFG+=' -nomake examples'     # Don't build any examples
CFG+=' -nomake translations' # Ignore the translations
CFG+=' -nomake tools'        # Don't built the tools

CFG+=' -no-exceptions'       # Don't use C++ exception
CFG+=' -no-stl'              # No need for STL compatibility

# Irrelevant Qt features (keep compatibility)
CFG+=' -no-avx'
CFG+=' -no-libmng'
CFG+=' -no-libtiff'
CFG+=' -no-neon'

# Unnecessary Qt modules
CFG+=' -no-declarative'
CFG+=' -no-multimedia'
CFG+=' -no-opengl'
CFG+=' -no-openvg'
CFG+=' -no-phonon'
CFG+=' -no-qt3support'
CFG+=' -no-script'
CFG+=' -no-scripttools'
CFG+=' -no-svg'
CFG+=' -no-xmlpatterns'

# Graphics
CFG+=' -graphicssystem raster' # default is the software rasterizer

# Mac
CFG+=' -cocoa'               # Cocoa only, ignore Carbon
CFG+=' -no-cups'             # Disable CUPS support
CFG+=' -no-dwarf2'

# Unix
CFG+=' -no-dbus'             # Disable D-Bus feature
CFG+=' -no-glib'             # No need for Glib integration
CFG+=' -no-gstreamer'        # Turn off GStreamer support
CFG+=' -no-gtkstyle'         # Disable theming integration with Gtk+
CFG+=' -no-sm'
CFG+=' -no-xinerama'
CFG+=' -no-xkb'

# Use the bundled libraries, vs system-installed
CFG+=' -qt-libjpeg'
CFG+=' -qt-libpng'
CFG+=' -qt-zlib'

# Useless styles
CFG+=' -D QT_NO_STYLESHEET'
CFG+=' -D QT_NO_STYLE_CDE'
CFG+=' -D QT_NO_STYLE_CLEANLOOKS'
CFG+=' -D QT_NO_STYLE_MOTIF'

./configure -prefix $PWD $CFG
make -j$COMPILE_JOBS

# Extra step to ensure the static libraries are found
cp -rp src/3rdparty/webkit/Source/JavaScriptCore/release/* lib/
cp -rp src/3rdparty/webkit/Source/WebCore/release/* lib/
