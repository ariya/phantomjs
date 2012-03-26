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

#include "config.h"

#if ENABLE(WEB_SOCKETS)

#include "WebSocketHandshakeResponse.h"

#include <wtf/Assertions.h>
#include <wtf/text/AtomicString.h>

using namespace std;

namespace WebCore {

WebSocketHandshakeResponse::ChallengeResponse::ChallengeResponse()
{
    memset(value, 0, sizeof(value));
}

void WebSocketHandshakeResponse::ChallengeResponse::set(const unsigned char challengeResponse[16])
{
    memcpy(value, challengeResponse, sizeof(value));
}

WebSocketHandshakeResponse::WebSocketHandshakeResponse()
{
}

WebSocketHandshakeResponse::~WebSocketHandshakeResponse()
{
}

int WebSocketHandshakeResponse::statusCode() const
{
    return m_statusCode;
}

void WebSocketHandshakeResponse::setStatusCode(int statusCode)
{
    ASSERT(statusCode >= 100 && statusCode < 600);
    m_statusCode = statusCode;
}

const String& WebSocketHandshakeResponse::statusText() const
{
    return m_statusText;
}

void WebSocketHandshakeResponse::setStatusText(const String& statusText)
{
    m_statusText = statusText;
}

const HTTPHeaderMap& WebSocketHandshakeResponse::headerFields() const
{
    return m_headerFields;
}

void WebSocketHandshakeResponse::addHeaderField(const AtomicString& name, const String& value)
{
    m_headerFields.add(name, value);
}

void WebSocketHandshakeResponse::clearHeaderFields()
{
    m_headerFields.clear();
}

const WebSocketHandshakeResponse::ChallengeResponse& WebSocketHandshakeResponse::challengeResponse() const
{
    return m_challengeResponse;
}

void WebSocketHandshakeResponse::setChallengeResponse(const unsigned char challengeResponse[16])
{
    m_challengeResponse.set(challengeResponse);
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)
