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

#ifndef LoopBlinnPathCache_h
#define LoopBlinnPathCache_h

#include <wtf/Noncopyable.h>
#include <wtf/Vector.h>

namespace WebCore {

// A cache of the processed triangle mesh for a given path. Because these
// might be expensive to allocate (using malloc/free internally), it is
// recommended to try to reuse them when possible.

// Uncomment the following to obtain debugging information for the edges
// facing the interior region of the mesh.
// #define LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES

class LoopBlinnPathCache {
    WTF_MAKE_NONCOPYABLE(LoopBlinnPathCache);
public:
    LoopBlinnPathCache();
    ~LoopBlinnPathCache();

    unsigned numberOfVertices() const { return m_vertices.size() / 2; }

    // Get the base pointer to the vertex information. There are two
    // coordinates per vertex. This pointer is valid until the cache is
    // cleared or another vertex is added. Returns 0 if there are no
    // vertices in the mesh.
    const float* vertices() const
    {
        if (!numberOfVertices())
            return 0;
        return m_vertices.data();
    }

    // Get the base pointer to the texture coordinate information. There
    // are three coordinates per vertex. This pointer is valid until the
    // cache is cleared or another vertex is added. Returns 0 if
    // there are no vertices in the mesh.
    const float* texcoords() const
    {
        if (!numberOfVertices())
            return 0;
        return m_texcoords.data();
    }

    // Adds a vertex's information to the cache. The first two arguments
    // are the x and y coordinates of the vertex on the plane; the last
    // three arguments are the cubic texture coordinates associated with
    // this vertex.
    void addVertex(float x, float y,
                   float /*k*/, float /*l*/, float /*m*/);

    unsigned numberOfInteriorVertices() const { return m_interiorVertices.size() / 2; }

    // Base pointer to the interior vertices; two coordinates per
    // vertex, which can be drawn as GL_TRIANGLES. Returns 0 if there
    // are no interior vertices in the mesh.
    const float* interiorVertices() const
    {
        if (!numberOfInteriorVertices())
            return 0;
        return m_interiorVertices.data();
    }

    void addInteriorVertex(float x, float y);

    // Clears all of the stored vertex information in this cache.
    void clear();

#ifdef LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES
    // The number of interior edge vertices
    unsigned numberOfInteriorEdgeVertices() const;
    // Base pointer to the interior vertices; two coordinates per
    // vertex, which can be drawn as GL_LINES. Returns 0 if there are
    // no interior edge vertices in the mesh.
    const float* interiorEdgeVertices() const;
    void addInteriorEdgeVertex(float x, float y);
#endif // LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES

private:
    // The two-dimensional vertices of the triangle mesh.
    Vector<float> m_vertices;

    // The three-dimensional cubic texture coordinates.
    Vector<float> m_texcoords;

    Vector<float> m_interiorVertices;

#ifdef LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES
    // The following is only for debugging
    Vector<float> m_interiorEdgeVertices;
#endif // LOOP_BLINN_PATH_CACHE_DEBUG_INTERIOR_EDGES
};

} // namespace WebCore

#endif // LoopBlinnPathCache_h
