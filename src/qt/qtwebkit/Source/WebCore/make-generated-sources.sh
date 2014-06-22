#!/bin/sh
export SOURCE_ROOT=$PWD
export SRCROOT=$PWD
export WebCore=$PWD

mkdir -p DerivedSources/WebCore &&
make -C DerivedSources/WebCore -f ../../DerivedSources.make $@
