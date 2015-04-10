/*
 * Copyright (C) 2004, 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2005 Nokia.  All rights reserved.
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
#include "FloatPoint.h"

#include "AffineTransform.h"
#include "FloatConversion.h"
#include "IntPoint.h"
#include "TransformationMatrix.h"
#include <limits>
#include <math.h>

namespace WebCore {

FloatPoint::FloatPoint(const IntPoint& p) : m_x(p.x()), m_y(p.y())
{
}

void FloatPoint::normalize()
{
    float tempLength = length();

    if (tempLength) {
        m_x /= tempLength;
        m_y /= tempLength;
    }
}

float FloatPoint::slopeAngleRadians() const
{
    return atan2f(m_y, m_x);
}

float FloatPoint::length() const
{
    return sqrtf(lengthSquared());
}

FloatPoint FloatPoint::matrixTransform(const AffineTransform& transform) const
{
    double newX, newY;
    transform.map(static_cast<double>(m_x), static_cast<double>(m_y), newX, newY);
    return narrowPrecision(newX, newY);
}

FloatPoint FloatPoint::matrixTransform(const TransformationMatrix& transform) const
{
    double newX, newY;
    transform.map(static_cast<double>(m_x), static_cast<double>(m_y), newX, newY);
    return narrowPrecision(newX, newY);
}

FloatPoint FloatPoint::narrowPrecision(double x, double y)
{
    return FloatPoint(narrowPrecisionToFloat(x), narrowPrecisionToFloat(y));
}

float findSlope(const FloatPoint& p1, const FloatPoint& p2, float& c)
{
    if (p2.x() == p1.x())
        return std::numeric_limits<float>::infinity();

    // y = mx + c
    float slope = (p2.y() - p1.y()) / (p2.x() - p1.x());
    c = p1.y() - slope * p1.x();
    return slope;
}

bool findIntersection(const FloatPoint& p1, const FloatPoint& p2, const FloatPoint& d1, const FloatPoint& d2, FloatPoint& intersection) 
{
    float pOffset = 0;
    float pSlope = findSlope(p1, p2, pOffset);

    float dOffset = 0;
    float dSlope = findSlope(d1, d2, dOffset);

    if (dSlope == pSlope)
        return false;
    
    if (pSlope == std::numeric_limits<float>::infinity()) {
        intersection.setX(p1.x());
        intersection.setY(dSlope * intersection.x() + dOffset);
        return true;
    }
    if (dSlope == std::numeric_limits<float>::infinity()) {
        intersection.setX(d1.x());
        intersection.setY(pSlope * intersection.x() + pOffset);
        return true;
    }
    
    // Find x at intersection, where ys overlap; x = (c' - c) / (m - m')
    intersection.setX((dOffset - pOffset) / (pSlope - dSlope));
    intersection.setY(pSlope * intersection.x() + pOffset);
    return true;
}

}
