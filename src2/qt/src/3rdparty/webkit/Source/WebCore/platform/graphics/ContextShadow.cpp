/*
 * Copyright (C) 2010 Sencha, Inc.
 * Copyright (C) 2010 Igalia S.L.
 *
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
#include "ContextShadow.h"

#include "AffineTransform.h"
#include "FloatQuad.h"
#include "GraphicsContext.h"
#include <cmath>
#include <wtf/MathExtras.h>
#include <wtf/Noncopyable.h>

using WTF::min;
using WTF::max;

namespace WebCore {

ContextShadow::ContextShadow()
    : m_type(NoShadow)
    , m_blurDistance(0)
    , m_layerContext(0)
    , m_shadowsIgnoreTransforms(false)
{
}

ContextShadow::ContextShadow(const Color& color, float radius, const FloatSize& offset)
    : m_color(color)
    , m_blurDistance(round(radius))
    , m_offset(offset)
    , m_layerContext(0)
    , m_shadowsIgnoreTransforms(false)
{
    // See comments in http://webkit.org/b/40793, it seems sensible
    // to follow Skia's limit of 128 pixels of blur radius
    m_blurDistance = min(m_blurDistance, 128);

    // The type of shadow is decided by the blur radius, shadow offset, and shadow color.
    if (!m_color.isValid() || !color.alpha()) {
        // Can't paint the shadow with invalid or invisible color.
        m_type = NoShadow;
    } else if (radius > 0) {
        // Shadow is always blurred, even the offset is zero.
        m_type = BlurShadow;
    } else if (!m_offset.width() && !m_offset.height()) {
        // Without blur and zero offset means the shadow is fully hidden.
        m_type = NoShadow;
    } else {
        m_type = SolidShadow;
    }
}

void ContextShadow::clear()
{
    m_type = NoShadow;
    m_color = Color();
    m_blurDistance = 0;
    m_offset = FloatSize();
}

bool ContextShadow::mustUseContextShadow(GraphicsContext* context)
{
    // We can't avoid ContextShadow, since the shadow has blur.
    if (m_type == ContextShadow::BlurShadow)
        return true;
    // We can avoid ContextShadow and optimize, since we're not drawing on a
    // canvas and box shadows are affected by the transformation matrix.
    if (!shadowsIgnoreTransforms())
        return false;
    // We can avoid ContextShadow, since there are no transformations to apply to the canvas.
    if (context->getCTM().isIdentity())
        return false;
    // Otherwise, no chance avoiding ContextShadow.
    return true;
}

// Instead of integer division, we use 17.15 for fixed-point division.
static const int BlurSumShift = 15;

// Check http://www.w3.org/TR/SVG/filters.html#feGaussianBlur.
// As noted in the SVG filter specification, running box blur 3x
// approximates a real gaussian blur nicely.

void ContextShadow::blurLayerImage(unsigned char* imageData, const IntSize& size, int rowStride)
{
#if CPU(BIG_ENDIAN)
    int channels[4] = { 0, 3, 2, 0 };
#elif CPU(MIDDLE_ENDIAN)
    int channels[4] = { 1, 2, 3, 1 };
#else
    int channels[4] = { 3, 0, 1, 3 };
#endif

    int d = max(2, static_cast<int>(floorf((2 / 3.f) * m_blurDistance)));
    int dmax = d >> 1;
    int dmin = dmax - 1 + (d & 1);
    if (dmin < 0)
        dmin = 0;

    // Two stages: horizontal and vertical
    for (int k = 0; k < 2; ++k) {

        unsigned char* pixels = imageData;
        int stride = (!k) ? 4 : rowStride;
        int delta = (!k) ? rowStride : 4;
        int jfinal = (!k) ? size.height() : size.width();
        int dim = (!k) ? size.width() : size.height();

        for (int j = 0; j < jfinal; ++j, pixels += delta) {

            // For each step, we blur the alpha in a channel and store the result
            // in another channel for the subsequent step.
            // We use sliding window algorithm to accumulate the alpha values.
            // This is much more efficient than computing the sum of each pixels
            // covered by the box kernel size for each x.

            for (int step = 0; step < 3; ++step) {
                int side1 = (!step) ? dmin : dmax;
                int side2 = (step == 1) ? dmin : dmax;
                int pixelCount = side1 + 1 + side2;
                int invCount = ((1 << BlurSumShift) + pixelCount - 1) / pixelCount;
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
                    *ptr = (sum * invCount) >> BlurSumShift;
                    sum += ((ofs < dim) ? *next : alpha2) - alpha1;
                }
                prev = pixels + channels[step];
                for (; ofs < dim; ptr += stride, prev += stride, next += stride, ++i, ++ofs) {
                    *ptr = (sum * invCount) >> BlurSumShift;
                    sum += (*next) - (*prev);
                }
                for (; i < dim; ptr += stride, prev += stride, ++i) {
                    *ptr = (sum * invCount) >> BlurSumShift;
                    sum += alpha2 - (*prev);
                }
            }
        }
    }
}

void ContextShadow::adjustBlurDistance(GraphicsContext* context)
{
    const AffineTransform transform = context->getCTM();

    // Adjust blur if we're scaling, since the radius must not be affected by transformations.
    if (transform.isIdentity())
        return;

    // Calculate transformed unit vectors.
    const FloatQuad unitQuad(FloatPoint(0, 0), FloatPoint(1, 0),
                             FloatPoint(0, 1), FloatPoint(1, 1));
    const FloatQuad transformedUnitQuad = transform.mapQuad(unitQuad);

    // Calculate X axis scale factor.
    const FloatSize xUnitChange = transformedUnitQuad.p2() - transformedUnitQuad.p1();
    const float xAxisScale = sqrtf(xUnitChange.width() * xUnitChange.width()
                                   + xUnitChange.height() * xUnitChange.height());

    // Calculate Y axis scale factor.
    const FloatSize yUnitChange = transformedUnitQuad.p3() - transformedUnitQuad.p1();
    const float yAxisScale = sqrtf(yUnitChange.width() * yUnitChange.width()
                                   + yUnitChange.height() * yUnitChange.height());

    // blurLayerImage() does not support per-axis blurring, so calculate a balanced scaling.
    const float scale = sqrtf(xAxisScale * yAxisScale);
    m_blurDistance = roundf(static_cast<float>(m_blurDistance) / scale);
}

IntRect ContextShadow::calculateLayerBoundingRect(GraphicsContext* context, const FloatRect& layerArea, const IntRect& clipRect)
{
    // Calculate the destination of the blurred and/or transformed layer.
    FloatRect layerFloatRect;
    float inflation = 0;

    const AffineTransform transform = context->getCTM();
    if (m_shadowsIgnoreTransforms && !transform.isIdentity()) {
        FloatQuad transformedPolygon = transform.mapQuad(FloatQuad(layerArea));
        transformedPolygon.move(m_offset);
        layerFloatRect = transform.inverse().mapQuad(transformedPolygon).boundingBox();
    } else {
        layerFloatRect = layerArea;
        layerFloatRect.move(m_offset);
    }

    // We expand the area by the blur radius to give extra space for the blur transition.
    if (m_type == BlurShadow) {
        layerFloatRect.inflate(m_blurDistance);
        inflation += m_blurDistance;
    }

    FloatRect unclippedLayerRect = layerFloatRect;

    if (!clipRect.contains(enclosingIntRect(layerFloatRect))) {
        // No need to have the buffer larger than the clip.
        layerFloatRect.intersect(clipRect);

        // If we are totally outside the clip region, we aren't painting at all.
        if (layerFloatRect.isEmpty())
            return IntRect(0, 0, 0, 0);

        // We adjust again because the pixels at the borders are still
        // potentially affected by the pixels outside the buffer.
        if (m_type == BlurShadow) {
            layerFloatRect.inflate(m_blurDistance);
            unclippedLayerRect.inflate(m_blurDistance);
            inflation += m_blurDistance;
        }
    }

    const int frameSize = inflation * 2;
    m_sourceRect = IntRect(0, 0, layerArea.width() + frameSize, layerArea.height() + frameSize);
    m_layerOrigin = FloatPoint(layerFloatRect.x(), layerFloatRect.y());

    const FloatPoint m_unclippedLayerOrigin = FloatPoint(unclippedLayerRect.x(), unclippedLayerRect.y());
    const FloatSize clippedOut = m_unclippedLayerOrigin - m_layerOrigin;

    // Set the origin as the top left corner of the scratch image, or, in case there's a clipped
    // out region, set the origin accordingly to the full bounding rect's top-left corner.
    const float translationX = -layerArea.x() + inflation - fabsf(clippedOut.width());
    const float translationY = -layerArea.y() + inflation - fabsf(clippedOut.height());
    m_layerContextTranslation = FloatPoint(translationX, translationY);

    return enclosingIntRect(layerFloatRect);
}

} // namespace WebCore
