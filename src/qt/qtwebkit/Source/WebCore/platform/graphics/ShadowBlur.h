/*
 * Copyright (C) 2011 Apple Inc.
 * Copyright (C) 2010 Sencha, Inc.
 * Copyright (C) 2010 Igalia S.L.
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ShadowBlur_h
#define ShadowBlur_h

#include "Color.h"
#include "ColorSpace.h"
#include "FloatRect.h"
#include "RoundedRect.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class AffineTransform;
class GraphicsContext;
struct GraphicsContextState;
class ImageBuffer;

class ShadowBlur {
    WTF_MAKE_NONCOPYABLE(ShadowBlur);
public:
    enum ShadowType {
        NoShadow,
        SolidShadow,
        BlurShadow
    };

    ShadowBlur(const FloatSize& radius, const FloatSize& offset, const Color&, ColorSpace);
    ShadowBlur(const GraphicsContextState&);
    ShadowBlur();

    void setShadowValues(const FloatSize&, const FloatSize& , const Color&, ColorSpace, bool ignoreTransforms = false);

    void setShadowsIgnoreTransforms(bool ignoreTransforms) { m_shadowsIgnoreTransforms = ignoreTransforms; }
    bool shadowsIgnoreTransforms() const { return m_shadowsIgnoreTransforms; }

    GraphicsContext* beginShadowLayer(GraphicsContext*, const FloatRect& layerArea);
    void endShadowLayer(GraphicsContext*);

    void drawRectShadow(GraphicsContext*, const FloatRect&, const RoundedRect::Radii&);
    void drawInsetShadow(GraphicsContext*, const FloatRect&, const FloatRect& holeRect, const RoundedRect::Radii& holeRadii);

    void blurLayerImage(unsigned char*, const IntSize&, int stride);

    void clear();

    ShadowType type() const { return m_type; }

private:
    void updateShadowBlurValues();

    void drawShadowBuffer(GraphicsContext*);

    void adjustBlurRadius(GraphicsContext*);
    
    enum ShadowDirection {
        OuterShadow,
        InnerShadow
    };
    
    IntRect calculateLayerBoundingRect(GraphicsContext*, const FloatRect& layerArea, const IntRect& clipRect);
    IntSize templateSize(const IntSize& blurredEdgeSize, const RoundedRect::Radii&) const;

    void drawRectShadowWithoutTiling(GraphicsContext*, const FloatRect&, const RoundedRect::Radii&, const IntRect& layerRect);
    void drawRectShadowWithTiling(GraphicsContext*, const FloatRect&, const RoundedRect::Radii&, const IntSize& shadowTemplateSize, const IntSize& blurredEdgeSize);

    void drawInsetShadowWithoutTiling(GraphicsContext*, const FloatRect&, const FloatRect& holeRect, const RoundedRect::Radii&, const IntRect& layerRect);
    void drawInsetShadowWithTiling(GraphicsContext*, const FloatRect&, const FloatRect& holeRect, const RoundedRect::Radii&, const IntSize& shadowTemplateSize, const IntSize& blurredEdgeSize);
    
    void drawLayerPieces(GraphicsContext*, const FloatRect& shadowBounds, const RoundedRect::Radii&, const IntSize& roundedRadius, const IntSize& templateSize, ShadowDirection);
    
    void blurShadowBuffer(const IntSize& templateSize);
    void blurAndColorShadowBuffer(const IntSize& templateSize);
    
    IntSize blurredEdgeSize() const;
    
    
    ShadowType m_type;

    Color m_color;
    ColorSpace m_colorSpace;
    FloatSize m_blurRadius;
    FloatSize m_offset;

    ImageBuffer* m_layerImage; // Buffer to where the temporary shadow will be drawn to.

    FloatRect m_sourceRect; // Sub-rect of m_layerImage that contains the shadow pixels.
    FloatPoint m_layerOrigin; // Top-left corner of the (possibly clipped) bounding rect to draw the shadow to.
    FloatSize m_layerSize; // Size of m_layerImage pixels that need blurring.
    FloatSize m_layerContextTranslation; // Translation to apply to m_layerContext for the shadow to be correctly clipped.

    bool m_shadowsIgnoreTransforms;
};

} // namespace WebCore

#endif // ShadowBlur_h
