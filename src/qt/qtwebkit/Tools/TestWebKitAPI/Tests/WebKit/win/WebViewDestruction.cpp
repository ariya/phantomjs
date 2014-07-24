/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "HostWindow.h"
#include "Test.h"
#include <WebCore/COMPtr.h>
#include <WebKit/WebKit.h>
#include <WebKit/WebKitCOMAPI.h>
#include <wtf/PassOwnPtr.h>

namespace TestWebKitAPI {

template <typename T>
static HRESULT WebKitCreateInstance(REFCLSID clsid, T** object)
{
    return WebKitCreateInstance(clsid, 0, __uuidof(T), reinterpret_cast<void**>(object));
}

class WebViewDestruction : public ::testing::Test {
protected:
    virtual void SetUp();
    virtual void TearDown();

    static int webViewCount();
    static void runMessagePump(DWORD timeoutMilliseconds);

    COMPtr<IWebView> m_webView;
};

class WebViewDestructionWithHostWindow : public WebViewDestruction {
protected:
    virtual void SetUp();
    virtual void TearDown();

    HostWindow m_window;
    HWND m_viewWindow;
};

void WebViewDestruction::SetUp()
{
    EXPECT_HRESULT_SUCCEEDED(WebKitCreateInstance(__uuidof(WebView), &m_webView));
}

int WebViewDestruction::webViewCount()
{
    COMPtr<IWebKitStatistics> statistics;
    if (FAILED(WebKitCreateInstance(__uuidof(WebKitStatistics), &statistics)))
        return -1;
    int count;
    if (FAILED(statistics->webViewCount(&count)))
        return -1;
    return count;
}

void WebViewDestructionWithHostWindow::SetUp()
{
    WebViewDestruction::SetUp();

    EXPECT_TRUE(m_window.initialize());
    EXPECT_HRESULT_SUCCEEDED(m_webView->setHostWindow(reinterpret_cast<OLE_HANDLE>(m_window.window())));
    EXPECT_HRESULT_SUCCEEDED(m_webView->initWithFrame(m_window.clientRect(), 0, 0));

    COMPtr<IWebViewPrivate> viewPrivate(Query, m_webView);
    ASSERT_NOT_NULL(viewPrivate);
    EXPECT_HRESULT_SUCCEEDED(viewPrivate->viewWindow(reinterpret_cast<OLE_HANDLE*>(&m_viewWindow)));
    EXPECT_TRUE(::IsWindow(m_viewWindow));
}

void WebViewDestruction::runMessagePump(DWORD timeoutMilliseconds)
{
    // FIXME: We should move this functionality to PlatformUtilities at some point.

    DWORD startTickCount = ::GetTickCount();
    MSG msg;
    BOOL result;
    while ((result = ::PeekMessageW(&msg, 0, 0, 0, PM_REMOVE)) && ::GetTickCount() - startTickCount <= timeoutMilliseconds) {
        if (result == -1)
            break;
        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

void WebViewDestruction::TearDown()
{
    // Allow window messages to be processed, because in some cases that would trigger a crash (e.g., <http://webkit.org/b/32827>).
    runMessagePump(50);

    // We haven't crashed. Release the WebView and ensure that its view window has been destroyed and the WebView doesn't leak.
    int currentWebViewCount = webViewCount();
    EXPECT_GT(currentWebViewCount, 0);

    m_webView = 0;

    EXPECT_EQ(webViewCount(), currentWebViewCount - 1);
}

void WebViewDestructionWithHostWindow::TearDown()
{
    WebViewDestruction::TearDown();

    EXPECT_FALSE(::IsWindow(m_viewWindow));
}

// Tests that releasing a WebView without calling IWebView::initWithFrame works.
TEST_F(WebViewDestruction, NoInitWithFrame)
{
}

TEST_F(WebViewDestruction, CloseWithoutInitWithFrame)
{
    EXPECT_HRESULT_SUCCEEDED(m_webView->close());
}

// Tests that calling IWebView::close without calling DestroyWindow, then releasing a WebView doesn't crash. <http://webkit.org/b/32827>
TEST_F(WebViewDestructionWithHostWindow, CloseWithoutDestroyViewWindow)
{
    EXPECT_HRESULT_SUCCEEDED(m_webView->close());
}

TEST_F(WebViewDestructionWithHostWindow, DestroyViewWindowWithoutClose)
{
    ::DestroyWindow(m_viewWindow);
}

TEST_F(WebViewDestructionWithHostWindow, CloseThenDestroyViewWindow)
{
    EXPECT_HRESULT_SUCCEEDED(m_webView->close());
    ::DestroyWindow(m_viewWindow);
}

TEST_F(WebViewDestructionWithHostWindow, DestroyViewWindowThenClose)
{
    ::DestroyWindow(m_viewWindow);
    EXPECT_HRESULT_SUCCEEDED(m_webView->close());
}

TEST_F(WebViewDestructionWithHostWindow, DestroyHostWindow)
{
    ::DestroyWindow(m_window.window());
}

TEST_F(WebViewDestructionWithHostWindow, DestroyHostWindowThenClose)
{
    ::DestroyWindow(m_window.window());
    EXPECT_HRESULT_SUCCEEDED(m_webView->close());
}

TEST_F(WebViewDestructionWithHostWindow, CloseThenDestroyHostWindow)
{
    EXPECT_HRESULT_SUCCEEDED(m_webView->close());
    ::DestroyWindow(m_window.window());
}

} // namespace WebKitAPITest
