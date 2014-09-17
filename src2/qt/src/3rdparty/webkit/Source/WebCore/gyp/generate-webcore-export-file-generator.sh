#!/bin/sh

OUTPUT_DIR="${BUILT_PRODUCTS_DIR}/DerivedSources/${PROJECT_NAME}"
mkdir -p "${OUTPUT_DIR}"
"${SRCROOT}"/../make-export-file-generator "${SRCROOT}/../WebCore.exp.in" "${OUTPUT_DIR}/ExportFileGenerator.cpp"
