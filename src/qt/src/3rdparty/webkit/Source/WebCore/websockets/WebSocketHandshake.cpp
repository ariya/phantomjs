/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#include "WebSocketHandshake.h"

#include "Cookie.h"
#include "CookieJar.h"
#include "Document.h"
#include "HTTPHeaderMap.h"
#include "KURL.h"
#include "Logging.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"
#include <wtf/CryptographicallyRandomNumber.h>
#include <wtf/MD5.h>
#include <wtf/StdLibExtras.h>
#include <wtf/StringExtras.h>
#include <wtf/Vector.h>
#include <wtf/text/AtomicString.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/StringConcatenate.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

static const char randomCharacterInSecWebSocketKey[] = "!\"#$%&'()*+,-./:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

static String resourceName(const KURL& url)
{
    String name = url.path();
    if (name.isEmpty())
        name = "/";
    if (!url.query().isNull())
        name += "?" + url.query();
    ASSERT(!name.isEmpty());
    ASSERT(!name.contains(' '));
    return name;
}

static String hostName(const KURL& url, bool secure)
{
    ASSERT(url.protocolIs("wss") == secure);
    StringBuilder builder;
    builder.append(url.host().lower());
    if (url.port() && ((!secure && url.port() != 80) || (secure && url.port() != 443))) {
        builder.append(':');
        builder.append(String::number(url.port()));
    }
    return builder.toString();
}

static const size_t maxConsoleMessageSize = 128;
static String trimConsoleMessage(const char* p, size_t len)
{
    String s = String(p, std::min<size_t>(len, maxConsoleMessageSize));
    if (len > maxConsoleMessageSize)
        s.append(horizontalEllipsis);
    return s;
}

static uint32_t randomNumberLessThan(uint32_t n)
{
    if (!n)
        return 0;
    if (n == std::numeric_limits<uint32_t>::max())
        return cryptographicallyRandomNumber();
    uint32_t max = std::numeric_limits<uint32_t>::max() - (std::numeric_limits<uint32_t>::max() % n);
    ASSERT(!(max % n));
    uint32_t v;
    do {
        v = cryptographicallyRandomNumber();
    } while (v >= max);
    return v % n;
}

static void generateSecWebSocketKey(uint32_t& number, String& key)
{
    uint32_t space = randomNumberLessThan(12) + 1;
    uint32_t max = 4294967295U / space;
    number = randomNumberLessThan(max);
    uint32_t product = number * space;

    String s = String::number(product);
    int n = randomNumberLessThan(12) + 1;
    DEFINE_STATIC_LOCAL(String, randomChars, (randomCharacterInSecWebSocketKey));
    for (int i = 0; i < n; i++) {
        int pos = randomNumberLessThan(s.length() + 1);
        int chpos = randomNumberLessThan(randomChars.length());
        s.insert(randomChars.substring(chpos, 1), pos);
    }
    DEFINE_STATIC_LOCAL(String, spaceChar, (" "));
    for (uint32_t i = 0; i < space; i++) {
        int pos = randomNumberLessThan(s.length() - 1) + 1;
        s.insert(spaceChar, pos);
    }
    ASSERT(s[0] != ' ');
    ASSERT(s[s.length() - 1] != ' ');
    key = s;
}

static void generateKey3(unsigned char key3[8])
{
    cryptographicallyRandomValues(key3, 8);
}

static void setChallengeNumber(unsigned char* buf, uint32_t number)
{
    unsigned char* p = buf + 3;
    for (int i = 0; i < 4; i++) {
        *p = number & 0xFF;
        --p;
        number >>= 8;
    }
}

