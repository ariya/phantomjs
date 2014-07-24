/*
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
#include "WebInspector.h"

#if ENABLE(INSPECTOR)
#include "WebFrame.h"
#include "WebInspectorFrontendClient.h"
#include "WebInspectorProxyMessages.h"
#include "WebPage.h"
#include "WebPageCreationParameters.h"
#include "WebProcess.h"
#include <WebCore/Frame.h>
#include <WebCore/InspectorController.h>
#include <WebCore/InspectorFrontendChannel.h>
#include <WebCore/InspectorFrontendClient.h>
#include <WebCore/Page.h>
#include <WebCore/ScriptController.h>
#include <WebCore/ScriptValue.h>
#include <wtf/text/StringConcatenate.h>

using namespace WebCore;

namespace WebKit {

PassRefPtr<WebInspector> WebInspector::create(WebPage* page, InspectorFrontendChannel* frontendChannel)
{
    return adoptRef(new WebInspector(page, frontendChannel));
}

WebInspector::WebInspector(WebPage* page, InspectorFrontendChannel* frontendChannel)
    : m_page(page)
    , m_inspectorPage(0)
    , m_frontendClient(0)
    , m_frontendChannel(frontendChannel)
#if PLATFORM(MAC)
    , m_hasLocalizedStringsURL(false)
    , m_usesWebKitUserInterface(false)
#endif
#if ENABLE(INSPECTOR_SERVER)
    , m_remoteFrontendConnected(false)
#endif
{
}

// Called from WebInspectorClient
WebPage* WebInspector::createInspectorPage()
{
    if (!m_page)
        return 0;

    ASSERT(!m_inspectorPage);
    ASSERT(!m_frontendClient);

    uint64_t inspectorPageID = 0;
    WebPageCreationParameters parameters;

    if (!WebProcess::shared().parentProcessConnection()->sendSync(Messages::WebInspectorProxy::CreateInspectorPage(),
            Messages::WebInspectorProxy::CreateInspectorPage::Reply(inspectorPageID, parameters),
            m_page->pageID(), CoreIPC::Connection::NoTimeout)) {
        return 0;
    }

    if (!inspectorPageID)
        return 0;

    WebProcess::shared().createWebPage(inspectorPageID, parameters);
    m_inspectorPage = WebProcess::shared().webPage(inspectorPageID);
    ASSERT(m_inspectorPage);

    OwnPtr<WebInspectorFrontendClient> frontendClient = adoptPtr(new WebInspectorFrontendClient(m_page, m_inspectorPage));
    m_frontendClient = frontendClient.get();
    m_inspectorPage->corePage()->inspectorController()->setInspectorFrontendClient(frontendClient.release());
    return m_inspectorPage;
}

void WebInspector::destroyInspectorPage()
{
    m_inspectorPage = 0;
    m_frontendClient = 0;
    m_frontendChannel = 0;
}

// Called from WebInspectorFrontendClient
void WebInspector::didClose()
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::DidClose(), m_page->pageID());
    destroyInspectorPage();
}

void WebInspector::bringToFront()
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::BringToFront(), m_page->pageID());
}

void WebInspector::inspectedURLChanged(const String& urlString)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::InspectedURLChanged(urlString), m_page->pageID());
}

void WebInspector::save(const String& filename, const String& content, bool forceSaveAs)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::Save(filename, content, forceSaveAs), m_page->pageID());
}

void WebInspector::append(const String& filename, const String& content)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::Append(filename, content), m_page->pageID());
}

void WebInspector::attachBottom()
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::AttachBottom(), m_page->pageID());
}

void WebInspector::attachRight()
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::AttachRight(), m_page->pageID());
}

void WebInspector::detach()
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::Detach(), m_page->pageID());
}

void WebInspector::setAttachedWindowHeight(unsigned height)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::SetAttachedWindowHeight(height), m_page->pageID());
}

void WebInspector::setAttachedWindowWidth(unsigned width)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::SetAttachedWindowWidth(width), m_page->pageID());
}

void WebInspector::setToolbarHeight(unsigned height)
{
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::SetToolbarHeight(height), m_page->pageID());
}

// Called by WebInspector messages
void WebInspector::show()
{
    m_page->corePage()->inspectorController()->show();
}

void WebInspector::close()
{
    m_page->corePage()->inspectorController()->close();
}

void WebInspector::didSave(const String& url)
{
    ASSERT(m_inspectorPage);
    m_inspectorPage->corePage()->mainFrame()->script()->executeScript(makeString("InspectorFrontendAPI.savedURL(\"", url, "\")"));
}

void WebInspector::didAppend(const String& url)
{
    ASSERT(m_inspectorPage);
    m_inspectorPage->corePage()->mainFrame()->script()->executeScript(makeString("InspectorFrontendAPI.appendedToURL(\"", url, "\")"));
}

void WebInspector::attachedBottom()
{
    if (m_frontendClient)
        m_frontendClient->setAttachedWindow(InspectorFrontendClient::DOCKED_TO_BOTTOM);
}

void WebInspector::attachedRight()
{
    if (m_frontendClient)
        m_frontendClient->setAttachedWindow(InspectorFrontendClient::DOCKED_TO_RIGHT);
}

void WebInspector::detached()
{
    if (m_frontendClient)
        m_frontendClient->setAttachedWindow(InspectorFrontendClient::UNDOCKED);
}

void WebInspector::evaluateScriptForTest(long callID, const String& script)
{
    m_page->corePage()->inspectorController()->evaluateForTestInFrontend(callID, script);
}

void WebInspector::showConsole()
{
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->showConsole();
}

void WebInspector::showResources()
{
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->showResources();
}

void WebInspector::showMainResourceForFrame(uint64_t frameID)
{
    WebFrame* frame = WebProcess::shared().webFrame(frameID);
    if (!frame)
        return;

    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->showMainResourceForFrame(frame->coreFrame());
}

void WebInspector::startJavaScriptDebugging()
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->setDebuggingEnabled(true);
#endif
}

void WebInspector::stopJavaScriptDebugging()
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->setDebuggingEnabled(false);
#endif
}

void WebInspector::setJavaScriptProfilingEnabled(bool enabled)
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_page->corePage()->inspectorController()->show();
    if (!m_frontendClient)
        return;

    m_page->corePage()->inspectorController()->setProfilerEnabled(enabled);
#endif
}

void WebInspector::startJavaScriptProfiling()
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->startProfilingJavaScript();
#endif
}

void WebInspector::stopJavaScriptProfiling()
{
#if ENABLE(JAVASCRIPT_DEBUGGER)
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->stopProfilingJavaScript();
#endif
}

void WebInspector::startPageProfiling()
{
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->setTimelineProfilingEnabled(true);
}

void WebInspector::stopPageProfiling()
{
    m_page->corePage()->inspectorController()->show();
    if (m_frontendClient)
        m_frontendClient->setTimelineProfilingEnabled(false);
}

void WebInspector::updateDockingAvailability()
{
    if (!m_frontendClient)
        return;

    bool canAttachWindow = m_frontendClient->canAttachWindow();
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::AttachAvailabilityChanged(canAttachWindow), m_page->pageID());
    m_frontendClient->setDockingUnavailable(!canAttachWindow);
}

#if ENABLE(INSPECTOR_SERVER)
void WebInspector::sendMessageToRemoteFrontend(const String& message)
{
    ASSERT(m_remoteFrontendConnected);
    WebProcess::shared().parentProcessConnection()->send(Messages::WebInspectorProxy::SendMessageToRemoteFrontend(message), m_page->pageID());
}

void WebInspector::dispatchMessageFromRemoteFrontend(const String& message)
{
    m_page->corePage()->inspectorController()->dispatchMessageFromFrontend(message);
}

void WebInspector::remoteFrontendConnected()
{
    ASSERT(!m_remoteFrontendConnected);
    // Switching between in-process and remote inspectors isn't supported yet.
    ASSERT(!m_inspectorPage);
    
    m_page->corePage()->inspectorController()->connectFrontend(m_frontendChannel);
    m_remoteFrontendConnected = true;
}

void WebInspector::remoteFrontendDisconnected()
{
    ASSERT(m_remoteFrontendConnected);
    m_page->corePage()->inspectorController()->disconnectFrontend();
    m_remoteFrontendConnected = false;
}
#endif

} // namespace WebKit

#endif // ENABLE(INSPECTOR)
