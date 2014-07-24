/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef OriginAccessEntry_h
#define OriginAccessEntry_h

#include <wtf/text/WTFString.h>

namespace WebCore {

class SecurityOrigin;

class OriginAccessEntry {
public:
    enum SubdomainSetting {
        AllowSubdomains,
        DisallowSubdomains
    };

    // If host is empty string and SubdomainSetting is AllowSubdomains, the entry will match all domains in the specified protocol.
    OriginAccessEntry(const String& protocol, const String& host, SubdomainSetting);
    bool matchesOrigin(const SecurityOrigin&) const;

    const String& protocol() const { return m_protocol; }
    const String& host() const { return m_host; }
    SubdomainSetting subdomainSettings() const { return m_subdomainSettings; }

private:
    String m_protocol;
    String m_host;
    SubdomainSetting m_subdomainSettings;
    bool m_hostIsIPAddress;
};

inline bool operator==(const OriginAccessEntry& a, const OriginAccessEntry& b)
{
    return equalIgnoringCase(a.protocol(), b.protocol()) && equalIgnoringCase(a.host(), b.host()) && a.subdomainSettings() == b.subdomainSettings();
}

inline bool operator!=(const OriginAccessEntry& a, const OriginAccessEntry& b)
{
    return !(a == b);
}

} // namespace WebCore

#endif // OriginAccessEntry_h
