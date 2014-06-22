/*
 * Copyright (C) 2009 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009 Brent Fulgham. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "stdafx.h"
#include "PrintWebUIDelegate.h"

#include <WebKit/WebKitCOMAPI.h>
#include <commctrl.h>
#include <commdlg.h>
#include <objbase.h>
#include <shlwapi.h>
#include <wininet.h>

static const int MARGIN = 20;

HRESULT PrintWebUIDelegate::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObject = static_cast<IWebUIDelegate*>(this);
    else if (IsEqualIID(riid, IID_IWebUIDelegate))
        *ppvObject = static_cast<IWebUIDelegate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG PrintWebUIDelegate::AddRef(void)
{
    return ++m_refCount;
}

ULONG PrintWebUIDelegate::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete this;

    return newRef;
}

HRESULT PrintWebUIDelegate::webViewPrintingMarginRect(IWebView* view, RECT* rect)
{
    if (!view || !rect)
        return E_POINTER;

    IWebFrame* mainFrame = 0;
    if (FAILED(view->mainFrame(&mainFrame)))
        return E_FAIL;

    IWebFramePrivate* privateFrame = 0;
    if (FAILED(mainFrame->QueryInterface(&privateFrame))) {
        mainFrame->Release();
        return E_FAIL;
    }

    privateFrame->frameBounds(rect);

    rect->left += MARGIN;
    rect->top += MARGIN;
    HDC dc = ::GetDC(0);
    rect->right = (::GetDeviceCaps(dc, LOGPIXELSX) * 6.5) - MARGIN;
    rect->bottom = (::GetDeviceCaps(dc, LOGPIXELSY) * 11) - MARGIN;
    ::ReleaseDC(0, dc);

    privateFrame->Release();
    mainFrame->Release();

    return S_OK;
}

HRESULT PrintWebUIDelegate::webViewHeaderHeight(IWebView* webView, float* height)
{
    if (!webView || !height)
        return E_POINTER;

    HDC dc = ::GetDC(0);

    TEXTMETRIC textMetric;
    ::GetTextMetrics(dc, &textMetric);
    ::ReleaseDC(0, dc);

    *height = 1.1 * textMetric.tmHeight;

    return S_OK;
}

HRESULT PrintWebUIDelegate::webViewFooterHeight(IWebView* webView, float* height)
{
    if (!webView || !height)
        return E_POINTER;

    HDC dc = ::GetDC(0);

    TEXTMETRIC textMetric;
    ::GetTextMetrics(dc, &textMetric);
    ::ReleaseDC(0, dc);

    *height = 1.1 * textMetric.tmHeight;

    return S_OK;
}

HRESULT PrintWebUIDelegate::drawHeaderInRect(
            /* [in] */ IWebView* webView,
            /* [in] */ RECT* rect,
            /* [in] */ OLE_HANDLE drawingContext)
{
    if (!webView || !rect)
        return E_POINTER;

    // Turn off header for now.
    HDC dc = reinterpret_cast<HDC>(drawingContext);

    HGDIOBJ hFont = ::GetStockObject(ANSI_VAR_FONT);
    HGDIOBJ hOldFont = ::SelectObject(dc, hFont);

    LPCWSTR header = L"[Sample Header]";
    size_t length = wcslen(header);

    int rc = ::DrawTextW(dc, header, length, rect, DT_LEFT | DT_NOCLIP | DT_VCENTER | DT_SINGLELINE);
    ::SelectObject(dc, hOldFont);

    if (!rc)
        return E_FAIL;

    ::MoveToEx(dc, rect->left, rect->bottom, 0);
    HGDIOBJ hPen = ::GetStockObject(BLACK_PEN);
    HGDIOBJ hOldPen = ::SelectObject(dc, hPen);
    ::LineTo(dc, rect->right, rect->bottom);
    ::SelectObject(dc, hOldPen);

    return S_OK;
}

HRESULT PrintWebUIDelegate::drawFooterInRect(
            /* [in] */ IWebView* webView,
            /* [in] */ RECT* rect,
            /* [in] */ OLE_HANDLE drawingContext,
            /* [in] */ UINT pageIndex,
            /* [in] */ UINT pageCount)
{
    if (!webView || !rect)
        return E_POINTER;

    HDC dc = reinterpret_cast<HDC>(drawingContext);

    HGDIOBJ hFont = ::GetStockObject(ANSI_VAR_FONT);
    HGDIOBJ hOldFont = ::SelectObject(dc, hFont);

    LPCWSTR footer = L"[Sample Footer]";
    size_t length = wcslen(footer);

    // Add a line, 1/10th inch above the footer text from left margin to right margin.
    ::MoveToEx(dc, rect->left, rect->top, 0);
    HGDIOBJ hPen = ::GetStockObject(BLACK_PEN);
    HGDIOBJ hOldPen = ::SelectObject(dc, hPen);
    ::LineTo(dc, rect->right, rect->top);
    ::SelectObject(dc, hOldPen);

    int rc = ::DrawTextW(dc, footer, length, rect, DT_LEFT | DT_NOCLIP | DT_VCENTER | DT_SINGLELINE);
    ::SelectObject(dc, hOldFont);

    if (!rc)
        return E_FAIL;

    return S_OK;
}
