/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebFullScreenManagerProxy_h
#define WebFullScreenManagerProxy_h

#if ENABLE(FULLSCREEN_API)

#include "MessageReceiver.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {
class IntRect;
}

#if PLATFORM(MAC)
OBJC_CLASS WKView;
#elif PLATFORM(GTK)
typedef struct _WebKitWebViewBase WebKitWebViewBase;
#elif PLATFORM(QT)
class QQuickWebView;
#endif

namespace WebKit {
    
#if PLATFORM(MAC)
typedef WKView PlatformWebView;
#elif PLATFORM(QT)
typedef QQuickWebView PlatformWebView;
#elif PLATFORM(GTK)
typedef WebKitWebViewBase PlatformWebView;
#elif PLATFORM(EFL)
typedef Evas_Object PlatformWebView;
#endif

class WebPageProxy;
class LayerTreeContext;

class WebFullScreenManagerProxy : public RefCounted<WebFullScreenManagerProxy>, public CoreIPC::MessageReceiver {
public:
    static PassRefPtr<WebFullScreenManagerProxy> create(WebPageProxy*);
    virtual ~WebFullScreenManagerProxy();

    void invalidate();

    void setWebView(PlatformWebView*);
    bool isFullScreen();
    void close();

    void willEnterFullScreen();
    void didEnterFullScreen();
    void willExitFullScreen();
    void didExitFullScreen();
    void setAnimatingFullScreen(bool);
    void requestExitFullScreen();
    void saveScrollPosition();
    void restoreScrollPosition();

private:
    explicit WebFullScreenManagerProxy(WebPageProxy*);

    void supportsFullScreen(bool withKeyboard, bool&);
    void enterFullScreen();
    void exitFullScreen();
    void beganEnterFullScreen(const WebCore::IntRect& initialFrame, const WebCore::IntRect& finalFrame);
    void beganExitFullScreen(const WebCore::IntRect& initialFrame, const WebCore::IntRect& finalFrame);

    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didReceiveSyncMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&, OwnPtr<CoreIPC::MessageEncoder>&) OVERRIDE;

    WebPageProxy* m_page;
    PlatformWebView* m_webView;

#if PLATFORM(EFL)
    bool m_hasRequestedFullScreen;
#endif
};

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)

#endif // WebFullScreenManagerProxy_h
