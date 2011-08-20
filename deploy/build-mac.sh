#!/bin/bash

QT_VERSION=4.7.3
QT_FOLDER=Qt-$QT_VERSION
QT_TARBALL=qt-everywhere-opensource-src-$QT_VERSION.tar.gz

# Tip: change this to local/shared mirror
QT_URL=http://get.qt.nokia.com/qt/source/$QT_TARBALL

COMPILE_JOBS=4

# Step 1: Download Qt source tarball

[ -f $QT_TARBALL ] || echo "Downloading Qt $QT_VERSION from Nokia. Please wait..."
[ -f $QT_TARBALL ] || curl -C - -O -S $QT_URL

# Step 2: Extract Qt source

[ -d $QT_FOLDER ] && rm -rf $QT_FOLDER
echo "Extracting Qt $QT_VERSION source tarball..."
echo
tar xzf $QT_TARBALL
mv qt-everywhere-opensource-src-$QT_VERSION Qt-$QT_VERSION

# Step 3: Build Qt

cd $QT_FOLDER
echo "Building Qt $QT_VERSION. Please wait..."
echo
./configure -confirm-license -opensource -prefix . -release -opensource -shared -fast -no-qt3support -no-xmlpatterns -no-declarative -no-script -no-scripttools -graphicssystem raster -qt-zlib -qt-gif -qt-libpng -qt-libmng -qt-libjpeg -nomake examples -nomake demos -nomake docs -nomake translations -no-nis -no-cups -no-iconv -no-dbus -no-dwarf2 -platform macx-llvm
make -j$COMPILE_JOBS
cd ..

PATH=`pwd`/$QT_FOLDER/bin:$PATH
