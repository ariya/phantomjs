/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(ACCELERATED_2D_CANVAS)

#include "LoopBlinnClassifier.h"

#include "LoopBlinnMathUtils.h"

namespace WebCore {

using LoopBlinnMathUtils::approxEqual;
using LoopBlinnMathUtils::roundToZero;

LoopBlinnClassifier::Result LoopBlinnClassifier::classify(const FloatPoint& c0,
                                                          const FloatPoint& c1,
                                                          const FloatPoint& c2,
                                                          const FloatPoint& c3)
{
    // Consult the chapter for the definitions of the following
    // (terse) variable names. Note that the b0..b3 coordinates are
    // homogeneous, so the "z" value (actually the w coordinate) must
    // be 1.0.
    FloatPoint3D b0(c0.x(), c0.y(), 1.0f);
    FloatPoint3D b1(c1.x(), c1.y(), 1.0f);
    FloatPoint3D b2(c2.x(), c2.y(), 1.0f);
    FloatPoint3D b3(c3.x(), c3.y(), 1.0f);

    // Compute a1..a3.
    float a1 = b0 * b3.cross(b2);
    float a2 = b1 * b0.cross(b3);
    float a3 = b2 * b1.cross(b0);

    // Compute d1..d3.
    float d1 = a1 - 2 * a2 + 3 * a3;
    float d2 = -a2 + 3 * a3;
    float d3 = 3 * a3;

    // Experimentation has shown that the texture coordinates computed
    // from these values quickly become huge, leading to roundoff errors
    // and artifacts in the shader. It turns out that if we normalize
    // the vector defined by (d1, d2, d3), this fixes the problem of the
    // texture coordinates getting too large without affecting the
    // classification results.
    FloatPoint3D nd(d1, d2, d3);
    nd.normalize();
    d1 = nd.x();
    d2 = nd.y();
    d3 = nd.z();

    // Compute the discriminant.
    // term0 is a common term in the computation which helps decide
    // which way to classify the cusp case: as serpentine or loop.
    float term0 = (3 * d2 * d2 - 4 * d1 * d3);
    float discriminant = d1 * d1 * term0;

    // Experimentation has also shown that when the classification is
    // near the boundary between one curve type and another, the shader
    // becomes numerically unstable, particularly with the cusp case.
    // Correct for this by rounding d1..d3 and the discriminant to zero
    // when they get near it.
    d1 = roundToZero(d1);
    d2 = roundToZero(d2);
    d3 = roundToZero(d3);
    discriminant = roundToZero(discriminant);

    // Do the classification.
    if (approxEqual(b0, b1) && approxEqual(b0, b2) && approxEqual(b0, b3))
        return Result(kPoint, d1, d2, d3);

    if (!discriminant) {
        if (!d1 && !d2) {
            if (!d3)
                return Result(kLine, d1, d2, d3);
            return Result(kQuadratic, d1, d2, d3);
        }

        if (!d1)
            return Result(kCusp, d1, d2, d3);

        // This is the boundary case described in Loop and Blinn's
        // SIGGRAPH '05 paper of a cusp with inflection at infinity.
        // Because term0 might not be exactly 0, we decide between using
        // the serpentine and loop cases depending on its sign to avoid
        // taking the square root of a negative number when computing the
        // cubic texture coordinates.
        if (term0 < 0)
            return Result(kLoop, d1, d2, d3);

        return Result(kSerpentine, d1, d2, d3);
    }

    if (discriminant > 0)
        return Result(kSerpentine, d1, d2, d3);

    // discriminant < 0
    return Result(kLoop, d1, d2, d3);
}

} // namespace WebCore

#endif
