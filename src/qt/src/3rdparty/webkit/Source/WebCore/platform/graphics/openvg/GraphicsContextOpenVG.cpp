/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "KURL.h"
#include "NotImplemented.h"
#include "PainterOpenVG.h"
#include "SurfaceOpenVG.h"

#include <wtf/Assertions.h>
#include <wtf/MathExtras.h>
#include <wtf/UnusedParam.h>
#include <wtf/Vector.h>

#if PLATFORM(EGL)
#include "EGLDisplayOpenVG.h"
#include "EGLUtils.h"
#include <egl.h>
#endif

namespace WebCore {

// typedef'ing doesn't work, let's inherit from PainterOpenVG instead
class GraphicsContextPlatformPrivate : public PainterOpenVG {
public:
    GraphicsContextPlatformPrivate(SurfaceOpenVG* surface)
        : PainterOpenVG(surface)
    {
    }
};

void GraphicsContext::platformInit(SurfaceOpenVG* surface)
{
    m_data = surface ? new GraphicsContextPlatformPrivate(surface) : 0;
    setPaintingDisabled(!surface);
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    if (paintingDisabled())
        return 0;

    return m_data->baseSurface();
}

AffineTransform GraphicsContext::getCTM() const
{
    if (paintingDisabled())
        return AffineTransform();

    return m_data->transformation();
}

void GraphicsContext::savePlatformState()
{
    if (paintingDisabled())
        return;

    m_data->save();
}

void GraphicsContext::restorePlatformState()
{
    if (paintingDisabled())
        return;

    m_data->restore();
}

void GraphicsContext::drawRect(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->drawRect(rect);
}

void GraphicsContext::drawLine(const IntPoint& from, const IntPoint& to)
{
    if (paintingDisabled())
        return;

    m_data->drawLine(from, to);
}

/**
 * Draw the largest ellipse that fits into the given rectangle.
 */
void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->drawEllipse(rect);
}

void GraphicsContext::strokeArc(const IntRect& rect, int startAngle, int angleSpan)
{
    if (paintingDisabled())
        return;

    m_data->drawArc(rect, startAngle, angleSpan, VG_STROKE_PATH);
}

void GraphicsContext::drawConvexPolygon(size_t numPoints, const FloatPoint* points, bool shouldAntialias)
{
    if (paintingDisabled())
        return;

    m_data->drawPolygon(numPoints, points);

    UNUSED_PARAM(shouldAntialias); // FIXME
}

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled())
        return;

    m_data->drawPath(path, VG_FILL_PATH, m_state.fillRule);
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled())
        return;

    m_data->drawPath(path, VG_STROKE_PATH, m_state.fillRule);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->drawRect(rect, VG_FILL_PATH);
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    Color oldColor = m_data->fillColor();
    m_data->setFillColor(color);
    m_data->drawRect(rect, VG_FILL_PATH);
    m_data->setFillColor(oldColor);

    UNUSED_PARAM(colorSpace); // FIXME
}

void GraphicsContext::fillRoundedRect(const IntRect& rect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    Color oldColor = m_data->fillColor();
    m_data->setFillColor(color);
    m_data->drawRoundedRect(rect, topLeft, topRight, bottomLeft, bottomRight, VG_FILL_PATH);
    m_data->setFillColor(oldColor);

    UNUSED_PARAM(colorSpace); // FIXME
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->intersectClipRect(rect);
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    m_data->clipPath(path, PainterOpenVG::IntersectClip, clipRule);
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int offset, const Color& color)
{
    if (paintingDisabled())
        return;

    if (rects.isEmpty())
        return;

    // FIXME: We just unite all focus ring rects into one for now.
    // We should outline the edge of the full region.
    offset += (width - 1) / 2;
    IntRect finalFocusRect;

    for (unsigned i = 0; i < rects.size(); i++) {
        IntRect focusRect = rects[i];
        focusRect.inflate(offset);
        finalFocusRect.unite(focusRect);
    }

    StrokeStyle oldStyle = m_data->strokeStyle();
    Color oldStrokeColor = m_data->strokeColor();
    m_data->setStrokeStyle(DashedStroke);
    m_data->setStrokeColor(color);
    strokeRect(FloatRect(finalFocusRect), 1.f);
    m_data->setStrokeStyle(oldStyle);
    m_data->setStrokeColor(oldStrokeColor);
}

void GraphicsContext::drawLineForText(const IntPoint& origin, int width, bool printing)
{
    if (paintingDisabled())
        return;

    if (width <= 0)
        return;

    StrokeStyle oldStyle = m_data->strokeStyle();
    m_data->setStrokeStyle(SolidStroke);
    drawLine(origin, origin + IntSize(width, 0));
    m_data->setStrokeStyle(oldStyle);

    UNUSED_PARAM(printing);
}

