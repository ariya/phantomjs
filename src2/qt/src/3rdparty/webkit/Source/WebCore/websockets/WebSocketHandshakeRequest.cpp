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

#include "WebSocketHandshakeRequest.h"

#include <cstring>

using namespace std;

namespace WebCore {

WebSocketHandshakeRequest::Key3::Key3()
{
    memset(value, 0, sizeof(value));
}

void WebSocketHandshakeRequest::Key3::set(const unsigned char key3[8])
{
    memcpy(value, key3, sizeof(value));
}

WebSocketHandshakeRequest::WebSocketHandshakeRequest(const String& requestMethod, const KURL& url)
    : m_url(url)
    , m_requestMethod(requestMethod)
{
}

WebSocketHandshakeRequest::~WebSocketHandshakeRequest()
{
}

String WebSocketHandshakeRequest::requestMethod() const
{
    return m_requestMethod;
}

KURL WebSocketHandshakeRequest::url() const
{
    return m_url;
}

void WebSocketHandshakeRequest::addHeaderField(const char* name, const String& value)
{
    m_headerFields.add(name, value);
}

const HTTPHeaderMap& WebSocketHandshakeRequest::headerFields() const
{
    return m_headerFields;
}

WebSocketHandshakeRequest::Key3 WebSocketHandshakeRequest::key3() const
{
    return m_key3;
}

void WebSocketHandshakeRequest::setKey3(const unsigned char key3[8])
{
    m_key3.set(key3);
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)
