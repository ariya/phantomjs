#!/usr/bin/env bash

cd `dirname $0`/..

./build.sh --qt-config "-debug -webkit-debug" || exit 1

rm deploy/*.tar.bz2
./deploy/package-linux-dynamic.sh || exit 1

pushd src/breakpad
./configure && make || exit 1
popd

./tools/dump-symbols.sh

# The minidump_stackwalk program is architecture-specific, so copy the
# binary for later use. This means that e.g. a developer on x86_64 can
# analyse a crash dump produced by a i686 user.
cp src/breakpad/src/processor/minidump_stackwalk symbols/

read -r -d '' README <<EOT
These are symbols files that can be used to analyse a crash dump
produced by the corresponding binary. To generate a crash report,
run:

./minidump_stackwalk /path/to/crash.dmp .
EOT

echo "$README" > symbols/README

tar -cjf $(ls deploy/*.bz2 | sed 's/\.tar\.bz2/-symbols.tar.bz2/') symbols/

echo "PhantomJS built and packaged:"
echo
cd deploy
ls -1 *.tar.bz2
