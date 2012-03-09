#!/bin/bash

cd `dirname $0`/..

mkdir phantomjs
cp -r examples ChangeLog LICENSE.BSD README.md bin phantomjs

QT_LIB=src/qt/lib
mkdir phantomjs/lib
cp -r $QT_LIB/libQtCore.* $QT_LIB/libQtGui.* $QT_LIB/libQtNetwork.* $QT_LIB/libQtWebKit.* phantomjs/lib

chrpath -r \$ORIGIN/../lib phantomjs/bin/phantomjs
tar -czf phantomjs.tar.gz phantomjs
rm -r phantomjs/
