#!/bin/sh

# Don't do anything for Debug builds, so the Inspector is easier to debug.
if [[ ${CONFIGURATION:=Debug} == "Debug" ]]; then
    exit
fi

# Combine all script resources in the inspector.html file.
"$SRCROOT/../inspector/combine-javascript-resources.pl" --input-html "${SRCROOT}/../inspector/front-end/inspector.html" --generated-scripts-dir ${BUILT_PRODUCTS_DIR}/DerivedSources/WebCore --output-dir "${DERIVED_FILE_DIR}/WebCore" --output-script-name inspector.js

# Inline script imports in HeapSnapshotWorker.js file.
"$SRCROOT/../inspector/inline-javascript-imports.py" "${SRCROOT}/../inspector/front-end/HeapSnapshotWorker.js" "${SRCROOT}/../inspector/front-end" "${DERIVED_FILE_DIR}/WebCore/HeapSnapshotWorker.js"

# Inline script imports in ScriptFormatterWorker.js file.
"$SRCROOT/../inspector/inline-javascript-imports.py" "${SRCROOT}/../inspector/front-end/ScriptFormatterWorker.js" "${SRCROOT}/../inspector/front-end" "${DERIVED_FILE_DIR}/WebCore/scriptFormatterWorker.js"

if [ -d "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector" ]; then
    # Remove any JavaScript files, since they will be replaced with the combined files.
    cd "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector"
    rm *.js

    # Copy the modified HTML file and the combined scripts.
    cp "${DERIVED_FILE_DIR}/WebCore/inspector.html" inspector.html
    cp "${DERIVED_FILE_DIR}/WebCore/inspector.js" inspector.js
    cp "${DERIVED_FILE_DIR}/WebCore/HeapSnapshotWorker.js" HeapSnapshotWorker.js
    cp "${DERIVED_FILE_DIR}/WebCore/scriptFormatterWorker.js" scriptFormatterWorker.js
fi
