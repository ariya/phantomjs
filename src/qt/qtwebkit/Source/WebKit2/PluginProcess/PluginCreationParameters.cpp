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
#include "PluginCreationParameters.h"

#if ENABLE(PLUGIN_PROCESS)

#include "ArgumentCoders.h"

namespace WebKit {

PluginCreationParameters::PluginCreationParameters()
    : pluginInstanceID(0)
    , windowNPObjectID(0)
    , contentsScaleFactor(1)
    , isPrivateBrowsingEnabled(false)
    , asynchronousCreationIncomplete(false)
    , artificialPluginInitializationDelayEnabled(false)
#if USE(ACCELERATED_COMPOSITING)
    , isAcceleratedCompositingEnabled(false)
#endif
{
}

void PluginCreationParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << pluginInstanceID;
    encoder << windowNPObjectID;
    encoder << parameters;
    encoder << userAgent;
    encoder << contentsScaleFactor;
    encoder << isPrivateBrowsingEnabled;
    encoder << asynchronousCreationIncomplete;
    encoder << artificialPluginInitializationDelayEnabled;

#if USE(ACCELERATED_COMPOSITING)
    encoder << isAcceleratedCompositingEnabled;
#endif
}

bool PluginCreationParameters::decode(CoreIPC::ArgumentDecoder& decoder, PluginCreationParameters& result)
{
    if (!decoder.decode(result.pluginInstanceID) || !result.pluginInstanceID)
        return false;

    if (!decoder.decode(result.windowNPObjectID))
        return false;

    if (!decoder.decode(result.parameters))
        return false;

    if (!decoder.decode(result.userAgent))
        return false;

    if (!decoder.decode(result.contentsScaleFactor))
        return false;

    if (!decoder.decode(result.isPrivateBrowsingEnabled))
        return false;

    if (!decoder.decode(result.asynchronousCreationIncomplete))
        return false;

    if (!decoder.decode(result.artificialPluginInitializationDelayEnabled))
        return false;

#if USE(ACCELERATED_COMPOSITING)
    if (!decoder.decode(result.isAcceleratedCompositingEnabled))
        return false;
#endif

    return true;
}


} // namespace WebKit

#endif // ENABLE(PLUGIN_PROCESS)
