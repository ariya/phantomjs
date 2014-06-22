/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 
#import "config.h"
#import "WebProcessProxy.h"

#import "WebContext.h"
#import "WebPageGroup.h"
#import "WebPreferences.h"
#import "WebProcessMessages.h"
#import "WKFullKeyboardAccessWatcher.h"

namespace WebKit {

bool WebProcessProxy::fullKeyboardAccessEnabled()
{
    return [WKFullKeyboardAccessWatcher fullKeyboardAccessEnabled];
}

#if HAVE(XPC)
static bool shouldUseXPC()
{
    if (id value = [[NSUserDefaults standardUserDefaults] objectForKey:@"WebKit2UseXPCServiceForWebProcess"])
        return [value boolValue];

#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
    return true;
#else
    return false;
#endif
}
#endif

void WebProcessProxy::platformGetLaunchOptions(ProcessLauncher::LaunchOptions& launchOptions)
{
    // We want the web process to match the architecture of the UI process.
    launchOptions.architecture = ProcessLauncher::LaunchOptions::MatchCurrentArchitecture;
    launchOptions.executableHeap = false;

#if HAVE(XPC)
    launchOptions.useXPC = shouldUseXPC();
#endif
}

bool WebProcessProxy::pageIsProcessSuppressible(WebPageProxy* page)
{
    return !page->isViewVisible() && page->pageGroup()->preferences()->pageVisibilityBasedProcessSuppressionEnabled();
}

bool WebProcessProxy::allPagesAreProcessSuppressible() const
{
    return (m_processSuppressiblePages.size() == m_pageMap.size()) && !m_processSuppressiblePages.isEmpty();
}

void WebProcessProxy::updateProcessSuppressionState()
{
    if (!isValid())
        return;

    bool canEnable = m_context->canEnableProcessSuppressionForWebProcess(this);
    if (m_processSuppressionEnabled == canEnable)
        return;
    m_processSuppressionEnabled = canEnable;

    connection()->send(Messages::WebProcess::SetProcessSuppressionEnabled(m_processSuppressionEnabled), 0);
}

} // namespace WebKit
