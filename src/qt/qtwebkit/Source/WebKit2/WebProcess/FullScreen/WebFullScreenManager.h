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
#ifndef WebFullScreenManager_h
#define WebFullScreenManager_h

#if ENABLE(FULLSCREEN_API)

#include <WebCore/IntRect.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace CoreIPC {
class Connection;
class MessageDecoder;
}

namespace WebCore {
class IntRect;
class Element;
class GraphicsLayer;
}

namespace WebKit {

class WebPage;

class WebFullScreenManager : public RefCounted<WebFullScreenManager> {
public:
    static PassRefPtr<WebFullScreenManager> create(WebPage*);
    virtual ~WebFullScreenManager();

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);

    bool supportsFullScreen(bool withKeyboard);
    void enterFullScreenForElement(WebCore::Element*);
    void exitFullScreenForElement(WebCore::Element*);

    void willEnterFullScreen();
    void didEnterFullScreen();
    void willExitFullScreen();
    void didExitFullScreen();

    WebCore::Element* element();

    void close();

protected:
    WebFullScreenManager(WebPage*);

    void setAnimatingFullScreen(bool);
    void requestExitFullScreen();
    void saveScrollPosition();
    void restoreScrollPosition();

    void didReceiveWebFullScreenManagerMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);

    WebCore::IntRect m_initialFrame;
    WebCore::IntRect m_finalFrame;
    WebCore::IntPoint m_scrollPosition;
    RefPtr<WebPage> m_page;
    RefPtr<WebCore::Element> m_element;
};

} // namespace WebKit

#endif // ENABLE(FULLSCREEN_API)

#endif // WebFullScreenManager_h
