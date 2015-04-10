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
#include "DrawingArea.h"

// Subclasses
#include "DrawingAreaImpl.h"

#if PLATFORM(MAC) && ENABLE(THREADED_SCROLLING)
#include "TiledCoreAnimationDrawingArea.h"
#endif

#if PLATFORM(MAC)
#include "RemoteLayerTreeDrawingArea.h"
#endif

#include "WebPageCreationParameters.h"

namespace WebKit {

PassOwnPtr<DrawingArea> DrawingArea::create(WebPage* webPage, const WebPageCreationParameters& parameters)
{
    switch (parameters.drawingAreaType) {
    case DrawingAreaTypeImpl:
        return DrawingAreaImpl::create(webPage, parameters);
#if PLATFORM(MAC) && ENABLE(THREADED_SCROLLING)
    case DrawingAreaTypeTiledCoreAnimation:
        return TiledCoreAnimationDrawingArea::create(webPage, parameters);
#endif
#if PLATFORM(MAC)
    case DrawingAreaTypeRemoteLayerTree:
        return RemoteLayerTreeDrawingArea::create(webPage, parameters);
#endif
    }

    return nullptr;
}

DrawingArea::DrawingArea(DrawingAreaType type, WebPage* webPage)
    : m_type(type)
    , m_webPage(webPage)
{
}

DrawingArea::~DrawingArea()
{
}

void DrawingArea::dispatchAfterEnsuringUpdatedScrollPosition(const Function<void ()>& function)
{
    // Scroll position updates are synchronous by default so we can just call the function right away here.
    function();
}

} // namespace WebKit
