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
#include "ScaleTransformOperation.h"

namespace WebCore {

PassRefPtr<TransformOperation> ScaleTransformOperation::blend(const TransformOperation* from, double progress, bool blendToIdentity)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToIdentity)
        return ScaleTransformOperation::create(m_x + (1. - m_x) * progress,
                                               m_y + (1. - m_y) * progress,
                                               m_z + (1. - m_z) * progress, m_type);
    
    const ScaleTransformOperation* fromOp = static_cast<const ScaleTransformOperation*>(from);
    double fromX = fromOp ? fromOp->m_x : 1.;
    double fromY = fromOp ? fromOp->m_y : 1.;
    double fromZ = fromOp ? fromOp->m_z : 1.;
    return ScaleTransformOperation::create(fromX + (m_x - fromX) * progress,
                                           fromY + (m_y - fromY) * progress,
                                           fromZ + (m_z - fromZ) * progress, m_type);
}

} // namespace WebCore
