/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef ANGLEWebKitBridge_h
#define ANGLEWebKitBridge_h

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

#if !PLATFORM(GTK) && !PLATFORM(EFL) && !PLATFORM(BLACKBERRY) && !PLATFORM(QT) && !PLATFORM(WIN)
#include "ANGLE/ShaderLang.h"
#elif PLATFORM(WIN)
#include "GLSLANG/ShaderLang.h"
#else
#include "ShaderLang.h"
#endif

namespace WebCore {

enum ANGLEShaderType {
    SHADER_TYPE_VERTEX = SH_VERTEX_SHADER,
    SHADER_TYPE_FRAGMENT = SH_FRAGMENT_SHADER,
};

enum ANGLEShaderSymbolType {
    SHADER_SYMBOL_TYPE_ATTRIBUTE,
    SHADER_SYMBOL_TYPE_UNIFORM
};

struct ANGLEShaderSymbol {
    ANGLEShaderSymbolType symbolType;
    String name;
    String mappedName;
    ShDataType dataType;
    int size;
    bool isArray;

    bool isSampler() const
    {
        return symbolType == SHADER_SYMBOL_TYPE_UNIFORM
            && (dataType == SH_SAMPLER_2D
            || dataType == SH_SAMPLER_CUBE
            || dataType == SH_SAMPLER_2D_RECT_ARB
            || dataType == SH_SAMPLER_EXTERNAL_OES);
    }
};

class ANGLEWebKitBridge {
public:

    ANGLEWebKitBridge(ShShaderOutput = SH_GLSL_OUTPUT, ShShaderSpec = SH_WEBGL_SPEC);
    ~ANGLEWebKitBridge();
    
    ShBuiltInResources getResources() { return m_resources; }
    void setResources(ShBuiltInResources);
    
    bool compileShaderSource(const char* shaderSource, ANGLEShaderType, String& translatedShaderSource, String& shaderValidationLog, Vector<ANGLEShaderSymbol>& symbols, int extraCompileOptions = 0);

private:

    void cleanupCompilers();

    bool builtCompilers;
    
    ShHandle m_fragmentCompiler;
    ShHandle m_vertexCompiler;

    ShShaderOutput m_shaderOutput;
    ShShaderSpec m_shaderSpec;

    ShBuiltInResources m_resources;
};

} // namespace WebCore

#endif
