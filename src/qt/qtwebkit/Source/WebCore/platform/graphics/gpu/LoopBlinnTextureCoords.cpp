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

#include "LoopBlinnTextureCoords.h"

#include <math.h>
#include <wtf/Assertions.h>

namespace WebCore {

LoopBlinnTextureCoords::Result LoopBlinnTextureCoords::compute(const LoopBlinnClassifier::Result& classification, LoopBlinnConstants::FillSide sideToFill)
{
    // Loop and Blinn's formulation states that the right side of the
    // curve is defined to be the inside (filled region), but for some
    // reason it looks like with the default orientation parameters we
    // are filling the left side of the curve. Regardless, because we
    // can receive arbitrarily oriented curves as input, we might have
    // to reverse the orientation of the cubic texture coordinates even
    // in cases where the paper doesn't say it is necessary.
    bool reverseOrientation = false;
    static const float OneThird = 1.0f / 3.0f;
    static const float TwoThirds = 2.0f / 3.0f;
    LoopBlinnClassifier::CurveType curveType = classification.curveType;

    LoopBlinnTextureCoords::Result result;

    switch (curveType) {
    case LoopBlinnClassifier::kSerpentine: {
        float t1 = sqrtf(9.0f * classification.d2 * classification.d2 - 12 * classification.d1 * classification.d3);
        float ls = 3.0f * classification.d2 - t1;
        float lt = 6.0f * classification.d1;
        float ms = 3.0f * classification.d2 + t1;
        float mt = lt;
        float ltMinusLs = lt - ls;
        float mtMinusMs = mt - ms;
        result.klmCoordinates[0] = FloatPoint3D(ls * ms,
                                                ls * ls * ls,
                                                ms * ms * ms);
        result.klmCoordinates[1] = FloatPoint3D(OneThird * (3.0f * ls * ms - ls * mt - lt * ms),
                                                ls * ls * (ls - lt),
                                                ms * ms * (ms - mt));
        result.klmCoordinates[2] = FloatPoint3D(OneThird * (lt * (mt - 2.0f * ms) + ls * (3.0f * ms - 2.0f * mt)),
                                                ltMinusLs * ltMinusLs * ls,
                                                mtMinusMs * mtMinusMs * ms);
        result.klmCoordinates[3] = FloatPoint3D(ltMinusLs * mtMinusMs,
                                                -(ltMinusLs * ltMinusLs * ltMinusLs),
                                                -(mtMinusMs * mtMinusMs * mtMinusMs));
        if (classification.d1 < 0.0f)
            reverseOrientation = true;
        break;
    }

    case LoopBlinnClassifier::kLoop: {
        float t1 = sqrtf(4.0f * classification.d1 * classification.d3 - 3.0f * classification.d2 * classification.d2);
        float ls = classification.d2 - t1;
        float lt = 2.0f * classification.d1;
        float ms = classification.d2 + t1;
        float mt = lt;

        // Figure out whether there is a rendering artifact requiring
        // the curve to be subdivided by the caller.
        float ql = ls / lt;
        float qm = ms / mt;
        if (0.0f < ql && ql < 1.0f) {
            result.hasRenderingArtifact = true;
            result.subdivisionParameterValue = ql;
            return result;
        }

        if (0.0f < qm && qm < 1.0f) {
            result.hasRenderingArtifact = true;
            result.subdivisionParameterValue = qm;
            return result;
        }

        float ltMinusLs = lt - ls;
        float mtMinusMs = mt - ms;
        result.klmCoordinates[0] = FloatPoint3D(ls * ms,
                                                ls * ls * ms,
                                                ls * ms * ms);
        result.klmCoordinates[1] = FloatPoint3D(OneThird * (-ls * mt - lt * ms + 3.0f * ls * ms),
                                                -OneThird * ls * (ls * (mt - 3.0f * ms) + 2.0f * lt * ms),
                                                -OneThird * ms * (ls * (2.0f * mt - 3.0f * ms) + lt * ms));
        result.klmCoordinates[2] = FloatPoint3D(OneThird * (lt * (mt - 2.0f * ms) + ls * (3.0f * ms - 2.0f * mt)),
                                                OneThird * (lt - ls) * (ls * (2.0f * mt - 3.0f * ms) + lt * ms),
                                                OneThird * (mt - ms) * (ls * (mt - 3.0f * ms) + 2.0f * lt * ms));
        result.klmCoordinates[3] = FloatPoint3D(ltMinusLs * mtMinusMs,
                                                -(ltMinusLs * ltMinusLs) * mtMinusMs,
                                                -ltMinusLs * mtMinusMs * mtMinusMs);
        reverseOrientation = ((classification.d1 > 0.0f && result.klmCoordinates[0].x() < 0.0f)
                           || (classification.d1 < 0.0f && result.klmCoordinates[0].x() > 0.0f));
        break;
    }

    case LoopBlinnClassifier::kCusp: {
        float ls = classification.d3;
        float lt = 3.0f * classification.d2;
        float lsMinusLt = ls - lt;
        result.klmCoordinates[0] = FloatPoint3D(ls,
                                                ls * ls * ls,
                                                1.0f);
        result.klmCoordinates[1] = FloatPoint3D(ls - OneThird * lt,
                                                ls * ls * lsMinusLt,
                                                1.0f);
        result.klmCoordinates[2] = FloatPoint3D(ls - TwoThirds * lt,
                                                lsMinusLt * lsMinusLt * ls,
                                                1.0f);
        result.klmCoordinates[3] = FloatPoint3D(lsMinusLt,
                                                lsMinusLt * lsMinusLt * lsMinusLt,
                                                1.0f);
        break;
    }

    case LoopBlinnClassifier::kQuadratic: {
        result.klmCoordinates[0] = FloatPoint3D(0, 0, 0);
        result.klmCoordinates[1] = FloatPoint3D(OneThird, 0, OneThird);
        result.klmCoordinates[2] = FloatPoint3D(TwoThirds, OneThird, TwoThirds);
        result.klmCoordinates[3] = FloatPoint3D(1, 1, 1);
        if (classification.d3 < 0)
            reverseOrientation = true;
        break;
    }

    case LoopBlinnClassifier::kLine:
    case LoopBlinnClassifier::kPoint:
        result.isLineOrPoint = true;
        break;

    default:
        ASSERT_NOT_REACHED();
        break;
    }

    if (sideToFill == LoopBlinnConstants::RightSide)
        reverseOrientation = !reverseOrientation;

    if (reverseOrientation) {
        for (int i = 0; i < 4; ++i) {
            result.klmCoordinates[i].setX(-result.klmCoordinates[i].x());
            result.klmCoordinates[i].setY(-result.klmCoordinates[i].y());
        }
    }

    return result;
}

} // namespace WebCore

#endif
