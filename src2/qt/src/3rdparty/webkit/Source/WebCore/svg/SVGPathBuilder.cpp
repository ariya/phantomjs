/*
 * Copyright (C) 2002, 2003 The Karbon Developers
 * Copyright (C) 2006 Alexander Kellett <lypanov@kde.org>
 * Copyright (C) 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007, 2009 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#if ENABLE(SVG)
#include "SVGPathBuilder.h"

namespace WebCore {

SVGPathBuilder::SVGPathBuilder()
    : m_path(0)
{
}

void SVGPathBuilder::moveTo(const FloatPoint& targetPoint, bool closed, PathCoordinateMode mode)
{
    ASSERT(m_path);
    m_current = mode == AbsoluteCoordinates ? targetPoint : m_current + targetPoint;
    if (closed && !m_path->isEmpty())
        m_path->closeSubpath();
    m_path->moveTo(m_current);
}

void SVGPathBuilder::lineTo(const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_path);
    m_current = mode == AbsoluteCoordinates ? targetPoint : m_current + targetPoint;
    m_path->addLineTo(m_current);
}

void SVGPathBuilder::curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode mode)
{
    ASSERT(m_path);
    if (mode == RelativeCoordinates) {
        m_path->addBezierCurveTo(m_current + point1, m_current + point2, m_current + targetPoint);
        m_current += targetPoint;
    } else {
        m_current = targetPoint;
        m_path->addBezierCurveTo(point1, point2, m_current);
    }    
}

void SVGPathBuilder::closePath()
{
    ASSERT(m_path);
    m_path->closeSubpath();
}

}

#endif // ENABLE(SVG)
