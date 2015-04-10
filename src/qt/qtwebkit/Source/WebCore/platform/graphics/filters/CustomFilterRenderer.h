/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2012 Company 100, Inc. All rights reserved.
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

#ifndef CustomFilterRenderer_h
#define CustomFilterRenderer_h

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#include "CustomFilterConstants.h"
#include "CustomFilterParameterList.h"
#include "GraphicsTypes3D.h"
#include "IntSize.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class CustomFilterArrayParameter;
class CustomFilterColorParameter;
class CustomFilterCompiledProgram;
class CustomFilterMesh;
class CustomFilterNumberParameter;
class CustomFilterTransformParameter;
class GraphicsContext3D;

class CustomFilterRenderer : public RefCounted<CustomFilterRenderer> {
public:
    static PassRefPtr<CustomFilterRenderer> create(PassRefPtr<GraphicsContext3D>, CustomFilterProgramType, const CustomFilterParameterList&,
        unsigned meshRows, unsigned meshColumns, CustomFilterMeshType);
    ~CustomFilterRenderer();

    bool premultipliedAlpha() const;
    bool programNeedsInputTexture() const;

    bool prepareForDrawing();

    void draw(Platform3DObject, const IntSize&);

    CustomFilterCompiledProgram* compiledProgram() const { return m_compiledProgram.get(); }
    void setCompiledProgram(PassRefPtr<CustomFilterCompiledProgram>);

private:
    CustomFilterRenderer(PassRefPtr<GraphicsContext3D>, CustomFilterProgramType, const CustomFilterParameterList&,
        unsigned meshRows, unsigned meshColumns, CustomFilterMeshType);

    void initializeCompiledProgramIfNeeded();
    void initializeMeshIfNeeded();

    void bindVertexAttribute(int attributeLocation, unsigned size, unsigned offset);
    void unbindVertexAttribute(int attributeLocation);
    void bindProgramArrayParameters(int uniformLocation, CustomFilterArrayParameter*);
    void bindProgramColorParameters(int uniformLocation, CustomFilterColorParameter*);
    void bindProgramMatrixParameters(int uniformLocation, CustomFilterArrayParameter*);
    void bindProgramNumberParameters(int uniformLocation, CustomFilterNumberParameter*);
    void bindProgramTransformParameter(int uniformLocation, CustomFilterTransformParameter*);
    void bindProgramParameters();
    void bindProgramAndBuffers(Platform3DObject inputTexture);
    void unbindVertexAttributes();

    RefPtr<GraphicsContext3D> m_context;
    RefPtr<CustomFilterCompiledProgram> m_compiledProgram;
    CustomFilterProgramType m_programType;
    RefPtr<CustomFilterMesh> m_mesh;
    IntSize m_contextSize;

    CustomFilterParameterList m_parameters;

    unsigned m_meshRows;
    unsigned m_meshColumns;
    CustomFilterMeshType m_meshType;
};

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#endif // CustomFilterRenderer_h
