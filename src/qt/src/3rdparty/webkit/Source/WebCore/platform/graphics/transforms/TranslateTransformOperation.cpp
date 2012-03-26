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
#include "TranslateTransformOperation.h"
#include "FloatConversion.h"

namespace WebCore {

PassRefPtr<TransformOperation> TranslateTransformOperation::blend(const TransformOperation* from, double progress, bool blendToIdentity)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToIdentity)
        return TranslateTransformOperation::create(Length(m_x.type()).blend(m_x, narrowPrecisionToFloat(progress)), 
                                                   Length(m_y.type()).blend(m_y, narrowPrecisionToFloat(progress)), 
                                                   Length(m_z.type()).blend(m_z, narrowPrecisionToFloat(progress)), m_type);

    const TranslateTransformOperation* fromOp = static_cast<const TranslateTransformOperation*>(from);
    Length fromX = fromOp ? fromOp->m_x : Length(m_x.type());
    Length fromY = fromOp ? fromOp->m_y : Length(m_y.type());
    Length fromZ = fromOp ? fromOp->m_z : Length(m_z.type());
    return TranslateTransformOperation::create(m_x.blend(fromX, narrowPrecisionToFloat(progress)), m_y.blend(fromY, narrowPrecisionToFloat(progress)), m_z.blend(fromZ, narrowPrecisionToFloat(progress)), m_type);
}

} // namespace WebCore
