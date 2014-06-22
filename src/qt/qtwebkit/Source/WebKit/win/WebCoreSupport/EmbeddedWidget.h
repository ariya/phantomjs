/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef EmbeddedWidget_h
#define EmbeddedWidget_h

#include "WebKit.h"
#include <WebCore/COMPtr.h>
#include <WebCore/IntRect.h>
#include <WebCore/PluginView.h>

namespace WebCore {
class HTMLPlugInElement;
class IntSize;
}

interface IWebEmbeddedView;

class EmbeddedWidget : public WebCore::Widget, public WebCore::PluginManualLoader {
public:
    static PassRefPtr<EmbeddedWidget> create(IWebEmbeddedView*, WebCore::HTMLPlugInElement*, HWND parentWindow, const WebCore::IntSize&);
    ~EmbeddedWidget();

private:
    EmbeddedWidget(IWebEmbeddedView* view, WebCore::HTMLPlugInElement* element)
        : m_view(view)
        , m_element(element)
        , m_window(0)
        , m_isVisible(false)
        , m_attachedToWindow(false)
    {
    }

    bool createWindow(HWND parentWindow, const WebCore::IntSize& size);

    virtual void didReceiveResponse(const WebCore::ResourceResponse&);
    virtual void didReceiveData(const char*, int);
    virtual void didFinishLoading();
    virtual void didFail(const WebCore::ResourceError&);

    virtual void invalidateRect(const WebCore::IntRect&);
    virtual void setFrameRect(const WebCore::IntRect&);
    virtual void frameRectsChanged();
    virtual void setFocus(bool);
    virtual void show();
    virtual void hide();
    virtual WebCore::IntRect windowClipRect() const;
    virtual void setParent(WebCore::ScrollView*);

    virtual void attachToWindow();
    virtual void detachFromWindow();

    COMPtr<IWebEmbeddedView> m_view;
    WebCore::HTMLPlugInElement* m_element;
    HWND m_window;

    bool m_isVisible;
    bool m_attachedToWindow;
        
    WebCore::IntRect m_clipRect; // The clip rect to apply to an embedded view.
    WebCore::IntRect m_windowRect; // Our window rect.
};

#endif // EmbeddedWidget_h
