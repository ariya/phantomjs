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

#include "config.h"

#if ENABLE(WEBGL)

#include "ANGLEWebKitBridge.h"
#include <wtf/OwnArrayPtr.h>

namespace WebCore {


ANGLEWebKitBridge::ANGLEWebKitBridge() :
    builtCompilers(false),
    m_fragmentCompiler(0),
    m_vertexCompiler(0)
{
    ShInitialize();
}

ANGLEWebKitBridge::~ANGLEWebKitBridge()
{
    cleanupCompilers();
}

void ANGLEWebKitBridge::cleanupCompilers()
{
    if (m_fragmentCompiler)
        ShDestruct(m_fragmentCompiler);
    m_fragmentCompiler = 0;
    if (m_vertexCompiler)
        ShDestruct(m_vertexCompiler);
    m_vertexCompiler = 0;

    builtCompilers = false;
}
    
void ANGLEWebKitBridge::setResources(ShBuiltInResources resources)
{
    // Resources are (possibly) changing - cleanup compilers if we had them already
    cleanupCompilers();
    
    m_resources = resources;
}

bool ANGLEWebKitBridge::validateShaderSource(const char* shaderSource, ANGLEShaderType shaderType, String& translatedShaderSource, String& shaderValidationLog)
{
    if (!builtCompilers) {
        m_fragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, SH_WEBGL_SPEC, &m_resources);
        m_vertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, SH_WEBGL_SPEC, &m_resources);
        if (!m_fragmentCompiler || !m_vertexCompiler) {
            cleanupCompilers();
            return false;
        }

        builtCompilers = true;
    }
    
    ShHandle compiler;

    if (shaderType == SHADER_TYPE_VERTEX)
        compiler = m_vertexCompiler;
    else
        compiler = m_fragmentCompiler;

    const char* const shaderSourceStrings[] = { shaderSource };

    bool validateSuccess = ShCompile(compiler, shaderSourceStrings, 1, SH_OBJECT_CODE);
    if (!validateSuccess) {
        int logSize = 0;
        ShGetInfo(compiler, SH_INFO_LOG_LENGTH, &logSize);
        if (logSize > 1) {
            OwnArrayPtr<char> logBuffer = adoptArrayPtr(new char[logSize]);
            if (logBuffer) {
                ShGetInfoLog(compiler, logBuffer.get());
                shaderValidationLog = logBuffer.get();
            }
        }
        return false;
    }

    int translationLength = 0;
    ShGetInfo(compiler, SH_OBJECT_CODE_LENGTH, &translationLength);
    if (translationLength > 1) {
        OwnArrayPtr<char> translationBuffer = adoptArrayPtr(new char[translationLength]);
        if (!translationBuffer)
            return false;
        ShGetObjectCode(compiler, translationBuffer.get());
        translatedShaderSource = translationBuffer.get();
    }

    return true;
}

}

#endif // ENABLE(WEBGL)
