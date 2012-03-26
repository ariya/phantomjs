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

#include "LoopBlinnLocalTriangulator.h"

#include "LoopBlinnMathUtils.h"
#include <algorithm>

namespace WebCore {

using LoopBlinnMathUtils::approxEqual;
using LoopBlinnMathUtils::linesIntersect;
using LoopBlinnMathUtils::pointInTriangle;

bool LoopBlinnLocalTriangulator::Triangle::contains(LoopBlinnLocalTriangulator::Vertex* v)
{
    return indexForVertex(v) >= 0;
}

LoopBlinnLocalTriangulator::Vertex* LoopBlinnLocalTriangulator::Triangle::nextVertex(LoopBlinnLocalTriangulator::Vertex* current, bool traverseCounterClockwise)
{
    int index = indexForVertex(current);
    ASSERT(index >= 0);
    if (traverseCounterClockwise)
        ++index;
    else
        --index;
    if (index < 0)
        index += 3;
    else
        index = index % 3;
    return m_vertices[index];
}

int LoopBlinnLocalTriangulator::Triangle::indexForVertex(LoopBlinnLocalTriangulator::Vertex* vertex)
{
    for (int i = 0; i < 3; ++i)
        if (m_vertices[i] == vertex)
            return i;
    return -1;
}

void LoopBlinnLocalTriangulator::Triangle::makeCounterClockwise()
{
    // Possibly swaps two vertices so that the triangle's vertices are
    // always specified in counterclockwise order. This orders the
    // vertices canonically when walking the interior edges from the
    // start to the end vertex.
    FloatPoint3D point0(m_vertices[0]->xyCoordinates());
    FloatPoint3D point1(m_vertices[1]->xyCoordinates());
    FloatPoint3D point2(m_vertices[2]->xyCoordinates());
    FloatPoint3D crossProduct = (point1 - point0).cross(point2 - point0);
    if (crossProduct.z() < 0)
        std::swap(m_vertices[1], m_vertices[2]);
}

LoopBlinnLocalTriangulator::LoopBlinnLocalTriangulator()
{
    reset();
}

void LoopBlinnLocalTriangulator::reset()
{
    m_numberOfTriangles = 0;
    m_numberOfInteriorVertices = 0;
    for (int i = 0; i < 4; ++i) {
        m_interiorVertices[i] = 0;
        m_vertices[i].resetFlags();
    }
}

void LoopBlinnLocalTriangulator::triangulate(InsideEdgeComputation computeInsideEdges, LoopBlinnConstants::FillSide sideToFill)
{
    triangulateHelper(sideToFill);

    if (computeInsideEdges == ComputeInsideEdges) {
        // We need to compute which vertices describe the path along the
        // interior portion of the shape, to feed these vertices to the
        // more general tessellation algorithm. It is possible that we
        // could determine this directly while producing triangles above.
        // Here we try to do it generally just by examining the triangles
        // that have already been produced. We walk around them in a
        // specific direction determined by which side of the curve is
        // being filled. We ignore the interior vertex unless it is also
        // the ending vertex, and skip the edges shared between two
        // triangles.
        Vertex* v = &m_vertices[0];
        addInteriorVertex(v);
        int numSteps = 0;
        while (!v->end() && numSteps < 4) {
            // Find the next vertex according to the above rules
            bool gotNext = false;
            for (int i = 0; i < numberOfTriangles() && !gotNext; ++i) {
                Triangle* tri = getTriangle(i);
                if (tri->contains(v)) {
                    Vertex* next = tri->nextVertex(v, sideToFill == LoopBlinnConstants::RightSide);
                    if (!next->marked() && !isSharedEdge(v, next) && (!next->interior() || next->end())) {
                        addInteriorVertex(next);
                        v = next;
                        // Break out of for loop
                        gotNext = true;
                    }
                }
            }
            ++numSteps;
        }
        if (!v->end()) {
            // Something went wrong with the above algorithm; add the last
            // vertex to the interior vertices anyway. (FIXME: should we
            // add an assert here and do more extensive testing?)
            addInteriorVertex(&m_vertices[3]);
        }
    }
}

void LoopBlinnLocalTriangulator::triangulateHelper(LoopBlinnConstants::FillSide sideToFill)
{
    reset();

    m_vertices[3].setEnd(true);

    // First test for degenerate cases.
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            if (approxEqual(m_vertices[i].xyCoordinates(), m_vertices[j].xyCoordinates())) {
                // Two of the vertices are coincident, so we can eliminate at
                // least one triangle. We might be able to eliminate the other
                // as well, but this seems sufficient to avoid degenerate
                // triangulations.
                int indices[3] = { 0 };
                int index = 0;
                for (int k = 0; k < 4; ++k)
                    if (k != j)
                        indices[index++] = k;
                addTriangle(&m_vertices[indices[0]],
                            &m_vertices[indices[1]],
                            &m_vertices[indices[2]]);
                return;
            }
        }
    }

    // See whether any of the points are fully contained in the
    // triangle defined by the other three.
    for (int i = 0; i < 4; ++i) {
        int indices[3] = { 0 };
        int index = 0;
        for (int j = 0; j < 4; ++j)
            if (i != j)
                indices[index++] = j;
        if (pointInTriangle(m_vertices[i].xyCoordinates(),
                            m_vertices[indices[0]].xyCoordinates(),
                            m_vertices[indices[1]].xyCoordinates(),
                            m_vertices[indices[2]].xyCoordinates())) {
            // Produce three triangles surrounding this interior vertex.
            for (int j = 0; j < 3; ++j)
                addTriangle(&m_vertices[indices[j % 3]],
                            &m_vertices[indices[(j + 1) % 3]],
                            &m_vertices[i]);
            // Mark the interior vertex so we ignore it if trying to trace
            // the interior edge.
            m_vertices[i].setInterior(true);
            return;
        }
    }

    // There are only a few permutations of the vertices, ignoring
    // rotations, which are irrelevant:
    //
    //  0--3  0--2  0--3  0--1  0--2  0--1
    //  |  |  |  |  |  |  |  |  |  |  |  |
    //  |  |  |  |  |  |  |  |  |  |  |  |
    //  1--2  1--3  2--1  2--3  3--1  3--2
    //
    // Note that three of these are reflections of each other.
    // Therefore there are only three possible triangulations:
    //
    //  0--3  0--2  0--3
    //  |\ |  |\ |  |\ |
    //  | \|  | \|  | \|
    //  1--2  1--3  2--1
    //
    // From which we can choose by seeing which of the potential
    // diagonals intersect. Note that we choose the shortest diagonal
    // to split the quad.
    if (linesIntersect(m_vertices[0].xyCoordinates(),
                       m_vertices[2].xyCoordinates(),
                       m_vertices[1].xyCoordinates(),
                       m_vertices[3].xyCoordinates())) {
        if ((m_vertices[2].xyCoordinates() - m_vertices[0].xyCoordinates()).diagonalLengthSquared() <
            (m_vertices[3].xyCoordinates() - m_vertices[1].xyCoordinates()).diagonalLengthSquared()) {
            addTriangle(&m_vertices[0], &m_vertices[1], &m_vertices[2]);
            addTriangle(&m_vertices[0], &m_vertices[2], &m_vertices[3]);
        } else {
            addTriangle(&m_vertices[0], &m_vertices[1], &m_vertices[3]);
            addTriangle(&m_vertices[1], &m_vertices[2], &m_vertices[3]);
        }
    } else if (linesIntersect(m_vertices[0].xyCoordinates(),
                              m_vertices[3].xyCoordinates(),
                              m_vertices[1].xyCoordinates(),
                              m_vertices[2].xyCoordinates())) {
        if ((m_vertices[3].xyCoordinates() - m_vertices[0].xyCoordinates()).diagonalLengthSquared() <
            (m_vertices[2].xyCoordinates() - m_vertices[1].xyCoordinates()).diagonalLengthSquared()) {
            addTriangle(&m_vertices[0], &m_vertices[1], &m_vertices[3]);
            addTriangle(&m_vertices[0], &m_vertices[3], &m_vertices[2]);
        } else {
            addTriangle(&m_vertices[0], &m_vertices[1], &m_vertices[2]);
            addTriangle(&m_vertices[2], &m_vertices[1], &m_vertices[3]);
        }
    } else {
        // Lines (0->1), (2->3) intersect -- or should, modulo numerical
        // precision issues
        if ((m_vertices[1].xyCoordinates() - m_vertices[0].xyCoordinates()).diagonalLengthSquared() <
            (m_vertices[3].xyCoordinates() - m_vertices[2].xyCoordinates()).diagonalLengthSquared()) {
            addTriangle(&m_vertices[0], &m_vertices[2], &m_vertices[1]);
            addTriangle(&m_vertices[0], &m_vertices[1], &m_vertices[3]);
        } else {
            addTriangle(&m_vertices[0], &m_vertices[2], &m_vertices[3]);
            addTriangle(&m_vertices[3], &m_vertices[2], &m_vertices[1]);
        }
    }
}

void LoopBlinnLocalTriangulator::addTriangle(Vertex* v0, Vertex* v1, Vertex* v2)
{
    ASSERT(m_numberOfTriangles < 3);
    m_triangles[m_numberOfTriangles++].setVertices(v0, v1, v2);
}

void LoopBlinnLocalTriangulator::addInteriorVertex(Vertex* v)
{
    ASSERT(m_numberOfInteriorVertices < 4);
    m_interiorVertices[m_numberOfInteriorVertices++] = v;
    v->setMarked(true);
}

bool LoopBlinnLocalTriangulator::isSharedEdge(Vertex* v0, Vertex* v1)
{
    bool haveEdge01 = false;
    bool haveEdge10 = false;
    for (int i = 0; i < numberOfTriangles(); ++i) {
        Triangle* tri = getTriangle(i);
        if (tri->contains(v0) && tri->nextVertex(v0, true) == v1)
            haveEdge01 = true;
        if (tri->contains(v1) && tri->nextVertex(v1, true) == v0)
            haveEdge10 = true;
    }
    return haveEdge01 && haveEdge10;
}

} // namespace WebCore

#endif
