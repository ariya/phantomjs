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

mkdir -p symbols
rm -r symbols/*

files=""
files+="bin/phantomjs "

if [[ $OSTYPE = darwin* ]]; then
    # To compile this program, run ../src/qt/bin/qmake dump-syms-mac.pro && make from tools/
    dump_syms="tools/dump_syms.app/Contents/MacOS/dump_syms"
else
    files+="src/qt/lib/libQtCore.so.4.8.2 "
    files+="src/qt/lib/libQtWebKit.so.4.9.2 "
    files+="src/qt/lib/libQtGui.so.4.8.2 "
    files+="src/qt/lib/libQtNetwork.so.4.8.2"

    # To compile this program, run ./configure && make from src/breakpad/
    dump_syms="src/breakpad/src/tools/linux/dump_syms/dump_syms"
fi

for file in $files; do
    name=`basename $file`
    $dump_syms $file > $name.sym
    dir=symbols/$name/`head -n1 $name.sym | cut -d ' ' -f 4`
    mkdir -p $dir
    mv $name.sym $dir
done
