#!/bin/bash

QT_FOLDER=$PWD/../src/qt

rm -rf $QT_FOLDER
rsync --files-from=filelist.txt ~/temp/qt $QT_FOLDER/
cp preconfig.sh $QT_FOLDER/
cp qscriptengine.h $QT_FOLDER/src/script/api/
