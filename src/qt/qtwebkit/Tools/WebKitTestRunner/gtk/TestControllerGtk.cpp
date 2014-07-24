/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestController.h"

#include <gtk/gtk.h>
#include <wtf/Platform.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WTR {

static guint gTimeoutSourceId = 0;

static void cancelTimeout()
{
    if (!gTimeoutSourceId)
        return;
    g_source_remove(gTimeoutSourceId);
    gTimeoutSourceId = 0;
}

void TestController::notifyDone()
{
    gtk_main_quit();
    cancelTimeout();
}

void TestController::platformInitialize()
{
}

void TestController::platformDestroy()
{
}

static gboolean timeoutCallback(gpointer)
{
    fprintf(stderr, "FAIL: TestControllerRunLoop timed out.\n");
    gtk_main_quit();
    return FALSE;
}

void TestController::platformRunUntil(bool&, double timeout)
{
    cancelTimeout();
    if (timeout != m_noTimeout)
        gTimeoutSourceId = g_timeout_add(timeout * 1000, timeoutCallback, 0);
    gtk_main();
}

static char* getEnvironmentVariableAsUTF8String(const char* variableName)
{
    const char* value = g_getenv(variableName);
    if (!value) {
        fprintf(stderr, "%s environment variable not found\n", variableName);
        exit(0);
    }
    gsize bytesWritten;
    return g_filename_to_utf8(value, -1, 0, &bytesWritten, 0);
}

void TestController::initializeInjectedBundlePath()
{
    GOwnPtr<char> utf8BundlePath(getEnvironmentVariableAsUTF8String("TEST_RUNNER_INJECTED_BUNDLE_FILENAME"));
    m_injectedBundlePath.adopt(WKStringCreateWithUTF8CString(utf8BundlePath.get()));
}

void TestController::initializeTestPluginDirectory()
{
    GOwnPtr<char> testPluginPath(getEnvironmentVariableAsUTF8String("TEST_RUNNER_TEST_PLUGIN_PATH"));
    m_testPluginDirectory.adopt(WKStringCreateWithUTF8CString(testPluginPath.get()));
}

void TestController::platformInitializeContext()
{
}

void TestController::runModal(PlatformWebView*)
{
    // FIXME: Need to implement this to test showModalDialog.
}

const char* TestController::platformLibraryPathForTesting()
{
    return 0;
}

} // namespace WTR
