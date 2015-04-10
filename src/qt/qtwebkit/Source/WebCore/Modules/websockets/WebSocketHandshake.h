/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebSocketHandshake_h
#define WebSocketHandshake_h

#if ENABLE(WEB_SOCKETS)

#include "KURL.h"
#include "ResourceResponse.h"
#include "WebSocketExtensionDispatcher.h"
#include "WebSocketExtensionProcessor.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ResourceRequest;
class ScriptExecutionContext;

class WebSocketHandshake {
    WTF_MAKE_NONCOPYABLE(WebSocketHandshake); WTF_MAKE_FAST_ALLOCATED;
public:
    enum Mode {
        Incomplete, Normal, Failed, Connected
    };
    WebSocketHandshake(const KURL&, const String& protocol, ScriptExecutionContext*);
    ~WebSocketHandshake();

    const KURL& url() const;
    void setURL(const KURL&);
    const String host() const;

    const String& clientProtocol() const;
    void setClientProtocol(const String&);

    bool secure() const;

    String clientOrigin() const;
    String clientLocation() const;

    CString clientHandshakeMessage() const;
    ResourceRequest clientHandshakeRequest() const;

    void reset();
    void clearScriptExecutionContext();

    int readServerHandshake(const char* header, size_t len);
    Mode mode() const;
    String failureReason() const; // Returns a string indicating the reason of failure if mode() == Failed.

    String serverWebSocketProtocol() const;
    String serverSetCookie() const;
    String serverSetCookie2() const;
    String serverUpgrade() const;
    String serverConnection() const;
    String serverWebSocketAccept() const;
    String acceptedExtensions() const;

    const ResourceResponse& serverHandshakeResponse() const;

    void addExtensionProcessor(PassOwnPtr<WebSocketExtensionProcessor>);

    static String getExpectedWebSocketAccept(const String& secWebSocketKey);

private:
    KURL httpURLForAuthenticationAndCookies() const;

    int readStatusLine(const char* header, size_t headerLength, int& statusCode, String& statusText);

    // Reads all headers except for the two predefined ones.
    const char* readHTTPHeaders(const char* start, const char* end);
    void processHeaders();
    bool checkResponseHeaders();

    KURL m_url;
    String m_clientProtocol;
    bool m_secure;
    ScriptExecutionContext* m_context;

    Mode m_mode;

    ResourceResponse m_serverHandshakeResponse;

    String m_failureReason;

    String m_secWebSocketKey;
    String m_expectedAccept;

    WebSocketExtensionDispatcher m_extensionDispatcher;
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketHandshake_h
