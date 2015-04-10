/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
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

#ifndef CustomFilterCompiledProgram_h
#define CustomFilterCompiledProgram_h

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#include "CustomFilterProgramInfo.h"
#include "GraphicsContext3D.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class CustomFilterGlobalContext;

class CustomFilterCompiledProgram: public RefCounted<CustomFilterCompiledProgram> {
public:
    static PassRefPtr<CustomFilterCompiledProgram> create(PassRefPtr<GraphicsContext3D> context, const String& validatedVertexShader, const String& validatedFragmentShader, CustomFilterProgramType programType)
    {
        return adoptRef(new CustomFilterCompiledProgram(context, validatedVertexShader, validatedFragmentShader, programType));
    }
    
    ~CustomFilterCompiledProgram();
    
    int positionAttribLocation() const { return m_positionAttribLocation; }
    int texAttribLocation() const { return m_texAttribLocation; }
    int meshAttribLocation() const { return m_meshAttribLocation; }
    int triangleAttribLocation() const { return m_triangleAttribLocation; }
    int meshBoxLocation() const { return m_meshBoxLocation; }
    int projectionMatrixLocation() const { return m_projectionMatrixLocation; }
    int tileSizeLocation() const { return m_tileSizeLocation; }
    int meshSizeLocation() const { return m_meshSizeLocation; }
    int samplerLocation() const { return m_samplerLocation; }
    int contentSamplerLocation() const { return m_contentSamplerLocation; }
    int samplerSizeLocation() const { return m_samplerSizeLocation; }

    int uniformLocationByName(const String&);
    
    bool isInitialized() const { return m_isInitialized; }
    
    Platform3DObject program() const { return m_program; }
private:
    CustomFilterCompiledProgram(PassRefPtr<GraphicsContext3D>, const String& validatedVertexShader, const String& validatedFragmentShader, CustomFilterProgramType);
    
    Platform3DObject compileShader(GC3Denum shaderType, const String& shaderString);
    Platform3DObject linkProgram(Platform3DObject vertexShader, Platform3DObject fragmentShader);
    void initializeParameterLocations(CustomFilterProgramType);
    
    RefPtr<GraphicsContext3D> m_context;
    Platform3DObject m_program;
    
    int m_positionAttribLocation;
    int m_texAttribLocation;
    int m_meshAttribLocation;
    int m_triangleAttribLocation;
    int m_meshBoxLocation;
    int m_projectionMatrixLocation;
    int m_tileSizeLocation;
    int m_meshSizeLocation;
    int m_samplerLocation;
    int m_samplerSizeLocation;
    int m_contentSamplerLocation;
    
    bool m_isInitialized;
};

}

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)

#endif
