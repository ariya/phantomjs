/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "TransformationMatrix.h"

#if USE(CA)

#include "FloatConversion.h"
#include <QuartzCore/CATransform3D.h>

namespace WebCore {

TransformationMatrix::TransformationMatrix(const CATransform3D& t)
{
    setMatrix(
        t.m11, t.m12, t.m13, t.m14,
        t.m21, t.m22, t.m23, t.m24, 
        t.m31, t.m32, t.m33, t.m34, 
        t.m41, t.m42, t.m43, t.m44);
}

TransformationMatrix::operator CATransform3D() const
{
    CATransform3D toT3D;
    toT3D.m11 = narrowPrecisionToFloat(m11());
    toT3D.m12 = narrowPrecisionToFloat(m12());
    toT3D.m13 = narrowPrecisionToFloat(m13());
    toT3D.m14 = narrowPrecisionToFloat(m14());
    toT3D.m21 = narrowPrecisionToFloat(m21());
    toT3D.m22 = narrowPrecisionToFloat(m22());
    toT3D.m23 = narrowPrecisionToFloat(m23());
    toT3D.m24 = narrowPrecisionToFloat(m24());
    toT3D.m31 = narrowPrecisionToFloat(m31());
    toT3D.m32 = narrowPrecisionToFloat(m32());
    toT3D.m33 = narrowPrecisionToFloat(m33());
    toT3D.m34 = narrowPrecisionToFloat(m34());
    toT3D.m41 = narrowPrecisionToFloat(m41());
    toT3D.m42 = narrowPrecisionToFloat(m42());
    toT3D.m43 = narrowPrecisionToFloat(m43());
    toT3D.m44 = narrowPrecisionToFloat(m44());
    return toT3D;
}

}

#endif // USE(CA)
