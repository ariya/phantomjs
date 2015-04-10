/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "WebDropSource.h"

#include "WebKitDLL.h"
#include "WebView.h"

#include <WebCore/Cursor.h>
#include <WebCore/DragActions.h>
#include <WebCore/EventHandler.h>
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformMouseEvent.h>
#include <wtf/CurrentTime.h>

#include <Windows.h>

using namespace WebCore;


HRESULT WebDropSource::createInstance(WebView* webView, IDropSource** result)
{
    if (!webView || !result)
        return E_INVALIDARG;
    *result = new WebDropSource(webView);
    return S_OK;
}

WebDropSource::WebDropSource(WebView* webView)
: m_ref(1)
, m_dropped(false) 
, m_webView(webView)
{
    gClassCount++;
    gClassNameCount.add("WebDropSource");
}

WebDropSource::~WebDropSource()
{
    gClassCount--;
    gClassNameCount.remove("WebDropSource");
}

STDMETHODIMP WebDropSource::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, IID_IUnknown) || 
        IsEqualIID(riid, IID_IDropSource)) {
        *ppvObject = this;
        AddRef();

        return S_OK;
    }

    return E_NOINTERFACE;
}

STDMETHODIMP_(ULONG) WebDropSource::AddRef(void)
{
    return InterlockedIncrement(&m_ref);
}

STDMETHODIMP_(ULONG) WebDropSource::Release(void)
{
    long c = InterlockedDecrement(&m_ref);
    if (c == 0)
        delete this;
    return c;
}

PlatformMouseEvent generateMouseEvent(WebView* webView, bool isDrag)
{
    POINTL pt;
    ::GetCursorPos((LPPOINT)&pt);
    POINTL localpt = pt;
    HWND viewWindow;
    if (SUCCEEDED(webView->viewWindow((OLE_HANDLE*)&viewWindow)))
        ::ScreenToClient(viewWindow, (LPPOINT)&localpt);
    return PlatformMouseEvent(IntPoint(localpt.x, localpt.y), IntPoint(pt.x, pt.y),
        isDrag ? LeftButton : NoButton, PlatformEvent::MouseMoved, 0, false, false, false, false, currentTime());
}

STDMETHODIMP WebDropSource::QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
{
    if (fEscapePressed || !(grfKeyState & (MK_LBUTTON|MK_RBUTTON))) {
        m_dropped = !fEscapePressed;
        return fEscapePressed? DRAGDROP_S_CANCEL : DRAGDROP_S_DROP;
    }

    return S_OK;
}

STDMETHODIMP WebDropSource::GiveFeedback(DWORD dwEffect)
{
    BOOL showCustomCursors;
    if (FAILED(WebPreferences::sharedStandardPreferences()->customDragCursorsEnabled(&showCustomCursors)))
        return DRAGDROP_S_USEDEFAULTCURSORS;

    // If we don't want to hide the stop icon, let Windows select the cursor.
    if (!showCustomCursors)
        return DRAGDROP_S_USEDEFAULTCURSORS;

    // If we are going to show something other than the not allowed arrow, then let Windows
    // show the cursor.
    if (dwEffect != DROPEFFECT_NONE)
        return DRAGDROP_S_USEDEFAULTCURSORS;
    
    HWND viewWindow;
    if (FAILED(m_webView->viewWindow(reinterpret_cast<OLE_HANDLE*>(&viewWindow))))
        return DRAGDROP_S_USEDEFAULTCURSORS;

    RECT webViewRect;
    GetWindowRect(viewWindow, &webViewRect);

    POINT cursorPoint;
    GetCursorPos(&cursorPoint);

    if (!PtInRect(&webViewRect, cursorPoint)) {
        // If our cursor is outside the bounds of the webView, we want to let Windows select the cursor.
        return DRAGDROP_S_USEDEFAULTCURSORS;
    }

    FrameView* view = m_webView->page()->mainFrame()->view();
    if (!view)
        return DRAGDROP_S_USEDEFAULTCURSORS;

    // When dragging inside a WebView and the drag is not allowed, don't show the not allowed icon,
    // instead, show the pointer cursor.   
    // FIXME <rdar://7577595>: Custom cursors aren't supported during drag and drop (default to pointer).
    view->setCursor(pointerCursor()); 
    return S_OK;
}
