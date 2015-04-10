/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Sencha, Inc. All rights reserved.
 * Copyright (C) 2010 Igalia S.L. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
 * Copyright (C) 2013 Digia Plc. and/or its subsidiary(-ies).
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

#include "config.h"
#include "ShadowBlur.h"

#include "AffineTransform.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "Timer.h"
#include <wtf/MathExtras.h>
#include <wtf/Noncopyable.h>

using namespace std;

namespace WebCore {

enum {
    leftLobe = 0,
    rightLobe = 1
};

static inline int roundUpToMultipleOf32(int d)
{
    return (1 + (d >> 5)) << 5;
}

// ShadowBlur needs a scratch image as the buffer for the blur filter.
// Instead of creating and destroying the buffer for every operation,
// we create a buffer which will be automatically purged via a timer.
class ScratchBuffer {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ScratchBuffer()
        : m_purgeTimer(this, &ScratchBuffer::timerFired)
        , m_lastWasInset(false)
#if !ASSERT_DISABLED
        , m_bufferInUse(false)
#endif
    {
    }
    
    ImageBuffer* getScratchBuffer(const IntSize& size)
    {
        ASSERT(!m_bufferInUse);
#if !ASSERT_DISABLED
        m_bufferInUse = true;
#endif
        // We do not need to recreate the buffer if the current buffer is large enough.
        if (m_imageBuffer && m_imageBuffer->logicalSize().width() >= size.width() && m_imageBuffer->logicalSize().height() >= size.height())
            return m_imageBuffer.get();

        // Round to the nearest 32 pixels so we do not grow the buffer for similar sized requests.
        IntSize roundedSize(roundUpToMultipleOf32(size.width()), roundUpToMultipleOf32(size.height()));

        clearScratchBuffer();
        m_imageBuffer = ImageBuffer::create(roundedSize, 1);
        return m_imageBuffer.get();
    }

    bool setCachedShadowValues(const FloatSize& radius, const Color& color, ColorSpace colorSpace, const FloatRect& shadowRect, const RoundedRect::Radii& radii, const FloatSize& layerSize)
    {
        if (!m_lastWasInset && m_lastRadius == radius && m_lastColor == color && m_lastColorSpace == colorSpace && m_lastShadowRect == shadowRect &&  m_lastRadii == radii && m_lastLayerSize == layerSize)
            return false;

        m_lastWasInset = false;
        m_lastRadius = radius;
        m_lastColor = color;
        m_lastColorSpace = colorSpace;
        m_lastShadowRect = shadowRect;
        m_lastRadii = radii;
        m_lastLayerSize = layerSize;

        return true;
    }

    bool setCachedInsetShadowValues(const FloatSize& radius, const Color& color, ColorSpace colorSpace, const FloatRect& bounds, const FloatRect& shadowRect, const RoundedRect::Radii& radii)
    {
        if (m_lastWasInset && m_lastRadius == radius && m_lastColor == color && m_lastColorSpace == colorSpace && m_lastInsetBounds == bounds && shadowRect == m_lastShadowRect && radii == m_lastRadii)
            return false;

        m_lastWasInset = true;
        m_lastInsetBounds = bounds;
        m_lastRadius = radius;
        m_lastColor = color;
        m_lastColorSpace = colorSpace;
        m_lastShadowRect = shadowRect;
        m_lastRadii = radii;

        return true;
    }

    void scheduleScratchBufferPurge()
    {
#if !ASSERT_DISABLED
        m_bufferInUse = false;
#endif
        if (m_purgeTimer.isActive())
            m_purgeTimer.stop();

        const double scratchBufferPurgeInterval = 2;
        m_purgeTimer.startOneShot(scratchBufferPurgeInterval);
    }
    
    static ScratchBuffer& shared();

private:
    void timerFired(Timer<ScratchBuffer>*)
    {
        clearScratchBuffer();
    }
    
    void clearScratchBuffer()
    {
        m_imageBuffer = nullptr;
        m_lastRadius = FloatSize();
    }

    OwnPtr<ImageBuffer> m_imageBuffer;
    Timer<ScratchBuffer> m_purgeTimer;
    
