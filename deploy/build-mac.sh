#!/bin/bash

QT_VERSION=4.7.4
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

# Step 3: Build Qt

cd $QT_FOLDER
patch configure ../allow-static-qtwebkit.patch
patch -p1 < ../qapplication_skip_qtmenu.patch
echo "Building Qt $QT_VERSION. Please wait..."
echo
./configure -opensource -confirm-license -release -static -no-exceptions -no-stl -no-xmlpatterns -no-phonon -no-qt3support -no-opengl -no-declarative -qt-libpng -qt-libjpeg -no-libmng -no-libtiff -D QT_NO_STYLE_CDE -D QT_NO_STYLE_CLEANLOOKS -D QT_NO_STYLE_MOTIF -D QT_NO_STYLE_PLASTIQUE -cocoa -prefix $PWD -arch x86 -nomake demos -nomake examples -nomake tools
make -j$COMPILE_JOBS
cd ..

# Extra step: copy JavaScriptCore/release, needed for jscore static lib
mkdir ../JavaScriptCore
cp -rp $QT_FOLDER/src/3rdparty/webkit/JavaScriptCore/release ../JavaScriptCore/

# Step 4: Build PhantomJS

echo "Building PhantomJS. Please wait..."
echo
cd ..
[ -f Makefile ] && make distclean
deploy/$QT_FOLDER/bin/qmake
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
