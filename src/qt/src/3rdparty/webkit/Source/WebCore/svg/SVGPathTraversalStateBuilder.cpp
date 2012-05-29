/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "SVGPathTraversalStateBuilder.h"

namespace WebCore {

SVGPathTraversalStateBuilder::SVGPathTraversalStateBuilder()
    : m_traversalState(0)
    , m_desiredLength(0)
{
}

void SVGPathTraversalStateBuilder::moveTo(const FloatPoint& targetPoint, bool, PathCoordinateMode)
{
    ASSERT(m_traversalState);
    m_traversalState->m_totalLength += m_traversalState->moveTo(targetPoint);
}

void SVGPathTraversalStateBuilder::lineTo(const FloatPoint& targetPoint, PathCoordinateMode)
{
    ASSERT(m_traversalState);
    m_traversalState->m_totalLength += m_traversalState->lineTo(targetPoint);
}

void SVGPathTraversalStateBuilder::curveToCubic(const FloatPoint& point1, const FloatPoint& point2, const FloatPoint& targetPoint, PathCoordinateMode)
{
    ASSERT(m_traversalState);
    m_traversalState->m_totalLength += m_traversalState->cubicBezierTo(point1, point2, targetPoint);
}

void SVGPathTraversalStateBuilder::closePath()
{
    ASSERT(m_traversalState);
    m_traversalState->m_totalLength += m_traversalState->closeSubpath();
}

void SVGPathTraversalStateBuilder::setDesiredLength(float desiredLength)
{
    ASSERT(m_traversalState);
    m_traversalState->m_desiredLength = desiredLength;
}

bool SVGPathTraversalStateBuilder::continueConsuming()
{
    ASSERT(m_traversalState);
    ASSERT(m_traversalState->m_action == PathTraversalState::TraversalSegmentAtLength);
    return m_traversalState->m_totalLength < m_traversalState->m_desiredLength;
}

void SVGPathTraversalStateBuilder::incrementPathSegmentCount()
{
    ASSERT(m_traversalState);
    ++m_traversalState->m_segmentIndex;
}

unsigned long SVGPathTraversalStateBuilder::pathSegmentIndex()
{
    ASSERT(m_traversalState);
    return m_traversalState->m_segmentIndex;
}

}

#endif // ENABLE(SVG)
