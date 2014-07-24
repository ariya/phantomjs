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
#include "WebKitDLL.h"
#include "WebKit.h"
#include "WebScrollBar.h"

#include <WebCore/GraphicsContext.h>
#include <WebCore/PlatformMouseEvent.h>
#include <WebCore/Scrollbar.h>
#include <WebCore/ScrollbarTheme.h>

using namespace WebCore;

// WebScrollBar ---------------------------------------------------------------------

WebScrollBar::WebScrollBar()
    : m_refCount(0)
    , m_containingWindow(0)
    , m_currentPosition(0)
{
    gClassCount++;
    gClassNameCount.add("WebScrollBar");
}

WebScrollBar::~WebScrollBar()
{
    gClassCount--;
    gClassNameCount.remove("WebScrollBar");
}

WebScrollBar* WebScrollBar::createInstance()
{
    WebScrollBar* instance = new WebScrollBar();
    instance->AddRef();
    return instance;
}

// IUnknown -------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE WebScrollBar::QueryInterface(REFIID riid, void** ppvObject)
{
    *ppvObject = 0;
    if (IsEqualGUID(riid, IID_IUnknown))
        *ppvObject = static_cast<IUnknown*>(this);
    else if (IsEqualGUID(riid, IID_IWebScrollBarPrivate))
        *ppvObject = static_cast<IWebScrollBarPrivate*>(this);
    else
        return E_NOINTERFACE;

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE WebScrollBar::AddRef(void)
{
    return ++m_refCount;
}

ULONG STDMETHODCALLTYPE WebScrollBar::Release(void)
{
    ULONG newRef = --m_refCount;
    if (!newRef)
        delete(this);

    return newRef;
}

// IWebScrollBarPrivate ------------------------------------------------------------------
HRESULT STDMETHODCALLTYPE WebScrollBar::init( 
    /* [in] */ IWebScrollBarDelegatePrivate* delegate,
    /* [in] */ OLE_HANDLE containingWindow,
    /* [in] */ WebScrollBarOrientation orientation,
    /* [in] */ WebScrollBarControlSize controlSize)
{
    if (!delegate || !containingWindow)
        return E_FAIL;
    ScrollbarOrientation webCoreOrientation = (ScrollbarOrientation) orientation;
    ScrollbarControlSize webCoreControlSize = (ScrollbarControlSize) controlSize;
    m_delegate = delegate;
    m_scrollBar = Scrollbar::createNativeScrollbar(this, webCoreOrientation, webCoreControlSize);
    if (!m_scrollBar)
        return E_FAIL;
    m_containingWindow = (HWND)(ULONG64)containingWindow;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::setEnabled(
    /* [in] */ BOOL enabled)
{
    m_scrollBar->setEnabled(!!enabled);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::setSteps( 
    /* [in] */ int lineStep,
    /* [in] */ int pageStep)
{
    m_scrollBar->setSteps(lineStep, pageStep);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::setProportion( 
    /* [in] */ int visibleSize,
    /* [in] */ int totalSize)
{
    m_scrollBar->setProportion(visibleSize, totalSize);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::setRect( 
    /* [in] */ RECT bounds)
{
    IntRect rect(bounds.left, bounds.top, bounds.right-bounds.left, bounds.bottom-bounds.top);
    m_scrollBar->setFrameRect(rect);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::setValue( 
    /* [in] */ int value)
{
    m_currentPosition = value;
    ScrollableArea::scrollToOffsetWithoutAnimation(m_scrollBar->orientation(), m_currentPosition);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::value(
    /* [retval][out] */ int* value)
{
    if (!value)
        return E_POINTER;
    *value = m_currentPosition;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::paint( 
    /* [in] */ HDC dc,
    /* [in] */ RECT damageRect)
{
    GraphicsContext context(dc);
    IntRect rect(damageRect.left, damageRect.top, damageRect.right-damageRect.left, damageRect.bottom-damageRect.top);
    m_scrollBar->paint(&context, rect);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::frameRect( 
    /* [retval][out] */ RECT* bounds)
{
    if (!bounds)
        return E_POINTER;
    IntRect rect = m_scrollBar->frameRect();
    bounds->left = rect.x();
    bounds->right = rect.maxX();
    bounds->top = rect.y();
    bounds->bottom = rect.maxY();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::width( 
    /* [retval][out] */ int* w)
{
    if (!w)
        return E_POINTER;
    *w = m_scrollBar->width();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::height( 
    /* [retval][out] */ int* h)
{
    if (!h)
        return E_POINTER;
    *h = m_scrollBar->height();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::requestedWidth( 
    /* [retval][out] */ int* w)
{
    if (!w)
        return E_POINTER;

    *w = m_scrollBar->orientation() == VerticalScrollbar ? ScrollbarTheme::theme()->scrollbarThickness(m_scrollBar->controlSize()) : -1;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::requestedHeight( 
    /* [retval][out] */ int* h)
{
    if (!h)
        return E_POINTER;

    *h = m_scrollBar->orientation() == HorizontalScrollbar ? ScrollbarTheme::theme()->scrollbarThickness(m_scrollBar->controlSize()) : -1;
    return S_OK;
}


HRESULT STDMETHODCALLTYPE WebScrollBar::handleMouseEvent( 
    OLE_HANDLE window,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    PlatformMouseEvent mouseEvent((HWND)(ULONG64)window, msg, wParam, lParam);
    switch (msg) {
        case WM_LBUTTONDOWN:
            m_scrollBar->mouseDown(mouseEvent);
            break;
        case WM_LBUTTONUP:
            m_scrollBar->mouseUp(mouseEvent);
            break;
        case WM_MOUSEMOVE:
            m_scrollBar->mouseMoved(mouseEvent);
            break;
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE WebScrollBar::scroll( 
    WebScrollDirection direction,
    WebScrollGranularity granularity,
    float multiplier)
{
    ScrollDirection webCoreScrollDirection = (ScrollDirection) direction;
    ScrollGranularity webCoreGranularity = (ScrollGranularity) granularity;
    ScrollableArea::scroll(webCoreScrollDirection, webCoreGranularity, multiplier);
    return S_OK;
}

// ScrollableArea -------------------------------------------------------

int WebScrollBar::scrollSize(ScrollbarOrientation orientation) const
{
    return (orientation == m_scrollBar->orientation()) ? (m_scrollBar->totalSize() - m_scrollBar->visibleSize()) : 0; 
}

int WebScrollBar::scrollPosition(Scrollbar*) const
{
    return m_currentPosition;
}

void WebScrollBar::setScrollOffset(const IntPoint& offset)
{
    m_currentPosition = (m_scrollBar->orientation() == HorizontalScrollbar) ? offset.x() : offset.y();
    m_delegate->valueChanged(this);
}

void WebScrollBar::invalidateScrollbarRect(Scrollbar*, const IntRect& rect)
{
    RECT r = rect;
    ::InvalidateRect(m_containingWindow, &r, false);
}

Scrollbar* WebScrollBar::horizontalScrollbar() const
{
    return m_scrollBar->orientation() == HorizontalScrollbar ? m_scrollBar.get() : 0;
}

Scrollbar* WebScrollBar::verticalScrollbar() const
{
    return m_scrollBar->orientation() == VerticalScrollbar ? m_scrollBar.get() : 0;
}

int WebScrollBar::visibleHeight() const
{
    return m_scrollBar->height();
}

int WebScrollBar::visibleWidth() const
{
    return m_scrollBar->width();
}

WebCore::IntSize WebScrollBar::contentsSize() const
{
    return m_scrollBar->frameRect().size();
}

bool WebScrollBar::scrollbarsCanBeActive() const
{
    return true;
}

WebCore::IntRect WebScrollBar::scrollableAreaBoundingBox() const
{
    return m_scrollBar->frameRect();
}
