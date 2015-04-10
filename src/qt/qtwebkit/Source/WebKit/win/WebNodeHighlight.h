/*
 * Copyright (C) 2007 Apple Inc.  All rights reserved.
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

#ifndef WebNodeHighlight_h
#define WebNodeHighlight_h

#include <WebCore/WindowMessageListener.h>
#include <windows.h>

class WebView;

class WebNodeHighlight : WebCore::WindowMessageListener {
public:
    WebNodeHighlight(WebView*);
    ~WebNodeHighlight();

    void setShowsWhileWebViewIsVisible(bool);

    bool isShowing() const;

    void update();

    void placeBehindWindow(HWND);

private:
    void show();
    void hide();

    bool isWebViewVisible() const;

    virtual void windowReceivedMessage(HWND, UINT message, WPARAM, LPARAM);

    void onWebViewShowWindow(bool showing);
    void onWebViewWindowPosChanged(WINDOWPOS*);
    void onRootWindowPosChanged(WINDOWPOS*);

#if ENABLE(INSPECTOR)
    WebView* m_inspectedWebView;
#endif // ENABLE(INSPECTOR)
    HWND m_inspectedWebViewWindow;
    HWND m_overlay;
    HWND m_observedWindow;
    bool m_showsWhileWebViewIsVisible;

    friend static LRESULT CALLBACK OverlayWndProc(HWND, UINT, WPARAM, LPARAM);
};

#endif // !defined(WebNodeHighlight_h)
