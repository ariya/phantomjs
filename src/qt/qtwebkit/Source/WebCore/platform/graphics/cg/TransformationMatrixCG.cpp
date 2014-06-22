/*
 * Copyright (C) 2005, 2006, 2007 Apple Inc.  All rights reserved.
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
#include "TransformationMatrix.h"

#if USE(CG)

#include <CoreGraphics/CGAffineTransform.h>
#include "FloatConversion.h"

namespace WebCore {

TransformationMatrix::TransformationMatrix(const CGAffineTransform& t)
{
    setA(t.a);
    setB(t.b);
    setC(t.c);
    setD(t.d);
    setE(t.tx);
    setF(t.ty);
}

TransformationMatrix::operator CGAffineTransform() const
{
    return CGAffineTransformMake(narrowPrecisionToCGFloat(a()),
                                 narrowPrecisionToCGFloat(b()),
                                 narrowPrecisionToCGFloat(c()),
                                 narrowPrecisionToCGFloat(d()),
                                 narrowPrecisionToCGFloat(e()),
                                 narrowPrecisionToCGFloat(f()));
}

AffineTransform::AffineTransform(const CGAffineTransform& t)
{
    setMatrix(t.a, t.b, t.c, t.d, t.tx, t.ty);
}

AffineTransform::operator CGAffineTransform() const
{
    return CGAffineTransformMake(narrowPrecisionToCGFloat(a()),
                                 narrowPrecisionToCGFloat(b()),
                                 narrowPrecisionToCGFloat(c()),
                                 narrowPrecisionToCGFloat(d()),
                                 narrowPrecisionToCGFloat(e()),
                                 narrowPrecisionToCGFloat(f()));
}

}

#endif // USE(CG)
