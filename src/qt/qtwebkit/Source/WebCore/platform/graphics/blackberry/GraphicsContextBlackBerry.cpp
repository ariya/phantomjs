/*
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "config.h"
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "Gradient.h"
#include "GraphicsContext3D.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "RoundedRect.h"
#include "TransformationMatrix.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <stdio.h>
#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

class GraphicsContextPlatformPrivate {
public:
    GraphicsContextPlatformPrivate(PlatformGraphicsContext* platformContext)
        : m_platformContext(platformContext)
        , m_isComposited(false)
    {
    }

    PlatformGraphicsContext* m_platformContext;
    bool m_isComposited;
};

void GraphicsContext::platformInit(PlatformGraphicsContext* platformContext)
{
    m_data = new GraphicsContextPlatformPrivate(platformContext);
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return m_data->m_platformContext;
}

void GraphicsContext::savePlatformState()
{
    if (paintingDisabled())
        return;

    platformContext()->save();
}

void GraphicsContext::restorePlatformState()
{
    if (paintingDisabled())
        return;

    platformContext()->restore();
    platformContext()->setFillColor(m_state.fillColor.rgb());
    if (hasShadow())
        setPlatformShadow(m_state.shadowOffset, m_state.shadowBlur, m_state.shadowColor, m_state.shadowColorSpace);
    else
        clearPlatformShadow();
    platformContext()->setStrokeColor(m_state.strokeColor.rgb());
    platformContext()->setStrokeStyle(static_cast<BlackBerry::Platform::Graphics::StrokeStyle>(m_state.strokeStyle));
    platformContext()->setStrokeThickness(m_state.strokeThickness);
    platformContext()->setTextDrawingMode(m_state.textDrawingMode);
}

void GraphicsContext::setIsAcceleratedContext(bool accelerated)
{
    m_data->m_isComposited = accelerated;
}

bool GraphicsContext::isAcceleratedContext() const
{
    return m_data->m_isComposited;
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    AffineTransform result;
    platformContext()->getTransform((double*)&result);
    return result;
}

void GraphicsContext::concatCTM(const AffineTransform& transformation)
{
    if (paintingDisabled())
        return;

    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current *= transformation;
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::setCTM(const AffineTransform& transformation)
{
    if (paintingDisabled())
        return;

    platformContext()->setTransform((const double*)&transformation);
}

void GraphicsContext::scale(const FloatSize& scaleFactors)
{
    if (paintingDisabled())
        return;

    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current.scaleNonUniform(scaleFactors.width(), scaleFactors.height());
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled())
        return;

    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current.rotate(rad2deg(radians));
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::translate(float dx, float dy)
{
    if (paintingDisabled())
        return;

    AffineTransform current;
    platformContext()->getTransform((double*)&current);
    current.translate(dx, dy);
    platformContext()->setTransform((double*)&current);
}

void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    platformContext()->addEllipse(FloatRect(rect));
}

void GraphicsContext::drawConvexPolygon(size_t numPoints, const FloatPoint* points, bool)
{
    if (paintingDisabled())
        return;

    platformContext()->addConvexPolygon(numPoints, (const BlackBerry::Platform::FloatPoint*)points);
}

void GraphicsContext::drawRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    platformContext()->addDrawRect(FloatRect(rect));
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    BlackBerry::Platform::Graphics::Gradient* platformGradient = fillGradient() ? fillGradient()->platformGradient() : 0;
    BlackBerry::Platform::Graphics::Pattern* platformPattern = fillPattern() ? fillPattern()->platformPattern(AffineTransform()) : 0;
    platformContext()->addFillRect(rect, platformGradient, platformPattern);
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    platformContext()->setFillColor(color.rgb());
    platformContext()->addFillRect(rect);
    platformContext()->setFillColor(m_state.fillColor.rgb());
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    platformContext()->addClearRect(rect);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    if (strokeGradient() || strokePattern()) {
        Path path;
        path.addRect(rect);
        float oldLineWidth = m_state.strokeThickness;
        setPlatformStrokeThickness(lineWidth);
        strokePath(path);
        setPlatformStrokeThickness(oldLineWidth);
    } else
        platformContext()->addStrokeRect(rect, lineWidth);
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    BlackBerry::Platform::FloatRoundedRect r = BlackBerry::Platform::FloatRoundedRect(FloatRect(rect), FloatSize(topLeft), FloatSize(topRight), FloatSize(bottomLeft), FloatSize(bottomRight));
    platformContext()->setFillColor(color.rgb());
    platformContext()->addRoundedRect(r);
    platformContext()->setFillColor(m_state.fillColor.rgb());
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode)
{
    return rect;
}

void GraphicsContext::setPlatformShadow(const FloatSize& offset, float blur, const Color& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    FloatSize adjustedOffset = offset;

    if (m_state.shadowsIgnoreTransforms) {
        // Meaning that this graphics context is associated with a CanvasRenderingContext
        // We flip the height since CG and HTML5 Canvas have opposite Y axis
        adjustedOffset.setHeight(-offset.height());
    }

    // if there is an invalid shadow color, we're supposed to use Apple's CG default
    platformContext()->setShadow(adjustedOffset, blur, color.isValid() ? color.rgb() : makeRGBA(0, 0, 0, 0xFF / 3), m_state.shadowsIgnoreTransforms);
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;

    platformContext()->clearShadow();
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    platformContext()->beginPlatformTransparencyLayer(opacity);
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    platformContext()->endPlatformTransparencyLayer();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::setLineCap(LineCap lc)
{
    if (paintingDisabled())
        return;

    platformContext()->setLineCap((BlackBerry::Platform::Graphics::LineCap)lc);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    if (paintingDisabled())
        return;

    platformContext()->setLineDash(dashes.data(), dashes.size(), dashOffset);
}

void GraphicsContext::setLineJoin(LineJoin lj)
{
    if (paintingDisabled())
        return;

    platformContext()->setLineJoin((BlackBerry::Platform::Graphics::LineJoin)lj); // naming coincides
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;

    platformContext()->setMiterLimit(limit);
}

void GraphicsContext::setAlpha(float opacity)
{
    if (paintingDisabled())
        return;

    platformContext()->setAlpha(opacity);
}


void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    platformContext()->clip(rect);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    platformContext()->clipOut(FloatRect(rect));
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool)
{
    if (paintingDisabled())
        return;

    platformContext()->clipConvexPolygon(numPoints, (const BlackBerry::Platform::FloatPoint*)points);
}

void GraphicsContext::clipRoundedRect(const RoundedRect& rect)
{
    if (paintingDisabled())
        return;

    BlackBerry::Platform::FloatRoundedRect r = BlackBerry::Platform::FloatRoundedRect(
        FloatRect(rect.rect()),
        FloatSize(rect.radii().topLeft()),
        FloatSize(rect.radii().topRight()),
        FloatSize(rect.radii().bottomLeft()),
        FloatSize(rect.radii().bottomRight()));
    platformContext()->clipRoundedRect(r);
}

void GraphicsContext::clipOutRoundedRect(const RoundedRect& rect)
{
    if (paintingDisabled())
        return;

    BlackBerry::Platform::FloatRoundedRect r = BlackBerry::Platform::FloatRoundedRect(
        FloatRect(rect.rect()),
        FloatSize(rect.radii().topLeft()),
        FloatSize(rect.radii().topRight()),
        FloatSize(rect.radii().bottomLeft()),
        FloatSize(rect.radii().bottomRight()));
    platformContext()->clipOutRoundedRect(r);
}

IntRect GraphicsContext::clipBounds() const
{
    // FIXME: return the max IntRect for now
    return IntRect(IntPoint(INT_MIN / 2, INT_MIN / 2), IntSize(INT_MAX, INT_MAX));
}

void GraphicsContext::setURLForRect(const KURL&, const IntRect&)
{
}

void GraphicsContext::setPlatformTextDrawingMode(TextDrawingModeFlags mode)
{
    if (paintingDisabled())
        return;

    platformContext()->setTextDrawingMode(mode);
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    platformContext()->setStrokeColor(color.rgb());
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle stroke)
{
    if (paintingDisabled())
        return;

    platformContext()->setStrokeStyle((BlackBerry::Platform::Graphics::StrokeStyle)stroke);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;

    platformContext()->setStrokeThickness(thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace)
{
    if (paintingDisabled())
        return;

    platformContext()->setFillColor(color.rgb());
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode)
{
    if (paintingDisabled())
        return;

    platformContext()->setCompositeOperation(static_cast<BlackBerry::Platform::Graphics::CompositeOperator>(op), blendMode);
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;

    platformContext()->setUseAntialiasing(enable);
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality interpolationQuality)
{
    platformContext()->setHighQualityFilter(interpolationQuality != InterpolationNone);
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    return platformContext()->highQualityFilter() ? InterpolationDefault : InterpolationNone;
}

}
