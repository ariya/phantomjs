#!/bin/sh

TRACING_D="$1/runtime/Tracing.d";
TRACING_H="$BUILT_PRODUCTS_DIR/DerivedSources/JavaScriptCore/TracingDtrace.h";

if [[ "${HAVE_DTRACE}" = "1" && "${TRACING_D}" -nt "${TRACING_H}" ]]; then
    dtrace -h -o "${TRACING_H}" -s "${TRACING_D}";
fi;

