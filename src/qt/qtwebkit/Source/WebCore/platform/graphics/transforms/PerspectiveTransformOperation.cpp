/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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
#include "PerspectiveTransformOperation.h"

#include "AnimationUtilities.h"
#include <wtf/MathExtras.h>

using namespace std;

namespace WebCore {

PassRefPtr<TransformOperation> PerspectiveTransformOperation::blend(const TransformOperation* from, double progress, bool blendToIdentity)
{
    if (from && !from->isSameType(*this))
        return this;
    
    if (blendToIdentity) {
        double p = floatValueForLength(m_p, 1);
        p = WebCore::blend(p, 1.0, progress); // FIXME: this seems wrong. https://bugs.webkit.org/show_bug.cgi?id=52700
        return PerspectiveTransformOperation::create(Length(clampToPositiveInteger(p), Fixed));
    }
    
    const PerspectiveTransformOperation* fromOp = static_cast<const PerspectiveTransformOperation*>(from);
    Length fromP = fromOp ? fromOp->m_p : Length(m_p.type());
    Length toP = m_p;

    TransformationMatrix fromT;
    TransformationMatrix toT;
    fromT.applyPerspective(floatValueForLength(fromP, 1));
    toT.applyPerspective(floatValueForLength(toP, 1));
    toT.blend(fromT, progress);
    TransformationMatrix::DecomposedType decomp;
    toT.decompose(decomp);

    if (decomp.perspectiveZ) {
        double val = -1.0 / decomp.perspectiveZ;
        return PerspectiveTransformOperation::create(Length(clampToPositiveInteger(val), Fixed));
    }
    return PerspectiveTransformOperation::create(Length(0, Fixed));
}

} // namespace WebCore
