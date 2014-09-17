/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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
#include "SVGTransformDistance.h"

#include "FloatConversion.h"
#include "FloatPoint.h"
#include "FloatSize.h"
#include "SVGTransform.h"

#include <math.h>

namespace WebCore {
    
SVGTransformDistance::SVGTransformDistance()
    : m_type(SVGTransform::SVG_TRANSFORM_UNKNOWN)
    , m_angle(0)
    , m_cx(0)
    , m_cy(0)
{
}

SVGTransformDistance::SVGTransformDistance(SVGTransform::SVGTransformType type, float angle, float cx, float cy, const AffineTransform& transform)
    : m_type(type)
    , m_angle(angle)
    , m_cx(cx)
    , m_cy(cy)
    , m_transform(transform)
{
}

SVGTransformDistance::SVGTransformDistance(const SVGTransform& fromSVGTransform, const SVGTransform& toSVGTransform)
    : m_type(fromSVGTransform.type())
    , m_angle(0)
    , m_cx(0)
    , m_cy(0)
{
    ASSERT(m_type == toSVGTransform.type());
    
    switch (m_type) {
    case SVGTransform::SVG_TRANSFORM_UNKNOWN:
        return;
    case SVGTransform::SVG_TRANSFORM_MATRIX:
        // FIXME: need to be able to subtract to matrices
        return;
    case SVGTransform::SVG_TRANSFORM_ROTATE: {
        FloatSize centerDistance = toSVGTransform.rotationCenter() - fromSVGTransform.rotationCenter();
        m_angle = toSVGTransform.angle() - fromSVGTransform.angle();
        m_cx = centerDistance.width();
        m_cy = centerDistance.height();
        return;
    }
    case SVGTransform::SVG_TRANSFORM_TRANSLATE: {
        FloatSize translationDistance = toSVGTransform.translate() - fromSVGTransform.translate();
        m_transform.translate(translationDistance.width(), translationDistance.height());
        return;
    }
    case SVGTransform::SVG_TRANSFORM_SCALE: {
        float scaleX = toSVGTransform.scale().width() - fromSVGTransform.scale().width();        
        float scaleY = toSVGTransform.scale().height() - fromSVGTransform.scale().height();
        m_transform.scaleNonUniform(scaleX, scaleY);
        return;
    }
    case SVGTransform::SVG_TRANSFORM_SKEWX:
    case SVGTransform::SVG_TRANSFORM_SKEWY:
        m_angle = toSVGTransform.angle() - fromSVGTransform.angle();
        return;
    }
}

SVGTransformDistance SVGTransformDistance::scaledDistance(float scaleFactor) const
{
    switch (m_type) {
    case SVGTransform::SVG_TRANSFORM_UNKNOWN:
        return SVGTransformDistance();
    case SVGTransform::SVG_TRANSFORM_ROTATE:
        return SVGTransformDistance(m_type, m_angle * scaleFactor, m_cx * scaleFactor, m_cy * scaleFactor, AffineTransform());
    case SVGTransform::SVG_TRANSFORM_SCALE:
    case SVGTransform::SVG_TRANSFORM_MATRIX:
        return SVGTransformDistance(m_type, m_angle * scaleFactor, m_cx * scaleFactor, m_cy * scaleFactor, AffineTransform(m_transform).scale(scaleFactor));
    case SVGTransform::SVG_TRANSFORM_TRANSLATE: {
        AffineTransform newTransform(m_transform);
        newTransform.setE(m_transform.e() * scaleFactor);
        newTransform.setF(m_transform.f() * scaleFactor);
        return SVGTransformDistance(m_type, 0, 0, 0, newTransform);
    }
    case SVGTransform::SVG_TRANSFORM_SKEWX:
    case SVGTransform::SVG_TRANSFORM_SKEWY:
        return SVGTransformDistance(m_type, m_angle * scaleFactor, m_cx * scaleFactor, m_cy * scaleFactor, AffineTransform());
    }
    
    ASSERT_NOT_REACHED();
    return SVGTransformDistance();
}

SVGTransform SVGTransformDistance::addSVGTransforms(const SVGTransform& first, const SVGTransform& second)
{
    ASSERT(first.type() == second.type());
    
    SVGTransform transform;
    
    switch (first.type()) {
    case SVGTransform::SVG_TRANSFORM_UNKNOWN:
        return SVGTransform();
    case SVGTransform::SVG_TRANSFORM_ROTATE: {
        transform.setRotate(first.angle() + second.angle(), first.rotationCenter().x() + second.rotationCenter().x(),
                            first.rotationCenter().y() + second.rotationCenter().y());
        return transform;
    }
    case SVGTransform::SVG_TRANSFORM_MATRIX:
        transform.setMatrix(first.matrix() * second.matrix());
        return transform;
    case SVGTransform::SVG_TRANSFORM_TRANSLATE: {
        float dx = first.translate().x() + second.translate().x();
        float dy = first.translate().y() + second.translate().y();
        transform.setTranslate(dx, dy);
        return transform;
    }
    case SVGTransform::SVG_TRANSFORM_SCALE: {
        FloatSize scale = first.scale() + second.scale();
        transform.setScale(scale.width(), scale.height());
        return transform;
    }
    case SVGTransform::SVG_TRANSFORM_SKEWX:
        transform.setSkewX(first.angle() + second.angle());
        return transform;
    case SVGTransform::SVG_TRANSFORM_SKEWY:
        transform.setSkewY(first.angle() + second.angle());
        return transform;
    }
    
    ASSERT_NOT_REACHED();
    return SVGTransform();
}

void SVGTransformDistance::addSVGTransform(const SVGTransform& transform, bool absoluteValue)
{
    // If this is the first add, set the type for this SVGTransformDistance
    if (m_type == SVGTransform::SVG_TRANSFORM_UNKNOWN)
        m_type = transform.type();
    
    ASSERT(m_type == transform.type());
    
    switch (m_type) {
    case SVGTransform::SVG_TRANSFORM_UNKNOWN:
        return;
    case SVGTransform::SVG_TRANSFORM_MATRIX:
        m_transform *= transform.matrix(); // FIXME: what does 'distance' between two transforms mean?  how should we respect 'absoluteValue' here?
        return;
    case SVGTransform::SVG_TRANSFORM_ROTATE:
        m_angle += absoluteValue ? fabsf(transform.angle()) : transform.angle();
        m_cx += absoluteValue ? fabsf(transform.rotationCenter().x()) : transform.rotationCenter().x();
        m_cy += absoluteValue ? fabsf(transform.rotationCenter().y()) : transform.rotationCenter().y();
        // fall through
    case SVGTransform::SVG_TRANSFORM_TRANSLATE: {
        float dx = absoluteValue ? fabsf(transform.translate().x()) : transform.translate().x();
        float dy = absoluteValue ? fabsf(transform.translate().y()) : transform.translate().y();
        m_transform.translate(dx, dy);
        return;
    }
    case SVGTransform::SVG_TRANSFORM_SCALE: {
        float scaleX = absoluteValue ? fabsf(transform.scale().width()) : transform.scale().width();
        float scaleY = absoluteValue ? fabsf(transform.scale().height()) : transform.scale().height();
        m_transform.scaleNonUniform(scaleX, scaleY);
        return;
    }
    case SVGTransform::SVG_TRANSFORM_SKEWX:
    case SVGTransform::SVG_TRANSFORM_SKEWY:
        m_angle += absoluteValue ? fabsf(transform.angle()) : transform.angle();
        return;
    }
    
    ASSERT_NOT_REACHED();
    return;
}

SVGTransform SVGTransformDistance::addToSVGTransform(const SVGTransform& transform) const
{
    ASSERT(m_type == transform.type() || transform == SVGTransform());
    
    SVGTransform newTransform(transform);
    
    switch (m_type) {
    case SVGTransform::SVG_TRANSFORM_UNKNOWN:
        return SVGTransform();
    case SVGTransform::SVG_TRANSFORM_MATRIX:
        return SVGTransform(transform.matrix() * m_transform);
    case SVGTransform::SVG_TRANSFORM_TRANSLATE: {
        FloatPoint translation = transform.translate();
        translation += FloatSize::narrowPrecision(m_transform.e(), m_transform.f());
        newTransform.setTranslate(translation.x(), translation.y());
        return newTransform;
    }
    case SVGTransform::SVG_TRANSFORM_SCALE: {
        FloatSize scale = transform.scale();
        scale += FloatSize::narrowPrecision(m_transform.a(), m_transform.d());
        newTransform.setScale(scale.width(), scale.height());
        return newTransform;
    }
    case SVGTransform::SVG_TRANSFORM_ROTATE: {
        // FIXME: I'm not certain the translation is calculated correctly here
        FloatPoint center = transform.rotationCenter();
        newTransform.setRotate(transform.angle() + m_angle,
                               center.x() + m_cx,
                               center.y() + m_cy);
        return newTransform;
    }
    case SVGTransform::SVG_TRANSFORM_SKEWX:
        newTransform.setSkewX(transform.angle() + m_angle);
        return newTransform;
    case SVGTransform::SVG_TRANSFORM_SKEWY:
        newTransform.setSkewY(transform.angle() + m_angle);
        return newTransform;
    }
    
    ASSERT_NOT_REACHED();
    return SVGTransform();
}

bool SVGTransformDistance::isZero() const
{
    return m_transform.isIdentity() && !m_angle;
}

float SVGTransformDistance::distance() const
{
    switch (m_type) {
    case SVGTransform::SVG_TRANSFORM_UNKNOWN:
        return 0;
    case SVGTransform::SVG_TRANSFORM_ROTATE:
        return sqrtf(m_angle * m_angle + m_cx * m_cx + m_cy * m_cy);
    case SVGTransform::SVG_TRANSFORM_MATRIX:
        return 0; // I'm not quite sure yet what distance between two matrices means.
    case SVGTransform::SVG_TRANSFORM_SCALE:
        return static_cast<float>(sqrt(m_transform.a() * m_transform.a() + m_transform.d() * m_transform.d()));
    case SVGTransform::SVG_TRANSFORM_TRANSLATE:
        return static_cast<float>(sqrt(m_transform.e() * m_transform.e() + m_transform.f() * m_transform.f()));
    case SVGTransform::SVG_TRANSFORM_SKEWX:
    case SVGTransform::SVG_TRANSFORM_SKEWY:
        return m_angle;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

}

#endif