void GraphicsContext::drawLineForTextChecking(const IntPoint& origin, int width, TextCheckingLineStyle style)
{
    if (paintingDisabled())
        return;

    notImplemented();
    UNUSED_PARAM(origin);
    UNUSED_PARAM(width);
    UNUSED_PARAM(style);
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& rect, RoundingMode)
{
    if (paintingDisabled())
        return FloatRect();

    return FloatRect(enclosingIntRect(m_data->transformation().mapRect(rect)));
}

void GraphicsContext::setPlatformShadow(const FloatSize& size, float blur, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    notImplemented();
    UNUSED_PARAM(size);
    UNUSED_PARAM(blur);
    UNUSED_PARAM(color);
    UNUSED_PARAM(colorSpace);
}

void GraphicsContext::clearPlatformShadow()
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::beginTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    notImplemented();
    UNUSED_PARAM(opacity);
}

void GraphicsContext::endTransparencyLayer()
{
    if (paintingDisabled())
        return;

    notImplemented();
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    CompositeOperator op = m_data->compositeOperation();
    m_data->setCompositeOperation(CompositeClear);
    m_data->drawRect(rect, VG_FILL_PATH);
    m_data->setCompositeOperation(op);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    float oldThickness = m_data->strokeThickness();
    m_data->setStrokeThickness(lineWidth);
    m_data->drawRect(rect, VG_STROKE_PATH);
    m_data->setStrokeThickness(oldThickness);
}

void GraphicsContext::setLineCap(LineCap lc)
{
    if (paintingDisabled())
        return;

    m_data->setLineCap(lc);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    if (paintingDisabled())
        return;

    m_data->setLineDash(dashes, dashOffset);
}

void GraphicsContext::setLineJoin(LineJoin lj)
{
    if (paintingDisabled())
        return;

    m_data->setLineJoin(lj);
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;

    m_data->setMiterLimit(limit);
}

void GraphicsContext::setAlpha(float opacity)
{
    if (paintingDisabled())
        return;

    m_data->setOpacity(opacity);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op)
{
    if (paintingDisabled())
        return;

    m_data->setCompositeOperation(op);
}

void GraphicsContext::clip(const Path& path)
{
    if (paintingDisabled())
        return;

    m_data->clipPath(path, PainterOpenVG::IntersectClip, m_state.fillRule);
}

void GraphicsContext::canvasClip(const Path& path)
{
    clip(path);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    m_data->clipPath(path, PainterOpenVG::SubtractClip, m_state.fillRule);
}

void GraphicsContext::scale(const FloatSize& scaleFactors)
{
    if (paintingDisabled())
        return;

    m_data->scale(scaleFactors);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled())
        return;

    m_data->rotate(radians);
}

void GraphicsContext::translate(float dx, float dy)
{
    if (paintingDisabled())
        return;

    m_data->translate(dx, dy);
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addRect(rect);
    m_data->clipPath(path, PainterOpenVG::SubtractClip, m_state.fillRule);
}

void GraphicsContext::clipToImageBuffer(const FloatRect& rect, const ImageBuffer* imageBuffer)
{
    if (paintingDisabled())
        return;

    notImplemented();
    UNUSED_PARAM(rect);
    UNUSED_PARAM(imageBuffer);
}

void GraphicsContext::addInnerRoundedRectClip(const IntRect& rect, int thickness)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addEllipse(rect);
    path.addEllipse(FloatRect(rect.x() + thickness, rect.y() + thickness,
        rect.width() - (thickness * 2), rect.height() - (thickness * 2)));

    m_data->clipPath(path, PainterOpenVG::IntersectClip, m_state.fillRule);
}

void GraphicsContext::concatCTM(const AffineTransform& transformation)
{
    if (paintingDisabled())
        return;

    m_data->concatTransformation(transformation);
}

void GraphicsContext::setCTM(const AffineTransform& transformation)
{
    if (paintingDisabled())
        return;

    m_data->setTransformation(transformation);
}

void GraphicsContext::setURLForRect(const KURL& link, const IntRect& destRect)
{
    notImplemented();
    UNUSED_PARAM(link);
    UNUSED_PARAM(destRect);
}

void GraphicsContext::setPlatformStrokeColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    m_data->setStrokeColor(color);

    UNUSED_PARAM(colorSpace); // FIXME
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;

    m_data->setStrokeStyle(strokeStyle);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;

    m_data->setStrokeThickness(thickness);
}

void GraphicsContext::setPlatformFillColor(const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled())
        return;

    m_data->setFillColor(color);

    UNUSED_PARAM(colorSpace); // FIXME
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;

    m_data->setAntialiasingEnabled(enable);
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality)
{
    notImplemented();
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    notImplemented();
    return InterpolationDefault;
}

}
