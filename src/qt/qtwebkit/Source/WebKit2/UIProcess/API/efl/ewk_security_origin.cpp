/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ewk_security_origin.h"

#include "WKAPICast.h"
#include "WKSecurityOrigin.h"
#include "WKString.h"
#include "ewk_security_origin_private.h"

using namespace WebKit;

EwkSecurityOrigin::EwkSecurityOrigin(WKSecurityOriginRef originRef)
    : m_wkOrigin(originRef)
    , m_host(AdoptWK, WKSecurityOriginCopyHost(originRef))
    , m_protocol(AdoptWK, WKSecurityOriginCopyProtocol(originRef))
{ }

EwkSecurityOrigin::EwkSecurityOrigin(const char* url)
    : m_wkOrigin(AdoptWK, WKSecurityOriginCreateFromString(adoptWK(WKStringCreateWithUTF8CString(url)).get()))
    , m_host(AdoptWK, WKSecurityOriginCopyHost(m_wkOrigin.get()))
    , m_protocol(AdoptWK, WKSecurityOriginCopyProtocol(m_wkOrigin.get()))
{ }

const char* EwkSecurityOrigin::host() const
{
    return m_host;
}

const char* EwkSecurityOrigin::protocol() const
{
    return m_protocol;
}

uint32_t EwkSecurityOrigin::port() const
{
    return WKSecurityOriginGetPort(m_wkOrigin.get());
}

const char* ewk_security_origin_host_get(const Ewk_Security_Origin* origin)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkSecurityOrigin, origin, impl, 0);

    return impl->host();
}

const char* ewk_security_origin_protocol_get(const Ewk_Security_Origin* origin)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkSecurityOrigin, origin, impl, 0);

    return impl->protocol();
}

uint32_t ewk_security_origin_port_get(const Ewk_Security_Origin* origin)
{
    EWK_OBJ_GET_IMPL_OR_RETURN(const EwkSecurityOrigin, origin, impl, 0);

    return impl->port();
}
