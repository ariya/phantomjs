#!/usr/bin/env bash

cd `dirname $0`/..

echo "Building Qt and PhantomJS with debugging symbols. If you have previously" \
     "built without debugging symbols, you should run:"
echo
echo "    $ git clean -xdff"
echo

# This incantation will cause Qt and WebKit and PhantomJS to all build in "release"
# mode, with compiler optimisations, but also with debug symbols. (We will strip the
# symbols in package.sh.)
CFLAGS=-g CXXFLAGS=-g ./build.sh --confirm --qt-config '-webkit-debug' --qmake-args "QMAKE_CFLAGS=-g QMAKE_CXXFLAGS=-g" || exit 1

# Package the release tarball
rm deploy/*.tar.bz2 2>/dev/null
./deploy/package.sh || exit 1

# Build the dump_syms program for dumping breakpad debugging symbols
if [[ $OSTYPE = darwin* ]]; then
    pushd tools
    ../src/qt/bin/qmake dump-syms-mac.pro && make
    popd
else
    pushd src/breakpad
    ./configure && make || exit 1
    popd
fi

# Dump and package the breakpad debugging symbols...

./tools/dump-symbols.sh

version=$(bin/phantomjs --version | sed 's/ /-/' | sed 's/[()]//g')
if [[ $OSTYPE = darwin* ]]; then
    symbols="phantomjs-$version-macosx-symbols"
else
    symbols="phantomjs-$version-linux-$(uname -m)-symbols"
fi

cp -r symbols/ $symbols

# The minidump_stackwalk program is architecture-specific, so copy the
# binary for later use. This means that e.g. a developer on x86_64 can
# analyse a crash dump produced by a i686 user.
#
# We don't yet have a process for building minidump_stackwalk on OS X
if [[ $OSTYPE != darwin* ]]; then
    cp src/breakpad/src/processor/minidump_stackwalk $symbols

    read -r -d '' README <<EOT
These are symbols files that can be used to analyse a crash dump
produced by the corresponding binary. To generate a crash report,
run:

./minidump_stackwalk /path/to/crash.dmp .
EOT

    echo "$README" > $symbols/README
fi

tar -cjf deploy/$symbols.tar.bz2 $symbols
rm -r $symbols

echo "PhantomJS built and packaged:"
echo
cd deploy
ls -1 *.tar.bz2
