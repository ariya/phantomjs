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

#ifndef WebScrollBar_h
#define WebScrollBar_h

#include "WebKit.h"

#include <wtf/RefPtr.h>
#include <wtf/OwnPtr.h>

#include <WebCore/COMPtr.h>
#include <WebCore/Scrollbar.h>
#include <WebCore/ScrollableArea.h>

namespace WebCore {
class Scrollbar;
}

class WebScrollBar : public IWebScrollBarPrivate, WebCore::ScrollableArea {
public:
    static WebScrollBar* createInstance();
protected:
    WebScrollBar();
    ~WebScrollBar();

public:
    // IUnknown
    virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject);
    virtual ULONG STDMETHODCALLTYPE AddRef(void);
    virtual ULONG STDMETHODCALLTYPE Release(void);

    // IWebScrollBarPrivate
    virtual HRESULT STDMETHODCALLTYPE init( 
        /* [in] */ IWebScrollBarDelegatePrivate* delegate,
        /* [in] */ OLE_HANDLE containingWindow,
        /* [in] */ WebScrollBarOrientation orientation,
        /* [in] */ WebScrollBarControlSize controlSize);
    
    virtual HRESULT STDMETHODCALLTYPE setEnabled( 
        /* [in] */ BOOL enabled);
    
    virtual HRESULT STDMETHODCALLTYPE setSteps( 
        /* [in] */ int lineStep,
        /* [in] */ int pageStep);
    
    virtual HRESULT STDMETHODCALLTYPE setProportion( 
        /* [in] */ int visibleSize,
        /* [in] */ int totalSize);
    
    virtual HRESULT STDMETHODCALLTYPE setRect( 
        /* [in] */ RECT bounds);
    
    virtual HRESULT STDMETHODCALLTYPE setValue( 
        /* [in] */ int value);
    
    virtual HRESULT STDMETHODCALLTYPE value( 
        /* [retval][out] */ int* value);
   
    virtual HRESULT STDMETHODCALLTYPE paint( 
        /* [in] */ HDC dc,
        /* [in] */ RECT damageRect);
    
    virtual HRESULT STDMETHODCALLTYPE frameRect( 
        /* [retval][out] */ RECT* bounds);
    
    virtual HRESULT STDMETHODCALLTYPE width( 
        /* [retval][out] */ int* w);
    
    virtual HRESULT STDMETHODCALLTYPE height( 
        /* [retval][out] */ int* h);
    
    virtual HRESULT STDMETHODCALLTYPE requestedWidth( 
        /* [retval][out] */ int* w);
    
    virtual HRESULT STDMETHODCALLTYPE requestedHeight( 
        /* [retval][out] */ int* h);

    virtual HRESULT STDMETHODCALLTYPE handleMouseEvent( 
        /* [in] */ OLE_HANDLE window,
        /* [in] */ UINT msg,
        /* [in] */ WPARAM wParam,
        /* [in] */ LPARAM lParam);
    
    virtual HRESULT STDMETHODCALLTYPE scroll( 
        /* [in] */ WebScrollDirection direction,
        /* [in] */ WebScrollGranularity granularity,
        /* [in] */ float multiplier);

protected:
    // ScrollableArea
    virtual int scrollSize(WebCore::ScrollbarOrientation) const;
    virtual int scrollPosition(WebCore::Scrollbar*) const;
    virtual void setScrollOffset(const WebCore::IntPoint&);
    virtual void invalidateScrollbarRect(WebCore::Scrollbar*, const WebCore::IntRect&);
    virtual void invalidateScrollCornerRect(const WebCore::IntRect&) { }
    virtual WebCore::ScrollableArea* enclosingScrollableArea() const { return 0; }
    virtual int visibleHeight() const OVERRIDE;
    virtual int visibleWidth() const OVERRIDE;
    virtual WebCore::IntSize contentsSize() const OVERRIDE;
    virtual bool scrollbarsCanBeActive() const OVERRIDE;
    virtual WebCore::IntRect scrollableAreaBoundingBox() const OVERRIDE;

    // FIXME: We should provide a way to set this value.
    virtual bool isActive() const { return true; }

    virtual bool isScrollCornerVisible() const { return false; }
    virtual WebCore::IntRect scrollCornerRect() const { return WebCore::IntRect(); }

    virtual WebCore::Scrollbar* horizontalScrollbar() const; 
    virtual WebCore::Scrollbar* verticalScrollbar() const; 

    ULONG m_refCount;
    HWND m_containingWindow;
    int m_currentPosition;
    RefPtr<WebCore::Scrollbar> m_scrollBar;
    COMPtr<IWebScrollBarDelegatePrivate> m_delegate;
};

#endif