static void generateExpectedChallengeResponse(uint32_t number1, uint32_t number2, unsigned char key3[8], unsigned char expectedChallenge[16])
{
    unsigned char challenge[16];
    setChallengeNumber(&challenge[0], number1);
    setChallengeNumber(&challenge[4], number2);
    memcpy(&challenge[8], key3, 8);
    MD5 md5;
    md5.addBytes(challenge, sizeof(challenge));
    Vector<uint8_t, 16> digest;
    md5.checksum(digest);
    memcpy(expectedChallenge, digest.data(), 16);
}

WebSocketHandshake::WebSocketHandshake(const KURL& url, const String& protocol, ScriptExecutionContext* context)
    : m_url(url)
    , m_clientProtocol(protocol)
    , m_secure(m_url.protocolIs("wss"))
    , m_context(context)
    , m_mode(Incomplete)
{
    uint32_t number1;
    uint32_t number2;
    generateSecWebSocketKey(number1, m_secWebSocketKey1);
    generateSecWebSocketKey(number2, m_secWebSocketKey2);
    generateKey3(m_key3);
    generateExpectedChallengeResponse(number1, number2, m_key3, m_expectedChallengeResponse);
}

WebSocketHandshake::~WebSocketHandshake()
{
}

const KURL& WebSocketHandshake::url() const
{
    return m_url;
}

void WebSocketHandshake::setURL(const KURL& url)
{
    m_url = url.copy();
}

const String WebSocketHandshake::host() const
{
    return m_url.host().lower();
}

const String& WebSocketHandshake::clientProtocol() const
{
    return m_clientProtocol;
}

void WebSocketHandshake::setClientProtocol(const String& protocol)
{
    m_clientProtocol = protocol;
}

bool WebSocketHandshake::secure() const
{
    return m_secure;
}

String WebSocketHandshake::clientOrigin() const
{
    return m_context->securityOrigin()->toString();
}

String WebSocketHandshake::clientLocation() const
{
    StringBuilder builder;
    builder.append(m_secure ? "wss" : "ws");
    builder.append("://");
    builder.append(hostName(m_url, m_secure));
    builder.append(resourceName(m_url));
    return builder.toString();
}

CString WebSocketHandshake::clientHandshakeMessage() const
{
    // Keep the following consistent with clientHandshakeRequest().
    StringBuilder builder;

    builder.append("GET ");
    builder.append(resourceName(m_url));
    builder.append(" HTTP/1.1\r\n");

    Vector<String> fields;
    fields.append("Upgrade: WebSocket");
    fields.append("Connection: Upgrade");
    fields.append("Host: " + hostName(m_url, m_secure));
    fields.append("Origin: " + clientOrigin());
    if (!m_clientProtocol.isEmpty())
        fields.append("Sec-WebSocket-Protocol: " + m_clientProtocol);

    KURL url = httpURLForAuthenticationAndCookies();
    if (m_context->isDocument()) {
        Document* document = static_cast<Document*>(m_context);
        String cookie = cookieRequestHeaderFieldValue(document, url);
        if (!cookie.isEmpty())
            fields.append("Cookie: " + cookie);
        // Set "Cookie2: <cookie>" if cookies 2 exists for url?
    }

    fields.append("Sec-WebSocket-Key1: " + m_secWebSocketKey1);
    fields.append("Sec-WebSocket-Key2: " + m_secWebSocketKey2);

    // Fields in the handshake are sent by the client in a random order; the
    // order is not meaningful.  Thus, it's ok to send the order we constructed
    // the fields.

    for (size_t i = 0; i < fields.size(); i++) {
        builder.append(fields[i]);
        builder.append("\r\n");
    }

    builder.append("\r\n");

    CString handshakeHeader = builder.toString().utf8();
    char* characterBuffer = 0;
    CString msg = CString::newUninitialized(handshakeHeader.length() + sizeof(m_key3), characterBuffer);
    memcpy(characterBuffer, handshakeHeader.data(), handshakeHeader.length());
    memcpy(characterBuffer + handshakeHeader.length(), m_key3, sizeof(m_key3));
    return msg;
}

