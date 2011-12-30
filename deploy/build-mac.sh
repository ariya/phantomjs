#!/bin/bash

QT_VERSION=4.8.0
QT_FOLDER=Qt-$QT_VERSION
QT_TARBALL=qt-everywhere-opensource-src-$QT_VERSION.tar.gz

# Tip: change this to local/shared mirror
QT_URL=http://get.qt.nokia.com/qt/source/$QT_TARBALL

COMPILE_JOBS=4

# Step 1: Download Qt source tarball
# Note: only if it does not exist yet in the current directory

if [ ! -f $QT_TARBALL ]
then
    echo "Downloading Qt $QT_VERSION from Nokia. Please wait..."
    if ! curl -C - -O -S $QT_URL
    then
        echo
        echo "Fatal error: fail to download from $QT_URL !"
        exit 1
    fi
fi

# Step 2: Extract Qt source

[ -d $QT_FOLDER ] && rm -rf $QT_FOLDER
echo "Extracting Qt $QT_VERSION source tarball..."
echo
tar xzf $QT_TARBALL
mv qt-everywhere-opensource-src-$QT_VERSION Qt-$QT_VERSION

# Step 3: Apply some patches

cd $QT_FOLDER
patch configure ../allow-static-qtwebkit.patch
patch -p1 < ../qapplication_skip_qtmenu.patch
patch -p1 < ../disable_quicktime_video.patch
patch -p1 < ../qt48_enable_file_input_click.patch

rm -rf src/3rdparty/webkit/Source/WebKit/qt/tests

# Step 4: Build Qt

echo "Building Qt $QT_VERSION. Please wait..."
echo

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

# Irrelevant Qt features
CFG+=' -no-libmng'
CFG+=' -no-libtiff'

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

# Sets the default graphics system to the raster engine
CFG+=' -graphicssystem raster'

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

# Useless styles
CFG+=' -D QT_NO_STYLESHEET'
CFG+=' -D QT_NO_STYLE_CDE'
CFG+=' -D QT_NO_STYLE_CLEANLOOKS'
CFG+=' -D QT_NO_STYLE_MOTIF'

./configure -prefix $PWD $CFG -arch x86
make -j$COMPILE_JOBS
cd ..

# Extra step to ensure the static libraries are found
cp -rp $QT_FOLDER/src/3rdparty/webkit/Source/JavaScriptCore/release/* $QT_FOLDER/lib
cp -rp $QT_FOLDER/src/3rdparty/webkit/Source/WebCore/release/* $QT_FOLDER/lib

# Step 5: Build PhantomJS

echo "Building PhantomJS. Please wait..."
echo
cd ..
[ -f Makefile ] && make distclean
deploy/$QT_FOLDER/bin/qmake
make -j$COMPILE_JOBS

# Step 6: Prepare for deployment

echo "Compressing PhantomJS executable..."
echo
strip bin/phantomjs
if [ `command -v upx` ]; then
    upx -9 bin/phantomjs
else
    echo "You don't have UPX. Consider installing it to reduce the executable size."
fi
