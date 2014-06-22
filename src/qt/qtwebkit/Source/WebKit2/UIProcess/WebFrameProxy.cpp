/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "WebFrameProxy.h"

#include "WebCertificateInfo.h"
#include "WebContext.h"
#include "WebFormSubmissionListenerProxy.h"
#include "WebFramePolicyListenerProxy.h"
#include "WebPageMessages.h"
#include "WebPageProxy.h"
#include <WebCore/DOMImplementation.h>
#include <WebCore/Image.h>
#include <WebCore/MIMETypeRegistry.h>
#include <stdio.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

WebFrameProxy::WebFrameProxy(WebPageProxy* page, uint64_t frameID)
    : m_page(page)
    , m_loadState(LoadStateFinished)
    , m_isFrameSet(false)
    , m_frameID(frameID)
{
    WebContext::statistics().wkFrameCount++;
}

WebFrameProxy::~WebFrameProxy()
{
    WebContext::statistics().wkFrameCount--;
}

void WebFrameProxy::disconnect()
{
    m_page = 0;

    if (m_activeListener) {
        m_activeListener->invalidate();
        m_activeListener = 0;
    }
}

bool WebFrameProxy::isMainFrame() const
{
    if (!m_page)
        return false;

    return this == m_page->mainFrame();
}

void WebFrameProxy::stopLoading() const
{
    if (!m_page)
        return;

    if (!m_page->isValid())
        return;

    m_page->process()->send(Messages::WebPage::StopLoadingFrame(m_frameID), m_page->pageID());
}
    
bool WebFrameProxy::canProvideSource() const
{
    return isDisplayingMarkupDocument();
}

bool WebFrameProxy::canShowMIMEType(const String& mimeType) const
{
    if (!m_page)
        return false;

    return m_page->canShowMIMEType(mimeType);
}

bool WebFrameProxy::isDisplayingStandaloneImageDocument() const
{
    return Image::supportsType(m_MIMEType);
}

bool WebFrameProxy::isDisplayingMarkupDocument() const
{
    // FIXME: This check should be moved to somewhere in WebCore.
    return m_MIMEType == "text/html" || m_MIMEType == "image/svg+xml" || m_MIMEType == "application/x-webarchive" || DOMImplementation::isXMLMIMEType(m_MIMEType);
}

bool WebFrameProxy::isDisplayingPDFDocument() const
{
    if (m_MIMEType.isEmpty())
        return false;

    return MIMETypeRegistry::isPDFOrPostScriptMIMEType(m_MIMEType);
}

void WebFrameProxy::didStartProvisionalLoad(const String& url)
{
    ASSERT(m_provisionalURL.isEmpty());
    m_loadState = LoadStateProvisional;
    m_provisionalURL = url;
}

void WebFrameProxy::didReceiveServerRedirectForProvisionalLoad(const String& url)
{
    ASSERT(m_loadState == LoadStateProvisional);
    m_provisionalURL = url;
}

void WebFrameProxy::didFailProvisionalLoad()
{
    ASSERT(m_loadState == LoadStateProvisional);
    m_loadState = LoadStateFinished;
    m_provisionalURL = String();
    m_unreachableURL = m_lastUnreachableURL;
}

void WebFrameProxy::didCommitLoad(const String& contentType, const PlatformCertificateInfo& certificateInfo)
{
    ASSERT(m_loadState == LoadStateProvisional);
    m_loadState = LoadStateCommitted;
    m_url = m_provisionalURL;
    m_provisionalURL = String();
    m_title = String();
    m_MIMEType = contentType;
    m_isFrameSet = false;
    m_certificateInfo = WebCertificateInfo::create(certificateInfo);
}

void WebFrameProxy::didFinishLoad()
{
    ASSERT(m_loadState == LoadStateCommitted);
    ASSERT(m_provisionalURL.isEmpty());
    m_loadState = LoadStateFinished;
}

void WebFrameProxy::didFailLoad()
{
    ASSERT(m_loadState == LoadStateCommitted);
    ASSERT(m_provisionalURL.isEmpty());
    m_loadState = LoadStateFinished;
}

void WebFrameProxy::didSameDocumentNavigation(const String& url)
{
    m_url = url;
}

void WebFrameProxy::didChangeTitle(const String& title)
{
    m_title = title;
}

void WebFrameProxy::receivedPolicyDecision(WebCore::PolicyAction action, uint64_t listenerID)
{
    if (!m_page)
        return;

    ASSERT(m_activeListener);
    ASSERT(m_activeListener->listenerID() == listenerID);
    m_page->receivedPolicyDecision(action, this, listenerID);
}

WebFramePolicyListenerProxy* WebFrameProxy::setUpPolicyListenerProxy(uint64_t listenerID)
{
    if (m_activeListener)
        m_activeListener->invalidate();
    m_activeListener = WebFramePolicyListenerProxy::create(this, listenerID);
    return static_cast<WebFramePolicyListenerProxy*>(m_activeListener.get());
}

WebFormSubmissionListenerProxy* WebFrameProxy::setUpFormSubmissionListenerProxy(uint64_t listenerID)
{
    if (m_activeListener)
        m_activeListener->invalidate();
    m_activeListener = WebFormSubmissionListenerProxy::create(this, listenerID);
    return static_cast<WebFormSubmissionListenerProxy*>(m_activeListener.get());
}

void WebFrameProxy::getWebArchive(PassRefPtr<DataCallback> callback)
{
    if (!m_page) {
        callback->invalidate();
        return;
    }

    m_page->getWebArchiveOfFrame(this, callback);
}

void WebFrameProxy::getMainResourceData(PassRefPtr<DataCallback> callback)
{
    if (!m_page) {
        callback->invalidate();
        return;
    }

    m_page->getMainResourceDataOfFrame(this, callback);
}

void WebFrameProxy::getResourceData(WebURL* resourceURL, PassRefPtr<DataCallback> callback)
{
    if (!m_page) {
        callback->invalidate();
        return;
    }

    m_page->getResourceDataFromFrame(this, resourceURL, callback);
}

void WebFrameProxy::setUnreachableURL(const String& unreachableURL)
{
    m_lastUnreachableURL = m_unreachableURL;
    m_unreachableURL = unreachableURL;
}

} // namespace WebKit