WebSocketHandshakeRequest WebSocketHandshake::clientHandshakeRequest() const
{
    // Keep the following consistent with clientHandshakeMessage().
    // FIXME: do we need to store m_secWebSocketKey1, m_secWebSocketKey2 and
    // m_key3 in WebSocketHandshakeRequest?
    WebSocketHandshakeRequest request("GET", m_url);
    request.addHeaderField("Upgrade", "WebSocket");
    request.addHeaderField("Connection", "Upgrade");
    request.addHeaderField("Host", hostName(m_url, m_secure));
    request.addHeaderField("Origin", clientOrigin());
    if (!m_clientProtocol.isEmpty())
        request.addHeaderField("Sec-WebSocket-Protocol:", m_clientProtocol);

    KURL url = httpURLForAuthenticationAndCookies();
    if (m_context->isDocument()) {
        Document* document = static_cast<Document*>(m_context);
        String cookie = cookieRequestHeaderFieldValue(document, url);
        if (!cookie.isEmpty())
            request.addHeaderField("Cookie", cookie);
        // Set "Cookie2: <cookie>" if cookies 2 exists for url?
    }

    request.addHeaderField("Sec-WebSocket-Key1", m_secWebSocketKey1);
    request.addHeaderField("Sec-WebSocket-Key2", m_secWebSocketKey2);
    request.setKey3(m_key3);

    return request;
}

void WebSocketHandshake::reset()
{
    m_mode = Incomplete;
}

void WebSocketHandshake::clearScriptExecutionContext()
{
    m_context = 0;
}

int WebSocketHandshake::readServerHandshake(const char* header, size_t len)
{
    m_mode = Incomplete;
    int statusCode;
    String statusText;
    int lineLength = readStatusLine(header, len, statusCode, statusText);
    if (lineLength == -1)
        return -1;
    if (statusCode == -1) {
        m_mode = Failed;
        return len;
    }
    LOG(Network, "response code: %d", statusCode);
    m_response.setStatusCode(statusCode);
    m_response.setStatusText(statusText);
    if (statusCode != 101) {
        m_mode = Failed;
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, makeString("Unexpected response code: ", String::number(statusCode)), 0, clientOrigin(), 0);
        return len;
    }
    m_mode = Normal;
    if (!strnstr(header, "\r\n\r\n", len)) {
        // Just hasn't been received fully yet.
        m_mode = Incomplete;
        return -1;
    }
    const char* p = readHTTPHeaders(header + lineLength, header + len);
    if (!p) {
        LOG(Network, "readHTTPHeaders failed");
        m_mode = Failed;
        return len;
    }
    if (!checkResponseHeaders()) {
        LOG(Network, "header process failed");
        m_mode = Failed;
        return p - header;
    }
    if (len < static_cast<size_t>(p - header + sizeof(m_expectedChallengeResponse))) {
        // Just hasn't been received /expected/ yet.
        m_mode = Incomplete;
        return -1;
    }
    m_response.setChallengeResponse(static_cast<const unsigned char*>(static_cast<const void*>(p)));
    if (memcmp(p, m_expectedChallengeResponse, sizeof(m_expectedChallengeResponse))) {
        m_mode = Failed;
        return (p - header) + sizeof(m_expectedChallengeResponse);
    }
    m_mode = Connected;
    return (p - header) + sizeof(m_expectedChallengeResponse);
}

WebSocketHandshake::Mode WebSocketHandshake::mode() const
{
    return m_mode;
}

String WebSocketHandshake::serverWebSocketOrigin() const
{
    return m_response.headerFields().get("sec-websocket-origin");
}

String WebSocketHandshake::serverWebSocketLocation() const
{
    return m_response.headerFields().get("sec-websocket-location");
}

String WebSocketHandshake::serverWebSocketProtocol() const
{
    return m_response.headerFields().get("sec-websocket-protocol");
}

