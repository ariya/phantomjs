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

#ifndef WebInspector_h
#define WebInspector_h

#if ENABLE(INSPECTOR)

#include "APIObject.h"
#include "Connection.h"
#include <wtf/Noncopyable.h>
#include <wtf/text/WTFString.h>

namespace WebCore {
class InspectorFrontendChannel;
}

namespace WebKit {

class WebInspectorFrontendClient;
class WebPage;
struct WebPageCreationParameters;

class WebInspector : public TypedAPIObject<APIObject::TypeBundleInspector> {
public:
    static PassRefPtr<WebInspector> create(WebPage*, WebCore::InspectorFrontendChannel*);

    WebPage* page() const { return m_page; }
    WebPage* inspectorPage() const { return m_inspectorPage; }

    // Implemented in generated WebInspectorMessageReceiver.cpp
    void didReceiveWebInspectorMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);

    // Called by WebInspector messages
    void show();
    void close();

    void didSave(const String& url);
    void didAppend(const String& url);

    void attachedBottom();
    void attachedRight();
    void detached();

    void evaluateScriptForTest(long callID, const String& script);

    void setJavaScriptProfilingEnabled(bool);
    void startPageProfiling();
    void stopPageProfiling();

#if ENABLE(INSPECTOR_SERVER)
    bool hasRemoteFrontendConnected() const { return m_remoteFrontendConnected; }
    void sendMessageToRemoteFrontend(const String& message);
    void dispatchMessageFromRemoteFrontend(const String& message);
    void remoteFrontendConnected();
    void remoteFrontendDisconnected();
#endif

#if PLATFORM(MAC)
    void setInspectorUsesWebKitUserInterface(bool);
#endif

private:
    friend class WebInspectorClient;
    friend class WebInspectorFrontendClient;

    explicit WebInspector(WebPage*, WebCore::InspectorFrontendChannel*);

    // Called from WebInspectorClient
    WebPage* createInspectorPage();
    void destroyInspectorPage();

    // Called from WebInspectorFrontendClient
    void didClose();
    void bringToFront();
    void inspectedURLChanged(const String&);

    bool canSave() const;
    void save(const String& filename, const String& content, bool forceSaveAs);
    void append(const String& filename, const String& content);

    void attachBottom();
    void attachRight();
    void detach();

    void setAttachedWindowHeight(unsigned);
    void setAttachedWindowWidth(unsigned);
    void setToolbarHeight(unsigned);

    // Implemented in platform WebInspector file
    String localizedStringsURL() const;

    void showConsole();

    void showResources();

    void showMainResourceForFrame(uint64_t frameID);

    void startJavaScriptDebugging();
    void stopJavaScriptDebugging();

    void startJavaScriptProfiling();
    void stopJavaScriptProfiling();

    void updateDockingAvailability();

    WebPage* m_page;
    WebPage* m_inspectorPage;
    WebInspectorFrontendClient* m_frontendClient;
    WebCore::InspectorFrontendChannel* m_frontendChannel;
#if PLATFORM(MAC)
    mutable String m_localizedStringsURL;
    mutable bool m_hasLocalizedStringsURL;
    bool m_usesWebKitUserInterface;
#endif
#if ENABLE(INSPECTOR_SERVER)
    bool m_remoteFrontendConnected;
#endif
};

} // namespace WebKit

#endif // ENABLE(INSPECTOR)

#endif // WebInspector_h
