#!/bin/sh

mkdir -p "${BUILT_PRODUCTS_DIR}/DerivedSources/WebCore"
cd "${BUILT_PRODUCTS_DIR}/DerivedSources/WebCore"

/bin/ln -sfh "${SRCROOT}/.." WebCore
export WebCore="WebCore"

if [ "${ACTION}" = "build" -o "${ACTION}" = "install" -o "${ACTION}" = "installhdrs" ]; then
    make --no-builtin-rules -f "WebCore/DerivedSources.make" -j `/usr/sbin/sysctl -n hw.availcpu`
fi
