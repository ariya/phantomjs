/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WebProtectionSpace.h"

#include <WebCore/SharedBuffer.h>

namespace WebKit {

WebProtectionSpace::WebProtectionSpace(const WebCore::ProtectionSpace& coreProtectionSpace)
    : m_coreProtectionSpace(coreProtectionSpace)
{
}

const String& WebProtectionSpace::host() const
{
    return m_coreProtectionSpace.host();
}

int WebProtectionSpace::port() const
{
    return m_coreProtectionSpace.port();
}

const String& WebProtectionSpace::realm() const
{
    return m_coreProtectionSpace.realm();
}

bool WebProtectionSpace::isProxy() const
{
    return m_coreProtectionSpace.isProxy();
}

WebCore::ProtectionSpaceServerType WebProtectionSpace::serverType() const
{
    return m_coreProtectionSpace.serverType();
}

bool WebProtectionSpace::receivesCredentialSecurely() const
{
    return m_coreProtectionSpace.receivesCredentialSecurely();
}

WebCore::ProtectionSpaceAuthenticationScheme WebProtectionSpace::authenticationScheme() const
{
    return m_coreProtectionSpace.authenticationScheme();
}

} // namespace WebKit
