#!/bin/sh

# Copy all the Inspector front-end resources.
ditto "${SRCROOT}/../inspector/front-end" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector"
ditto "${BUILT_PRODUCTS_DIR}/DerivedSources/WebCore/InspectorBackendStub.js" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector"

# Remove the WebKit.qrc file since it is not used on the Mac (this file is for Qt).
rm -f "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector/WebKit.qrc"

# Remove *.re2js files, they are only used to generate some .js files.
rm -f "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector/"*.re2js

# Remove any .svn directories that may have been copied over.
find "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/inspector" -name ".svn" -type d | xargs rm -rf
