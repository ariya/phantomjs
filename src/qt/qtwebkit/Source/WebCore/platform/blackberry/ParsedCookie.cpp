/*
 * Copyright (C) 2008, 2009 Julien Chaffraix <julien.chaffraix@gmail.com>
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ParsedCookie.h"

#include "CookieManager.h"
#include "KURL.h"
#include "Logging.h"
#include <curl/curl.h>
#include <wtf/CurrentTime.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

ParsedCookie::ParsedCookie(double currentTime)
    : m_expiry(-1)
    , m_creationTime(currentTime)
    , m_lastAccessed(currentTime)
    , m_isSecure(false)
    , m_isHttpOnly(false)
    , m_isSession(true)
    , m_isForceExpired(false)
    , m_domainIsIPAddress(false)
{
}

ParsedCookie::ParsedCookie(const String& name, const String& value, const String& domain, const String& protocol, const String& path, double expiry, double lastAccessed, double creationTime, bool isSecure, bool isHttpOnly)
    : m_name(name)
    , m_value(value)
    , m_domain(domain)
    , m_protocol(protocol)
    , m_path(path)
    , m_expiry(expiry)
    , m_creationTime(creationTime)
    , m_lastAccessed(lastAccessed)
    , m_isSecure(isSecure)
    , m_isHttpOnly(isHttpOnly)
    , m_isSession(false)
    , m_isForceExpired(false)
    , m_domainIsIPAddress(false)
{
}

void ParsedCookie::setExpiry(const String& expiry)
{
    // If a cookie has both the Max-Age and the Expires attribute,
    // the Max-Age attribute has precedence and controls the expiration date of the cookie.
    if (m_expiry != -1 || expiry.isEmpty())
        return;

    m_isSession = false;

    m_expiry = static_cast<double>(curl_getdate(expiry.utf8().data(), 0));

    if (m_expiry == -1) {
        LOG_ERROR("Could not parse date");
        // In this case, consider that the cookie is session only
        m_isSession = true;
    }
}

void ParsedCookie::setMaxAge(const String& maxAge)
{
    // According to the HTTP Cookie specification (RFC6265, http://tools.ietf.org/html/rfc6265),
    // the first character can be a DIGIT or a "-", and the reminder
    // of the value can only contain DIGIT characters.
    if (maxAge.isEmpty() || (maxAge[0] != '-' && !isASCIIDigit(maxAge[0]))) {
        LOG_ERROR("Could not parse Max-Age : %s, first character can only be '-' or ascii digit.", maxAge.ascii().data());
        return;
    }

    bool ok;
    int value = maxAge.toIntStrict(&ok);

    if (!ok) {
        LOG_ERROR("Could not parse Max-Age : %s", maxAge.ascii().data());
        return;
    }
    m_expiry = value;
    m_isSession = false;

    // If maxAge value is not positive, let expiry-time be the earliest representable time.
    if (m_expiry > 0)
        m_expiry += currentTime();
    else
        m_expiry = 0;
}

bool ParsedCookie::hasExpired() const
{
    // Session cookies do not expire, they will just not be saved to the backing store.
    return !m_isSession && (m_isForceExpired || m_expiry < currentTime());
}

bool ParsedCookie::isUnderSizeLimit() const
{
    return m_value.length() <= CookieManager::maxCookieLength() && m_name.length() <= CookieManager::maxCookieLength();
}

String ParsedCookie::toString() const
{
    StringBuilder builder;
    builder.append(name());
    builder.append(" = ");
    builder.append(value());
    builder.append("; Domain = ");
    builder.append(domain());
    builder.append("; Path = ");
    builder.append(path());
    builder.append("; Protocol = ");
    builder.append(protocol());
    return builder.toString();
}

String ParsedCookie::toNameValuePair() const
{
    static const String equal("=");

    size_t cookieLength = m_name.length() + m_value.length() + 2;
    Vector<UChar> result;
    result.reserveInitialCapacity(cookieLength);
    append(result, m_name);
    append(result, equal);
    append(result, m_value);

    return String::adopt(result);
}

void ParsedCookie::appendWebCoreCookie(Vector<Cookie>& cookieVector) const
{
    cookieVector.append(Cookie(String(m_name), String(m_value), String(m_domain),
        // We multiply m_expiry by 1000 to convert from seconds to milliseconds.
        // This value is passed to Web Inspector and used in the JavaScript Date constructor.
        String(m_path), (m_expiry * 1000), m_isHttpOnly, m_isSecure, m_isSession));
}
} // namespace WebCore
