/*
 *  Copyright (C) 2007-2009 Torch Mobile, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "Path.h"

#include "AffineTransform.h"
#include "FloatRect.h"
#include "NotImplemented.h"
#include "PlatformPathWinCE.h"
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

Path::Path()
    : m_path(new PlatformPath())
{
}

Path::Path(const Path& other)
    : m_path(new PlatformPath(*other.m_path))
{
}

Path::~Path()
{
    delete m_path;
}

Path& Path::operator=(const Path& other)
{
    if (&other != this) {
        delete m_path;
        m_path = new PlatformPath(*other.m_path);
    }
    return *this;
}

bool Path::contains(const FloatPoint& point, WindRule rule) const
{
    return m_path->contains(point, rule);
}

void Path::translate(const FloatSize& size)
{
    m_path->translate(size);
}

FloatRect Path::boundingRect() const
{
    return m_path->boundingRect();
}

void Path::moveTo(const FloatPoint& point)
{
    m_path->moveTo(point);
}

void Path::addLineTo(const FloatPoint& point)
{
    m_path->addLineTo(point);
}

void Path::addQuadCurveTo(const FloatPoint& cp, const FloatPoint& p)
{
    m_path->addQuadCurveTo(cp, p);
}

void Path::addBezierCurveTo(const FloatPoint& cp1, const FloatPoint& cp2, const FloatPoint& p)
{
    m_path->addBezierCurveTo(cp1, cp2, p);
}

void Path::addArcTo(const FloatPoint& p1, const FloatPoint& p2, float radius)
{
    m_path->addArcTo(p1, p2, radius);
}

void Path::closeSubpath()
{
    m_path->closeSubpath();
}

void Path::addArc(const FloatPoint& p, float r, float sar, float ear, bool anticlockwise)
{
    m_path->addEllipse(p, r, r, sar, ear, anticlockwise);
}

void Path::addRect(const FloatRect& r)
{
    m_path->addRect(r);
}

void Path::addEllipse(const FloatRect& r)
{
    m_path->addEllipse(r);
}

void Path::clear()
{
    m_path->clear();
}

bool Path::isEmpty() const
{
    return m_path->isEmpty();
}

void Path::apply(void* info, PathApplierFunction function) const
{
    m_path->apply(info, function);
}

void Path::transform(const AffineTransform& t)
{
    m_path->transform(t);
}

FloatRect Path::strokeBoundingRect(StrokeStyleApplier*) const
{
    notImplemented();
    return FloatRect();
}

bool Path::strokeContains(StrokeStyleApplier*, const FloatPoint&) const
{
    notImplemented();
    return false;
}

bool Path::hasCurrentPoint() const
{
    // Not sure if this is correct. At the meantime, we do what other ports
    // do.
    // See https://bugs.webkit.org/show_bug.cgi?id=27266,
    // https://bugs.webkit.org/show_bug.cgi?id=27187, and 
    // http://trac.webkit.org/changeset/45873
    return !isEmpty();
}

FloatPoint Path::currentPoint() const
{
    return m_path->lastPoint();
}

} // namespace WebCore
