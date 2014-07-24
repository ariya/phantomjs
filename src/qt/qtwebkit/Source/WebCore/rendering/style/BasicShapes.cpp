/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#include "BasicShapes.h"

#include "FloatRect.h"
#include "LengthFunctions.h"
#include "Path.h"

namespace WebCore {

bool BasicShape::canBlend(const BasicShape* other) const
{
    // FIXME: Support animations between different shapes in the future.
    if (type() != other->type())
        return false;

    // Just polygons with same number of vertices can be animated.
    if (type() == BasicShape::BasicShapePolygonType
        && static_cast<const BasicShapePolygon*>(this)->values().size() != static_cast<const BasicShapePolygon*>(other)->values().size())
        return false;

    return true;
}

void BasicShapeRectangle::path(Path& path, const FloatRect& boundingBox)
{
    ASSERT(path.isEmpty());
    path.addRoundedRect(
        FloatRect(
            floatValueForLength(m_x, boundingBox.width()) + boundingBox.x(),
            floatValueForLength(m_y, boundingBox.height()) + boundingBox.y(),
            floatValueForLength(m_width, boundingBox.width()),
            floatValueForLength(m_height, boundingBox.height())
        ),
        FloatSize(
            floatValueForLength(m_cornerRadiusX, boundingBox.width()),
            floatValueForLength(m_cornerRadiusY, boundingBox.height())
        )
    );
}

PassRefPtr<BasicShape> BasicShapeRectangle::blend(const BasicShape* other, double progress) const
{
    ASSERT(type() == other->type());

    const BasicShapeRectangle* o = static_cast<const BasicShapeRectangle*>(other);
    RefPtr<BasicShapeRectangle> result =  BasicShapeRectangle::create();
    result->setX(m_x.blend(o->x(), progress));
    result->setY(m_y.blend(o->y(), progress));
    result->setWidth(m_width.blend(o->width(), progress));
    result->setHeight(m_height.blend(o->height(), progress));
    result->setCornerRadiusX(m_cornerRadiusX.blend(o->cornerRadiusX(), progress));
    result->setCornerRadiusY(m_cornerRadiusY.blend(o->cornerRadiusY(), progress));
    return result.release();
}

void BasicShapeCircle::path(Path& path, const FloatRect& boundingBox)
{
    ASSERT(path.isEmpty());
    float diagonal = sqrtf((boundingBox.width() * boundingBox.width() + boundingBox.height() * boundingBox.height()) / 2);
    float centerX = floatValueForLength(m_centerX, boundingBox.width());
    float centerY = floatValueForLength(m_centerY, boundingBox.height());
    float radius = floatValueForLength(m_radius, diagonal);
    path.addEllipse(FloatRect(
        centerX - radius + boundingBox.x(),
        centerY - radius + boundingBox.y(),
        radius * 2,
        radius * 2
    ));
}

PassRefPtr<BasicShape> BasicShapeCircle::blend(const BasicShape* other, double progress) const
{
    ASSERT(type() == other->type());

    const BasicShapeCircle* o = static_cast<const BasicShapeCircle*>(other);
    RefPtr<BasicShapeCircle> result =  BasicShapeCircle::create();
    result->setCenterX(m_centerX.blend(o->centerX(), progress));
    result->setCenterY(m_centerY.blend(o->centerY(), progress));
    result->setRadius(m_radius.blend(o->radius(), progress));
    return result.release();
}

void BasicShapeEllipse::path(Path& path, const FloatRect& boundingBox)
{
    ASSERT(path.isEmpty());
    float centerX = floatValueForLength(m_centerX, boundingBox.width());
    float centerY = floatValueForLength(m_centerY, boundingBox.height());
    float radiusX = floatValueForLength(m_radiusX, boundingBox.width());
    float radiusY = floatValueForLength(m_radiusY, boundingBox.height());
    path.addEllipse(FloatRect(
        centerX - radiusX + boundingBox.x(),
        centerY - radiusY + boundingBox.y(),
        radiusX * 2,
        radiusY * 2
    ));
}

PassRefPtr<BasicShape> BasicShapeEllipse::blend(const BasicShape* other, double progress) const
{
    ASSERT(type() == other->type());

    const BasicShapeEllipse* o = static_cast<const BasicShapeEllipse*>(other);
    RefPtr<BasicShapeEllipse> result =  BasicShapeEllipse::create();
    result->setCenterX(m_centerX.blend(o->centerX(), progress));
    result->setCenterY(m_centerY.blend(o->centerY(), progress));
    result->setRadiusX(m_radiusX.blend(o->radiusX(), progress));
    result->setRadiusY(m_radiusY.blend(o->radiusY(), progress));
    return result.release();
}

void BasicShapePolygon::path(Path& path, const FloatRect& boundingBox)
{
    ASSERT(path.isEmpty());
    ASSERT(!(m_values.size() % 2));
    size_t length = m_values.size();
    
    if (!length)
        return;

    path.moveTo(FloatPoint(floatValueForLength(m_values.at(0), boundingBox.width()) + boundingBox.x(),
        floatValueForLength(m_values.at(1), boundingBox.height()) + boundingBox.y()));
    for (size_t i = 2; i < length; i = i + 2) {
        path.addLineTo(FloatPoint(floatValueForLength(m_values.at(i), boundingBox.width()) + boundingBox.x(),
            floatValueForLength(m_values.at(i + 1), boundingBox.height()) + boundingBox.y()));
    }
    path.closeSubpath();
}

PassRefPtr<BasicShape> BasicShapePolygon::blend(const BasicShape* other, double progress) const
{
    ASSERT(type() == other->type());

    const BasicShapePolygon* o = static_cast<const BasicShapePolygon*>(other);
    ASSERT(m_values.size() == o->values().size());
    ASSERT(!(m_values.size() % 2));

    size_t length = m_values.size();
    RefPtr<BasicShapePolygon> result = BasicShapePolygon::create();
    if (!length)
        return result.release();

    result->setWindRule(o->windRule());

    for (size_t i = 0; i < length; i = i + 2) {
        result->appendPoint(m_values.at(i).blend(o->values().at(i), progress),
            m_values.at(i + 1).blend(o->values().at(i + 1), progress));
    }

    return result.release();
}

void BasicShapeInsetRectangle::path(Path& path, const FloatRect& boundingBox)
{
    ASSERT(path.isEmpty());
    float left = floatValueForLength(m_left, boundingBox.width());
    float top = floatValueForLength(m_top, boundingBox.height());
    path.addRoundedRect(
        FloatRect(
            left + boundingBox.x(),
            top + boundingBox.y(),
            std::max<float>(boundingBox.width() - left - floatValueForLength(m_right, boundingBox.width()), 0),
            std::max<float>(boundingBox.height() - top - floatValueForLength(m_bottom, boundingBox.height()), 0)
        ),
        FloatSize(
            floatValueForLength(m_cornerRadiusX, boundingBox.width()),
            floatValueForLength(m_cornerRadiusY, boundingBox.height())
        )
    );
}

PassRefPtr<BasicShape> BasicShapeInsetRectangle::blend(const BasicShape* other, double progress) const
{
    ASSERT(type() == other->type());

    const BasicShapeInsetRectangle* o = static_cast<const BasicShapeInsetRectangle*>(other);
    RefPtr<BasicShapeInsetRectangle> result =  BasicShapeInsetRectangle::create();
    result->setTop(m_top.blend(o->top(), progress));
    result->setRight(m_right.blend(o->right(), progress));
    result->setBottom(m_bottom.blend(o->bottom(), progress));
    result->setLeft(m_left.blend(o->left(), progress));
    result->setCornerRadiusX(m_cornerRadiusX.blend(o->cornerRadiusX(), progress));
    result->setCornerRadiusY(m_cornerRadiusY.blend(o->cornerRadiusY(), progress));
    return result.release();
}
}
