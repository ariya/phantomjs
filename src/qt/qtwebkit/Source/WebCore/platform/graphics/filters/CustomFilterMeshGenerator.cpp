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

#include "config.h"

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
#include "CustomFilterMeshGenerator.h"

namespace WebCore {

#ifndef NDEBUG
// Use "call 'WebCore::s_dumpCustomFilterMeshBuffers' = 1" in GDB to activate printing of the mesh buffers.
static bool s_dumpCustomFilterMeshBuffers = false;
#endif

CustomFilterMeshGenerator::CustomFilterMeshGenerator(unsigned columns, unsigned rows, const FloatRect& meshBox, CustomFilterMeshType meshType)
    : m_meshType(meshType)
    , m_points(columns + 1, rows + 1)
    , m_tiles(columns, rows)
    , m_tileSizeInPixels(meshBox.width() / m_tiles.width(), meshBox.height() / m_tiles.height())
    , m_tileSizeInDeviceSpace(1.0f / m_tiles.width(), 1.0f / m_tiles.height())
    , m_meshBox(meshBox)
{
    // Build the two buffers needed to draw triangles:
    // * m_vertices has a number of float attributes that will be passed to the vertex shader
    // for each computed vertex. This number is calculated in floatsPerVertex() based on the meshType.
    // * m_indices is a buffer that will have 3 indices per triangle. Each index will point inside
    // the m_vertices buffer.
    m_vertices.reserveCapacity(verticesCount() * floatsPerVertex());
    m_indices.reserveCapacity(indicesCount());

    // Based on the meshType there can be two types of meshes.
    // * attached: each triangle uses vertices from the neighbor triangles. This is useful to save some GPU memory
    // when there's no need to explode the tiles.
    // * detached: each triangle has its own vertices. This means each triangle can be moved independently and a vec3
    // attribute is passed, so that each vertex can be uniquely identified.
    if (m_meshType == MeshTypeAttached)
        generateAttachedMesh();
    else
        generateDetachedMesh();

#ifndef NDEBUG
    if (s_dumpCustomFilterMeshBuffers)
        dumpBuffers();
#endif
}

void CustomFilterMeshGenerator::addAttachedMeshIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle)
{
    UNUSED_PARAM(triangle);
    m_indices.append((quadY + triangleY) * m_points.width() + (quadX + triangleX));
}

void CustomFilterMeshGenerator::generateAttachedMesh()
{
    for (int j = 0; j < m_points.height(); ++j) {
        for (int i = 0; i < m_points.width(); ++i)
            addAttachedMeshVertexAttributes(i, j);
    }

    for (int j = 0; j < m_tiles.height(); ++j) {
        for (int i = 0; i < m_tiles.width(); ++i)
            addTile<&CustomFilterMeshGenerator::addAttachedMeshIndex>(i, j);
    }
}

void CustomFilterMeshGenerator::addDetachedMeshVertexAndIndex(int quadX, int quadY, int triangleX, int triangleY, int triangle)
{
    addDetachedMeshVertexAttributes(quadX, quadY, triangleX, triangleY, triangle);
    m_indices.append(m_indices.size());
}

void CustomFilterMeshGenerator::generateDetachedMesh()
{
    for (int j = 0; j < m_tiles.height(); ++j) {
        for (int i = 0; i < m_tiles.width(); ++i)
            addTile<&CustomFilterMeshGenerator::addDetachedMeshVertexAndIndex>(i, j);
    }
}

void CustomFilterMeshGenerator::addPositionAttribute(int quadX, int quadY)
{
    // vec4 a_position
    m_vertices.append(m_tileSizeInPixels.width() * quadX - 0.5f + m_meshBox.x());
    m_vertices.append(m_tileSizeInPixels.height() * quadY - 0.5f + m_meshBox.y());
    m_vertices.append(0.0f); // z
    m_vertices.append(1.0f);
}

void CustomFilterMeshGenerator::addTexCoordAttribute(int quadX, int quadY)
{
    // vec2 a_texCoord
    m_vertices.append(m_tileSizeInPixels.width() * quadX + m_meshBox.x());
    m_vertices.append(m_tileSizeInPixels.height() * quadY + m_meshBox.y());
}

void CustomFilterMeshGenerator::addMeshCoordAttribute(int quadX, int quadY)
{
    // vec2 a_meshCoord
    m_vertices.append(m_tileSizeInDeviceSpace.width() * quadX);
    m_vertices.append(m_tileSizeInDeviceSpace.height() * quadY);
}

void CustomFilterMeshGenerator::addTriangleCoordAttribute(int quadX, int quadY, int triangle)
{
    // vec3 a_triangleCoord
    m_vertices.append(quadX);
    m_vertices.append(quadY);
    m_vertices.append(triangle);
}

void CustomFilterMeshGenerator::addAttachedMeshVertexAttributes(int quadX, int quadY)
{
    addPositionAttribute(quadX, quadY);
    addTexCoordAttribute(quadX, quadY);
    addMeshCoordAttribute(quadX, quadY);
}

void CustomFilterMeshGenerator::addDetachedMeshVertexAttributes(int quadX, int quadY, int triangleX, int triangleY, int triangle)
{
    addAttachedMeshVertexAttributes(quadX + triangleX, quadY + triangleY);
    addTriangleCoordAttribute(quadX, quadY, triangle);
}

#ifndef NDEBUG
void CustomFilterMeshGenerator::dumpBuffers() const
{
    printf("Mesh buffers: Points.width(): %d, Points.height(): %d meshBox: %f, %f, %f, %f, type: %s\n",
        m_points.width(), m_points.height(), m_meshBox.x(), m_meshBox.y(), m_meshBox.width(), m_meshBox.height(),
        (m_meshType == MeshTypeAttached) ? "Attached" : "Detached");
    printf("---Vertex:\n\t");
    for (unsigned i = 0; i < m_vertices.size(); ++i) {
        printf("%f ", m_vertices.at(i));
        if (!((i + 1) % floatsPerVertex()))
            printf("\n\t");
    }
    printf("\n---Indices: ");
    for (unsigned i = 0; i < m_indices.size(); ++i)
        printf("%d ", m_indices.at(i));
    printf("\n");
}
#endif

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

