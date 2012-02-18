#!/bin/sh

mkdir -p "${BUILT_PRODUCTS_DIR}/DerivedSources/JavaScriptCore/docs"
cd "${BUILT_PRODUCTS_DIR}/DerivedSources/JavaScriptCore"

/bin/ln -sfh "${SRCROOT}/.." JavaScriptCore
export JavaScriptCore="JavaScriptCore"

make --no-builtin-rules -f "JavaScriptCore/DerivedSources.make" -j `/usr/sbin/sysctl -n hw.ncpu`
