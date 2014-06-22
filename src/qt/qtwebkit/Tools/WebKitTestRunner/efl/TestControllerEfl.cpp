/*
 * Copyright (C) 2012 Samsung Electronics
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "TestController.h"

#include <Ecore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wtf/Platform.h>
#include <wtf/text/WTFString.h>

namespace WTR {

static Ecore_Timer* timer = 0;

static Eina_Bool timerFired(void*)
{
    timer = 0;
    ecore_main_loop_quit();
    return ECORE_CALLBACK_CANCEL;
}

void TestController::notifyDone()
{
    if (!timer)
        return;

    ecore_timer_del(timer);
    timer = 0;
    ecore_main_loop_quit();
}

void TestController::platformInitialize()
{
    const char* isDebugging = getenv("WEB_PROCESS_CMD_PREFIX");
    if (isDebugging && *isDebugging) {
        m_useWaitToDumpWatchdogTimer = false;
        m_forceNoTimeout = true;
    }
}

void TestController::platformDestroy()
{
}

void TestController::platformRunUntil(bool& condition, double timeout)
{
    if (timeout == m_noTimeout) {
        // Never timeout if we are debugging or not meant to timeout.
        while (!condition)
            ecore_main_loop_iterate();

        return;
    }
    timer = ecore_timer_loop_add(timeout, timerFired, 0);
    ecore_main_loop_begin();
}

static const char* getEnvironmentVariableOrExit(const char* variableName)
{
    const char* value = getenv(variableName);
    if (!value) {
        fprintf(stderr, "%s environment variable not found\n", variableName);
        exit(0);
    }

    return value;
}

void TestController::initializeInjectedBundlePath()
{
    const char* bundlePath = getEnvironmentVariableOrExit("TEST_RUNNER_INJECTED_BUNDLE_FILENAME");
    m_injectedBundlePath.adopt(WKStringCreateWithUTF8CString(bundlePath));
}

void TestController::initializeTestPluginDirectory()
{
    const char* pluginPath = getEnvironmentVariableOrExit("TEST_RUNNER_PLUGIN_PATH");
    m_testPluginDirectory.adopt(WKStringCreateWithUTF8CString(pluginPath));
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