    FloatRect m_lastInsetBounds;
    FloatRect m_lastShadowRect;
    RoundedRect::Radii m_lastRadii;
    Color m_lastColor;
    ColorSpace m_lastColorSpace;
    FloatSize m_lastRadius;
    bool m_lastWasInset;
    FloatSize m_lastLayerSize;
    
#if !ASSERT_DISABLED
    bool m_bufferInUse;
#endif
};

ScratchBuffer& ScratchBuffer::shared()
{
    DEFINE_STATIC_LOCAL(ScratchBuffer, scratchBuffer, ());
    return scratchBuffer;
}

static const int templateSideLength = 1;

#if USE(CG)
static float radiusToLegacyRadius(float radius)
{
    return radius > 8 ? 8 + 4 * sqrt((radius - 8) / 2) : radius;
}
#endif

ShadowBlur::ShadowBlur(const FloatSize& radius, const FloatSize& offset, const Color& color, ColorSpace colorSpace)
    : m_color(color)
    , m_colorSpace(colorSpace)
    , m_blurRadius(radius)
    , m_offset(offset)
    , m_layerImage(0)
    , m_shadowsIgnoreTransforms(false)
{
    updateShadowBlurValues();
}

ShadowBlur::ShadowBlur(const GraphicsContextState& state)
    : m_color(state.shadowColor)
    , m_colorSpace(state.shadowColorSpace)
    , m_blurRadius(state.shadowBlur, state.shadowBlur)
    , m_offset(state.shadowOffset)
    , m_layerImage(0)
    , m_shadowsIgnoreTransforms(state.shadowsIgnoreTransforms)
{
#if USE(CG)
    if (state.shadowsUseLegacyRadius) {
        float shadowBlur = radiusToLegacyRadius(state.shadowBlur);
        m_blurRadius = FloatSize(shadowBlur, shadowBlur);
    }
#endif
    updateShadowBlurValues();
}

ShadowBlur::ShadowBlur()
    : m_type(NoShadow)
    , m_blurRadius(0, 0)
    , m_shadowsIgnoreTransforms(false)
{
}

void ShadowBlur::setShadowValues(const FloatSize& radius, const FloatSize& offset, const Color& color, ColorSpace colorSpace, bool ignoreTransforms)
{
    m_blurRadius = radius;
    m_offset = offset;
    m_color = color;
    m_colorSpace = colorSpace;
    m_shadowsIgnoreTransforms = ignoreTransforms;

    updateShadowBlurValues();
}

void ShadowBlur::updateShadowBlurValues()
{
    // Limit blur radius to 128 to avoid lots of very expensive blurring.
    m_blurRadius = m_blurRadius.shrunkTo(FloatSize(128, 128));

    // The type of shadow is decided by the blur radius, shadow offset, and shadow color.
    if (!m_color.isValid() || !m_color.alpha()) {
        // Can't paint the shadow with invalid or invisible color.
        m_type = NoShadow;
    } else if (m_blurRadius.width() > 0 || m_blurRadius.height() > 0) {
        // Shadow is always blurred, even the offset is zero.
        m_type = BlurShadow;
    } else if (!m_offset.width() && !m_offset.height()) {
        // Without blur and zero offset means the shadow is fully hidden.
        m_type = NoShadow;
    } else
        m_type = SolidShadow;
}

// Instead of integer division, we use 17.15 for fixed-point division.
static const int blurSumShift = 15;

// Takes a two dimensional array with three rows and two columns for the lobes.
static void calculateLobes(int lobes[][2], float blurRadius, bool shadowsIgnoreTransforms)
{
    int diameter;
    if (shadowsIgnoreTransforms)
        diameter = max(2, static_cast<int>(floorf((2 / 3.f) * blurRadius))); // Canvas shadow. FIXME: we should adjust the blur radius higher up.
    else {
        // http://dev.w3.org/csswg/css3-background/#box-shadow
        // Approximate a Gaussian blur with a standard deviation equal to half the blur radius,
        // which http://www.w3.org/TR/SVG/filters.html#feGaussianBlurElement tell us how to do.
        // However, shadows rendered according to that spec will extend a little further than m_blurRadius,
        // so we apply a fudge factor to bring the radius down slightly.
        float stdDev = blurRadius / 2;
        const float gaussianKernelFactor = 3 / 4.f * sqrtf(2 * piFloat);
        const float fudgeFactor = 0.88f;
        diameter = max(2, static_cast<int>(floorf(stdDev * gaussianKernelFactor * fudgeFactor + 0.5f)));
    }

    if (diameter & 1) {
        // if d is odd, use three box-blurs of size 'd', centered on the output pixel.
        int lobeSize = (diameter - 1) / 2;
        lobes[0][leftLobe] = lobeSize;
        lobes[0][rightLobe] = lobeSize;
        lobes[1][leftLobe] = lobeSize;
        lobes[1][rightLobe] = lobeSize;
        lobes[2][leftLobe] = lobeSize;
        lobes[2][rightLobe] = lobeSize;
    } else {
        // if d is even, two box-blurs of size 'd' (the first one centered on the pixel boundary
        // between the output pixel and the one to the left, the second one centered on the pixel
        // boundary between the output pixel and the one to the right) and one box blur of size 'd+1' centered on the output pixel
        int lobeSize = diameter / 2;
        lobes[0][leftLobe] = lobeSize;
        lobes[0][rightLobe] = lobeSize - 1;
        lobes[1][leftLobe] = lobeSize - 1;
        lobes[1][rightLobe] = lobeSize;
        lobes[2][leftLobe] = lobeSize;
        lobes[2][rightLobe] = lobeSize;
    }
}

void ShadowBlur::clear()
{
    m_type = NoShadow;
    m_color = Color();
    m_blurRadius = FloatSize();
    m_offset = FloatSize();
}

void ShadowBlur::blurLayerImage(unsigned char* imageData, const IntSize& size, int rowStride)
{
    const int channels[4] = { 3, 0, 1, 3 };

    int lobes[3][2]; // indexed by pass, and left/right lobe
    calculateLobes(lobes, m_blurRadius.width(), m_shadowsIgnoreTransforms);

    // First pass is horizontal.
    int stride = 4;
    int delta = rowStride;
    int final = size.height();
    int dim = size.width();

    // Two stages: horizontal and vertical
    for (int pass = 0; pass < 2; ++pass) {
        unsigned char* pixels = imageData;
        
        if (!pass && !m_blurRadius.width())
            final = 0; // Do no work if horizonal blur is zero.

        for (int j = 0; j < final; ++j, pixels += delta) {
            // For each step, we blur the alpha in a channel and store the result
            // in another channel for the subsequent step.
            // We use sliding window algorithm to accumulate the alpha values.
            // This is much more efficient than computing the sum of each pixels
            // covered by the box kernel size for each x.
            for (int step = 0; step < 3; ++step) {
                int side1 = lobes[step][leftLobe];
                int side2 = lobes[step][rightLobe];
                int pixelCount = side1 + 1 + side2;
                int invCount = ((1 << blurSumShift) + pixelCount - 1) / pixelCount;
                int ofs = 1 + side2;
                int alpha1 = pixels[channels[step]];
                int alpha2 = pixels[(dim - 1) * stride + channels[step]];

                unsigned char* ptr = pixels + channels[step + 1];
                unsigned char* prev = pixels + stride + channels[step];
                unsigned char* next = pixels + ofs * stride + channels[step];

                int i;
                int sum = side1 * alpha1 + alpha1;
                int limit = (dim < side2 + 1) ? dim : side2 + 1;

                for (i = 1; i < limit; ++i, prev += stride)
                    sum += *prev;

                if (limit <= side2)
                    sum += (side2 - limit + 1) * alpha2;

                limit = (side1 < dim) ? side1 : dim;
                for (i = 0; i < limit; ptr += stride, next += stride, ++i, ++ofs) {
                    *ptr = (sum * invCount) >> blurSumShift;
                    sum += ((ofs < dim) ? *next : alpha2) - alpha1;
                }
                
                prev = pixels + channels[step];
                for (; ofs < dim; ptr += stride, prev += stride, next += stride, ++i, ++ofs) {
                    *ptr = (sum * invCount) >> blurSumShift;
                    sum += (*next) - (*prev);
                }
                
                for (; i < dim; ptr += stride, prev += stride, ++i) {
                    *ptr = (sum * invCount) >> blurSumShift;
                    sum += alpha2 - (*prev);
                }
            }
        }

        // Last pass is vertical.
        stride = rowStride;
        delta = 4;
        final = size.width();
        dim = size.height();

        if (!m_blurRadius.height())
            break;

        if (m_blurRadius.width() != m_blurRadius.height())
            calculateLobes(lobes, m_blurRadius.height(), m_shadowsIgnoreTransforms);
    }
}

void ShadowBlur::adjustBlurRadius(GraphicsContext* context)
{
    if (!m_shadowsIgnoreTransforms)
        return;

    AffineTransform transform = context->getCTM();
    m_blurRadius.scale(1 / static_cast<float>(transform.xScale()), 1 / static_cast<float>(transform.yScale()));
}

IntSize ShadowBlur::blurredEdgeSize() const
{
    IntSize edgeSize = expandedIntSize(m_blurRadius);

    // To avoid slowing down blurLayerImage() for radius == 1, we give it two empty pixels on each side.
    if (edgeSize.width() == 1)
        edgeSize.setWidth(2);

    if (edgeSize.height() == 1)
        edgeSize.setHeight(2);

    return edgeSize;
}

IntRect ShadowBlur::calculateLayerBoundingRect(GraphicsContext* context, const FloatRect& shadowedRect, const IntRect& clipRect)
{
    IntSize edgeSize = blurredEdgeSize();

    // Calculate the destination of the blurred and/or transformed layer.
    FloatRect layerRect;
    IntSize inflation;

    const AffineTransform transform = context->getCTM();
    if (m_shadowsIgnoreTransforms && !transform.isIdentity()) {
        FloatQuad transformedPolygon = transform.mapQuad(FloatQuad(shadowedRect));
        transformedPolygon.move(m_offset);
        layerRect = transform.inverse().mapQuad(transformedPolygon).boundingBox();
    } else {
        layerRect = shadowedRect;
        layerRect.move(m_offset);
    }

    // We expand the area by the blur radius to give extra space for the blur transition.
    if (m_type == BlurShadow) {
        layerRect.inflateX(edgeSize.width());
        layerRect.inflateY(edgeSize.height());
        inflation = edgeSize;
    }

    FloatRect unclippedLayerRect = layerRect;

    if (!clipRect.contains(enclosingIntRect(layerRect))) {
        // If we are totally outside the clip region, we aren't painting at all.
        if (intersection(layerRect, clipRect).isEmpty())
            return IntRect();

        IntRect inflatedClip = clipRect;
        // Pixels at the edges can be affected by pixels outside the buffer,
        // so intersect with the clip inflated by the blur.
        if (m_type == BlurShadow) {
            inflatedClip.inflateX(edgeSize.width());
            inflatedClip.inflateY(edgeSize.height());
        } else {
            // Enlarge the clipping area 1 pixel so that the fill does not
            // bleed (due to antialiasing) even if the unaligned clip rect occurred
            inflatedClip.inflateX(1);
            inflatedClip.inflateY(1);
        }
        
        layerRect.intersect(inflatedClip);
    }

    IntSize frameSize = inflation;
    frameSize.scale(2);
    m_sourceRect = FloatRect(0, 0, shadowedRect.width() + frameSize.width(), shadowedRect.height() + frameSize.height());
    m_layerOrigin = FloatPoint(layerRect.x(), layerRect.y());
    m_layerSize = layerRect.size();

    const FloatPoint unclippedLayerOrigin = FloatPoint(unclippedLayerRect.x(), unclippedLayerRect.y());
    const FloatSize clippedOut = unclippedLayerOrigin - m_layerOrigin;

    // Set the origin as the top left corner of the scratch image, or, in case there's a clipped
    // out region, set the origin accordingly to the full bounding rect's top-left corner.
    float translationX = -shadowedRect.x() + inflation.width() - fabsf(clippedOut.width());
    float translationY = -shadowedRect.y() + inflation.height() - fabsf(clippedOut.height());
    m_layerContextTranslation = FloatSize(translationX, translationY);

    return enclosingIntRect(layerRect);
}

void ShadowBlur::drawShadowBuffer(GraphicsContext* graphicsContext)
{
    if (!m_layerImage)
        return;

    GraphicsContextStateSaver stateSaver(*graphicsContext);

    IntSize bufferSize = m_layerImage->internalSize();
    if (bufferSize != m_layerSize) {
        // The rect passed to clipToImageBuffer() has to be the size of the entire buffer,
        // but we may not have cleared it all, so clip to the filled part first.
        graphicsContext->clip(FloatRect(m_layerOrigin, m_layerSize));
    }
    graphicsContext->clipToImageBuffer(m_layerImage, FloatRect(m_layerOrigin, bufferSize));
    graphicsContext->setFillColor(m_color, m_colorSpace);

    graphicsContext->clearShadow();
    graphicsContext->fillRect(FloatRect(m_layerOrigin, m_sourceRect.size()));
}

static void computeSliceSizesFromRadii(const IntSize& twiceRadius, const RoundedRect::Radii& radii, int& leftSlice, int& rightSlice, int& topSlice, int& bottomSlice)
{
    leftSlice = twiceRadius.width() + max(radii.topLeft().width(), radii.bottomLeft().width()); 
    rightSlice = twiceRadius.width() + max(radii.topRight().width(), radii.bottomRight().width()); 

    topSlice = twiceRadius.height() + max(radii.topLeft().height(), radii.topRight().height());
    bottomSlice = twiceRadius.height() + max(radii.bottomLeft().height(), radii.bottomRight().height());
}

IntSize ShadowBlur::templateSize(const IntSize& radiusPadding, const RoundedRect::Radii& radii) const
{
    const int templateSideLength = 1;

    int leftSlice;
    int rightSlice;
    int topSlice;
    int bottomSlice;
    
    IntSize blurExpansion = radiusPadding;
    blurExpansion.scale(2);

    computeSliceSizesFromRadii(blurExpansion, radii, leftSlice, rightSlice, topSlice, bottomSlice);
    
    return IntSize(templateSideLength + leftSlice + rightSlice,
                   templateSideLength + topSlice + bottomSlice);
}

void ShadowBlur::drawRectShadow(GraphicsContext* graphicsContext, const FloatRect& shadowedRect, const RoundedRect::Radii& radii)
{
    IntRect layerRect = calculateLayerBoundingRect(graphicsContext, shadowedRect, graphicsContext->clipBounds());
    if (layerRect.isEmpty())
        return;

    adjustBlurRadius(graphicsContext);

    // drawRectShadowWithTiling does not work with rotations.
    // https://bugs.webkit.org/show_bug.cgi?id=45042
    if (!graphicsContext->getCTM().preservesAxisAlignment() || m_type != BlurShadow) {
        drawRectShadowWithoutTiling(graphicsContext, shadowedRect, radii, layerRect);
        return;
    }

    IntSize edgeSize = blurredEdgeSize();
    IntSize templateSize = this->templateSize(edgeSize, radii);

    if (templateSize.width() > shadowedRect.width() || templateSize.height() > shadowedRect.height()
        || (templateSize.width() * templateSize.height() > m_sourceRect.width() * m_sourceRect.height())) {
        drawRectShadowWithoutTiling(graphicsContext, shadowedRect, radii, layerRect);
        return;
    }

    drawRectShadowWithTiling(graphicsContext, shadowedRect, radii, templateSize, edgeSize);
}

void ShadowBlur::drawInsetShadow(GraphicsContext* graphicsContext, const FloatRect& rect, const FloatRect& holeRect, const RoundedRect::Radii& holeRadii)
{
    IntRect layerRect = calculateLayerBoundingRect(graphicsContext, rect, graphicsContext->clipBounds());
    if (layerRect.isEmpty())
        return;

    adjustBlurRadius(graphicsContext);

    // drawInsetShadowWithTiling does not work with rotations.
    // https://bugs.webkit.org/show_bug.cgi?id=45042
    if (!graphicsContext->getCTM().preservesAxisAlignment() || m_type != BlurShadow) {
        drawInsetShadowWithoutTiling(graphicsContext, rect, holeRect, holeRadii, layerRect);
        return;
    }

    IntSize edgeSize = blurredEdgeSize();
    IntSize templateSize = this->templateSize(edgeSize, holeRadii);

    if (templateSize.width() > holeRect.width() || templateSize.height() > holeRect.height()
        || (templateSize.width() * templateSize.height() > holeRect.width() * holeRect.height())) {
        drawInsetShadowWithoutTiling(graphicsContext, rect, holeRect, holeRadii, layerRect);
        return;
    }

    drawInsetShadowWithTiling(graphicsContext, rect, holeRect, holeRadii, templateSize, edgeSize);
}

void ShadowBlur::drawRectShadowWithoutTiling(GraphicsContext* graphicsContext, const FloatRect& shadowedRect, const RoundedRect::Radii& radii, const IntRect& layerRect)
{
    m_layerImage = ScratchBuffer::shared().getScratchBuffer(layerRect.size());
    if (!m_layerImage)
        return;

    FloatRect bufferRelativeShadowedRect = shadowedRect;
    bufferRelativeShadowedRect.move(m_layerContextTranslation);

    // Only redraw in the scratch buffer if its cached contents don't match our needs
    bool redrawNeeded = ScratchBuffer::shared().setCachedShadowValues(m_blurRadius, Color::black, ColorSpaceDeviceRGB, bufferRelativeShadowedRect, radii, m_layerSize);
    if (redrawNeeded) {
        GraphicsContext* shadowContext = m_layerImage->context();
        GraphicsContextStateSaver stateSaver(*shadowContext);

        // Add a pixel to avoid later edge aliasing when rotated.
        shadowContext->clearRect(FloatRect(0, 0, m_layerSize.width() + 1, m_layerSize.height() + 1));
        shadowContext->translate(m_layerContextTranslation);
        shadowContext->setFillColor(Color::black, ColorSpaceDeviceRGB);
        if (radii.isZero())
            shadowContext->fillRect(shadowedRect);
        else {
            Path path;
            path.addRoundedRect(shadowedRect, radii.topLeft(), radii.topRight(), radii.bottomLeft(), radii.bottomRight());
            shadowContext->fillPath(path);
        }

        blurShadowBuffer(expandedIntSize(m_layerSize));
    }
    
    drawShadowBuffer(graphicsContext);
    m_layerImage = 0;
    ScratchBuffer::shared().scheduleScratchBufferPurge();
}

void ShadowBlur::drawInsetShadowWithoutTiling(GraphicsContext* graphicsContext, const FloatRect& rect, const FloatRect& holeRect, const RoundedRect::Radii& holeRadii, const IntRect& layerRect)
{
    m_layerImage = ScratchBuffer::shared().getScratchBuffer(layerRect.size());
    if (!m_layerImage)
        return;

    FloatRect bufferRelativeRect = rect;
    bufferRelativeRect.move(m_layerContextTranslation);

    FloatRect bufferRelativeHoleRect = holeRect;
    bufferRelativeHoleRect.move(m_layerContextTranslation);

    // Only redraw in the scratch buffer if its cached contents don't match our needs
    bool redrawNeeded = ScratchBuffer::shared().setCachedInsetShadowValues(m_blurRadius, Color::black, ColorSpaceDeviceRGB, bufferRelativeRect, bufferRelativeHoleRect, holeRadii);
    if (redrawNeeded) {
        GraphicsContext* shadowContext = m_layerImage->context();
        GraphicsContextStateSaver stateSaver(*shadowContext);

        // Add a pixel to avoid later edge aliasing when rotated.
        shadowContext->clearRect(FloatRect(0, 0, m_layerSize.width() + 1, m_layerSize.height() + 1));
        shadowContext->translate(m_layerContextTranslation);

        Path path;
        path.addRect(rect);
        if (holeRadii.isZero())
            path.addRect(holeRect);
        else
            path.addRoundedRect(holeRect, holeRadii.topLeft(), holeRadii.topRight(), holeRadii.bottomLeft(), holeRadii.bottomRight());

        shadowContext->setFillRule(RULE_EVENODD);
        shadowContext->setFillColor(Color::black, ColorSpaceDeviceRGB);
        shadowContext->fillPath(path);

        blurShadowBuffer(expandedIntSize(m_layerSize));
    }
    
    drawShadowBuffer(graphicsContext);
    m_layerImage = 0;
    ScratchBuffer::shared().scheduleScratchBufferPurge();
}

/*
  These functions use tiling to improve the performance of the shadow
  drawing of rounded rectangles. The code basically does the following
  steps:

     1. Calculate the size of the shadow template, a rectangle that
     contains all the necessary tiles to draw the complete shadow.

     2. If that size is smaller than the real rectangle render the new
     template rectangle and its shadow in a new surface, in other case
     render the shadow of the real rectangle in the destination
     surface.

     3. Calculate the sizes and positions of the tiles and their
     destinations and use drawPattern to render the final shadow. The
     code divides the rendering in 8 tiles:

        1 | 2 | 3
       -----------
        4 |   | 5
       -----------
        6 | 7 | 8

     The corners are directly copied from the template rectangle to the
     real one and the side tiles are 1 pixel width, we use them as
     tiles to cover the destination side. The corner tiles are bigger
     than just the side of the rounded corner, we need to increase it
     because the modifications caused by the corner over the blur
     effect. We fill the central or outer part with solid color to complete
     the shadow.
 */

void ShadowBlur::drawInsetShadowWithTiling(GraphicsContext* graphicsContext, const FloatRect& rect, const FloatRect& holeRect, const RoundedRect::Radii& radii, const IntSize& templateSize, const IntSize& edgeSize)
{
    m_layerImage = ScratchBuffer::shared().getScratchBuffer(templateSize);
    if (!m_layerImage)
        return;

    // Draw the rectangle with hole.
    FloatRect templateBounds(0, 0, templateSize.width(), templateSize.height());
    FloatRect templateHole = FloatRect(edgeSize.width(), edgeSize.height(), templateSize.width() - 2 * edgeSize.width(), templateSize.height() - 2 * edgeSize.height());

    // Only redraw in the scratch buffer if its cached contents don't match our needs
    bool redrawNeeded = ScratchBuffer::shared().setCachedInsetShadowValues(m_blurRadius, m_color, m_colorSpace, templateBounds, templateHole, radii);
    if (redrawNeeded) {
        // Draw shadow into a new ImageBuffer.
        GraphicsContext* shadowContext = m_layerImage->context();
        GraphicsContextStateSaver shadowStateSaver(*shadowContext);
        shadowContext->clearRect(templateBounds);
        shadowContext->setFillRule(RULE_EVENODD);
        shadowContext->setFillColor(Color::black, ColorSpaceDeviceRGB);

        Path path;
        path.addRect(templateBounds);
        if (radii.isZero())
            path.addRect(templateHole);
        else
            path.addRoundedRect(templateHole, radii.topLeft(), radii.topRight(), radii.bottomLeft(), radii.bottomRight());

        shadowContext->fillPath(path);

        blurAndColorShadowBuffer(templateSize);
    }
    FloatSize offset = m_offset;
    if (shadowsIgnoreTransforms()) {
        AffineTransform transform = graphicsContext->getCTM();
        offset.scale(1 / transform.xScale(), 1 / transform.yScale());
    }

    FloatRect boundingRect = rect;
    boundingRect.move(offset);

    FloatRect destHoleRect = holeRect;
    destHoleRect.move(offset);
    FloatRect destHoleBounds = destHoleRect;
    destHoleBounds.inflateX(edgeSize.width());
    destHoleBounds.inflateY(edgeSize.height());

    // Fill the external part of the shadow (which may be visible because of offset).
    Path exteriorPath;
    exteriorPath.addRect(boundingRect);
    exteriorPath.addRect(destHoleBounds);

    {
        GraphicsContextStateSaver fillStateSaver(*graphicsContext);
        graphicsContext->setFillRule(RULE_EVENODD);
        graphicsContext->setFillColor(m_color, m_colorSpace);
        graphicsContext->clearShadow();
        graphicsContext->fillPath(exteriorPath);
    }
    
    drawLayerPieces(graphicsContext, destHoleBounds, radii, edgeSize, templateSize, InnerShadow);

    m_layerImage = 0;
    ScratchBuffer::shared().scheduleScratchBufferPurge();
}

void ShadowBlur::drawRectShadowWithTiling(GraphicsContext* graphicsContext, const FloatRect& shadowedRect, const RoundedRect::Radii& radii, const IntSize& templateSize, const IntSize& edgeSize)
{
    m_layerImage = ScratchBuffer::shared().getScratchBuffer(templateSize);
    if (!m_layerImage)
        return;

    FloatRect templateShadow = FloatRect(edgeSize.width(), edgeSize.height(), templateSize.width() - 2 * edgeSize.width(), templateSize.height() - 2 * edgeSize.height());

    // Only redraw in the scratch buffer if its cached contents don't match our needs
    bool redrawNeeded = ScratchBuffer::shared().setCachedShadowValues(m_blurRadius, m_color, m_colorSpace, templateShadow, radii, m_layerSize);
    if (redrawNeeded) {
        // Draw shadow into the ImageBuffer.
        GraphicsContext* shadowContext = m_layerImage->context();
        GraphicsContextStateSaver shadowStateSaver(*shadowContext);

        shadowContext->clearRect(FloatRect(0, 0, templateSize.width(), templateSize.height()));
        shadowContext->setFillColor(Color::black, ColorSpaceDeviceRGB);
        
        if (radii.isZero())
            shadowContext->fillRect(templateShadow);
        else {
            Path path;
            path.addRoundedRect(templateShadow, radii.topLeft(), radii.topRight(), radii.bottomLeft(), radii.bottomRight());
            shadowContext->fillPath(path);
        }

        blurAndColorShadowBuffer(templateSize);
    }
    FloatSize offset = m_offset;
    if (shadowsIgnoreTransforms()) {
        AffineTransform transform = graphicsContext->getCTM();
        offset.scale(1 / transform.xScale(), 1 / transform.yScale());
    }

    FloatRect shadowBounds = shadowedRect;
    shadowBounds.move(offset);
    shadowBounds.inflateX(edgeSize.width());
    shadowBounds.inflateY(edgeSize.height());

    drawLayerPieces(graphicsContext, shadowBounds, radii, edgeSize, templateSize, OuterShadow);

    m_layerImage = 0;
    ScratchBuffer::shared().scheduleScratchBufferPurge();
}

void ShadowBlur::drawLayerPieces(GraphicsContext* graphicsContext, const FloatRect& shadowBounds, const RoundedRect::Radii& radii, const IntSize& bufferPadding, const IntSize& templateSize, ShadowDirection direction)
{
    const IntSize twiceRadius = IntSize(bufferPadding.width() * 2, bufferPadding.height() * 2);

    int leftSlice;
    int rightSlice;
    int topSlice;
    int bottomSlice;
    computeSliceSizesFromRadii(twiceRadius, radii, leftSlice, rightSlice, topSlice, bottomSlice);

    int centerWidth = shadowBounds.width() - leftSlice - rightSlice;
    int centerHeight = shadowBounds.height() - topSlice - bottomSlice;

    if (direction == OuterShadow) {
        FloatRect shadowInterior(shadowBounds.x() + leftSlice, shadowBounds.y() + topSlice, centerWidth, centerHeight);
        if (!shadowInterior.isEmpty()) {
            GraphicsContextStateSaver stateSaver(*graphicsContext);
            graphicsContext->setFillColor(m_color, m_colorSpace);
            graphicsContext->clearShadow();
            graphicsContext->fillRect(shadowInterior);
        }
    }

    GraphicsContextStateSaver stateSaver(*graphicsContext);
    graphicsContext->setFillColor(m_color, m_colorSpace);
    graphicsContext->clearShadow();

    // Note that drawing the ImageBuffer is faster than creating a Image and drawing that,
    // because ImageBuffer::draw() knows that it doesn't have to copy the image bits.
    FloatRect centerRect(shadowBounds.x() + leftSlice, shadowBounds.y() + topSlice, centerWidth, centerHeight);
    centerRect = graphicsContext->roundToDevicePixels(centerRect);
    
    // Top side.
    FloatRect tileRect = FloatRect(leftSlice, 0, templateSideLength, topSlice);
    FloatRect destRect = FloatRect(centerRect.x(), centerRect.y() - topSlice, centerRect.width(), topSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Draw the bottom side.
    tileRect.setY(templateSize.height() - bottomSlice);
    tileRect.setHeight(bottomSlice);
    destRect.setY(centerRect.maxY());
    destRect.setHeight(bottomSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Left side.
    tileRect = FloatRect(0, topSlice, leftSlice, templateSideLength);
    destRect = FloatRect(centerRect.x() - leftSlice, centerRect.y(), leftSlice, centerRect.height());
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Right side.
    tileRect.setX(templateSize.width() - rightSlice);
    tileRect.setWidth(rightSlice);
    destRect.setX(centerRect.maxX());
    destRect.setWidth(rightSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Top left corner.
    tileRect = FloatRect(0, 0, leftSlice, topSlice);
    destRect = FloatRect(centerRect.x() - leftSlice, centerRect.y() - topSlice, leftSlice, topSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Top right corner.
    tileRect = FloatRect(templateSize.width() - rightSlice, 0, rightSlice, topSlice);
    destRect = FloatRect(centerRect.maxX(), centerRect.y() - topSlice, rightSlice, topSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Bottom right corner.
    tileRect = FloatRect(templateSize.width() - rightSlice, templateSize.height() - bottomSlice, rightSlice, bottomSlice);
    destRect = FloatRect(centerRect.maxX(), centerRect.maxY(), rightSlice, bottomSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);

    // Bottom left corner.
    tileRect = FloatRect(0, templateSize.height() - bottomSlice, leftSlice, bottomSlice);
    destRect = FloatRect(centerRect.x() - leftSlice, centerRect.maxY(), leftSlice, bottomSlice);
    graphicsContext->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, destRect, tileRect);
}


void ShadowBlur::blurShadowBuffer(const IntSize& templateSize)
{
    if (m_type != BlurShadow)
        return;

    IntRect blurRect(IntPoint(), templateSize);
    RefPtr<Uint8ClampedArray> layerData = m_layerImage->getUnmultipliedImageData(blurRect);
    blurLayerImage(layerData->data(), blurRect.size(), blurRect.width() * 4);
    m_layerImage->putByteArray(Unmultiplied, layerData.get(), blurRect.size(), blurRect, IntPoint());
}

void ShadowBlur::blurAndColorShadowBuffer(const IntSize& templateSize)
{
    blurShadowBuffer(templateSize);

    // Mask the image with the shadow color.
    GraphicsContext* shadowContext = m_layerImage->context();
    GraphicsContextStateSaver stateSaver(*shadowContext);
    shadowContext->setCompositeOperation(CompositeSourceIn);
    shadowContext->setFillColor(m_color, m_colorSpace);
    shadowContext->fillRect(FloatRect(0, 0, templateSize.width(), templateSize.height()));
}

GraphicsContext* ShadowBlur::beginShadowLayer(GraphicsContext *context, const FloatRect& layerArea)
{
    adjustBlurRadius(context);

    IntRect layerRect = calculateLayerBoundingRect(context, layerArea, context->clipBounds());

    if (layerRect.isEmpty())
        return 0;

    // We reset the scratch buffer values here, because the buffer will no longer contain
    // data from any previous rectangle or inset shadows drawn via the tiling path.
    ScratchBuffer::shared().setCachedShadowValues(FloatSize(), Color::black, ColorSpaceDeviceRGB, IntRect(), RoundedRect::Radii(), m_layerSize);
    m_layerImage = ScratchBuffer::shared().getScratchBuffer(layerRect.size());

    GraphicsContext* shadowContext = m_layerImage->context();
    shadowContext->save();

    // Add a pixel to avoid later edge aliasing when rotated.
    shadowContext->clearRect(FloatRect(0, 0, m_layerSize.width() + 1, m_layerSize.height() + 1));

    shadowContext->translate(m_layerContextTranslation);
    return shadowContext;
}

void ShadowBlur::endShadowLayer(GraphicsContext* context)
{
    m_layerImage->context()->restore();

    blurAndColorShadowBuffer(expandedIntSize(m_layerSize));
    GraphicsContextStateSaver stateSave(*context);

    context->clearShadow();
    context->drawImageBuffer(m_layerImage, ColorSpaceDeviceRGB, roundedIntPoint(m_layerOrigin), IntRect(0, 0, m_layerSize.width(), m_layerSize.height()), context->compositeOperation());

    m_layerImage = 0;
    ScratchBuffer::shared().scheduleScratchBufferPurge();
}

} // namespace WebCore
