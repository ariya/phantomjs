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

#ifndef WebRenderLayer_h
#define WebRenderLayer_h

#include "WebRenderObject.h"

namespace WebCore {
    class RenderLayer;
}

namespace WebKit {

class WebPage;

class WebRenderLayer : public TypedAPIObject<APIObject::TypeRenderLayer> {
public:
    enum CompositingLayerType { None, Normal, Tiled, Media, Container };

    static PassRefPtr<WebRenderLayer> create(WebPage*);
    static PassRefPtr<WebRenderLayer> create(PassRefPtr<WebRenderObject> renderer, bool isReflection, bool isClipping, bool isClipped, CompositingLayerType type,
        WebCore::IntRect absoluteBoundingBox, PassRefPtr<MutableArray> negativeZOrderList, PassRefPtr<MutableArray> normalFlowList, PassRefPtr<MutableArray> positiveZOrderList)
    {
        return adoptRef(new WebRenderLayer(renderer, isReflection, isClipping, isClipped, type, absoluteBoundingBox, negativeZOrderList, normalFlowList, positiveZOrderList));
    }

    ImmutableArray* negativeZOrderList() const { return m_negativeZOrderList.get(); }
    ImmutableArray* normalFlowList() const { return m_normalFlowList.get(); }
    ImmutableArray* positiveZOrderList() const { return m_positiveZOrderList.get(); }

    WebRenderObject* renderer() const { return m_renderer.get(); }
    bool isReflection() const { return m_isReflection; }
    bool isClipping() const { return m_isClipping; }
    bool isClipped() const { return m_isClipped; }
    CompositingLayerType compositingLayerType() const { return m_compositingLayerType; } 
    WebCore::IntRect absoluteBoundingBox() const { return m_absoluteBoundingBox; }

private:
    WebRenderLayer(WebCore::RenderLayer*);
    WebRenderLayer(PassRefPtr<WebRenderObject> renderer, bool isReflection, bool isClipping, bool isClipped, CompositingLayerType type, WebCore::IntRect absoluteBoundingBox,
        PassRefPtr<MutableArray> negativeZOrderList, PassRefPtr<MutableArray> normalFlowList, PassRefPtr<MutableArray> positiveZOrderList)
        : m_renderer(renderer)
        , m_isReflection(isReflection)
        , m_isClipping(isClipping)
        , m_isClipped(isClipped)
        , m_compositingLayerType(type)
        , m_absoluteBoundingBox(absoluteBoundingBox)
        , m_negativeZOrderList(negativeZOrderList)
        , m_normalFlowList(normalFlowList)
        , m_positiveZOrderList(positiveZOrderList)
    {
    }

    static PassRefPtr<MutableArray> createArrayFromLayerList(Vector<WebCore::RenderLayer*>*);

    RefPtr<WebRenderObject> m_renderer;
    bool m_isReflection;
    bool m_isClipping;
    bool m_isClipped;
    CompositingLayerType m_compositingLayerType;
    WebCore::IntRect m_absoluteBoundingBox;

    RefPtr<MutableArray> m_negativeZOrderList;
    RefPtr<MutableArray> m_normalFlowList;
    RefPtr<MutableArray> m_positiveZOrderList;
};

} // namespace WebKit

#endif // WebRenderLayer_h
