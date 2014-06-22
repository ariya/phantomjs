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

#ifndef WKRenderLayer_h
#define WKRenderLayer_h


#include <WebKit2/WKBase.h>
#include <WebKit2/WKGeometry.h>

#ifdef __cplusplus
extern "C" {
#endif

WK_EXPORT WKTypeID WKRenderLayerGetTypeID();

WK_EXPORT WKRenderObjectRef WKRenderLayerGetRenderer(WKRenderLayerRef renderLayer);

// FIXME: Remove this function once Safari does not require it.
WK_EXPORT WKStringRef WKRenderLayerCopyRendererName(WKRenderLayerRef renderLayer);

// FIXME: Remove these three functions once Safari does not require them.
WK_EXPORT WKStringRef WKRenderLayerCopyElementTagName(WKRenderLayerRef renderLayer);
WK_EXPORT WKStringRef WKRenderLayerCopyElementID(WKRenderLayerRef renderLayer);
WK_EXPORT WKArrayRef WKRenderLayerGetElementClassNames(WKRenderLayerRef renderLayer);

WK_EXPORT WKRect WKRenderLayerGetAbsoluteBounds(WKRenderLayerRef renderLayer);

WK_EXPORT bool WKRenderLayerIsClipping(WKRenderLayerRef renderLayer);
WK_EXPORT bool WKRenderLayerIsClipped(WKRenderLayerRef renderLayer);
WK_EXPORT bool WKRenderLayerIsReflection(WKRenderLayerRef renderLayer);

enum WKCompositingLayerType {
    kWKCompositingLayerTypeNone,
    kWKCompositingLayerTypeNormal,
    kWKCompositingLayerTypeTiled,
    kWKCompositingLayerTypeMedia,
    kWKCompositingLayerTypeContainer
};
typedef enum WKCompositingLayerType WKCompositingLayerType;

WK_EXPORT WKCompositingLayerType WKRenderLayerGetCompositingLayerType(WKRenderLayerRef renderLayer);

WK_EXPORT WKArrayRef WKRenderLayerGetNegativeZOrderList(WKRenderLayerRef renderLayer);
WK_EXPORT WKArrayRef WKRenderLayerGetNormalFlowList(WKRenderLayerRef renderLayer);
WK_EXPORT WKArrayRef WKRenderLayerGetPositiveZOrderList(WKRenderLayerRef renderLayer);

#ifdef __cplusplus
}
#endif

#endif // WKRenderLayer_h
