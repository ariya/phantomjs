/*
 * Copyright (C) 2012 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY IGALIA S.L. ``AS IS'' AND ANY
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
#include "AuthenticationChallenge.h"

#include "ResourceError.h"
#include <libsoup/soup.h>

namespace WebCore {

static ProtectionSpaceServerType protectionSpaceServerTypeFromURI(SoupURI* uri, bool isForProxy)
{
    if (uri->scheme == SOUP_URI_SCHEME_HTTPS)
        return isForProxy ? ProtectionSpaceProxyHTTPS : ProtectionSpaceServerHTTPS;
    if (uri->scheme == SOUP_URI_SCHEME_HTTP)
        return isForProxy ? ProtectionSpaceProxyHTTP : ProtectionSpaceServerHTTP;
    if (uri->scheme == SOUP_URI_SCHEME_FTP)
        return isForProxy ? ProtectionSpaceProxyFTP : ProtectionSpaceServerFTP;
    return isForProxy ? ProtectionSpaceProxyHTTP : ProtectionSpaceServerHTTP;
}

static ProtectionSpace protectionSpaceFromSoupAuthAndMessage(SoupAuth* soupAuth, SoupMessage* message)
{
    const char* schemeName = soup_auth_get_scheme_name(soupAuth);
    ProtectionSpaceAuthenticationScheme scheme;
    if (!g_ascii_strcasecmp(schemeName, "basic"))
        scheme = ProtectionSpaceAuthenticationSchemeHTTPBasic;
    else if (!g_ascii_strcasecmp(schemeName, "digest"))
        scheme = ProtectionSpaceAuthenticationSchemeHTTPDigest;
    else if (!g_ascii_strcasecmp(schemeName, "ntlm"))
        scheme = ProtectionSpaceAuthenticationSchemeNTLM;
    else if (!g_ascii_strcasecmp(schemeName, "negotiate"))
        scheme = ProtectionSpaceAuthenticationSchemeNegotiate;
    else
        scheme = ProtectionSpaceAuthenticationSchemeUnknown;

    SoupURI* soupURI = soup_message_get_uri(message);
    return ProtectionSpace(String::fromUTF8(soup_uri_get_host(soupURI)), soup_uri_get_port(soupURI),
        protectionSpaceServerTypeFromURI(soupURI, soup_auth_is_for_proxy(soupAuth)),
        String::fromUTF8(soup_auth_get_realm(soupAuth)), scheme);
}

AuthenticationChallenge::AuthenticationChallenge(SoupSession* soupSession, SoupMessage* soupMessage, SoupAuth* soupAuth, bool retrying, AuthenticationClient* client)
    : AuthenticationChallengeBase(protectionSpaceFromSoupAuthAndMessage(soupAuth, soupMessage),
        Credential(), // proposedCredentials
        retrying ? 1 : 0, // previousFailureCount
        soupMessage, // failureResponse
        ResourceError::authenticationError(soupMessage))
    , m_soupSession(soupSession)
    , m_soupMessage(soupMessage)
    , m_soupAuth(soupAuth)
    , m_authenticationClient(client)
{
}

bool AuthenticationChallenge::platformCompare(const AuthenticationChallenge& a, const AuthenticationChallenge& b)
{
    return a.soupSession() == b.soupSession()
        && a.soupMessage() == b.soupMessage()
        && a.soupAuth() == b.soupAuth();
}

} // namespace WebCore
