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

#if USE(3D_GRAPHICS)

#include "ANGLEWebKitBridge.h"
#include <wtf/OwnArrayPtr.h>

namespace WebCore {

// Temporary typedef to support an incompatible change in the ANGLE API.
#if !defined(ANGLE_SH_VERSION) || ANGLE_SH_VERSION < 108
typedef int ANGLEGetInfoType;
#else
typedef size_t ANGLEGetInfoType;
#endif

inline static ANGLEGetInfoType getValidationResultValue(const ShHandle compiler, ShShaderInfo shaderInfo)
{
    ANGLEGetInfoType value = 0;
    ShGetInfo(compiler, shaderInfo, &value);
    return value;
}

static bool getSymbolInfo(ShHandle compiler, ShShaderInfo symbolType, Vector<ANGLEShaderSymbol>& symbols)
{
    ShShaderInfo symbolMaxNameLengthType;

    switch (symbolType) {
    case SH_ACTIVE_ATTRIBUTES:
        symbolMaxNameLengthType = SH_ACTIVE_ATTRIBUTE_MAX_LENGTH;
        break;
    case SH_ACTIVE_UNIFORMS:
        symbolMaxNameLengthType = SH_ACTIVE_UNIFORM_MAX_LENGTH;
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    ANGLEGetInfoType numSymbols = getValidationResultValue(compiler, symbolType);

    ANGLEGetInfoType maxNameLength = getValidationResultValue(compiler, symbolMaxNameLengthType);
    if (maxNameLength <= 1)
        return false;

    ANGLEGetInfoType maxMappedNameLength = getValidationResultValue(compiler, SH_MAPPED_NAME_MAX_LENGTH);
    if (maxMappedNameLength <= 1)
        return false;

    // The maximum allowed symbol name length is 256 characters.
    Vector<char, 256> nameBuffer(maxNameLength);
    Vector<char, 256> mappedNameBuffer(maxMappedNameLength);
    
    for (ANGLEGetInfoType i = 0; i < numSymbols; ++i) {
        ANGLEShaderSymbol symbol;
        ANGLEGetInfoType nameLength = 0;
        switch (symbolType) {
        case SH_ACTIVE_ATTRIBUTES:
            symbol.symbolType = SHADER_SYMBOL_TYPE_ATTRIBUTE;
            ShGetActiveAttrib(compiler, i, &nameLength, &symbol.size, &symbol.dataType, nameBuffer.data(), mappedNameBuffer.data());
            break;
        case SH_ACTIVE_UNIFORMS:
            symbol.symbolType = SHADER_SYMBOL_TYPE_UNIFORM;
            ShGetActiveUniform(compiler, i, &nameLength, &symbol.size, &symbol.dataType, nameBuffer.data(), mappedNameBuffer.data());
            break;
        default:
            ASSERT_NOT_REACHED();
            return false;
        }
        if (!nameLength)
            return false;
        
        // The ShGetActive* calls above are guaranteed to produce null-terminated strings for
        // nameBuffer and mappedNameBuffer. Also, the character set for symbol names
        // is a subset of Latin-1 as specified by the OpenGL ES Shading Language, Section 3.1 and
        // WebGL, Section "Characters Outside the GLSL Source Character Set".

        String name = String(nameBuffer.data());
        String mappedName = String(mappedNameBuffer.data());
        
        // ANGLE returns array names in the format "array[0]".
        // The only way to know if a symbol is an array is to check if it ends with "[0]".
        // We can't check the size because regular symbols and arrays of length 1 both have a size of 1.
        symbol.isArray = name.endsWith("[0]") && mappedName.endsWith("[0]");
        if (symbol.isArray) {
            // Add a symbol for the array name without the "[0]" suffix.
            name.truncate(name.length() - 3);
            mappedName.truncate(mappedName.length() - 3);
        }

        symbol.name = name;
        symbol.mappedName = mappedName;
        symbols.append(symbol);
    
        if (symbol.isArray) {
            // Add symbols for each array element.
            symbol.isArray = false;
            for (int i = 0; i < symbol.size; i++) {
                String arrayBrackets = "[" + String::number(i) + "]";
                symbol.name = name + arrayBrackets;
                symbol.mappedName = mappedName + arrayBrackets;
                symbols.append(symbol);
            }
        }
    }
    return true;
}

ANGLEWebKitBridge::ANGLEWebKitBridge(ShShaderOutput shaderOutput, ShShaderSpec shaderSpec)
    : builtCompilers(false)
    , m_fragmentCompiler(0)
    , m_vertexCompiler(0)
    , m_shaderOutput(shaderOutput)
    , m_shaderSpec(shaderSpec)
{
    // This is a no-op if it's already initialized.
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

bool ANGLEWebKitBridge::compileShaderSource(const char* shaderSource, ANGLEShaderType shaderType, String& translatedShaderSource, String& shaderValidationLog, Vector<ANGLEShaderSymbol>& symbols, int extraCompileOptions)
{
    if (!builtCompilers) {
        m_fragmentCompiler = ShConstructCompiler(SH_FRAGMENT_SHADER, m_shaderSpec, m_shaderOutput, &m_resources);
        m_vertexCompiler = ShConstructCompiler(SH_VERTEX_SHADER, m_shaderSpec, m_shaderOutput, &m_resources);
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

    bool validateSuccess = ShCompile(compiler, shaderSourceStrings, 1, SH_OBJECT_CODE | SH_ATTRIBUTES_UNIFORMS | extraCompileOptions);
    if (!validateSuccess) {
        int logSize = getValidationResultValue(compiler, SH_INFO_LOG_LENGTH);
        if (logSize > 1) {
            OwnArrayPtr<char> logBuffer = adoptArrayPtr(new char[logSize]);
            if (logBuffer) {
                ShGetInfoLog(compiler, logBuffer.get());
                shaderValidationLog = logBuffer.get();
            }
        }
        return false;
    }

    int translationLength = getValidationResultValue(compiler, SH_OBJECT_CODE_LENGTH);
    if (translationLength > 1) {
        OwnArrayPtr<char> translationBuffer = adoptArrayPtr(new char[translationLength]);
        if (!translationBuffer)
            return false;
        ShGetObjectCode(compiler, translationBuffer.get());
        translatedShaderSource = translationBuffer.get();
    }
    
    if (!getSymbolInfo(compiler, SH_ACTIVE_ATTRIBUTES, symbols))
        return false;
    if (!getSymbolInfo(compiler, SH_ACTIVE_UNIFORMS, symbols))
        return false;

    return true;
}

}

#endif // USE(3D_GRAPHICS)