String WebSocketHandshake::serverSetCookie() const
{
    return m_response.headerFields().get("set-cookie");
}

String WebSocketHandshake::serverSetCookie2() const
{
    return m_response.headerFields().get("set-cookie2");
}

String WebSocketHandshake::serverUpgrade() const
{
    return m_response.headerFields().get("upgrade");
}

String WebSocketHandshake::serverConnection() const
{
    return m_response.headerFields().get("connection");
}

const WebSocketHandshakeResponse& WebSocketHandshake::serverHandshakeResponse() const
{
    return m_response;
}

KURL WebSocketHandshake::httpURLForAuthenticationAndCookies() const
{
    KURL url = m_url.copy();
    bool couldSetProtocol = url.setProtocol(m_secure ? "https" : "http");
    ASSERT_UNUSED(couldSetProtocol, couldSetProtocol);
    return url;
}

// Returns the header length (including "\r\n"), or -1 if we have not received enough data yet.
// If the line is malformed or the status code is not a 3-digit number,
// statusCode and statusText will be set to -1 and a null string, respectively.
int WebSocketHandshake::readStatusLine(const char* header, size_t headerLength, int& statusCode, String& statusText)
{
    // Arbitrary size limit to prevent the server from sending an unbounded
    // amount of data with no newlines and forcing us to buffer it all.
    static const int maximumLength = 1024;

    statusCode = -1;
    statusText = String();

    const char* space1 = 0;
    const char* space2 = 0;
    const char* p;
    size_t consumedLength;

    for (p = header, consumedLength = 0; consumedLength < headerLength; p++, consumedLength++) {
        if (*p == ' ') {
            if (!space1)
                space1 = p;
            else if (!space2)
                space2 = p;
        } else if (*p == '\0') {
            // The caller isn't prepared to deal with null bytes in status
            // line. WebSockets specification doesn't prohibit this, but HTTP
            // does, so we'll just treat this as an error. 
            m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Status line contains embedded null", 0, clientOrigin(), 0);
            return p + 1 - header;
        } else if (*p == '\n')
            break;
    }
    if (consumedLength == headerLength)
        return -1; // We have not received '\n' yet.

    const char* end = p + 1;
    if (end - header > maximumLength) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Status line is too long", 0, clientOrigin(), 0);
        return maximumLength;
    }
    int lineLength = end - header;

    if (!space1 || !space2) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "No response code found: " + trimConsoleMessage(header, lineLength - 1), 0, clientOrigin(), 0);
        return lineLength;
    }

    // The line must end with "\r\n".
    if (*(end - 2) != '\r') {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Status line does not end with CRLF", 0, clientOrigin(), 0);
        return lineLength;
    }

    String statusCodeString(space1 + 1, space2 - space1 - 1);
    if (statusCodeString.length() != 3) // Status code must consist of three digits.
        return lineLength;
    for (int i = 0; i < 3; ++i)
        if (statusCodeString[i] < '0' || statusCodeString[i] > '9') {
            m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Invalid status code: " + statusCodeString, 0, clientOrigin(), 0);
            return lineLength;
        }

    bool ok = false;
    statusCode = statusCodeString.toInt(&ok);
    ASSERT(ok);

    statusText = String(space2 + 1, end - space2 - 3); // Exclude "\r\n".
    return lineLength;
}

