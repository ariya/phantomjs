/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc.  All rights reserved.
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
#include "WebInspectorClient.h"

#if ENABLE(INSPECTOR)

#include "WebInspectorDelegate.h"
#include "WebKit.h"
#include "WebMutableURLRequest.h"
#include "WebNodeHighlight.h"
#include "WebView.h"

#include <WebCore/BString.h>
#include <WebCore/Element.h>
#include <WebCore/FloatRect.h>
#include <WebCore/FrameView.h>
#include <WebCore/InspectorController.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/Page.h>
#include <WebCore/RenderObject.h>
#include <WebCore/WindowMessageBroadcaster.h>

#include <wchar.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/StringConcatenate.h>

using namespace WebCore;

static LPCTSTR kWebInspectorWindowClassName = TEXT("WebInspectorWindowClass");
static ATOM registerWindowClass();
static LPCTSTR kWebInspectorPointerProp = TEXT("WebInspectorPointer");

static const IntRect& defaultWindowRect()
{
    static IntRect rect(60, 200, 750, 650);
    return rect;
}

static CFBundleRef getWebKitBundle()
{
    return CFBundleGetBundleWithIdentifier(CFSTR("com.apple.WebKit"));
}

WebInspectorClient::WebInspectorClient(WebView* webView)
    : m_inspectedWebView(webView)
    , m_frontendPage(0)
    , m_frontendClient(0)
{
    ASSERT(m_inspectedWebView);
    m_inspectedWebView->viewWindow((OLE_HANDLE*)&m_inspectedWebViewHwnd);
}

WebInspectorClient::~WebInspectorClient()
{
}

void WebInspectorClient::inspectorDestroyed()
{
    closeInspectorFrontend();
    delete this;
}

WebCore::InspectorFrontendChannel* WebInspectorClient::openInspectorFrontend(InspectorController* inspectorController)
{
    registerWindowClass();

    HWND frontendHwnd = ::CreateWindowEx(0, kWebInspectorWindowClassName, 0, WS_OVERLAPPEDWINDOW,
        defaultWindowRect().x(), defaultWindowRect().y(), defaultWindowRect().width(), defaultWindowRect().height(),
        0, 0, 0, 0);

    if (!frontendHwnd)
        return 0;

    COMPtr<WebView> frontendWebView(AdoptCOM, WebView::createInstance());

    if (FAILED(frontendWebView->setHostWindow((OLE_HANDLE)(ULONG64)frontendHwnd)))
        return 0;

    RECT rect;
    GetClientRect(frontendHwnd, &rect);
    if (FAILED(frontendWebView->initWithFrame(rect, 0, 0)))
        return 0;

    COMPtr<WebInspectorDelegate> delegate(AdoptCOM, WebInspectorDelegate::createInstance());
    if (FAILED(frontendWebView->setUIDelegate(delegate.get())))
        return 0;

    // Keep preferences separate from the rest of the client, making sure we are using expected preference values.
    // FIXME: It's crazy that we have to do this song and dance to end up with
    // a private WebPreferences object, even within WebKit. We should make this
    // process simpler, and consider whether we can make it simpler for WebKit
    // clients as well.
    COMPtr<WebPreferences> tempPreferences(AdoptCOM, WebPreferences::createInstance());
    COMPtr<IWebPreferences> iPreferences;
    if (FAILED(tempPreferences->initWithIdentifier(BString(L"WebInspectorPreferences"), &iPreferences)))
        return 0;
    COMPtr<WebPreferences> preferences(Query, iPreferences);
    if (!preferences)
        return 0;
    if (FAILED(preferences->setAutosaves(FALSE)))
        return 0;
    if (FAILED(preferences->setLoadsImagesAutomatically(TRUE)))
        return 0;
    if (FAILED(preferences->setAuthorAndUserStylesEnabled(TRUE)))
        return 0;
    if (FAILED(preferences->setAllowsAnimatedImages(TRUE)))
        return 0;
    if (FAILED(preferences->setLoadsImagesAutomatically(TRUE)))
        return 0;
    if (FAILED(preferences->setPlugInsEnabled(FALSE)))
        return 0;
    if (FAILED(preferences->setJavaEnabled(FALSE)))
        return 0;
    if (FAILED(preferences->setUserStyleSheetEnabled(FALSE)))
        return 0;
    if (FAILED(preferences->setTabsToLinks(FALSE)))
        return 0;
    if (FAILED(preferences->setMinimumFontSize(0)))
        return 0;
    if (FAILED(preferences->setMinimumLogicalFontSize(9)))
        return 0;
    if (FAILED(preferences->setFixedFontFamily(BString(L"Courier New"))))
        return 0;
    if (FAILED(preferences->setDefaultFixedFontSize(13)))
        return 0;

    if (FAILED(frontendWebView->setPreferences(preferences.get())))
        return 0;

    frontendWebView->setProhibitsMainFrameScrolling(TRUE);

    HWND frontendWebViewHwnd;
    if (FAILED(frontendWebView->viewWindow(reinterpret_cast<OLE_HANDLE*>(&frontendWebViewHwnd))))
        return 0;

    COMPtr<WebMutableURLRequest> request(AdoptCOM, WebMutableURLRequest::createInstance());

    RetainPtr<CFURLRef> htmlURLRef = adoptCF(CFBundleCopyResourceURL(getWebKitBundle(), CFSTR("inspector"), CFSTR("html"), CFSTR("inspector")));
    if (!htmlURLRef)
        return 0;

    CFStringRef urlStringRef = ::CFURLGetString(htmlURLRef.get());
    if (FAILED(request->initWithURL(BString(urlStringRef), WebURLRequestUseProtocolCachePolicy, 60)))
        return 0;

    if (FAILED(frontendWebView->topLevelFrame()->loadRequest(request.get())))
        return 0;

    m_frontendPage = core(frontendWebView.get());
    OwnPtr<WebInspectorFrontendClient> frontendClient = adoptPtr(new WebInspectorFrontendClient(m_inspectedWebView, m_inspectedWebViewHwnd, frontendHwnd, frontendWebView, frontendWebViewHwnd, this, createFrontendSettings()));
    m_frontendClient = frontendClient.get();
    m_frontendPage->inspectorController()->setInspectorFrontendClient(frontendClient.release());
    m_frontendHwnd = frontendHwnd;
    return this;
}

