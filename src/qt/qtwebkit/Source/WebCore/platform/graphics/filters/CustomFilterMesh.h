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

#ifndef CustomFilterMesh_h
#define CustomFilterMesh_h

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#include "CustomFilterOperation.h"
#include "FloatRect.h"
#include "GraphicsTypes3D.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class GraphicsContext3D;

class CustomFilterMesh : public RefCounted<CustomFilterMesh> {
public:
    static PassRefPtr<CustomFilterMesh> create(GraphicsContext3D* context, unsigned cols, unsigned rows, const FloatRect& meshBox, CustomFilterMeshType meshType)
    {
        return adoptRef(new CustomFilterMesh(context, cols, rows, meshBox, meshType));
    }
    ~CustomFilterMesh();

    Platform3DObject verticesBufferObject() const { return m_verticesBufferObject; }
    unsigned bytesPerVertex() const { return m_bytesPerVertex; }
    
    Platform3DObject elementsBufferObject() const { return m_elementsBufferObject; }
    unsigned indicesCount() const { return m_indicesCount; }
    
    const FloatRect& meshBox() const { return m_meshBox; }
    CustomFilterMeshType meshType() const { return m_meshType; }

private:
    CustomFilterMesh(GraphicsContext3D*, unsigned cols, unsigned rows, const FloatRect& meshBox, CustomFilterMeshType);
    
    GraphicsContext3D* m_context;
    
    Platform3DObject m_verticesBufferObject;
    unsigned m_bytesPerVertex;
    
    Platform3DObject m_elementsBufferObject;
    unsigned m_indicesCount;
    
    FloatRect m_meshBox;
    CustomFilterMeshType m_meshType;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#endif // CustomFilterMesh_h
