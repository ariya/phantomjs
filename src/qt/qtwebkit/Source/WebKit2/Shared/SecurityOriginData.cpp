/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "SecurityOriginData.h"

#include "APIObject.h"
#include "ImmutableArray.h"
#include "WebCoreArgumentCoders.h"
#include "WebSecurityOrigin.h"

using namespace WebCore;

namespace WebKit {

SecurityOriginData SecurityOriginData::fromSecurityOrigin(SecurityOrigin* securityOrigin)
{
    SecurityOriginData securityOriginData;

    securityOriginData.protocol = securityOrigin->protocol();
    securityOriginData.host = securityOrigin->host();
    securityOriginData.port = securityOrigin->port();

    return securityOriginData;
}

PassRefPtr<SecurityOrigin> SecurityOriginData::securityOrigin() const
{
    return SecurityOrigin::create(protocol, host, port);
}

void SecurityOriginData::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << protocol;
    encoder << host;
    encoder << port;
}

bool SecurityOriginData::decode(CoreIPC::ArgumentDecoder& decoder, SecurityOriginData& securityOriginData)
{
    if (!decoder.decode(securityOriginData.protocol))
        return false;
    if (!decoder.decode(securityOriginData.host))
        return false;
    if (!decoder.decode(securityOriginData.port))
        return false;

    return true;
}

void performAPICallbackWithSecurityOriginDataVector(const Vector<SecurityOriginData>& originDatas, ArrayCallback* callback)
{
    if (!callback) {
        // FIXME: Log error or assert.
        return;
    }
    
    size_t originDataCount = originDatas.size();
    Vector<RefPtr<APIObject> > securityOrigins;
    securityOrigins.reserveCapacity(originDataCount);

    for (size_t i = 0; i < originDataCount; ++i) {
        SecurityOriginData originData = originDatas[i];
        RefPtr<APIObject> origin = WebSecurityOrigin::create(originData.protocol, originData.host, originData.port);
        if (!origin)
            continue;
        securityOrigins.uncheckedAppend(origin);
    }

    callback->performCallbackWithReturnValue(ImmutableArray::adopt(securityOrigins).get());
}

} // namespace WebKit
