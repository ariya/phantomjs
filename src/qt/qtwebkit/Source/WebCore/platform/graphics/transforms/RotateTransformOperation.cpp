/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "RotateTransformOperation.h"

#include "AnimationUtilities.h"
#include <algorithm>
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

PassRefPtr<TransformOperation> RotateTransformOperation::blend(const TransformOperation* from, double progress, bool blendToIdentity)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToIdentity)
        return RotateTransformOperation::create(m_x, m_y, m_z, m_angle - m_angle * progress, m_type);
    
    const RotateTransformOperation* fromOp = static_cast<const RotateTransformOperation*>(from);
    
    // Optimize for single axis rotation
    if (!fromOp || (fromOp->m_x == 0 && fromOp->m_y == 0 && fromOp->m_z == 1) || 
                   (fromOp->m_x == 0 && fromOp->m_y == 1 && fromOp->m_z == 0) || 
                   (fromOp->m_x == 1 && fromOp->m_y == 0 && fromOp->m_z == 0)) {
        double fromAngle = fromOp ? fromOp->m_angle : 0;
        return RotateTransformOperation::create(fromOp ? fromOp->m_x : m_x, 
                                                fromOp ? fromOp->m_y : m_y, 
                                                fromOp ? fromOp->m_z : m_z, 
                                                WebCore::blend(fromAngle, m_angle, progress), m_type);
    }

    const RotateTransformOperation* toOp = this;

    // Create the 2 rotation matrices
    TransformationMatrix fromT;
    TransformationMatrix toT;
    fromT.rotate3d((fromOp ? fromOp->m_x : 0),
        (fromOp ? fromOp->m_y : 0),
        (fromOp ? fromOp->m_z : 1),
        (fromOp ? fromOp->m_angle : 0));

    toT.rotate3d((toOp ? toOp->m_x : 0),
        (toOp ? toOp->m_y : 0),
        (toOp ? toOp->m_z : 1),
        (toOp ? toOp->m_angle : 0));
    
    // Blend them
    toT.blend(fromT, progress);
    
    // Extract the result as a quaternion
    TransformationMatrix::DecomposedType decomp;
    toT.decompose(decomp);
    
    // Convert that to Axis/Angle form
    double x = -decomp.quaternionX;
    double y = -decomp.quaternionY;
    double z = -decomp.quaternionZ;
    double length = sqrt(x * x + y * y + z * z);
    double angle = 0;
    
    if (length > 0.00001) {
        x /= length;
        y /= length;
        z /= length;
        angle = rad2deg(acos(decomp.quaternionW) * 2);
    } else {
        x = 0;
        y = 0;
        z = 1;
    }
    return RotateTransformOperation::create(x, y, z, angle, ROTATE_3D);
}

} // namespace WebCore
