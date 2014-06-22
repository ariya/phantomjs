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
#include "PluginProcessCreationParameters.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentCoders.h"

namespace WebKit {

PluginProcessCreationParameters::PluginProcessCreationParameters()
    : supportsAsynchronousPluginInitialization(false)
{
}

void PluginProcessCreationParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder.encodeEnum(processType);
    encoder << supportsAsynchronousPluginInitialization;
    encoder << minimumLifetime;
    encoder << terminationTimeout;
#if PLATFORM(MAC)
    encoder << acceleratedCompositingPort;
#endif
}

bool PluginProcessCreationParameters::decode(CoreIPC::ArgumentDecoder& decoder, PluginProcessCreationParameters& result)
{
    if (!decoder.decodeEnum(result.processType))
        return false;
    if (!decoder.decode(result.supportsAsynchronousPluginInitialization))
        return false;
    if (!decoder.decode(result.minimumLifetime))
        return false;
    if (!decoder.decode(result.terminationTimeout))
        return false;
#if PLATFORM(MAC)
    if (!decoder.decode(result.acceleratedCompositingPort))
        return false;
#endif

    return true;
}


} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
