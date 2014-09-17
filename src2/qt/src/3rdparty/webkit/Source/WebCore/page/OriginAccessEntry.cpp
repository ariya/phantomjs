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

#include "config.h"
#include "OriginAccessEntry.h"

#include "SecurityOrigin.h"

namespace WebCore {
    
OriginAccessEntry::OriginAccessEntry(const String& protocol, const String& host, SubdomainSetting subdomainSetting)
    : m_protocol(protocol.lower())
    , m_host(host.lower())
    , m_subdomainSettings(subdomainSetting)
{
    ASSERT(subdomainSetting == AllowSubdomains || subdomainSetting == DisallowSubdomains);

    // Assume that any host that ends with a digit is trying to be an IP address.
    m_hostIsIPAddress = !m_host.isEmpty() && isASCIIDigit(m_host[m_host.length() - 1]);
}

bool OriginAccessEntry::matchesOrigin(const SecurityOrigin& origin) const
{
    ASSERT(origin.host() == origin.host().lower());
    ASSERT(origin.protocol() == origin.protocol().lower());

    if (m_protocol != origin.protocol())
        return false;
    
    // Special case: Include subdomains and empty host means "all hosts, including ip addresses".
    if (m_subdomainSettings == AllowSubdomains && m_host.isEmpty())
        return true;
    
    // Exact match.
    if (m_host == origin.host())
        return true;
    
    // Otherwise we can only match if we're matching subdomains.
    if (m_subdomainSettings == DisallowSubdomains)
        return false;
    
    // Don't try to do subdomain matching on IP addresses.
    if (m_hostIsIPAddress)
        return false;
    
    // Match subdomains.
    if (origin.host().length() > m_host.length() && origin.host()[origin.host().length() - m_host.length() - 1] == '.' && origin.host().endsWith(m_host))
        return true;
    
    return false;
}
    
} // namespace WebCore
