#!/usr/bin/env bash

cd `dirname $0`/..

echo "Building Qt and PhantomJS with debugging symbols. If you have previously" \
     "built without debugging symbols, you should run:"
echo
echo "    $ git clean -xdff"
echo

./build.py --confirm --release-debug "$@" || exit 1

# Package the release tarball
rm deploy/*.tar.bz2 2>/dev/null
./deploy/package.sh || exit 1

echo "PhantomJS built and packaged:"
echo
cd deploy
ls -1 *.tar.bz2
