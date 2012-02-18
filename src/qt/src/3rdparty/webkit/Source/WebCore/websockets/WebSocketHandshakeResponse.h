/*
 * Copyright (C) 2010 Google Inc.  All rights reserved.
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

#ifndef WebSocketHandshakeResponse_h
#define WebSocketHandshakeResponse_h

#if ENABLE(WEB_SOCKETS)

#include "HTTPHeaderMap.h"
#include "PlatformString.h"
#include <wtf/Forward.h>

namespace WebCore {

class WebSocketHandshakeResponse {
public:
    WebSocketHandshakeResponse();
    ~WebSocketHandshakeResponse();

    int statusCode() const;
    void setStatusCode(int statusCode);
    const String& statusText() const;
    void setStatusText(const String& statusText);
    const HTTPHeaderMap& headerFields() const;
    void addHeaderField(const AtomicString& name, const String& value);
    void clearHeaderFields();

    struct ChallengeResponse {
        unsigned char value[16];

        ChallengeResponse();
        void set(const unsigned char challengeResponse[16]);
    };
    const ChallengeResponse& challengeResponse() const;
    void setChallengeResponse(const unsigned char challengeResponse[16]);

private:
    int m_statusCode;
    String m_statusText;
    HTTPHeaderMap m_headerFields;
    ChallengeResponse m_challengeResponse;
};

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)

#endif // WebSocketHandshakeResponse_h
