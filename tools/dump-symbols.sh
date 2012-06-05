#!/bin/bash

# Generates debugging symbols for breakpad. qt and phantomjs must have
# been compiled in debug mode. So do:
#
#  $ make distclean && cd src/qt && make clean && cd ../..
#  $ ./build.sh --qt-config "-debug -webkit-debug"
#  $ tools/dump-symbols.sh
#
# To display the crash report:
#
#  $ tools/crash-report.sh /tmp/5e2cc287-96c8-7a1b-59c79999-00fa22a2.dmp

rm -r symbols/*

files=""
files+="bin/phantomjs "
files+="src/qt/lib/libQtCore.so.4.8.0 "
files+="src/qt/lib/libQtWebKit.so.4.9.0 "
files+="src/qt/lib/libQtGui.so.4.8.0 "
files+="src/qt/lib/libQtNetwork.so.4.8.0"

for file in $files; do
    name=`basename $file`
    src/breakpad/src/tools/linux/dump_syms/dump_syms $file > $name.sym
    dir=symbols/$name/`head -n1 $name.sym | cut -d ' ' -f 4`
    mkdir -p $dir
    mv $name.sym $dir
done
