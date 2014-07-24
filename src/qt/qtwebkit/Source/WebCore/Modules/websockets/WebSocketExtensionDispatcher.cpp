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

#include "config.h"

#if ENABLE(WEB_SOCKETS)

#include "WebSocketExtensionDispatcher.h"

#include "WebSocketExtensionParser.h"

#include <wtf/ASCIICType.h>
#include <wtf/HashMap.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

void WebSocketExtensionDispatcher::reset()
{
    m_processors.clear();
}

void WebSocketExtensionDispatcher::addProcessor(PassOwnPtr<WebSocketExtensionProcessor> processor)
{
    for (size_t i = 0; i < m_processors.size(); ++i) {
        if (m_processors[i]->extensionToken() == processor->extensionToken())
            return;
    }
    ASSERT(processor->handshakeString().length());
    ASSERT(!processor->handshakeString().contains('\n'));
    ASSERT(!processor->handshakeString().contains(static_cast<UChar>('\0')));
    m_processors.append(processor);
}

const String WebSocketExtensionDispatcher::createHeaderValue() const
{
    size_t numProcessors = m_processors.size();
    if (!numProcessors)
        return String();

    StringBuilder builder;
    builder.append(m_processors[0]->handshakeString());
    for (size_t i = 1; i < numProcessors; ++i) {
        builder.append(", ");
        builder.append(m_processors[i]->handshakeString());
    }
    return builder.toString();
}

void WebSocketExtensionDispatcher::appendAcceptedExtension(const String& extensionToken, HashMap<String, String>& extensionParameters)
{
    if (!m_acceptedExtensionsBuilder.isEmpty())
        m_acceptedExtensionsBuilder.append(", ");
    m_acceptedExtensionsBuilder.append(extensionToken);
    // FIXME: Should use ListHashSet to keep the order of the parameters.
    for (HashMap<String, String>::const_iterator iterator = extensionParameters.begin(); iterator != extensionParameters.end(); ++iterator) {
        m_acceptedExtensionsBuilder.append("; ");
        m_acceptedExtensionsBuilder.append(iterator->key);
        if (!iterator->value.isNull()) {
            m_acceptedExtensionsBuilder.append("=");
            m_acceptedExtensionsBuilder.append(iterator->value);
        }
    }
}

void WebSocketExtensionDispatcher::fail(const String& reason)
{
    m_failureReason = reason;
    m_acceptedExtensionsBuilder.clear();
}

bool WebSocketExtensionDispatcher::processHeaderValue(const String& headerValue)
{
    if (!headerValue.length())
        return true;

    // If we don't send Sec-WebSocket-Extensions header, the server should not return the header.
    if (!m_processors.size()) {
        fail("Received unexpected Sec-WebSocket-Extensions header");
        return false;
    }

    const CString headerValueData = headerValue.utf8();
    WebSocketExtensionParser parser(headerValueData.data(), headerValueData.data() + headerValueData.length());
    while (!parser.finished()) {
        String extensionToken;
        HashMap<String, String> extensionParameters;
        if (!parser.parseExtension(extensionToken, extensionParameters)) {
            fail("Sec-WebSocket-Extensions header is invalid");
            return false;
        }

        size_t index;
        for (index = 0; index < m_processors.size(); ++index) {
            WebSocketExtensionProcessor* processor = m_processors[index].get();
            if (extensionToken == processor->extensionToken()) {
                if (processor->processResponse(extensionParameters)) {
                    appendAcceptedExtension(extensionToken, extensionParameters);
                    break;
                }
                fail(processor->failureReason());
                return false;
            }
        }
        // There is no extension which can process the response.
        if (index == m_processors.size()) {
            fail("Received unexpected extension: " + extensionToken);
            return false;
        }
    }
    return parser.parsedSuccessfully();
}

String WebSocketExtensionDispatcher::acceptedExtensions() const
{
    if (m_acceptedExtensionsBuilder.isEmpty())
        return String();
    return m_acceptedExtensionsBuilder.toStringPreserveCapacity();
}

String WebSocketExtensionDispatcher::failureReason() const
{
    return m_failureReason;
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)
