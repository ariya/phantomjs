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

#ifndef LoopBlinnLocalTriangulator_h
#define LoopBlinnLocalTriangulator_h

#include "FloatPoint.h"
#include "FloatPoint3D.h"
#include "LoopBlinnConstants.h"
#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>

namespace WebCore {

// Performs a localized triangulation of the triangle mesh
// corresponding to the four control point vertices of a cubic curve
// segment.
class LoopBlinnLocalTriangulator {
    WTF_MAKE_NONCOPYABLE(LoopBlinnLocalTriangulator);
public:
    // The vertices that the triangulator operates upon, containing both
    // the position information as well as the cubic texture
    // coordinates.
    class Vertex {
        WTF_MAKE_NONCOPYABLE(Vertex);
    public:
        Vertex()
        {
            resetFlags();
        }

        const FloatPoint& xyCoordinates() const
        {
            return m_xyCoordinates;
        }

        const FloatPoint3D& klmCoordinates() const
        {
            return m_klmCoordinates;
        }

        // Sets the position and texture coordinates of the vertex.
        void set(float x, float y,
                 float k, float l, float m)
        {
            m_xyCoordinates.set(x, y);
            m_klmCoordinates.set(k, l, m);
        }

        // Flags for walking from the start vertex to the end vertex.
        bool end()
        {
            return m_end;
        }

        void setEnd(bool end)
        {
            m_end = end;
        }

        bool marked()
        {
            return m_marked;
        }

        void setMarked(bool marked)
        {
            m_marked = marked;
        }

        bool interior()
        {
            return m_interior;
        }

        void setInterior(bool interior)
        {
            m_interior = interior;
        }

        void resetFlags()
        {
            m_end = false;
            m_marked = false;
            m_interior = false;
        }

    private:
        // 2D coordinates of the vertex in the plane.
        FloatPoint m_xyCoordinates;
        // Cubic texture coordinates for rendering the curve.
        FloatPoint3D m_klmCoordinates;

        // Flags for walking from the start vertex to the end vertex.
        bool m_end;
        bool m_marked;
        bool m_interior;
    };

    // The triangles the Triangulator produces.
    class Triangle {
    public:
        Triangle()
        {
            m_vertices[0] = 0;
            m_vertices[1] = 0;
            m_vertices[2] = 0;
        }

        // Gets the vertex at the given index, 0 <= index < 3.
        Vertex* getVertex(int index)
        {
            ASSERT(index >= 0 && index < 3);
            return m_vertices[index];
        }

        // Returns true if this triangle contains the given vertex (by
        // identity, not geometrically).
        bool contains(Vertex* v);

        // Returns the vertex following the current one in the specified
        // direction, counterclockwise or clockwise.
        Vertex* nextVertex(Vertex* current, bool traverseCounterClockwise);

        // Sets the vertices of this triangle, potentially reordering them
        // to produce a canonical orientation.
        void setVertices(Vertex* v0,
                         Vertex* v1,
                         Vertex* v2)
        {
            m_vertices[0] = v0;
            m_vertices[1] = v1;
            m_vertices[2] = v2;
            makeCounterClockwise();
        }

    private:
        // Returns the index [0..2] associated with the given vertex, or
        // -1 if not found.
        int indexForVertex(Vertex* vertex);

        // Reorders the vertices in this triangle to make them
        // counterclockwise when viewed in the 2D plane, in order to
        // achieve a canonical ordering.
        void makeCounterClockwise();

        // Note: these are raw pointers because they point to the
        // m_vertices contained in the surrounding triangulator.
        Vertex* m_vertices[3];
    };

    LoopBlinnLocalTriangulator();

    // Resets the triangulator's state. After each triangulation and
    // before the next, call this to re-initialize the internal
    // vertices' state.
    void reset();

    // Returns a mutable vertex stored in the triangulator. Use this to
    // set up the vertices before a triangulation.
    Vertex* getVertex(int index)
    {
        ASSERT(index >= 0 && index < 4);
        return &m_vertices[index];
    }

    enum InsideEdgeComputation {
        ComputeInsideEdges,
        DontComputeInsideEdges
    };

    // Once the vertices' contents have been set up, call triangulate()
    // to recompute the triangles.
    //
    // If computeInsideEdges is ComputeInsideEdges, then sideToFill
    // will be used to determine which side of the cubic curve defined
    // by the four control points is to be filled.
    //
    // The triangulation obeys the following guarantees:
    //   - If the convex hull is a quadrilateral, then the shortest edge
    //     will be chosen for the cut into two triangles.
    //   - If one of the vertices is contained in the triangle spanned
    //     by the other three, three triangles will be produced.
    void triangulate(InsideEdgeComputation computeInsideEdges,
                     LoopBlinnConstants::FillSide sideToFill);

    // Number of triangles computed by triangulate().
    int numberOfTriangles() const
    {
        return m_numberOfTriangles;
    }

    // Returns the computed triangle at index, 0 <= index < numberOfTriangles().
    Triangle* getTriangle(int index)
    {
        ASSERT(index >= 0 && index < m_numberOfTriangles);
        return &m_triangles[index];
    }

    // Number of vertices facing the inside of the shape, if
    // ComputeInsideEdges was passed when triangulate() was called.
    int numberOfInteriorVertices() const
    {
        return m_numberOfInteriorVertices;
    }

    // Fetches the given interior vertex, 0 <= index < numberOfInteriorVertices().
    Vertex* getInteriorVertex(int index)
    {
        ASSERT(index >= 0 && index < m_numberOfInteriorVertices);
        return m_interiorVertices[index];
    }

private:
    void triangulateHelper(LoopBlinnConstants::FillSide sideToFill);

    // Adds a triangle to the triangulation.
    void addTriangle(Vertex* v0, Vertex* v1, Vertex* v2);

    // Adds a vertex to the list of interior vertices.
    void addInteriorVertex(Vertex* v);

    // Indicates whether the edge between vertex v0 and v1 is shared
    // between two or more triangles.
    bool isSharedEdge(Vertex* v0, Vertex* v1);

    // The vertices being triangulated.
    Vertex m_vertices[4];

    // The vertices corresponding to the edges facing the inside of the
    // shape, in order from the start vertex to the end vertex. The more
    // general triangulation algorithm tessellates this interior region.
    Vertex* m_interiorVertices[4];
    // The number of interior vertices that are valid for the current
    // triangulation.
    int m_numberOfInteriorVertices;

    // There can be at most three triangles computed by this local
    // algorithm, which occurs when one of the vertices is contained in
    // the triangle spanned by the other three. Most of the time the
    // algorithm computes two triangles.
    Triangle m_triangles[3];
    int m_numberOfTriangles;
};

} // namespace WebCore

#endif // LoopBlinnLocalTriangulator_h
