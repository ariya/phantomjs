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

#ifndef CustomFilterValidatedProgram_h
#define CustomFilterValidatedProgram_h

#if ENABLE(CSS_SHADERS)

#include "CustomFilterProgramInfo.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

// PlatformCompiledProgram defines a type that is compatible with the framework used to implement accelerated compositing on a particular platform.
#if PLATFORM(BLACKBERRY)
namespace WebCore {
class LayerCompiledProgram;
}
typedef WebCore::LayerCompiledProgram PlatformCompiledProgram;
#endif

namespace WebCore {

struct ANGLEShaderSymbol;
class CustomFilterCompiledProgram;
class CustomFilterGlobalContext;

#if USE(TEXTURE_MAPPER)
class TextureMapperPlatformCompiledProgram;
typedef TextureMapperPlatformCompiledProgram PlatformCompiledProgram;
#endif

//
// A unique combination of vertex shader and fragment shader is only validated and compiled once.
// All shaders are validated through ANGLE in CustomFilterValidatedProgram before being compiled by the GraphicsContext3D in CustomFilterCompiledProgram.
// For shaders that use the CSS mix function, CustomFilterValidatedProgram adds shader code to perform DOM texture access, blending, and compositing.
//
class CustomFilterValidatedProgram : public RefCounted<CustomFilterValidatedProgram> {
public:
    static PassRefPtr<CustomFilterValidatedProgram> create(CustomFilterGlobalContext* globalContext, const CustomFilterProgramInfo& programInfo)
    {
        return adoptRef(new CustomFilterValidatedProgram(globalContext, programInfo));
    }

    ~CustomFilterValidatedProgram();

    const CustomFilterProgramInfo& programInfo() const { return m_programInfo; }
    CustomFilterProgramInfo validatedProgramInfo() const;
    
    PassRefPtr<CustomFilterCompiledProgram> compiledProgram();
    void setCompiledProgram(PassRefPtr<CustomFilterCompiledProgram>);

    const String& validatedVertexShader() const 
    {
        ASSERT(m_isInitialized); 
        return m_validatedVertexShader; 
    }

    const String& validatedFragmentShader() const 
    { 
        ASSERT(m_isInitialized); 
        return m_validatedFragmentShader; 
    }

#if PLATFORM(BLACKBERRY) || USE(TEXTURE_MAPPER)
    PlatformCompiledProgram* platformCompiledProgram();
#endif

    bool isInitialized() const { return m_isInitialized; }
private:
    CustomFilterValidatedProgram(CustomFilterGlobalContext*, const CustomFilterProgramInfo&);

    void platformInit();
    void platformDestroy();

    static String defaultVertexShaderString();
    static String defaultFragmentShaderString();

    static String blendFunctionString(BlendMode);
    static String compositeFunctionString(CompositeOperator);

    void rewriteMixVertexShader(const Vector<ANGLEShaderSymbol>& symbols);
    void rewriteMixFragmentShader();

    bool needsInputTexture() const;

    CustomFilterProgramInfo m_programInfo;

    String m_validatedVertexShader;
    String m_validatedFragmentShader;

    RefPtr<CustomFilterCompiledProgram> m_compiledProgram;
#if PLATFORM(BLACKBERRY) || USE(TEXTURE_MAPPER)
    PlatformCompiledProgram* m_platformCompiledProgram;
#endif

    bool m_isInitialized;
};

}

#endif // ENABLE(CSS_SHADERS)

#endif
