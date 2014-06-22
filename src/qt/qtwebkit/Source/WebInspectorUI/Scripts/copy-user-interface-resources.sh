#!/bin/sh

LICENSE=$(cat <<EOF
/*
 * Copyright (C) 2007-2013 Apple Inc. All rights reserved.
 * Copyright (C) 2009-2011 Google Inc. All rights reserved.
 * Copyright (C) 2009-2010 Joseph Pecoraro. All rights reserved.
 * Copyright (C) 2008 Matt Lilek. All rights reserved.
 * Copyright (C) 2008-2009 Anthony Ricaud <rik@webkit.org>
 * Copyright (C) 2009 280 North Inc. All Rights Reserved.
 * Copyright (C) 2010 Nikita Vasilyev. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */
EOF
)

CODE_MIRROR_LICENSE=$(echo "/*" && sed 's/^/ * /' "${SRCROOT}/UserInterface/External/CodeMirror/LICENSE" && echo " */")

if [[ ${COMBINE_INSPECTOR_RESOURCES:=YES} == "YES" ]]; then
    # Combine the JavaScript and CSS files in Production builds into single files (Main.js and Main.css).
    "${SRCROOT}/Scripts/combine-resources.pl" --input-html "${SRCROOT}/UserInterface/Main.html" --derived-sources-dir "${DERIVED_SOURCES_DIR}" --output-dir "${DERIVED_SOURCES_DIR}" --output-script-name "Main.js" --output-style-name "Main.css"

    # Combine the CodeMirror JavaScript and CSS files in Production builds into single files (CodeMirror.js and CodeMirror.css).
    "${SRCROOT}/Scripts/combine-resources.pl" --input-dir "External/CodeMirror" --input-html "${DERIVED_SOURCES_DIR}/Main.html" --input-html-dir "${SRCROOT}/UserInterface" --derived-sources-dir "${DERIVED_SOURCES_DIR}" --output-dir "${DERIVED_SOURCES_DIR}" --output-script-name "CodeMirror.js" --output-style-name "CodeMirror.css"

    # Remove console.assert calls from the Main.js file.
    "${SRCROOT}/Scripts/remove-console-asserts.pl" --input-script "${DERIVED_SOURCES_DIR}/Main.js" --output-script "${DERIVED_SOURCES_DIR}/Main.js"

    # Export the license into Main.js.
    echo "${LICENSE}" > "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Main.js"

    # Export the license into CodeMirror.js and CodeMirror.css.
    echo "${CODE_MIRROR_LICENSE}" > "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/CodeMirror.js"
    echo "${CODE_MIRROR_LICENSE}" > "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/CodeMirror.css"

    # Minify the Main.js and Main.css files, with Main.js appending to the license that was exported above.
    python "${SRCROOT}/Scripts/jsmin.py" <"${DERIVED_SOURCES_DIR}/Main.js" >>"${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Main.js"
    python "${SRCROOT}/Scripts/cssmin.py" <"${DERIVED_SOURCES_DIR}/Main.css" >"${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Main.css"

    # Minify the CodeMirror.js and CodeMirror.css files, appending to the license that was exported above.
    python "${SRCROOT}/Scripts/jsmin.py" <"${DERIVED_SOURCES_DIR}/CodeMirror.js" >>"${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/CodeMirror.js"
    python "${SRCROOT}/Scripts/cssmin.py" <"${DERIVED_SOURCES_DIR}/CodeMirror.css" >>"${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/CodeMirror.css"

    # Copy over Main.html and the Images directory.
    ditto "${DERIVED_SOURCES_DIR}/Main.html" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Main.html"
    ditto "${SRCROOT}/UserInterface/Images" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Images"

    # Copy over files that are dynamically loaded. The default InspectorBackendCommands.js and the Legacy directory.
    ditto "${SRCROOT}/UserInterface/InspectorBackendCommands.js" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/InspectorBackendCommands.js"
    ditto "${SRCROOT}/UserInterface/Legacy" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}/Legacy"
else
    # Keep the files separate for engineering builds.
    ditto "${SRCROOT}/UserInterface" "${TARGET_BUILD_DIR}/${UNLOCALIZED_RESOURCES_FOLDER_PATH}"
fi
