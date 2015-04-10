/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "NetworkProcessCreationParameters.h"

#if ENABLE(NETWORK_PROCESS)

#include "ArgumentCoders.h"

namespace WebKit {

NetworkProcessCreationParameters::NetworkProcessCreationParameters()
{
}

void NetworkProcessCreationParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << diskCacheDirectory;
    encoder << diskCacheDirectoryExtensionHandle;
    encoder << privateBrowsingEnabled;
    encoder.encodeEnum(cacheModel);
#if PLATFORM(MAC)
    encoder << parentProcessName;
    encoder << uiProcessBundleIdentifier;
    encoder << nsURLCacheMemoryCapacity;
    encoder << nsURLCacheDiskCapacity;
#if ENABLE(CUSTOM_PROTOCOLS)
    encoder << urlSchemesRegisteredForCustomProtocols;
#endif
    encoder << httpProxy;
    encoder << httpsProxy;
#endif
}

bool NetworkProcessCreationParameters::decode(CoreIPC::ArgumentDecoder& decoder, NetworkProcessCreationParameters& result)
{
    if (!decoder.decode(result.diskCacheDirectory))
        return false;
    if (!decoder.decode(result.diskCacheDirectoryExtensionHandle))
        return false;
    if (!decoder.decode(result.privateBrowsingEnabled))
        return false;
    if (!decoder.decodeEnum(result.cacheModel))
        return false;
#if PLATFORM(MAC)
    if (!decoder.decode(result.parentProcessName))
        return false;
    if (!decoder.decode(result.uiProcessBundleIdentifier))
        return false;
    if (!decoder.decode(result.nsURLCacheMemoryCapacity))
        return false;
    if (!decoder.decode(result.nsURLCacheDiskCapacity))
        return false;
#if ENABLE(CUSTOM_PROTOCOLS)
    if (!decoder.decode(result.urlSchemesRegisteredForCustomProtocols))
        return false;
#endif
    if (!decoder.decode(result.httpProxy))
        return false;
    if (!decoder.decode(result.httpsProxy))
        return false;
#endif

    return true;
}

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)
