/*
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef CustomFilterMeshGenerator_h
#define CustomFilterMeshGenerator_h

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#include "CustomFilterConstants.h"
#include "CustomFilterOperation.h"
#include "FloatRect.h"
#include <stdio.h>

namespace WebCore {

class CustomFilterMeshGenerator {
public:
    // Lines and columns are the values passed in CSS. The result is vertex mesh that has 'rows' numbers of rows
    // and 'columns' number of columns with a total of 'rows + 1' * 'columns + 1' vertices.
    // MeshBox is the filtered area calculated defined using the border-box, padding-box, content-box or filter-box
    // attributes. A value of (0, 0, 1, 1) will cover the entire output surface.
    CustomFilterMeshGenerator(unsigned columns, unsigned rows, const FloatRect& meshBox, CustomFilterMeshType);

    const Vector<float>& vertices() const { return m_vertices; }
    const Vector<uint16_t>& indices() const { return m_indices; }

    const IntSize& points() const { return m_points; }
    unsigned pointsCount() const { return m_points.width() * m_points.height(); }

    const IntSize& tiles() const { return m_tiles; }
    unsigned tilesCount() const { return m_tiles.width() * m_tiles.height(); }

    unsigned indicesCount() const
    {
        const unsigned trianglesPerTile = 2;
        const unsigned indicesPerTriangle = 3;
        return tilesCount() * trianglesPerTile * indicesPerTriangle;
    }

    unsigned floatsPerVertex() const
    {
        static const unsigned AttachedMeshVertexSize = PositionAttribSize + TexAttribSize + MeshAttribSize;
        static const unsigned DetachedMeshVertexSize = AttachedMeshVertexSize + TriangleAttribSize;
        return m_meshType == MeshTypeAttached ? AttachedMeshVertexSize : DetachedMeshVertexSize;
    }

    unsigned verticesCount() const
    {
        return m_meshType == MeshTypeAttached ? pointsCount() : indicesCount();
    }

private:
    typedef void (CustomFilterMeshGenerator::*AddTriangleVertexFunction)(int quadX, int quadY, int triangleX, int triangleY, int triangle);

    template <AddTriangleVertexFunction addTriangleVertex>
    void addTile(int quadX, int quadY)
    {
        ((*this).*(addTriangleVertex))(quadX, quadY, 0, 0, 1);
        ((*this).*(addTriangleVertex))(quadX, quadY, 1, 0, 2);
        ((*this).*(addTriangleVertex))(quadX, quadY, 1, 1, 3);
        ((*this).*(addTriangleVertex))(quadX, quadY, 0, 0, 4);
        ((*this).*(addTriangleVertex))(quadX, quadY, 1, 1, 5);
        ((*this).*(addTriangleVertex))(quadX, quadY, 0, 1, 6);
    }

    void addAttachedMeshIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle);

    void generateAttachedMesh();

    void addDetachedMeshVertexAndIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle);

    void generateDetachedMesh();
    void addPositionAttribute(int quadX, int quadY);
    void addTexCoordAttribute(int quadX, int quadY);
    void addMeshCoordAttribute(int quadX, int quadY);
    void addTriangleCoordAttribute(int quadX, int quadY, int triangle);
    void addAttachedMeshVertexAttributes(int quadX, int quadY);
    void addDetachedMeshVertexAttributes(int quadX, int quadY, int triangleX, int triangleY, int triangle);

#ifndef NDEBUG
    void dumpBuffers() const;
#endif

private:
    Vector<float> m_vertices;
    Vector<uint16_t> m_indices;

    CustomFilterMeshType m_meshType;
    IntSize m_points;
    IntSize m_tiles;
    FloatSize m_tileSizeInPixels;
    FloatSize m_tileSizeInDeviceSpace;
    FloatRect m_meshBox;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#endif // CustomFilterMeshGenerator_h