void WebInspectorClient::closeInspectorFrontend()
{
    if (m_frontendClient)
        m_frontendClient->destroyInspectorView(false);
}

void WebInspectorClient::bringFrontendToFront()
{
    m_frontendClient->bringToFront();
}

void WebInspectorClient::highlight()
{
    bool creatingHighlight = !m_highlight;

    if (creatingHighlight)
        m_highlight = adoptPtr(new WebNodeHighlight(m_inspectedWebView));

    if (m_highlight->isShowing())
        m_highlight->update();
    else
        m_highlight->setShowsWhileWebViewIsVisible(true);

    if (creatingHighlight && IsWindowVisible(m_frontendHwnd))
        m_highlight->placeBehindWindow(m_frontendHwnd);
}

void WebInspectorClient::hideHighlight()
{
    if (m_highlight)
        m_highlight->setShowsWhileWebViewIsVisible(false);
}

void WebInspectorClient::updateHighlight()
{
    if (m_highlight && m_highlight->isShowing())
        m_highlight->update();
}

void WebInspectorClient::releaseFrontend()
{
    m_frontendClient = 0;
    m_frontendPage = 0;
    m_frontendHwnd = 0;
}

WebInspectorFrontendClient::WebInspectorFrontendClient(WebView* inspectedWebView, HWND inspectedWebViewHwnd, HWND frontendHwnd, const COMPtr<WebView>& frontendWebView, HWND frontendWebViewHwnd, WebInspectorClient* inspectorClient, PassOwnPtr<Settings> settings)
    : InspectorFrontendClientLocal(inspectedWebView->page()->inspectorController(),  core(frontendWebView.get()), settings)
    , m_inspectedWebView(inspectedWebView)
    , m_inspectedWebViewHwnd(inspectedWebViewHwnd)
    , m_inspectorClient(inspectorClient)
    , m_frontendHwnd(frontendHwnd)
    , m_frontendWebView(frontendWebView)
    , m_frontendWebViewHwnd(frontendWebViewHwnd)
    , m_attached(false)
    , m_destroyingInspectorView(false)
{
    ::SetProp(frontendHwnd, kWebInspectorPointerProp, reinterpret_cast<HANDLE>(this));
    // FIXME: Implement window size/position save/restore
#if 0
    [self setWindowFrameAutosaveName:@"Web Inspector"];
#endif
}

WebInspectorFrontendClient::~WebInspectorFrontendClient()
{
    destroyInspectorView(true);
}

void WebInspectorFrontendClient::frontendLoaded()
{
    InspectorFrontendClientLocal::frontendLoaded();

    if (m_attached)
        restoreAttachedWindowHeight();

    setAttachedWindow(m_attached ? DOCKED_TO_BOTTOM : UNDOCKED);
}

String WebInspectorFrontendClient::localizedStringsURL()
{
    RetainPtr<CFURLRef> url = adoptCF(CFBundleCopyResourceURL(getWebKitBundle(), CFSTR("localizedStrings"), CFSTR("js"), 0));
    if (!url)
        return String();

    return CFURLGetString(url.get());
}

void WebInspectorFrontendClient::bringToFront()
{
    showWindowWithoutNotifications();
}

void WebInspectorFrontendClient::closeWindow()
{
    destroyInspectorView(true);
}

