/*
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "WebPageProxy.h"

#include "PageClient.h"
#include "UserAgentQt.h"
#include "WebKitVersion.h"
#include "WebPageMessages.h"
#include "WebProcessProxy.h"
#include <WebCore/Editor.h>
#include <WebCore/NotImplemented.h>

#if HAVE(QTQUICK)
#include "QtNetworkReplyData.h"
#include "QtPageClient.h"
#include "qquicknetworkreply_p.h"
#endif

using namespace WebCore;

namespace WebKit {

String WebPageProxy::standardUserAgent(const String& applicationNameForUserAgent)
{
    return UserAgentQt::standardUserAgent(applicationNameForUserAgent, WEBKIT_MAJOR_VERSION, WEBKIT_MINOR_VERSION);
}

void WebPageProxy::saveRecentSearches(const String&, const Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::loadRecentSearches(const String&, Vector<String>&)
{
    notImplemented();
}

void WebPageProxy::registerApplicationScheme(const String& scheme)
{
    process()->send(Messages::WebPage::RegisterApplicationScheme(scheme), m_pageID);
}

void WebPageProxy::resolveApplicationSchemeRequest(QtNetworkRequestData request)
{
#if HAVE(QTQUICK)
    RefPtr<QtRefCountedNetworkRequestData> requestData = adoptRef(new QtRefCountedNetworkRequestData(request));
    m_applicationSchemeRequests.add(requestData);
    static_cast<QtPageClient*>(m_pageClient)->handleApplicationSchemeRequest(requestData);
#endif
}

void WebPageProxy::sendApplicationSchemeReply(const QQuickNetworkReply* reply)
{
#if HAVE(QTQUICK)
    RefPtr<QtRefCountedNetworkRequestData> requestData = reply->networkRequestData();
    if (m_applicationSchemeRequests.contains(requestData)) {
        RefPtr<QtRefCountedNetworkReplyData> replyData = reply->networkReplyData();
        process()->send(Messages::WebPage::ApplicationSchemeReply(replyData->data()), pageID());
        m_applicationSchemeRequests.remove(requestData);
    }
#endif
}

void WebPageProxy::authenticationRequiredRequest(const String& hostname, const String& realm, const String& prefilledUsername, String& username, String& password)
{
    m_pageClient->handleAuthenticationRequiredRequest(hostname, realm, prefilledUsername, username, password);
}

void WebPageProxy::proxyAuthenticationRequiredRequest(const String& hostname, uint16_t port, const String& prefilledUsername, String& username, String& password)
{
    m_pageClient->handleProxyAuthenticationRequiredRequest(hostname, port, prefilledUsername, username, password);
}

void WebPageProxy::certificateVerificationRequest(const String& hostname, bool& ignoreErrors)
{
    m_pageClient->handleCertificateVerificationRequest(hostname, ignoreErrors);
}

#if PLUGIN_ARCHITECTURE(X11)
void WebPageProxy::createPluginContainer(uint64_t& windowID)
{
    notImplemented();
}

void WebPageProxy::windowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID)
{
    notImplemented();
}
#endif

void WebPageProxy::changeSelectedIndex(int32_t selectedIndex)
{
    process()->send(Messages::WebPage::SelectedIndex(selectedIndex), m_pageID);
}

void WebPageProxy::closePopupMenu()
{
    process()->send(Messages::WebPage::HidePopupMenu(), m_pageID);
}

void WebPageProxy::willSetInputMethodState()
{
    m_pageClient->handleWillSetInputMethodState();
}

} // namespace WebKit
