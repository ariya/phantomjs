/*
 Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.

 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.

 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "GraphicsLayerTransform.h"

namespace WebCore {

GraphicsLayerTransform::GraphicsLayerTransform()
    : m_anchorPoint(0.5, 0.5, 0)
    , m_flattening(true)
    , m_dirty(false) // false by default since all default values would be combined as the identity matrix
    , m_childrenDirty(false)
{
}

void GraphicsLayerTransform::setPosition(const FloatPoint& position)
{
    if (m_position == position)
        return;
    m_position = position;
    m_dirty = true;
}

void GraphicsLayerTransform::setSize(const FloatSize& size)
{
    if (m_size == size)
        return;
    m_size = size;
    m_dirty = true;
}

void GraphicsLayerTransform::setAnchorPoint(const FloatPoint3D& anchorPoint)
{
    if (m_anchorPoint == anchorPoint)
        return;
    m_anchorPoint = anchorPoint;
    m_dirty = true;
}

void GraphicsLayerTransform::setFlattening(bool flattening)
{
    if (m_flattening == flattening)
        return;
    m_flattening = flattening;
    m_dirty = true;
}

void GraphicsLayerTransform::setLocalTransform(const TransformationMatrix& transform)
{
    if (m_local == transform)
        return;
    m_local = transform;
    m_dirty = true;
}

void GraphicsLayerTransform::setChildrenTransform(const TransformationMatrix& transform)
{
    if (m_children == transform)
        return;
    m_children = transform;
    m_dirty = true;
}

TransformationMatrix GraphicsLayerTransform::combined()
{
    ASSERT(!m_dirty);
    return m_combined;
}

TransformationMatrix GraphicsLayerTransform::combinedForChildren()
{
    ASSERT(!m_dirty);
    if (m_childrenDirty)
        combineTransformsForChildren();
    return m_combinedForChildren;
}

void GraphicsLayerTransform::combineTransforms(const TransformationMatrix& parentTransform)
{
    float originX = m_anchorPoint.x() * m_size.width();
    float originY = m_anchorPoint.y() * m_size.height();
    m_combined =
        TransformationMatrix(parentTransform)
            .translate3d(originX + m_position.x(), originY + m_position.y(), m_anchorPoint.z() )
            .multiply(m_local);

    // The children transform will take it from here, if it gets used.
    m_combinedForChildren = m_combined;
    m_combined.translate3d(-originX, -originY, -m_anchorPoint.z());

    m_dirty = false;
    m_childrenDirty = true;
}

void GraphicsLayerTransform::combineTransformsForChildren()
{
    ASSERT(!m_dirty);
    ASSERT(m_childrenDirty);

    float originX = m_anchorPoint.x() * m_size.width();
    float originY = m_anchorPoint.y() * m_size.height();

    // In case a parent had preserves3D and this layer has not, flatten our children.
    if (m_flattening)
        m_combinedForChildren = m_combinedForChildren.to2dTransform();
    m_combinedForChildren.multiply(m_children);
    m_combinedForChildren.translate3d(-originX, -originY, -m_anchorPoint.z());

    m_childrenDirty = false;
}

}
