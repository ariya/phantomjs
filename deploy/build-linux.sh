#!/bin/bash

COMPILE_JOBS=4
DEPLOY_DIR=`dirname $0`

if [ "$1" = "--qt-4.8" ]
then
    QT_VERSION=4.8.0
else
    QT_VERSION=4.7.4
fi

QT_FOLDER=$DEPLOY_DIR/Qt-$QT_VERSION
echo "Building Qt $QT_VERSION"

QT_TARBALL=$DEPLOY_DIR/qt-everywhere-opensource-src-$QT_VERSION.tar.gz

# Tip: change this to local/shared mirror
QT_URL=http://get.qt.nokia.com/qt/source/qt-everywhere-opensource-src-$QT_VERSION.tar.gz


# Step 1: Download Qt source tarball
# Note: only if it does not exist yet in the current directory
if [ ! -f $QT_TARBALL ]
then
    echo "Downloading Qt $QT_VERSION from Nokia. Please wait..."
    if ! curl -C - -o $QT_TARBALL -S $QT_URL
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
tar -C $DEPLOY_DIR -xzf $QT_TARBALL
mv $DEPLOY_DIR/qt-everywhere-opensource-src-$QT_VERSION $QT_FOLDER


# Step 3: Build Qt

pushd $QT_FOLDER

EXTRA_FLAGS=""
if [ $QT_VERSION = 4.8.0 ] ; then
    echo "Patching Qt 4.8"
    patch -p1 < ../qt48_enable_debugger.patch
    patch -p1 < ../qt48_fix_inspector.patch
    patch -p1 < ../qt48_headless_and_pdf_fixes.patch
    patch -p1 < ../qt48_enable_file_input_click.patch
    # Build in lighthose mode for an x-less build
    if [ "$2" = "--headless" ] ; then
        echo "Building 4.8 in qpa headless mode"
        EXTRA_FLAGS="-qpa"
    fi
else
    echo "Patching Qt 4.7"
    patch configure ../allow-static-qtwebkit.patch
    # Qt 4.8 doesn't allow static builds of QtWebkit-2.2
    EXTRA_FLAGS="-static"
fi

patch -p1 < ../qapplication_skip_qtmenu.patch
echo "Building Qt $QT_VERSION. Please wait..."
echo
./configure -opensource -confirm-license -release -webkit -graphicssystem raster -no-exceptions -no-dbus -no-glib -no-gstreamer -no-stl -no-xmlpatterns -no-phonon -no-multimedia -no-qt3support -no-opengl -no-openvg -no-svg -no-declarative -no-gtkstyle -no-xkb -no-xinput -no-xinerama -no-sm -no-cups -qt-libpng -qt-libjpeg -no-libmng -no-libtiff -D QT_NO_STYLE_CDE -D QT_NO_STYLE_CLEANLOOKS -D QT_NO_STYLE_MOTIF -D QT_NO_STYLE_PLASTIQUE -prefix $PWD -nomake demos -nomake examples -nomake tools -nomake docs -nomake translations $EXTRA_FLAGS
make -j$COMPILE_JOBS
popd


if [ $QT_VERSION != 4.8.0 ] ; then
    # Extra step: copy JavaScriptCore/release, needed for jscore static lib
    mkdir ../JavaScriptCore
    cp -rp $QT_FOLDER/src/3rdparty/webkit/JavaScriptCore/release ../JavaScriptCore/
fi

# Step 4: Build PhantomJS

echo "Building PhantomJS. Please wait..."
echo
cd $DEPLOY_DIR/..
[ -f Makefile ] && make distclean
$QT_FOLDER/bin/qmake
make -j$COMPILE_JOBS

# Step 5: Prepare for deployment

echo "Compressing PhantomJS executable..."
echo
strip bin/phantomjs
if [ `command -v upx` ]; then
    upx -9 bin/phantomjs
else
    echo "You don't have UPX. Consider installing it to reduce the executable size."
fi
