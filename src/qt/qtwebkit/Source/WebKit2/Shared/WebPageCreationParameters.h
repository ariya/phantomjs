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

#ifndef WebPageCreationParameters_h
#define WebPageCreationParameters_h

#include "DrawingAreaInfo.h"
#include "SessionState.h"
#include "WebPageGroupData.h"
#include "WebPreferencesStore.h"
#include <WebCore/IntSize.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(MAC)
#include "ColorSpaceData.h"
#endif

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

struct WebPageCreationParameters {
    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, WebPageCreationParameters&);

    WebCore::IntSize viewSize;

    bool isActive;
    bool isFocused;
    bool isVisible;
    bool isInWindow;
    
    WebPreferencesStore store;
    DrawingAreaType drawingAreaType;
    WebPageGroupData pageGroupData;

    bool drawsBackground;
    bool drawsTransparentBackground;

    WebCore::Color underlayColor;

    bool areMemoryCacheClientCallsEnabled;

    bool useFixedLayout;
    WebCore::IntSize fixedLayoutSize;

    bool suppressScrollbarAnimations;

    WebCore::Pagination::Mode paginationMode;
    bool paginationBehavesLikeColumns;
    double pageLength;
    double gapBetweenPages;

    String userAgent;

    SessionState sessionState;
    uint64_t highestUsedBackForwardItemID;

    bool canRunBeforeUnloadConfirmPanel;
    bool canRunModal;

    float deviceScaleFactor;
    
    float mediaVolume;
    bool mayStartMediaWhenInWindow;

    WebCore::IntSize minimumLayoutSize;
    
    WebCore::ScrollPinningBehavior scrollPinningBehavior;

#if PLATFORM(MAC)
    LayerHostingMode layerHostingMode;
    ColorSpaceData colorSpace;
#endif
};

} // namespace WebKit

#endif // WebPageCreationParameters_h
