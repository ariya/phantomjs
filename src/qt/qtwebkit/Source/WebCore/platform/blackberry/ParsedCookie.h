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

#ifndef ParsedCookie_h
#define ParsedCookie_h

#include "Cookie.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WTF {
class String;
}
namespace WebCore {

class KURL;

// This class represents a cookie internally
// It can either be created by the CookieParser which will then fill it
// or it can be created by the backing store filling it in the constructor.
class ParsedCookie : public RefCounted<ParsedCookie> {
public:

    // Default cookie : empty domain, non secure and session
    static PassRefPtr<ParsedCookie> create(double currentTime) { return adoptRef(new ParsedCookie(currentTime)); }

    // For backing store cookies (those cookies are never session cookies).
    static PassRefPtr<ParsedCookie> create(const String& name, const String& value, const String& domain, const String& protocol, const String& path, double expiry, double lastAccessed, double creationTime, bool isSecure, bool isHttpOnly)  { return adoptRef(new ParsedCookie(name, value, domain, protocol, path, expiry, lastAccessed, creationTime, isSecure, isHttpOnly)); };

    const String& name() const { return m_name; }
    void setName(const String& name) { m_name = name; }

    const String& value() const { return m_value; }
    void setValue(const String& value) { m_value = value; }

    const String& path() const { return m_path; }
    void setPath(const String& path) { m_path = path; }

    const String& domain() const { return m_domain; }
    void setDomain(const String& domain, bool domainIsIPAddress = false) { m_domain = domain.lower(); m_domainIsIPAddress = domainIsIPAddress; }

    const String& protocol() const { return m_protocol; }
    void setProtocol(const String& protocol) { m_protocol = protocol; }

    double expiry() const { return m_expiry; }
    void setExpiry(const String&);
    void forceExpire() { m_isForceExpired  = true; }
    void setMaxAge(const String&);

    double lastAccessed() const { return m_lastAccessed; }
    void setLastAccessed(double lastAccessed) { m_lastAccessed = lastAccessed;}

    double creationTime() const { return m_creationTime; }
    void setCreationTime(double creationTime) { m_creationTime = creationTime; }

    bool isSecure() const { return m_isSecure; }
    void setSecureFlag(bool secure) { m_isSecure = secure; }

    bool isHttpOnly() const { return m_isHttpOnly; }
    void setIsHttpOnly(bool isHttpOnly) { m_isHttpOnly = isHttpOnly; }

    bool isSession() const { return m_isSession; }

    bool isHostOnly() const { return !m_domain.startsWith("."); }

    bool hasExpired() const;
    bool isForceExpired() const { return m_isForceExpired; }
    bool isUnderSizeLimit() const;
    bool domainIsIPAddress() const { return m_domainIsIPAddress; }

    String toString() const;
    String toNameValuePair() const;
    void appendWebCoreCookie(Vector<Cookie>& cookieVector) const;

private:
    // Default cookie : empty domain, non secure and session
    ParsedCookie(double currentTime);

    // For backing store cookies (those cookies are never session cookies).
    ParsedCookie(const String& name, const String& value, const String& domain, const String& protocol, const String& path, double expiry, double lastAccessed, double creationTime, bool isSecure, bool isHttpOnly);

    String m_name;
    String m_value;
    String m_domain;
    String m_protocol;
    String m_path;
    double m_expiry;
    double m_creationTime;
    // This is used for the LRU replacement policy.
    double m_lastAccessed;

    bool m_isSecure;
    bool m_isHttpOnly;
    bool m_isSession;
    bool m_isForceExpired;
    bool m_domainIsIPAddress;
};

} // namespace WebCore

#endif // ParsedCookie_h
