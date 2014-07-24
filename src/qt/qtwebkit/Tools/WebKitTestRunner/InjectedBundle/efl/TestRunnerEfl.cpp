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
#include "TestRunner.h"

#include "InjectedBundle.h"
#include <Ecore.h>
#include <JavaScriptCore/OpaqueJSString.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WTR {

static Eina_Bool waitToDumpWatchdogTimerCallback(void*)
{
    InjectedBundle::shared().testRunner()->waitToDumpWatchdogTimerFired();
    return false;
}

void TestRunner::platformInitialize()
{
    m_waitToDumpWatchdogTimer = 0;
}

void TestRunner::invalidateWaitToDumpWatchdogTimer()
{
    if (!m_waitToDumpWatchdogTimer)
        return;

    ecore_timer_del(m_waitToDumpWatchdogTimer);
    m_waitToDumpWatchdogTimer = 0;
}

void TestRunner::initializeWaitToDumpWatchdogTimerIfNeeded()
{
    if (m_waitToDumpWatchdogTimer)
        return;

    m_waitToDumpWatchdogTimer = ecore_timer_loop_add(waitToDumpWatchdogTimerInterval,
                                                     waitToDumpWatchdogTimerCallback, 0);
}

JSRetainPtr<JSStringRef> TestRunner::pathToLocalResource(JSStringRef url)
{
    String requestedUrl(url->characters(), url->length());
    String resourceRoot;
    String requestedRoot;

    if (requestedUrl.find("LayoutTests") != notFound) {
        // If the URL contains LayoutTests we need to remap that to
        // LOCAL_RESOURCE_ROOT which is the path of the LayoutTests directory
        // within the WebKit source tree.
        requestedRoot = "/tmp/LayoutTests";
        resourceRoot = getenv("LOCAL_RESOURCE_ROOT");
    } else if (requestedUrl.find("tmp") != notFound) {
        // If the URL is a child of /tmp we need to convert it to be a child
        // DUMPRENDERTREE_TEMP replace tmp with DUMPRENDERTREE_TEMP
        requestedRoot = "/tmp";
        resourceRoot = getenv("DUMPRENDERTREE_TEMP");
    }

    size_t indexOfRootStart = requestedUrl.reverseFind(requestedRoot);
    size_t indexOfSeparatorAfterRoot = indexOfRootStart + requestedRoot.length();
    String fullPathToUrl = "file://" + resourceRoot + requestedUrl.substring(indexOfSeparatorAfterRoot);

    return JSStringCreateWithUTF8CString(fullPathToUrl.utf8().data());
}

JSRetainPtr<JSStringRef> TestRunner::platformName()
{
    JSRetainPtr<JSStringRef> platformName(Adopt, JSStringCreateWithUTF8CString("efl"));
    return platformName;
}

} // namespace WTR
