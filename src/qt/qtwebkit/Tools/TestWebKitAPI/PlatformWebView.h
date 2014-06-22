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

#ifndef PlatformWebView_h
#define PlatformWebView_h

#if USE(CG)
#include <CoreGraphics/CGGeometry.h>
#endif

#ifdef __APPLE__
#ifdef __OBJC__
@class WKView;
@class NSWindow;
#else
class WKView;
class NSWindow;
#endif
typedef WKView *PlatformWKView;
typedef NSWindow *PlatformWindow;
#elif defined(WIN32) || defined(_WIN32)
typedef WKViewRef PlatformWKView;
typedef HWND PlatformWindow;
#elif PLATFORM(GTK)
typedef WKViewRef PlatformWKView;
typedef GtkWidget *PlatformWindow;
#elif PLATFORM(EFL)
typedef struct _Ecore_Evas Ecore_Evas;
#if USE(EO)
typedef struct _Eo Evas_Object;
#else
typedef struct _Evas_Object Evas_Object;
#endif
typedef Evas_Object* PlatformWKView;
typedef Ecore_Evas* PlatformWindow;
#elif PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QQuickView;
QT_END_NAMESPACE
class QQuickWebView;
typedef QQuickWebView* PlatformWKView;
typedef QQuickView* PlatformWindow;
#endif

namespace TestWebKitAPI {

#if PLATFORM(WIN)
class WindowMessageObserver;
#endif

class PlatformWebView {
public:
    PlatformWebView(WKContextRef, WKPageGroupRef = 0);
    ~PlatformWebView();

    WKPageRef page() const;
    PlatformWKView platformView() const { return m_view; }
    void resizeTo(unsigned width, unsigned height);
    void focus();

    void simulateSpacebarKeyPress();
    void simulateAltKeyPress();
    void simulateRightClick(unsigned x, unsigned y);
    void simulateMouseMove(unsigned x, unsigned y);

#if PLATFORM(WIN)
    void simulateAKeyDown();
    void setParentWindowMessageObserver(WindowMessageObserver* observer) { m_parentWindowMessageObserver = observer; }
#endif

private:
#if PLATFORM(WIN)
    static void registerWindowClass();
    static LRESULT CALLBACK wndProc(HWND, UINT message, WPARAM, LPARAM);
#endif

    PlatformWKView m_view;
    PlatformWindow m_window;

#if PLATFORM(WIN)
    WindowMessageObserver* m_parentWindowMessageObserver;
#endif
};

} // namespace TestWebKitAPI

#endif // PlatformWebView_h
