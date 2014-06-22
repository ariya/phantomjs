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
#include "WKRenderLayer.h"

#include "WKAPICast.h"
#include "WebRenderLayer.h"

using namespace WebCore;
using namespace WebKit;

WKTypeID WKRenderLayerGetTypeID()
{
    return toAPI(WebRenderLayer::APIType);
}

WKRenderObjectRef WKRenderLayerGetRenderer(WKRenderLayerRef renderLayerRef)
{
    return toAPI(toImpl(renderLayerRef)->renderer());
}

WKStringRef WKRenderLayerCopyRendererName(WKRenderLayerRef renderLayerRef)
{
    return toCopiedAPI(toImpl(renderLayerRef)->renderer()->name());
}

WKStringRef WKRenderLayerCopyElementTagName(WKRenderLayerRef renderLayerRef)
{
    WebRenderLayer* renderLayer = toImpl(renderLayerRef);
    if (!renderLayer->renderer()->elementTagName().isNull())
        return toCopiedAPI(renderLayer->renderer()->elementTagName());

    return 0;
}

WKStringRef WKRenderLayerCopyElementID(WKRenderLayerRef renderLayerRef)
{
    WebRenderLayer* renderLayer = toImpl(renderLayerRef);
    if (!renderLayer->renderer()->elementID().isNull())
        return toCopiedAPI(renderLayer->renderer()->elementID());

    return 0;
}

WKArrayRef WKRenderLayerGetElementClassNames(WKRenderLayerRef renderLayerRef)
{
    return toAPI(toImpl(renderLayerRef)->renderer()->elementClassNames());
}

WKRect WKRenderLayerGetAbsoluteBounds(WKRenderLayerRef renderLayerRef)
{
    IntRect bounds = toImpl(renderLayerRef)->absoluteBoundingBox();
    return WKRectMake(bounds.x(), bounds.y(), bounds.width(), bounds.height());
}

bool WKRenderLayerIsClipping(WKRenderLayerRef renderLayerRef)
{
    return toImpl(renderLayerRef)->isClipping();
}

bool WKRenderLayerIsClipped(WKRenderLayerRef renderLayerRef)
{
    return toImpl(renderLayerRef)->isClipped();
}

bool WKRenderLayerIsReflection(WKRenderLayerRef renderLayerRef)
{
    return toImpl(renderLayerRef)->isReflection();
}

WKCompositingLayerType WKRenderLayerGetCompositingLayerType(WKRenderLayerRef renderLayerRef)
{
    switch (toImpl(renderLayerRef)->compositingLayerType()) {
    case WebRenderLayer::None:
        return kWKCompositingLayerTypeNone;
    case WebRenderLayer::Normal:
        return kWKCompositingLayerTypeNormal;
    case WebRenderLayer::Tiled:
        return kWKCompositingLayerTypeTiled;
    case WebRenderLayer::Media:
        return kWKCompositingLayerTypeMedia;
    case WebRenderLayer::Container:
        return kWKCompositingLayerTypeContainer;
    }

    ASSERT_NOT_REACHED();
    return kWKCompositingLayerTypeNone;
}

WKArrayRef WKRenderLayerGetNegativeZOrderList(WKRenderLayerRef renderLayerRef)
{
    return toAPI(toImpl(renderLayerRef)->negativeZOrderList());
}

WKArrayRef WKRenderLayerGetNormalFlowList(WKRenderLayerRef renderLayerRef)
{
    return toAPI(toImpl(renderLayerRef)->normalFlowList());
}

WKArrayRef WKRenderLayerGetPositiveZOrderList(WKRenderLayerRef renderLayerRef)
{
    return toAPI(toImpl(renderLayerRef)->positiveZOrderList());
}
