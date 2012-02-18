/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

// The main entry point for Loop and Blinn's GPU accelerated curve
// rendering algorithm.

#ifndef LoopBlinnPathProcessor_h
#define LoopBlinnPathProcessor_h

#include <wtf/Noncopyable.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

// We use a namespace for classes which are simply implementation
// details of the algorithm but which we need to reference from the
// class definition.
namespace LoopBlinnPathProcessorImplementation {

class Contour;
class Segment;

} // namespace LoopBlinnPathProcessorImplementation

class Path;
class LoopBlinnPathCache;
class PODArena;

// The LoopBlinnPathProcessor turns a Path (assumed to contain one or
// more closed regions) into a set of exterior and interior triangles,
// stored in the LoopBlinnPathCache. The exterior triangles have
// associated 3D texture coordinates which are used to evaluate the
// curve's inside/outside function on a per-pixel basis. The interior
// triangles are filled with 100% opacity.
//
// Note that the fill style and management of multiple layers are
// separate concerns, handled at a higher level with shaders and
// polygon offsets.
class LoopBlinnPathProcessor {
public:
    LoopBlinnPathProcessor();
    explicit LoopBlinnPathProcessor(PassRefPtr<PODArena>);
    ~LoopBlinnPathProcessor();

    // Transforms the given path into a triangle mesh for rendering
    // using Loop and Blinn's shader, placing the result into the given
    // LoopBlinnPathCache.
    void process(const Path&, LoopBlinnPathCache&);

#ifndef NDEBUG
    // Enables or disables verbose logging in debug mode.
    void setVerboseLogging(bool onOrOff);
#endif

private:
    // Builds a list of contours for the given path.
    void buildContours(const Path&);

    // Determines whether the left or right side of each contour should
    // be filled.
    void determineSidesToFill();

    // Determines whether the given (closed) contour is oriented
    // clockwise or counterclockwise.
    void determineOrientation(LoopBlinnPathProcessorImplementation::Contour*);

    // Subdivides the curves so that there are no overlaps of the
    // triangles associated with the curves' control points.
    void subdivideCurves();

    // Helper function used during curve subdivision.
    void conditionallySubdivide(LoopBlinnPathProcessorImplementation::Segment*,
                                Vector<LoopBlinnPathProcessorImplementation::Segment*>& nextSegments);

    // Tessellates the interior regions of the contours.
    void tessellateInterior(LoopBlinnPathCache&);

#ifndef NDEBUG
    // For debugging the orientation computation. Returns all of the
    // segments overlapping the given Y coordinate.
    Vector<LoopBlinnPathProcessorImplementation::Segment*> allSegmentsOverlappingY(LoopBlinnPathProcessorImplementation::Contour*, float x, float y);

    // For debugging the curve subdivision algorithm. Subdivides the
    // curves using an alternate, slow (O(n^3)) algorithm.
    void subdivideCurvesSlow();
#endif

    // PODArena from which to allocate temporary objects.
    RefPtr<PODArena> m_arena;

    // The contours described by the path.
    Vector<LoopBlinnPathProcessorImplementation::Contour*> m_contours;

#ifndef NDEBUG
    // Whether or not to perform verbose logging in debug mode.
    bool m_verboseLogging;
#endif
};

} // namespace WebCore

#endif // LoopBlinnPathProcessor_h
