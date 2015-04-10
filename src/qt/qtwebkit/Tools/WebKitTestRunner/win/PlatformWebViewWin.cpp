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
#include "PlatformWebView.h"

namespace WTR {

static LPCWSTR hostWindowClassName = L"WTRWebViewHostWindow";

static void registerWindowClass()
{
    static bool initialized;
    if (initialized)
        return;
    initialized = true;

    WNDCLASSEXW wndClass = {0};
    wndClass.cbSize = sizeof(wndClass);
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = DefWindowProcW;
    wndClass.hCursor = LoadCursor(0, IDC_ARROW);
    wndClass.hInstance = GetModuleHandle(0);
    wndClass.lpszClassName = hostWindowClassName;

    RegisterClassExW(&wndClass);
}

PlatformWebView::PlatformWebView(WKContextRef contextRef, WKPageGroupRef pageGroupRef, WKPageRef /* relatedPage */, WKDictionaryRef /*options*/)
    : m_windowIsKey(true)
{
    registerWindowClass();

    RECT viewRect = {0, 0, 800, 600};
    m_window = CreateWindowExW(0, hostWindowClassName, L"WebKitTestRunner", WS_OVERLAPPEDWINDOW, 0 /*XOFFSET*/, 0 /*YOFFSET*/, viewRect.right, viewRect.bottom, 0, 0, GetModuleHandle(0), 0);
    m_view = WKViewCreate(viewRect, contextRef, pageGroupRef, m_window);
    WKViewSetIsInWindow(m_view, true);
}

PlatformWebView::~PlatformWebView()
{
    if (::IsWindow(m_window))
        ::DestroyWindow(m_window);
    WKRelease(m_view);
}

void PlatformWebView::resizeTo(unsigned width, unsigned height)
{
    ::SetWindowPos(WKViewGetWindow(m_view), 0, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOCOPYBITS);
}

WKPageRef PlatformWebView::page()
{
    return WKViewGetPage(m_view);
}

void PlatformWebView::focus()
{
    ::SetFocus(::WKViewGetWindow(m_view));
}

WKRect PlatformWebView::windowFrame()
{
    // Implement.

    WKRect wkFrame;
    wkFrame.origin.x = 0;
    wkFrame.origin.y = 0;
    wkFrame.size.width = 0;
    wkFrame.size.height = 0;
    return wkFrame;
}

void PlatformWebView::setWindowFrame(WKRect)
{
    // Implement.
}


void PlatformWebView::addChromeInputField()
{
}

void PlatformWebView::removeChromeInputField()
{
}

void PlatformWebView::makeWebViewFirstResponder()
{
}

WKRetainPtr<WKImageRef> PlatformWebView::windowSnapshotImage()
{
    // FIXME: implement to capture pixels in the UI process,
    // which may be necessary to capture things like 3D transforms.
    return 0;
}

void PlatformWebView::didInitializeClients()
{
}

} // namespace WTR
