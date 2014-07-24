/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef WebInspectorServer_h
#define WebInspectorServer_h

#if ENABLE(INSPECTOR_SERVER)

#include "WebSocketServer.h"
#include "WebSocketServerClient.h"
#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

class WebInspectorProxy;

class WebInspectorServer : public WebSocketServer, public WebSocketServerClient {
public:
    typedef HashMap<unsigned, WebInspectorProxy*> ClientMap;
    static WebInspectorServer& shared();

    // Page registry to manage known pages.
    int registerPage(WebInspectorProxy* client);
    void unregisterPage(int pageId);
    String inspectorUrlForPageID(int pageId);
    void sendMessageOverConnection(unsigned pageIdForConnection, const String& message);

private:
    WebInspectorServer();
    ~WebInspectorServer();

    // WebSocketServerClient implementation. Events coming from remote connections.
    virtual void didReceiveUnrecognizedHTTPRequest(WebSocketServerConnection*, PassRefPtr<HTTPRequest>);
    virtual bool didReceiveWebSocketUpgradeHTTPRequest(WebSocketServerConnection*, PassRefPtr<HTTPRequest>);
    virtual void didEstablishWebSocketConnection(WebSocketServerConnection*, PassRefPtr<HTTPRequest>);
    virtual void didReceiveWebSocketMessage(WebSocketServerConnection*, const String& message);
    virtual void didCloseWebSocketConnection(WebSocketServerConnection*);

    bool platformResourceForPath(const String& path, Vector<char>& data, String& contentType);
#if PLATFORM(QT) || PLATFORM(GTK) || PLATFORM(EFL)
    void buildPageList(Vector<char>& data, String& contentType);
#endif

    void closeConnection(WebInspectorProxy*, WebSocketServerConnection*);

#if PLATFORM(GTK)
    String inspectorServerFilesPath();
    String m_inspectorServerFilesPath;
#endif
    unsigned m_nextAvailablePageId;
    ClientMap m_clientMap;
    HashMap<unsigned, WebSocketServerConnection*> m_connectionMap;
};

}

#endif // ENABLE(INSPECTOR_SERVER)

#endif // WebInspectorServer_h
