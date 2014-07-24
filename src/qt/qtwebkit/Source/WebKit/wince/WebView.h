/*
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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

#ifndef WebView_h
#define WebView_h

#include "IntRect.h"
#include <windows.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>

namespace WTF {
class String;
}

namespace WebCore {
class Frame;
class Page;
class FrameView;
class HTMLFrameOwnerElement;
class KURL;
class ResourceRequest;
}

class WebView {
public:
    enum Features {
        NoFeature = 0,
        EnableDoubleBuffering = 1 << 0
    };

    WebView(HWND hwnd, unsigned features = EnableDoubleBuffering);
    ~WebView();

    static void initialize(HINSTANCE instanceHandle);
    static void cleanup();

    HWND windowHandle() const { return m_windowHandle; }
    WebCore::Frame* frame() const { return m_frame; }
    WebCore::Page* page() const { return m_page; }
    WebCore::FrameView* view() const;

    void load(LPCWSTR url);
    void load(const WTF::String &url);
    void load(const WebCore::ResourceRequest &request);
    void reload();
    void stop();

    void frameRect(RECT* rect) const;

    PassRefPtr<WebCore::Frame> createFrame(const WebCore::KURL&, const WTF::String&, WebCore::HTMLFrameOwnerElement*, const WTF::String&, bool, int, int);

    // JavaScript Dialog
    void runJavaScriptAlert(const WTF::String& message);
    bool runJavaScriptConfirm(const WTF::String& message);
    bool runJavaScriptPrompt(const WTF::String& message, const WTF::String& defaultValue, WTF::String& result);

private:
    static LRESULT CALLBACK webViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT wndProc(HWND, UINT, WPARAM, LPARAM);

    bool handlePaint(HWND hWnd);
    bool handleMouseEvent(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool handleMouseWheel(HWND hWnd, WPARAM wParam, LPARAM lParam, bool isHorizontal);
    bool handleKeyDown(WPARAM virtualKeyCode, LPARAM keyData, bool systemKeyDown);
    bool handleKeyPress(WPARAM charCode, LPARAM keyData, bool systemKeyDown);
    bool handleKeyUp(WPARAM virtualKeyCode, LPARAM keyData, bool systemKeyDown);

    void paint(HDC hDC, const WebCore::IntRect& clipRect);

    WebCore::Frame* m_frame;
    WebCore::Page* m_page;
    HWND m_parentWindowHandle;
    HWND m_windowHandle;
    bool m_enableDoubleBuffer;
    OwnPtr<HDC> m_doubleBufferDC;
    OwnPtr<HBITMAP> m_doubleBufferBitmap;
};

#endif // WebView_h
