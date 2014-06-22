/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#include "config.h"
#include "WebInspector.h"

#if ENABLE(INSPECTOR)

#include <WebCore/FileSystem.h>
#include <glib.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

bool WebInspector::canSave() const
{
    return false;
}

String WebInspector::localizedStringsURL() const
{
    GOwnPtr<gchar> filePath;
    const gchar* environmentPath = g_getenv("WEBKIT_INSPECTOR_PATH");
    if (environmentPath && g_file_test(environmentPath, G_FILE_TEST_IS_DIR))
        filePath.set(g_build_filename(environmentPath, "localizedStrings.js", NULL));
    else
        filePath.set(g_build_filename(WebCore::sharedResourcesPath().data(), "webinspector", "localizedStrings.js", NULL));

    GOwnPtr<gchar> fileURI(g_filename_to_uri(filePath.get(), 0, 0));
    return WebCore::filenameToString(fileURI.get());
}

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