const char* WebSocketHandshake::readHTTPHeaders(const char* start, const char* end)
{
    m_response.clearHeaderFields();

    Vector<char> name;
    Vector<char> value;
    for (const char* p = start; p < end; p++) {
        name.clear();
        value.clear();

        for (; p < end; p++) {
            switch (*p) {
            case '\r':
                if (name.isEmpty()) {
                    if (p + 1 < end && *(p + 1) == '\n')
                        return p + 2;
                    m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "CR doesn't follow LF at " + trimConsoleMessage(p, end - p), 0, clientOrigin(), 0);
                    return 0;
                }
                m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Unexpected CR in name at " + trimConsoleMessage(name.data(), name.size()), 0, clientOrigin(), 0);
                return 0;
            case '\n':
                m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Unexpected LF in name at " + trimConsoleMessage(name.data(), name.size()), 0, clientOrigin(), 0);
                return 0;
            case ':':
                break;
            default:
                name.append(*p);
                continue;
            }
            if (*p == ':') {
                ++p;
                break;
            }
        }

        for (; p < end && *p == 0x20; p++) { }

        for (; p < end; p++) {
            switch (*p) {
            case '\r':
                break;
            case '\n':
                m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Unexpected LF in value at " + trimConsoleMessage(value.data(), value.size()), 0, clientOrigin(), 0);
                return 0;
            default:
                value.append(*p);
            }
            if (*p == '\r') {
                ++p;
                break;
            }
        }
        if (p >= end || *p != '\n') {
            m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "CR doesn't follow LF after value at " + trimConsoleMessage(p, end - p), 0, clientOrigin(), 0);
            return 0;
        }
        AtomicString nameStr = AtomicString::fromUTF8(name.data(), name.size());
        String valueStr = String::fromUTF8(value.data(), value.size());
        if (nameStr.isNull()) {
            m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "invalid UTF-8 sequence in header name", 0, clientOrigin(), 0);
            return 0;
        }
        if (valueStr.isNull()) {
            m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "invalid UTF-8 sequence in header value", 0, clientOrigin(), 0);
            return 0;
        }
        LOG(Network, "name=%s value=%s", nameStr.string().utf8().data(), valueStr.utf8().data());
        m_response.addHeaderField(nameStr, valueStr);
    }
    ASSERT_NOT_REACHED();
    return 0;
}

bool WebSocketHandshake::checkResponseHeaders()
{
    const String& serverWebSocketLocation = this->serverWebSocketLocation();
    const String& serverWebSocketOrigin = this->serverWebSocketOrigin();
    const String& serverWebSocketProtocol = this->serverWebSocketProtocol();
    const String& serverUpgrade = this->serverUpgrade();
    const String& serverConnection = this->serverConnection();

    if (serverUpgrade.isNull()) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: 'Upgrade' header is missing", 0, clientOrigin(), 0);
        return false;
    }
    if (serverConnection.isNull()) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: 'Connection' header is missing", 0, clientOrigin(), 0);
        return false;
    }
    if (serverWebSocketOrigin.isNull()) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: 'Sec-WebSocket-Origin' header is missing", 0, clientOrigin(), 0);
        return false;
    }
    if (serverWebSocketLocation.isNull()) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: 'Sec-WebSocket-Location' header is missing", 0, clientOrigin(), 0);
        return false;
    }

    if (!equalIgnoringCase(serverUpgrade, "websocket")) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: 'Upgrade' header value is not 'WebSocket'", 0, clientOrigin(), 0);
        return false;
    }
    if (!equalIgnoringCase(serverConnection, "upgrade")) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: 'Connection' header value is not 'Upgrade'", 0, clientOrigin(), 0);
        return false;
    }

    if (clientOrigin() != serverWebSocketOrigin) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: origin mismatch: " + clientOrigin() + " != " + serverWebSocketOrigin, 0, clientOrigin(), 0);
        return false;
    }
    if (clientLocation() != serverWebSocketLocation) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: location mismatch: " + clientLocation() + " != " + serverWebSocketLocation, 0, clientOrigin(), 0);
        return false;
    }
    if (!m_clientProtocol.isEmpty() && m_clientProtocol != serverWebSocketProtocol) {
        m_context->addMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, "Error during WebSocket handshake: protocol mismatch: " + m_clientProtocol + " != " + serverWebSocketProtocol, 0, clientOrigin(), 0);
        return false;
    }
    return true;
}

} // namespace WebCore

#endif // ENABLE(WEB_SOCKETS)
