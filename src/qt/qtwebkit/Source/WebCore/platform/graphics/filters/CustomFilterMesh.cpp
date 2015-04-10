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
#include "CustomFilterMesh.h"
#include "CustomFilterMeshGenerator.h"
#include "GraphicsContext3D.h"

namespace WebCore {

CustomFilterMesh::CustomFilterMesh(GraphicsContext3D* context, unsigned columns, unsigned rows,
    const FloatRect& meshBox, CustomFilterMeshType meshType)
    : m_context(context)
    , m_verticesBufferObject(0)
    , m_elementsBufferObject(0)
    , m_meshBox(meshBox)
    , m_meshType(meshType)
{
    CustomFilterMeshGenerator generator(columns, rows, meshBox, meshType);
    m_indicesCount = generator.indicesCount();
    m_bytesPerVertex = generator.floatsPerVertex() * sizeof(float);    

    m_context->makeContextCurrent();

    m_verticesBufferObject = m_context->createBuffer();
    m_context->bindBuffer(GraphicsContext3D::ARRAY_BUFFER, m_verticesBufferObject);
    m_context->bufferData(GraphicsContext3D::ARRAY_BUFFER, generator.vertices().size() * sizeof(float), generator.vertices().data(), GraphicsContext3D::STATIC_DRAW);
    
    m_elementsBufferObject = m_context->createBuffer();
    m_context->bindBuffer(GraphicsContext3D::ELEMENT_ARRAY_BUFFER, m_elementsBufferObject);
    m_context->bufferData(GraphicsContext3D::ELEMENT_ARRAY_BUFFER, generator.indices().size() * sizeof(uint16_t), generator.indices().data(), GraphicsContext3D::STATIC_DRAW);
}

CustomFilterMesh::~CustomFilterMesh()
{
    m_context->makeContextCurrent();
    m_context->deleteBuffer(m_verticesBufferObject);
    m_context->deleteBuffer(m_elementsBufferObject);
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

