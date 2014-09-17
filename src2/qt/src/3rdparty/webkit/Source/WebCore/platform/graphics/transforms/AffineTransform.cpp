/*
 * Copyright (C) 2005, 2006 Apple Computer, Inc.  All rights reserved.
 *               2010 Dirk Schulze <krit@webkit.org>
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
#include "AffineTransform.h"

#include "FloatConversion.h"
#include "FloatQuad.h"
#include "FloatRect.h"
#include "IntRect.h"

#include <wtf/MathExtras.h>

namespace WebCore {

static void affineTransformDecompose(const AffineTransform& matrix, double sr[9])
{
    AffineTransform m(matrix);

    // Compute scaling factors
    double sx = matrix.xScale();
    double sy = matrix.yScale();

    // Compute cross product of transformed unit vectors. If negative,
    // one axis was flipped.
    if (m.a() * m.d() - m.c() * m.b() < 0.0) {
        // Flip axis with minimum unit vector dot product
        if (m.a() < m.d())
            sx = -sx;
        else
            sy = -sy;
    }

    // Remove scale from matrix
    m.scale(1.0 / sx, 1.0 / sy);

    // Compute rotation
    double angle = atan2(m.b(), m.a());

    // Remove rotation from matrix
    m.rotate(rad2deg(-angle));

    // Return results
    sr[0] = sx;
    sr[1] = sy;
    sr[2] = angle;
    sr[3] = m.a();
    sr[4] = m.b();
    sr[5] = m.c();
    sr[6] = m.d();
    sr[7] = m.e();
    sr[8] = m.f();
}

static void affineTransformCompose(AffineTransform& m, const double sr[9])
{
    m.setA(sr[3]);
    m.setB(sr[4]);
    m.setC(sr[5]);
    m.setD(sr[6]);
    m.setE(sr[7]);
    m.setF(sr[8]);
    m.rotate(rad2deg(sr[2]));
    m.scale(sr[0], sr[1]);
}

AffineTransform::AffineTransform()
{
    setMatrix(1, 0, 0, 1, 0, 0);
}

AffineTransform::AffineTransform(double a, double b, double c, double d, double e, double f)
{
    setMatrix(a, b, c, d, e, f);
}

void AffineTransform::makeIdentity()
{
    setMatrix(1, 0, 0, 1, 0, 0);
}

void AffineTransform::setMatrix(double a, double b, double c, double d, double e, double f)
{
    m_transform[0] = a;
    m_transform[1] = b;
    m_transform[2] = c;
    m_transform[3] = d;
    m_transform[4] = e;
    m_transform[5] = f;
}

bool AffineTransform::isIdentity() const
{
    return (m_transform[0] == 1 && m_transform[1] == 0
         && m_transform[2] == 0 && m_transform[3] == 1
         && m_transform[4] == 0 && m_transform[5] == 0);
}

double AffineTransform::xScale() const
{
    return sqrt(m_transform[0] * m_transform[0] + m_transform[1] * m_transform[1]);
}

double AffineTransform::yScale() const
{
    return sqrt(m_transform[2] * m_transform[2] + m_transform[3] * m_transform[3]);
}

double AffineTransform::det() const
{
    return m_transform[0] * m_transform[3] - m_transform[1] * m_transform[2];
}

bool AffineTransform::isInvertible() const
{
    return det() != 0.0;
}

AffineTransform AffineTransform::inverse() const
{
    double determinant = det();
    if (determinant == 0.0)
        return AffineTransform();

    AffineTransform result;
    if (isIdentityOrTranslation()) {
        result.m_transform[4] = -m_transform[4];
        result.m_transform[5] = -m_transform[5];
        return result;
    }

    result.m_transform[0] = m_transform[3] / determinant;
    result.m_transform[1] = -m_transform[1] / determinant;
    result.m_transform[2] = -m_transform[2] / determinant;
    result.m_transform[3] = m_transform[0] / determinant;
    result.m_transform[4] = (m_transform[2] * m_transform[5]
                           - m_transform[3] * m_transform[4]) / determinant;
    result.m_transform[5] = (m_transform[1] * m_transform[4]
                           - m_transform[0] * m_transform[5]) / determinant;

    return result;
}


// Multiplies this AffineTransform by the provided AffineTransform - i.e.
// this = this * other;
AffineTransform& AffineTransform::multiply(const AffineTransform& other)
{
    AffineTransform trans;
    
    trans.m_transform[0] = other.m_transform[0] * m_transform[0] + other.m_transform[1] * m_transform[2];
    trans.m_transform[1] = other.m_transform[0] * m_transform[1] + other.m_transform[1] * m_transform[3];
    trans.m_transform[2] = other.m_transform[2] * m_transform[0] + other.m_transform[3] * m_transform[2];
    trans.m_transform[3] = other.m_transform[2] * m_transform[1] + other.m_transform[3] * m_transform[3];
    trans.m_transform[4] = other.m_transform[4] * m_transform[0] + other.m_transform[5] * m_transform[2] + m_transform[4];
    trans.m_transform[5] = other.m_transform[4] * m_transform[1] + other.m_transform[5] * m_transform[3] + m_transform[5];

    setMatrix(trans.m_transform);
    return *this;
}

AffineTransform& AffineTransform::rotate(double a)
{
    // angle is in degree. Switch to radian
    a = deg2rad(a);
    double cosAngle = cos(a);
    double sinAngle = sin(a);
    AffineTransform rot(cosAngle, sinAngle, -sinAngle, cosAngle, 0, 0);

    multiply(rot);
    return *this;
}

AffineTransform& AffineTransform::scale(double s)
{
    return scale(s, s);
}

AffineTransform& AffineTransform::scale(double sx, double sy)
{
    m_transform[0] *= sx;
    m_transform[1] *= sx;
    m_transform[2] *= sy;
    m_transform[3] *= sy;
    return *this;
}

// *this = *this * translation
AffineTransform& AffineTransform::translate(double tx, double ty)
{
    if (isIdentityOrTranslation()) {
        m_transform[4] += tx;
        m_transform[5] += ty;
        return *this;
    }
        
    m_transform[4] += tx * m_transform[0] + ty * m_transform[2];
    m_transform[5] += tx * m_transform[1] + ty * m_transform[3];
    return *this;
}

AffineTransform& AffineTransform::scaleNonUniform(double sx, double sy)
{
    return scale(sx, sy);
}

AffineTransform& AffineTransform::rotateFromVector(double x, double y)
{
    return rotate(rad2deg(atan2(y, x)));
}

AffineTransform& AffineTransform::flipX()
{
    return scale(-1, 1);
}

AffineTransform& AffineTransform::flipY()
{
    return scale(1, -1);
}

AffineTransform& AffineTransform::shear(double sx, double sy)
{
    double a = m_transform[0];
    double b = m_transform[1];

    m_transform[0] += sy * m_transform[2];
    m_transform[1] += sy * m_transform[3];
    m_transform[2] += sx * a;
    m_transform[3] += sx * b;

    return *this;
}

AffineTransform& AffineTransform::skew(double angleX, double angleY)
{
    return shear(tan(deg2rad(angleX)), tan(deg2rad(angleY)));
}

AffineTransform& AffineTransform::skewX(double angle)
{
    return shear(tan(deg2rad(angle)), 0);
}

AffineTransform& AffineTransform::skewY(double angle)
{
    return shear(0, tan(deg2rad(angle)));
}

AffineTransform makeMapBetweenRects(const FloatRect& source, const FloatRect& dest)
{
    AffineTransform transform;
    transform.translate(dest.x() - source.x(), dest.y() - source.y());
    transform.scale(dest.width() / source.width(), dest.height() / source.height());
    return transform;
}

void AffineTransform::map(double x, double y, double& x2, double& y2) const
{
    x2 = (m_transform[0] * x + m_transform[2] * y + m_transform[4]);
    y2 = (m_transform[1] * x + m_transform[3] * y + m_transform[5]);
}

IntPoint AffineTransform::mapPoint(const IntPoint& point) const
{
    double x2, y2;
    map(point.x(), point.y(), x2, y2);
    
    // Round the point.
    return IntPoint(lround(x2), lround(y2));
}

FloatPoint AffineTransform::mapPoint(const FloatPoint& point) const
{
    double x2, y2;
    map(point.x(), point.y(), x2, y2);

    return FloatPoint(narrowPrecisionToFloat(x2), narrowPrecisionToFloat(y2));
}

IntRect AffineTransform::mapRect(const IntRect &rect) const
{
    return enclosingIntRect(mapRect(FloatRect(rect)));
}

FloatRect AffineTransform::mapRect(const FloatRect& rect) const
{
    if (isIdentityOrTranslation()) {
        FloatRect mappedRect(rect);
        mappedRect.move(narrowPrecisionToFloat(m_transform[4]), narrowPrecisionToFloat(m_transform[5]));
        return mappedRect;
    }

    FloatQuad result;
    result.setP1(mapPoint(rect.location()));
    result.setP2(mapPoint(FloatPoint(rect.maxX(), rect.y())));
    result.setP3(mapPoint(FloatPoint(rect.maxX(), rect.maxY())));
    result.setP4(mapPoint(FloatPoint(rect.x(), rect.maxY())));
    return result.boundingBox();
}

FloatQuad AffineTransform::mapQuad(const FloatQuad& q) const
{
    if (isIdentityOrTranslation()) {
        FloatQuad mappedQuad(q);
        mappedQuad.move(narrowPrecisionToFloat(m_transform[4]), narrowPrecisionToFloat(m_transform[5]));
        return mappedQuad;
    }

    FloatQuad result;
    result.setP1(mapPoint(q.p1()));
    result.setP2(mapPoint(q.p2()));
    result.setP3(mapPoint(q.p3()));
    result.setP4(mapPoint(q.p4()));
    return result;
}

void AffineTransform::blend(const AffineTransform& from, double progress)
{
    double srA[9], srB[9];

    affineTransformDecompose(from, srA);
    affineTransformDecompose(*this, srB);

    // If x-axis of one is flipped, and y-axis of the other, convert to an unflipped rotation.
    if ((srA[0] < 0 && srB[1] < 0) || (srA[1] < 0 &&  srB[0] < 0)) {
        srA[0] = -srA[0];
        srA[1] = -srA[1];
        srA[2] += srA[2] < 0 ? piDouble : -piDouble;
    }

    // Don't rotate the long way around.
    srA[2] = fmod(srA[2], 2.0 * piDouble);
    srB[2] = fmod(srB[2], 2.0 * piDouble);

    if (fabs(srA[2] - srB[2]) > piDouble) {
        if (srA[2] > srB[2])
            srA[2] -= piDouble * 2.0;
        else
            srB[2] -= piDouble * 2.0;
    }

    for (int i = 0; i < 9; i++)
        srA[i] = srA[i] + progress * (srB[i] - srA[i]);

    affineTransformCompose(*this, srA);
}

TransformationMatrix AffineTransform::toTransformationMatrix() const
{
    return TransformationMatrix(m_transform[0], m_transform[1], m_transform[2],
                                m_transform[3], m_transform[4], m_transform[5]);
}

}
