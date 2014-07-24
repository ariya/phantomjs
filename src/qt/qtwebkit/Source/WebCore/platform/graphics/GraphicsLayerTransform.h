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

#ifndef GraphicsLayerTransform_h
#define GraphicsLayerTransform_h

#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "FloatSize.h"
#include "TransformationMatrix.h"

namespace WebCore {

class GraphicsLayerTransform {
public:
    GraphicsLayerTransform();
    void setPosition(const FloatPoint&);
    void setSize(const FloatSize&);
    void setAnchorPoint(const FloatPoint3D&);
    void setFlattening(bool);
    void setLocalTransform(const TransformationMatrix&);
    void setChildrenTransform(const TransformationMatrix&);
    TransformationMatrix combined();
    TransformationMatrix combinedForChildren();

    void combineTransforms(const TransformationMatrix& parentTransform);

private:
    void combineTransformsForChildren();

    FloatPoint3D m_anchorPoint;
    FloatPoint m_position;
    FloatSize m_size;
    bool m_flattening;
    bool m_dirty;
    bool m_childrenDirty;

    TransformationMatrix m_local;
    TransformationMatrix m_children;
    TransformationMatrix m_combined;
    TransformationMatrix m_combinedForChildren;
};

}

#endif // GraphicsLayerTransform_h
