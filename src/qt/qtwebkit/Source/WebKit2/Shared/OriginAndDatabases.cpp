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
#include "OriginAndDatabases.h"

#if ENABLE(SQL_DATABASE)

#include "WebCoreArgumentCoders.h"

using namespace WebCore;

namespace WebKit {

void OriginAndDatabases::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << originIdentifier;
    encoder << originQuota;
    encoder << originUsage;
    encoder << databases;
}

bool OriginAndDatabases::decode(CoreIPC::ArgumentDecoder& decoder, OriginAndDatabases& originAndDatabases)
{
    if (!decoder.decode(originAndDatabases.originIdentifier))
        return false;
    if (!decoder.decode(originAndDatabases.originQuota))
        return false;
    if (!decoder.decode(originAndDatabases.originUsage))
        return false;
    if (!decoder.decode(originAndDatabases.databases))
        return false;

    return true;
}

} // namespace WebKit

#endif // ENABLE(SQL_DATABASE)
