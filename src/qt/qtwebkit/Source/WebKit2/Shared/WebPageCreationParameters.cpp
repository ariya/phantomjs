/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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
#include "WebPageCreationParameters.h"

#include "WebCoreArgumentCoders.h"

namespace WebKit {

void WebPageCreationParameters::encode(CoreIPC::ArgumentEncoder& encoder) const
{
    encoder << viewSize;
    encoder << isActive;
    encoder << isFocused;
    encoder << isVisible;
    encoder << isInWindow;

    encoder << store;
    encoder.encodeEnum(drawingAreaType);
    encoder << pageGroupData;
    encoder << drawsBackground;
    encoder << drawsTransparentBackground;
    encoder << underlayColor;
    encoder << areMemoryCacheClientCallsEnabled;
    encoder << useFixedLayout;
    encoder << fixedLayoutSize;
    encoder.encodeEnum(paginationMode);
    encoder << paginationBehavesLikeColumns;
    encoder << pageLength;
    encoder << gapBetweenPages;
    encoder << userAgent;
    encoder << sessionState;
    encoder << highestUsedBackForwardItemID;
    encoder << canRunBeforeUnloadConfirmPanel;
    encoder << canRunModal;
    encoder << deviceScaleFactor;
    encoder << mediaVolume;
    encoder << mayStartMediaWhenInWindow;
    encoder << minimumLayoutSize;
    encoder.encodeEnum(scrollPinningBehavior);

#if PLATFORM(MAC)
    encoder.encodeEnum(layerHostingMode);
    encoder << colorSpace;
#endif
}

bool WebPageCreationParameters::decode(CoreIPC::ArgumentDecoder& decoder, WebPageCreationParameters& parameters)
{
    if (!decoder.decode(parameters.viewSize))
        return false;
    if (!decoder.decode(parameters.isActive))
        return false;
    if (!decoder.decode(parameters.isFocused))
        return false;
    if (!decoder.decode(parameters.isVisible))
        return false;
    if (!decoder.decode(parameters.isInWindow))
        return false;
    if (!decoder.decode(parameters.store))
        return false;
    if (!decoder.decodeEnum(parameters.drawingAreaType))
        return false;
    if (!decoder.decode(parameters.pageGroupData))
        return false;
    if (!decoder.decode(parameters.drawsBackground))
        return false;
    if (!decoder.decode(parameters.drawsTransparentBackground))
        return false;
    if (!decoder.decode(parameters.underlayColor))
        return false;
    if (!decoder.decode(parameters.areMemoryCacheClientCallsEnabled))
        return false;
    if (!decoder.decode(parameters.useFixedLayout))
        return false;
    if (!decoder.decode(parameters.fixedLayoutSize))
        return false;
    if (!decoder.decodeEnum(parameters.paginationMode))
        return false;
    if (!decoder.decode(parameters.paginationBehavesLikeColumns))
        return false;
    if (!decoder.decode(parameters.pageLength))
        return false;
    if (!decoder.decode(parameters.gapBetweenPages))
        return false;
    if (!decoder.decode(parameters.userAgent))
        return false;
    if (!decoder.decode(parameters.sessionState))
        return false;
    if (!decoder.decode(parameters.highestUsedBackForwardItemID))
        return false;
    if (!decoder.decode(parameters.canRunBeforeUnloadConfirmPanel))
        return false;
    if (!decoder.decode(parameters.canRunModal))
        return false;
    if (!decoder.decode(parameters.deviceScaleFactor))
        return false;
    if (!decoder.decode(parameters.mediaVolume))
        return false;
    if (!decoder.decode(parameters.mayStartMediaWhenInWindow))
        return false;
    if (!decoder.decode(parameters.minimumLayoutSize))
        return false;
    if (!decoder.decodeEnum(parameters.scrollPinningBehavior))
        return false;
    
#if PLATFORM(MAC)
    if (!decoder.decodeEnum(parameters.layerHostingMode))
        return false;
    if (!decoder.decode(parameters.colorSpace))
        return false;
#endif

    return true;
}

} // namespace WebKit
