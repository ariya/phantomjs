/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(3D_GRAPHICS)
#include "Extensions3DOpenGLCommon.h"

#include "ANGLEWebKitBridge.h"
#include "GraphicsContext3D.h"

#if PLATFORM(BLACKBERRY)
#include <BlackBerryPlatformLog.h>
#endif

#if USE(OPENGL_ES_2)
#include "OpenGLESShims.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#elif PLATFORM(MAC)
#include <OpenGL/gl.h>
#elif PLATFORM(GTK) || PLATFORM(EFL) || PLATFORM(QT) || PLATFORM(WIN)
#include "OpenGLShims.h"
#endif

#include <wtf/MainThread.h>
#include <wtf/Vector.h>

namespace WebCore {

Extensions3DOpenGLCommon::Extensions3DOpenGLCommon(GraphicsContext3D* context)
    : m_initializedAvailableExtensions(false)
    , m_context(context)
    , m_isNVIDIA(false)
    , m_isAMD(false)
    , m_isIntel(false)
    , m_maySupportMultisampling(true)
    , m_requiresBuiltInFunctionEmulation(false)
{
    m_vendor = String(reinterpret_cast<const char*>(::glGetString(GL_VENDOR)));

    Vector<String> vendorComponents;
    m_vendor.lower().split(' ', vendorComponents);
    if (vendorComponents.contains("nvidia"))
        m_isNVIDIA = true;
    if (vendorComponents.contains("ati") || vendorComponents.contains("amd"))
        m_isAMD = true;
    if (vendorComponents.contains("intel"))
        m_isIntel = true;

#if PLATFORM(MAC)
    if (m_isAMD || m_isIntel)
        m_requiresBuiltInFunctionEmulation = true;

    // Currently in Mac we only allow multisampling if the vendor is NVIDIA,
    // or if the vendor is AMD/ATI and the system is 10.7.2 and above.

    bool systemSupportsMultisampling = true;
#if !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED < 1080
    ASSERT(isMainThread());
    static SInt32 version;
    if (!version) {
        if (Gestalt(gestaltSystemVersion, &version) != noErr)
            systemSupportsMultisampling = false;
    }
    // See https://bugs.webkit.org/show_bug.cgi?id=77922 for more details
    if (systemSupportsMultisampling)
        systemSupportsMultisampling = version >= 0x1072;
#endif // SNOW_LEOPARD and LION

    if (m_isAMD && !systemSupportsMultisampling)
        m_maySupportMultisampling = false;
#endif
}

Extensions3DOpenGLCommon::~Extensions3DOpenGLCommon()
{
}

bool Extensions3DOpenGLCommon::supports(const String& name)
{
    if (!m_initializedAvailableExtensions)
        initializeAvailableExtensions();

    return supportsExtension(name);
}

void Extensions3DOpenGLCommon::ensureEnabled(const String& name)
{
    if (name == "GL_OES_standard_derivatives") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.OES_standard_derivatives) {
            ANGLEResources.OES_standard_derivatives = 1;
            compiler.setResources(ANGLEResources);
        }
    } else if (name == "GL_EXT_draw_buffers") {
        // Enable support in ANGLE (if not enabled already)
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        ShBuiltInResources ANGLEResources = compiler.getResources();
        if (!ANGLEResources.EXT_draw_buffers) {
            ANGLEResources.EXT_draw_buffers = 1;
            m_context->getIntegerv(Extensions3D::MAX_DRAW_BUFFERS_EXT, &ANGLEResources.MaxDrawBuffers);
            compiler.setResources(ANGLEResources);
        }
    }
}

bool Extensions3DOpenGLCommon::isEnabled(const String& name)
{
    if (name == "GL_OES_standard_derivatives") {
        ANGLEWebKitBridge& compiler = m_context->m_compiler;
        return compiler.getResources().OES_standard_derivatives;
    }
    return supports(name);
}

int Extensions3DOpenGLCommon::getGraphicsResetStatusARB()
{
    return GraphicsContext3D::NO_ERROR;
}

String Extensions3DOpenGLCommon::getTranslatedShaderSourceANGLE(Platform3DObject shader)
{
    ASSERT(shader);
    int GLshaderType;
    ANGLEShaderType shaderType;

    ANGLEWebKitBridge& compiler = m_context->m_compiler;

    m_context->getShaderiv(shader, GraphicsContext3D::SHADER_TYPE, &GLshaderType);

    if (GLshaderType == GraphicsContext3D::VERTEX_SHADER)
        shaderType = SHADER_TYPE_VERTEX;
    else if (GLshaderType == GraphicsContext3D::FRAGMENT_SHADER)
        shaderType = SHADER_TYPE_FRAGMENT;
    else
        return ""; // Invalid shader type.

    HashMap<Platform3DObject, GraphicsContext3D::ShaderSourceEntry>::iterator result = m_context->m_shaderSourceMap.find(shader);

    if (result == m_context->m_shaderSourceMap.end())
        return "";

    GraphicsContext3D::ShaderSourceEntry& entry = result->value;

    String translatedShaderSource;
    String shaderInfoLog;
    int extraCompileOptions = SH_MAP_LONG_VARIABLE_NAMES | SH_CLAMP_INDIRECT_ARRAY_BOUNDS;

    if (m_requiresBuiltInFunctionEmulation)
        extraCompileOptions |= SH_EMULATE_BUILT_IN_FUNCTIONS;

    Vector<ANGLEShaderSymbol> symbols;
    bool isValid = compiler.compileShaderSource(entry.source.utf8().data(), shaderType, translatedShaderSource, shaderInfoLog, symbols, extraCompileOptions);

    entry.log = shaderInfoLog;
    entry.isValid = isValid;

    size_t numSymbols = symbols.size();
    for (size_t i = 0; i < numSymbols; ++i) {
        ANGLEShaderSymbol shaderSymbol = symbols[i];
        GraphicsContext3D::SymbolInfo symbolInfo(shaderSymbol.dataType, shaderSymbol.size, shaderSymbol.mappedName);
        entry.symbolMap(shaderSymbol.symbolType).set(shaderSymbol.name, symbolInfo);
    }

    if (!isValid)
        return "";

    return translatedShaderSource;
}

void Extensions3DOpenGLCommon::initializeAvailableExtensions()
{
    String extensionsString = getExtensions();
    Vector<String> availableExtensions;
    extensionsString.split(" ", availableExtensions);
    for (size_t i = 0; i < availableExtensions.size(); ++i)
        m_availableExtensions.add(availableExtensions[i]);
    m_initializedAvailableExtensions = true;
}

void Extensions3DOpenGLCommon::readnPixelsEXT(int, int, GC3Dsizei, GC3Dsizei, GC3Denum, GC3Denum, GC3Dsizei, void *)
{
    m_context->synthesizeGLError(GL_INVALID_OPERATION);
}

void Extensions3DOpenGLCommon::getnUniformfvEXT(GC3Duint, int, GC3Dsizei, float *)
{
    m_context->synthesizeGLError(GL_INVALID_OPERATION);
}

void Extensions3DOpenGLCommon::getnUniformivEXT(GC3Duint, int, GC3Dsizei, int *)
{
    m_context->synthesizeGLError(GL_INVALID_OPERATION);
}

} // namespace WebCore

#endif // USE(3D_GRAPHICS)
