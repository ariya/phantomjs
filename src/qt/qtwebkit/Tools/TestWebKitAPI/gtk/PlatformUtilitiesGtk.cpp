/*
 * Copyright (C) 2012 Igalia S.L.
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
#include "PlatformUtilities.h"

#include <gtk/gtk.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>

namespace TestWebKitAPI {
namespace Util {

static gboolean checkTestFinished(gpointer userData)
{
    bool* done = static_cast<bool*>(userData);

    if (*done)
        gtk_main_quit();

    return !*done;
}

void run(bool* done)
{
    g_idle_add(checkTestFinished, done);
    gtk_main();
}

void sleep(double seconds)
{
    g_usleep(seconds * 1000000);
}

static char* getFilenameFromEnvironmentVariableAsUTF8(const char* variableName)
{
    const char* value = g_getenv(variableName);
    if (!value) {
        g_printerr("%s environment variable not found\n", variableName);
        exit(1);
    }
    gsize bytesWritten;
    return g_filename_to_utf8(value, -1, 0, &bytesWritten, 0);
}

WKStringRef createInjectedBundlePath()
{
    GOwnPtr<char> injectedBundlePath(getFilenameFromEnvironmentVariableAsUTF8("TEST_WEBKIT_API_WEBKIT2_INJECTED_BUNDLE_PATH"));
    GOwnPtr<char> injectedBundleFilename(g_build_filename(injectedBundlePath.get(), "libTestWebKitAPIInjectedBundle.la", NULL));
    return WKStringCreateWithUTF8CString(injectedBundleFilename.get());
}

WKURLRef createURLForResource(const char* resource, const char* extension)
{
    GOwnPtr<char> testResourcesPath(getFilenameFromEnvironmentVariableAsUTF8("TEST_WEBKIT_API_WEBKIT2_RESOURCES_PATH"));
    GOwnPtr<char> resourceBasename(g_strdup_printf("%s.%s", resource, extension));
    GOwnPtr<char> resourceFilename(g_build_filename(testResourcesPath.get(), resourceBasename.get(), NULL));
    GRefPtr<GFile> resourceFile = adoptGRef(g_file_new_for_path(resourceFilename.get()));
    GOwnPtr<char> resourceURI(g_file_get_uri(resourceFile.get()));
    return WKURLCreateWithUTF8CString(resourceURI.get());
}

WKURLRef URLForNonExistentResource()
{
    return WKURLCreateWithUTF8CString("file:///does-not-exist.html");
}

bool isKeyDown(WKNativeEventPtr event)
{
    return event->type == GDK_KEY_PRESS;
}

} // namespace Util
} // namespace TestWebKitAPI
