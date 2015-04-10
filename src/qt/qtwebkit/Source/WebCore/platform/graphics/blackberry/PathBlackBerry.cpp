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
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "NotImplemented.h"
#include "StrokeStyleApplier.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformPath.h>
#include <stdio.h>
#include <wtf/MathExtras.h>

namespace WebCore {

static GraphicsContext* scratchContext()
{
    static BlackBerry::Platform::Graphics::Buffer* buffer = BlackBerry::Platform::Graphics::createBuffer(IntSize(), BlackBerry::Platform::Graphics::NeverBacked);
    static PlatformGraphicsContext* pgc = lockBufferDrawable(buffer);
    static GraphicsContext gc(pgc);
    return &gc;
}

Path::Path() : m_path(new BlackBerry::Platform::Graphics::Path)
{
}

Path::Path(const PlatformPath& path)
    : m_path(new BlackBerry::Platform::Graphics::Path(path))
{
}

Path::~Path()
{
    if (m_path) {
        delete m_path;
        m_path = 0;
    }
}

Path::Path(const Path& other)
{
    m_path = new BlackBerry::Platform::Graphics::Path(*other.m_path);
}

Path& Path::operator=(const Path& other)
{
    if (m_path) {
        delete m_path;
        m_path = 0;
    }
    m_path = new BlackBerry::Platform::Graphics::Path(*other.m_path);
    return *this;
}

FloatPoint Path::currentPoint() const
{
    return m_path->currentPoint();
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    return m_path->contains(point, (BlackBerry::Platform::Graphics::WindRule)(rule));
}

bool Path::strokeContains(StrokeStyleApplier* applier, const FloatPoint& point) const
{
    GraphicsContext* scratch = scratchContext();
    scratch->save();

    if (applier)
        applier->strokeStyle(scratch);

    bool result = scratchContext()->platformContext()->strokeContains(*m_path, point);
    scratch->restore();
    return result;
}

void Path::translate(const FloatSize& size)
{
    AffineTransform transformation;
    transformation.translate(size.width(), size.height());
    transform(transformation);
}

FloatRect Path::boundingRect() const
{
    return m_path->getBounds();
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier* applier) const
{
    GraphicsContext* scratch = scratchContext();
    scratch->save();

    if (applier)
        applier->strokeStyle(scratch);

    FloatRect result = scratch->platformContext()->getStrokeBounds(*m_path);

    scratch->restore();
    return result;
}

void Path::moveTo(const FloatPoint& point)
{
    m_path->moveTo(point);
}

void Path::addLineTo(const FloatPoint& point)
{
    m_path->addLineTo(point);
}

void Path::addQuadCurveTo(const FloatPoint& controlPoint, const FloatPoint& endPoint)
{
    m_path->addQuadCurveTo(controlPoint, endPoint);
}

void Path::addBezierCurveTo(const FloatPoint& controlPoint1, const FloatPoint& controlPoint2, const FloatPoint& endPoint)
{
    m_path->addBezierCurveTo(controlPoint1, controlPoint2, endPoint);
}

void Path::addArcTo(const FloatPoint& point1, const FloatPoint& point2, float radius)
{
    m_path->addArcTo(point1, point2, radius);
}

void Path::closeSubpath()
{
    m_path->closeSubpath();
}

void Path::addArc(const FloatPoint& center, float radius, float startAngle, float endAngle, bool anticlockwise)
{
    m_path->addArc(center, radius, startAngle, endAngle, anticlockwise);
}

void Path::addRect(const FloatRect& rect)
{
    m_path->addRect(rect);
}

void Path::addEllipse(const FloatRect& rect)
{
    m_path->addEllipse(rect);
}

void Path::platformAddPathForRoundedRect(const FloatRect& rect, const FloatSize& topLeftRadius, const FloatSize& topRightRadius, const FloatSize& bottomLeftRadius, const FloatSize& bottomRightRadius)
{
    if (rect.isEmpty())
        return;

    if (rect.width() < topLeftRadius.width() + topRightRadius.width()
        || rect.width() < bottomLeftRadius.width() + bottomRightRadius.width()
        || rect.height() < topLeftRadius.height() + bottomLeftRadius.height()
        || rect.height() < topRightRadius.height() + bottomRightRadius.height()) {
        // If all the radii cannot be accommodated, return a rect.
        addRect(rect);
        return;
    }

    BlackBerry::Platform::FloatRoundedRect rr(rect, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius);
    // Make sure the generic path reflects the contents of the rounded rects
    m_path->addRoundedRect(rr);
}

void Path::clear()
{
    m_path->reset();
}

bool Path::isEmpty() const
{
    return m_path->isEmpty();
}

bool Path::hasCurrentPoint() const
{
    return m_path->hasCurrentPoint();
}

void Path::apply(void* info, PathApplierFunction function) const
{
    m_path->apply(info, (void*)function);
}

void Path::transform(const AffineTransform& transformation)
{
    m_path->transform(reinterpret_cast<const double*>(&transformation));
}

void GraphicsContext::fillPath(const Path& path)
{
    BlackBerry::Platform::Graphics::Path* pp = path.platformPath();
    if (!pp->isEmpty()) {
        BlackBerry::Platform::Graphics::Gradient* platformGradient = fillGradient() ? fillGradient()->platformGradient() : 0;
        BlackBerry::Platform::Graphics::Pattern* platformPattern = fillPattern() ? fillPattern()->platformPattern(AffineTransform()) : 0;
        platformContext()->addFillPath(*pp, (BlackBerry::Platform::Graphics::WindRule)m_state.fillRule, platformGradient, platformPattern);
    }
}

void GraphicsContext::strokePath(const Path& path)
{
    BlackBerry::Platform::Graphics::Path* pp = path.platformPath();
    if (!pp->isEmpty()) {
        BlackBerry::Platform::Graphics::Gradient* gradient = strokeGradient() ? strokeGradient()->platformGradient() : 0;
        BlackBerry::Platform::Graphics::Pattern* pattern = strokePattern() ? strokePattern()->platformPattern(AffineTransform()) : 0;
        platformContext()->addStrokePath(*pp, gradient, pattern);
    }
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>&, int, int, const Color&)
{
    notImplemented();
}

void GraphicsContext::drawFocusRing(const Path&, int, int, const Color&)
{
    notImplemented();
}

void GraphicsContext::drawLine(const IntPoint& from, const IntPoint& to)
{
    platformContext()->addDrawLine(from, to);
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint& pt, float width, DocumentMarkerLineStyle style)
{
    platformContext()->addDrawLineForDocumentMarker(pt, width, (BlackBerry::Platform::Graphics::DocumentMarkerLineStyle)style);
}

void GraphicsContext::drawLineForText(const FloatPoint& pt, float width, bool printing)
{
    platformContext()->addDrawLineForText(pt, width, printing);
}

// FIXME: don't ignore the winding rule. https://bugs.webkit.org/show_bug.cgi?id=107064
void GraphicsContext::clip(const Path& path, WindRule)
{
    BlackBerry::Platform::Graphics::Path* pp = path.platformPath();
    pp->applyAsClip(platformContext());
}

void GraphicsContext::clipPath(const Path& path, WindRule)
{
    if (path.platformPath()->isRectangular())
        platformContext()->clip(path.boundingRect());
    else
        clip(path);
}

void GraphicsContext::canvasClip(const Path& path, WindRule fillRule)
{
    clip(path, fillRule);
}

void GraphicsContext::clipOut(const Path& path)
{
    BlackBerry::Platform::Graphics::Path* pp = path.platformPath();
    pp->applyAsClipOut(platformContext());
}

}
