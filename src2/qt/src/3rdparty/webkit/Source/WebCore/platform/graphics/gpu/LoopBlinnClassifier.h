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

// Cubic curve classification algorithm from "Rendering Vector Art on
// the GPU" by Loop and Blinn, GPU Gems 3, Chapter 25:
// http://http.developer.nvidia.com/GPUGems3/gpugems3_ch25.html .

#ifndef LoopBlinnClassifier_h
#define LoopBlinnClassifier_h

#include <wtf/Noncopyable.h>

namespace WebCore {

class FloatPoint;

// Classifies cubic curves into specific types.
class LoopBlinnClassifier {
    WTF_MAKE_NONCOPYABLE(LoopBlinnClassifier);
public:
    // The types of cubic curves.
    enum CurveType {
        kSerpentine,
        kCusp,
        kLoop,
        kQuadratic,
        kLine,
        kPoint
    };

    // The result of the classifier.
    struct Result {
    public:
        Result(CurveType inputCurveType, float inputD1, float inputD2, float inputD3)
            : curveType(inputCurveType)
            , d1(inputD1)
            , d2(inputD2)
            , d3(inputD3) { }

        CurveType curveType;

        // These are coefficients used later in the computation of
        // texture coordinates per vertex.
        float d1;
        float d2;
        float d3;
    };

    // Classifies the given cubic bezier curve starting at c0, ending
    // at c3, and affected by control points c1 and c2.
    static Result classify(const FloatPoint& c0,
                           const FloatPoint& c1,
                           const FloatPoint& c2,
                           const FloatPoint& c3);

private:
    // This class does not need to be instantiated.
    LoopBlinnClassifier() { }
};

} // namespace WebCore

#endif // LoopBlinnClassifier_h