void WebInspectorFrontendClient::attachWindow(DockSide)
{
    if (m_attached)
        return;

    m_inspectorClient->setInspectorStartsAttached(true);

    closeWindowWithoutNotifications();
    // We need to set the attached window's height before we actually attach the window.
    // Make sure that m_attached is true so that calling setAttachedWindowHeight from restoreAttachedWindowHeight doesn't return early. 
    m_attached = true;
    // Immediately after calling showWindowWithoutNotifications(), the parent frameview's visibleHeight incorrectly returns 0 always (Windows only).
    // We are expecting this value to be just the height of the parent window when we call restoreAttachedWindowHeight, which it is before
    // calling showWindowWithoutNotifications().
    restoreAttachedWindowHeight();
    showWindowWithoutNotifications();
}

void WebInspectorFrontendClient::detachWindow()
{
    if (!m_attached)
        return;

    m_inspectorClient->setInspectorStartsAttached(false);

    closeWindowWithoutNotifications();
    showWindowWithoutNotifications();
}

void WebInspectorFrontendClient::setAttachedWindowHeight(unsigned height)
{
    if (!m_attached)
        return;

    HWND hostWindow;
    if (!SUCCEEDED(m_inspectedWebView->hostWindow((OLE_HANDLE*)&hostWindow)))
        return;

    RECT hostWindowRect;
    GetClientRect(hostWindow, &hostWindowRect);

    RECT inspectedRect;
    GetClientRect(m_inspectedWebViewHwnd, &inspectedRect);

    int totalHeight = hostWindowRect.bottom - hostWindowRect.top;
    int webViewWidth = inspectedRect.right - inspectedRect.left;

    SetWindowPos(m_frontendWebViewHwnd, 0, 0, totalHeight - height, webViewWidth, height, SWP_NOZORDER);

    // We want to set the inspected web view height to the totalHeight, because the height adjustment
    // of the inspected web view happens in onWebViewWindowPosChanging, not here.
    SetWindowPos(m_inspectedWebViewHwnd, 0, 0, 0, webViewWidth, totalHeight, SWP_NOZORDER);

    RedrawWindow(m_frontendWebViewHwnd, 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW); 
    RedrawWindow(m_inspectedWebViewHwnd, 0, 0, RDW_INVALIDATE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

void WebInspectorFrontendClient::setAttachedWindowWidth(unsigned)
{
    notImplemented();
}

void WebInspectorFrontendClient::setToolbarHeight(unsigned)
{
    notImplemented();
}

void WebInspectorFrontendClient::inspectedURLChanged(const String& newURL)
{
    m_inspectedURL = newURL;
    updateWindowTitle();
}

void WebInspectorFrontendClient::closeWindowWithoutNotifications()
{
    if (!m_frontendHwnd)
        return;

    if (!m_attached) {
        ShowWindow(m_frontendHwnd, SW_HIDE);
        return;
    }

    ASSERT(m_frontendWebView);
    ASSERT(m_inspectedWebViewHwnd);
    ASSERT(!IsWindowVisible(m_frontendHwnd));

    // Remove the Inspector's WebView from the inspected WebView's parent window.
    WindowMessageBroadcaster::removeListener(m_inspectedWebViewHwnd, this);

    m_attached = false;

    m_frontendWebView->setHostWindow(reinterpret_cast<OLE_HANDLE>(m_frontendHwnd));

    // Make sure everything has the right size/position.
    HWND hostWindow;
    if (SUCCEEDED(m_inspectedWebView->hostWindow((OLE_HANDLE*)&hostWindow)))
        SendMessage(hostWindow, WM_SIZE, 0, 0);
}

void WebInspectorFrontendClient::showWindowWithoutNotifications()
{
    if (!m_frontendHwnd)
        return;

    ASSERT(m_frontendWebView);
    ASSERT(m_inspectedWebViewHwnd);

    bool shouldAttach = false;
    if (m_attached)
        shouldAttach = true;
    else {
        // If no preference is set - default to an attached window. This is important for inspector LayoutTests.
        // FIXME: This flag can be fetched directly from the flags storage.
        shouldAttach = m_inspectorClient->inspectorStartsAttached();

        if (shouldAttach && !canAttachWindow())
            shouldAttach = false;
    }

    if (!shouldAttach) {
        // Put the Inspector's WebView inside our window and show it.
        m_frontendWebView->setHostWindow(reinterpret_cast<OLE_HANDLE>(m_frontendHwnd));
        SendMessage(m_frontendHwnd, WM_SIZE, 0, 0);
        updateWindowTitle();

        SetWindowPos(m_frontendHwnd, HWND_TOP, 0, 0, 0, 0, SWP_SHOWWINDOW | SWP_NOMOVE | SWP_NOSIZE);
        return;
    }

    // Put the Inspector's WebView inside the inspected WebView's parent window.
    WindowMessageBroadcaster::addListener(m_inspectedWebViewHwnd, this);

    HWND hostWindow;
    if (FAILED(m_inspectedWebView->hostWindow(reinterpret_cast<OLE_HANDLE*>(&hostWindow))))
        return;

    m_frontendWebView->setHostWindow(reinterpret_cast<OLE_HANDLE>(hostWindow));

    // Then hide our own window.
    ShowWindow(m_frontendHwnd, SW_HIDE);

    m_attached = true;

    // Make sure everything has the right size/position.
    SendMessage(hostWindow, WM_SIZE, 0, 0);
    m_inspectorClient->updateHighlight();
}

void WebInspectorFrontendClient::destroyInspectorView(bool notifyInspectorController)
{
    m_inspectorClient->releaseFrontend();

    if (m_destroyingInspectorView)
        return;
    m_destroyingInspectorView = true;

    closeWindowWithoutNotifications();

    if (notifyInspectorController) {
        m_inspectedWebView->page()->inspectorController()->disconnectFrontend();
        m_inspectorClient->updateHighlight();
    }
    ::DestroyWindow(m_frontendHwnd);
}

void WebInspectorFrontendClient::updateWindowTitle()
{
    String title = makeString("Web Inspector ", static_cast<UChar>(0x2014), ' ', m_inspectedURL);
    ::SetWindowText(m_frontendHwnd, title.charactersWithNullTermination().data());
}

LRESULT WebInspectorFrontendClient::onGetMinMaxInfo(WPARAM, LPARAM lParam)
{
    MINMAXINFO* info = reinterpret_cast<MINMAXINFO*>(lParam);
    POINT size = {400, 400};
    info->ptMinTrackSize = size;

    return 0;
}

LRESULT WebInspectorFrontendClient::onSize(WPARAM, LPARAM)
{
    RECT rect;
    ::GetClientRect(m_frontendHwnd, &rect);

    ::SetWindowPos(m_frontendWebViewHwnd, 0, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);

    return 0;
}

LRESULT WebInspectorFrontendClient::onClose(WPARAM, LPARAM)
{
    ::ShowWindow(m_frontendHwnd, SW_HIDE);
    m_inspectedWebView->page()->inspectorController()->close();

    return 0;
}

LRESULT WebInspectorFrontendClient::onSetFocus()
{
    SetFocus(m_frontendWebViewHwnd);
    return 0;
}

void WebInspectorFrontendClient::onWebViewWindowPosChanging(WPARAM, LPARAM lParam)
{
    ASSERT(m_attached);

    WINDOWPOS* windowPos = reinterpret_cast<WINDOWPOS*>(lParam);
    ASSERT_ARG(lParam, windowPos);

    if (windowPos->flags & SWP_NOSIZE)
        return;

    RECT inspectorRect;
    GetClientRect(m_frontendWebViewHwnd, &inspectorRect);
    unsigned inspectorHeight = inspectorRect.bottom - inspectorRect.top;

    windowPos->cy -= inspectorHeight;

    SetWindowPos(m_frontendWebViewHwnd, 0, windowPos->x, windowPos->y + windowPos->cy, windowPos->cx, inspectorHeight, SWP_NOZORDER);
}

static LRESULT CALLBACK WebInspectorWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    WebInspectorFrontendClient* client = reinterpret_cast<WebInspectorFrontendClient*>(::GetProp(hwnd, kWebInspectorPointerProp));
    if (!client)
        return ::DefWindowProc(hwnd, msg, wParam, lParam);

    switch (msg) {
        case WM_GETMINMAXINFO:
            return client->onGetMinMaxInfo(wParam, lParam);
        case WM_SIZE:
            return client->onSize(wParam, lParam);
        case WM_CLOSE:
            return client->onClose(wParam, lParam);
        case WM_SETFOCUS:
            return client->onSetFocus();
        default:
            break;
    }

    return ::DefWindowProc(hwnd, msg, wParam, lParam);
}

void WebInspectorFrontendClient::windowReceivedMessage(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
        case WM_WINDOWPOSCHANGING:
            onWebViewWindowPosChanging(wParam, lParam);
            break;
        default:
            break;
    }
}

static ATOM registerWindowClass()
{
    static bool haveRegisteredWindowClass = false;

    if (haveRegisteredWindowClass)
        return true;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = 0;
    wcex.lpfnWndProc    = WebInspectorWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = 0;
    wcex.hIcon          = 0;
    wcex.hCursor        = LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground  = 0;
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = kWebInspectorWindowClassName;
    wcex.hIconSm        = 0;

    haveRegisteredWindowClass = true;

    return ::RegisterClassEx(&wcex);
}

#endif // ENABLE(INSPECTOR)
