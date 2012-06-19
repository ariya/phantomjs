#!/usr/bin/env bash

cd `dirname $0`/..

echo "Building Qt and PhantomJS in debug mode. If you have previously" \
     "built in release mode, you should run:"
echo
echo "    $ make clean && cd src/qt && make clean && cd ../.."
echo

# Build the project
./build.sh --qt-config "-debug -webkit-debug" --qmake-args "CONFIG-=release CONFIG+=debug" || exit 1

# Package the release tarball
rm deploy/*.tar.bz2 2>/dev/null
./deploy/package.sh || exit 1

# Build the dump_syms program for dumping breakpad debugging symbols
if [[ $OSTYPE = darwin* ]]; then
    pushd tools
    ../src/qt/bin/qmake dump-syms-mac.pro
    popd
else
    pushd src/breakpad
    ./configure && make || exit 1
    popd
fi

# Dump and package the breakpad debugging symbols...

./tools/dump-symbols.sh

# The minidump_stackwalk program is architecture-specific, so copy the
# binary for later use. This means that e.g. a developer on x86_64 can
# analyse a crash dump produced by a i686 user.
#
# We don't yet have a process for building minidump_stackwalk on OS X
if [[ $OSTYPE != darwin* ]]; then
    cp src/breakpad/src/processor/minidump_stackwalk symbols/

    read -r -d '' README <<EOT
These are symbols files that can be used to analyse a crash dump
produced by the corresponding binary. To generate a crash report,
run:

./minidump_stackwalk /path/to/crash.dmp .
EOT

    echo "$README" > symbols/README
fi

tar -cjf $(ls deploy/*.bz2 | sed 's/\.tar\.bz2/-symbols.tar.bz2/') symbols/

echo "PhantomJS built and packaged:"
echo
cd deploy
ls -1 *.tar.bz2
