/*
 * Copyright (C) 2012 Google Inc.  All rights reserved.
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

#ifndef WebSocketExtensionProcessor_h
#define WebSocketExtensionProcessor_h

#if ENABLE(WEB_SOCKETS)

#include <wtf/HashMap.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class WebSocketExtensionProcessor {
public:
    virtual ~WebSocketExtensionProcessor() { }

    String extensionToken() const { return m_extensionToken; }

    // The return value of this method will be a part of the value of
    // Sec-WebSocket-Extensions.
    virtual String handshakeString() = 0;

    // This should validate the server's response parameters which are passed
    // as HashMap<key, value>. This may also do something for the extension.
    // Note that this method may be called more than once when the server
    // response contains duplicate extension token that matches extensionToken().
    virtual bool processResponse(const HashMap<String, String>&) = 0;

    // If procecssResponse() returns false, this should provide the reason.
    virtual String failureReason() { return "Extension " + m_extensionToken + " failed"; }

protected:
    explicit WebSocketExtensionProcessor(const String& extensionToken)
        : m_extensionToken(extensionToken)
    {
    }

private:
    String m_extensionToken;
};

}

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketExtensionProcessor_h
