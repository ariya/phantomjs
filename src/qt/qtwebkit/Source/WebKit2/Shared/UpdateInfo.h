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

#ifndef UpdateInfo_h
#define UpdateInfo_h

#include "ShareableBitmap.h"
#include <WebCore/IntRect.h>
#include <wtf/Noncopyable.h>

namespace CoreIPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

class UpdateInfo {
    WTF_MAKE_NONCOPYABLE(UpdateInfo);

public:
    UpdateInfo() { }

    void encode(CoreIPC::ArgumentEncoder&) const;
    static bool decode(CoreIPC::ArgumentDecoder&, UpdateInfo&);

    // The size of the web view.
    WebCore::IntSize viewSize;
    float deviceScaleFactor;

    // The rect and delta to be scrolled.
    WebCore::IntRect scrollRect;
    WebCore::IntSize scrollOffset;
    
    // The bounds of the update rects.
    WebCore::IntRect updateRectBounds;

    // All the update rects, in view coordinates.
    Vector<WebCore::IntRect> updateRects;

    // The page scale factor used to render this update.
    float updateScaleFactor;

    // The handle of the shareable bitmap containing the updates. Will be null if there are no updates.
    ShareableBitmap::Handle bitmapHandle;

    // The offset in the bitmap where the rendered contents are.
    WebCore::IntPoint bitmapOffset;
};

} // namespace WebKit

#endif // UpdateInfo_h
