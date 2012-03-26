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

#ifndef LoopBlinnTextureCoords_h
#define LoopBlinnTextureCoords_h

#include "FloatPoint3D.h"
#include "LoopBlinnClassifier.h"
#include "LoopBlinnConstants.h"

#include <wtf/Noncopyable.h>

namespace WebCore {

// Computes three-dimensional texture coordinates for the control
// points of a cubic curve for rendering via the shader in "Rendering
// Vector Art on the GPU" by Loop and Blinn, GPU Gems 3, Chapter 25.
class LoopBlinnTextureCoords {
public:
    // Container for the cubic texture coordinates and other associated
    // information.
    struct Result {
        Result()
            : isLineOrPoint(false)
            , hasRenderingArtifact(false)
            , subdivisionParameterValue(0.0f) { }

        // The (k, l, m) texture coordinates that are to be associated
        // with the four control points of the cubic curve.
        FloatPoint3D klmCoordinates[4];

        // Indicates whether the curve is a line or a point, in which case
        // we do not need to add its triangles to the mesh.
        bool isLineOrPoint;

        // For the loop case, indicates whether a rendering artifact was
        // detected, in which case the curve needs to be further
        // subdivided.
        bool hasRenderingArtifact;

        // If a rendering artifact will occur for the given loop curve,
        // this is the parameter value (0 <= value <= 1) at which the
        // curve needs to be subdivided to fix the artifact.
        float subdivisionParameterValue;
    };

    // Computes the texture coordinates for a cubic curve segment's
    // control points, given the classification of the curve as well as
    // an indication of which side is to be filled.
    static Result compute(const LoopBlinnClassifier::Result& classification,
                          LoopBlinnConstants::FillSide sideToFill);

private:
    // This class does not need to be instantiated.
    LoopBlinnTextureCoords() { }
};

} // namespace WebCore

#endif // LoopBlinnTextureCoords_h
