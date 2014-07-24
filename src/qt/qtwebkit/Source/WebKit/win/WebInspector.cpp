/*
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebInspector.h"

#if ENABLE(INSPECTOR)

#include "WebInspectorClient.h"
#include "WebKitDLL.h"
#include "WebView.h"
#include <WebCore/InspectorController.h>
#include <WebCore/Page.h>
#include <wtf/Assertions.h>

using namespace WebCore;

WebInspector* WebInspector::createInstance(WebView* webView, WebInspectorClient* inspectorClient)
{
    WebInspector* inspector = new WebInspector(webView, inspectorClient);
    inspector->AddRef();
    return inspector;
}

WebInspector::WebInspector(WebView* webView, WebInspectorClient* inspectorClient)
    : m_refCount(0)
    , m_webView(webView)
    , m_inspectorClient(inspectorClient)
{
    ASSERT_ARG(webView, webView);

    gClassCount++;
    gClassNameCount.add("WebInspector");
}

WebInspector::~WebInspector()
{
    gClassCount--;
    gClassNameCount.remove("WebInspector");
}

WebInspectorFrontendClient* WebInspector::frontendClient()
{
    return m_inspectorClient ? m_inspectorClient->frontendClient() : 0;
}

void WebInspector::webViewClosed()
{
    m_webView = 0;
    m_inspectorClient = 0;
}

HRESULT STDMETHODCALLTYPE WebInspector::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IWebInspector))
        *ppvObject = static_cast<IWebInspector*>(this);
    else if (IsEqualGUID(riid, IID_IWebInspectorPrivate))
        *ppvObject = static_cast<IWebInspectorPrivate*>(this);
    else if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebInspector*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebInspector::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebInspector::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT STDMETHODCALLTYPE WebInspector::show()
{
    if (m_webView)
        if (Page* page = m_webView->page())
            page->inspectorController()->show();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::showConsole()
{
    if (frontendClient())
        frontendClient()->showConsole();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::unused1()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::close()
{
    if (m_webView)
        if (Page* page = m_webView->page())
            page->inspectorController()->close();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::attach()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::detach()
{
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::isDebuggingJavaScript(BOOL* isDebugging)
{
    if (!isDebugging)
        return E_POINTER;

    *isDebugging = FALSE;

    if (!frontendClient())
        return S_OK;

    *isDebugging = frontendClient()->isDebuggingEnabled();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::toggleDebuggingJavaScript()
{
    show();

    if (!frontendClient())
        return S_OK;

    if (frontendClient()->isDebuggingEnabled())
        frontendClient()->setDebuggingEnabled(false);
    else
        frontendClient()->setDebuggingEnabled(true);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::isProfilingJavaScript(BOOL* isProfiling)
{
    if (!isProfiling)
        return E_POINTER;

    *isProfiling = FALSE;

    if (!frontendClient())
        return S_OK;

    *isProfiling = frontendClient()->isProfilingJavaScript();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::toggleProfilingJavaScript()
{
    show();

    if (!frontendClient())
        return S_OK;

    if (frontendClient()->isProfilingJavaScript())
        frontendClient()->stopProfilingJavaScript();
    else
        frontendClient()->startProfilingJavaScript();

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::isJavaScriptProfilingEnabled(BOOL* isProfilingEnabled)
{
    if (!isProfilingEnabled)
        return E_POINTER;

    *isProfilingEnabled = FALSE;

    if (!m_webView)
        return S_OK;

    Page* page = m_webView->page();
    if (!page)
        return S_OK;

    *isProfilingEnabled = page->inspectorController()->profilerEnabled();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::setJavaScriptProfilingEnabled(BOOL enabled)
{
    if (!m_webView)
        return S_OK;

    Page* page = m_webView->page();
    if (!page)
        return S_OK;

    page->inspectorController()->setProfilerEnabled(enabled);

    return S_OK;
}

HRESULT STDMETHODCALLTYPE  WebInspector::evaluateInFrontend(ULONG callId, BSTR bScript)
{
    if (!m_webView)
        return S_OK;

    Page* page = m_webView->page();
    if (!page)
        return S_OK;

    String script(bScript, SysStringLen(bScript));
    page->inspectorController()->evaluateForTestInFrontend(callId, script);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::isTimelineProfilingEnabled(BOOL* isEnabled)
{
    if (!isEnabled)
        return E_POINTER;

    *isEnabled = FALSE;

    if (!frontendClient())
        return S_OK;

    *isEnabled = frontendClient()->isTimelineProfilingEnabled();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebInspector::setTimelineProfilingEnabled(BOOL enabled)
{
    show();

    if (!frontendClient())
        return S_OK;

    frontendClient()->setTimelineProfilingEnabled(enabled);
    return S_OK;
}

#endif // ENABLE(INSPECTOR)
