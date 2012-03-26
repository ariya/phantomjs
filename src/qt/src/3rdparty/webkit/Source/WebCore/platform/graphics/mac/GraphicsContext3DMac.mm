/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#include "GraphicsContext3D.h"

#import "BlockExceptions.h"

#include "ANGLE/ShaderLang.h"
#include "ArrayBuffer.h"
#include "ArrayBufferView.h"
#include "CanvasRenderingContext.h"
#include <CoreGraphics/CGBitmapContext.h>
#include "Extensions3DOpenGL.h"
#include "Float32Array.h"
#include "GraphicsContext.h"
#include "HTMLCanvasElement.h"
#include "ImageBuffer.h"
#include "Int32Array.h"
#include "NotImplemented.h"
#include <OpenGL/CGLRenderers.h>
#include <OpenGL/gl.h>
#include "Uint8Array.h"
#include "WebGLLayer.h"
#include "WebGLObject.h"
#include <wtf/UnusedParam.h>
#include <wtf/text/CString.h>

namespace WebCore {

static void setPixelFormat(Vector<CGLPixelFormatAttribute>& attribs, int colorBits, int depthBits, bool accelerated, bool supersample, bool closest)
{
    attribs.clear();
    
    attribs.append(kCGLPFAColorSize);
    attribs.append(static_cast<CGLPixelFormatAttribute>(colorBits));
    attribs.append(kCGLPFADepthSize);
    attribs.append(static_cast<CGLPixelFormatAttribute>(depthBits));
    
    if (accelerated)
        attribs.append(kCGLPFAAccelerated);
    else {
        attribs.append(kCGLPFARendererID);
        attribs.append(static_cast<CGLPixelFormatAttribute>(kCGLRendererGenericFloatID));
    }
        
    if (supersample)
        attribs.append(kCGLPFASupersample);
        
    if (closest)
        attribs.append(kCGLPFAClosestPolicy);
        
    attribs.append(static_cast<CGLPixelFormatAttribute>(0));
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to the HostWindow.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attrs, hostWindow, false));
    return context->m_contextObj ? context.release() : 0;
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, bool)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_contextObj(0)
    , m_attrs(attrs)
    , m_texture(0)
    , m_compositorTexture(0)
    , m_fbo(0)
    , m_depthStencilBuffer(0)
    , m_layerComposited(false)
    , m_internalColorFormat(0)
    , m_boundFBO(0)
    , m_activeTexture(0)
    , m_boundTexture0(0)
    , m_multisampleFBO(0)
    , m_multisampleDepthStencilBuffer(0)
    , m_multisampleColorBuffer(0)
{
    UNUSED_PARAM(hostWindow);

    Vector<CGLPixelFormatAttribute> attribs;
    CGLPixelFormatObj pixelFormatObj = 0;
    GLint numPixelFormats = 0;
    
    // We will try:
    //
    //  1) 32 bit RGBA/32 bit depth/accelerated/supersampled
    //  2) 32 bit RGBA/32 bit depth/accelerated
    //  3) 32 bit RGBA/16 bit depth/accelerated
    //  4) closest to 32 bit RGBA/16 bit depth/software renderer
    //
    //  If none of that works, we simply fail and set m_contextObj to 0.
    
    setPixelFormat(attribs, 32, 32, true, true, false);
    CGLChoosePixelFormat(attribs.data(), &pixelFormatObj, &numPixelFormats);
    if (numPixelFormats == 0) {
        setPixelFormat(attribs, 32, 32, true, false, false);
        CGLChoosePixelFormat(attribs.data(), &pixelFormatObj, &numPixelFormats);
        
        if (numPixelFormats == 0) {
            setPixelFormat(attribs, 32, 16, true, false, false);
            CGLChoosePixelFormat(attribs.data(), &pixelFormatObj, &numPixelFormats);
        
            if (numPixelFormats == 0) {
                setPixelFormat(attribs, 32, 16, false, false, true);
                CGLChoosePixelFormat(attribs.data(), &pixelFormatObj, &numPixelFormats);
        
                if (numPixelFormats == 0) {
                    // Could not find an acceptable renderer - fail
                    return;
                }
            }
        }
    }
    
    CGLError err = CGLCreateContext(pixelFormatObj, 0, &m_contextObj);
    CGLDestroyPixelFormat(pixelFormatObj);
    
    if (err != kCGLNoError || !m_contextObj) {
        // Could not create the context - fail
        m_contextObj = 0;
        return;
    }

    // Set the current context to the one given to us.
    CGLSetCurrentContext(m_contextObj);
    
    validateAttributes();

    // Create the WebGLLayer
    BEGIN_BLOCK_OBJC_EXCEPTIONS
        m_webGLLayer.adoptNS([[WebGLLayer alloc] initWithGraphicsContext3D:this]);
#ifndef NDEBUG
        [m_webGLLayer.get() setName:@"WebGL Layer"];
#endif    
    END_BLOCK_OBJC_EXCEPTIONS
    
    // create a texture to render into
    ::glGenTextures(1, &m_texture);
    ::glBindTexture(GL_TEXTURE_2D, m_texture);
    ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ::glGenTextures(1, &m_compositorTexture);
    ::glBindTexture(GL_TEXTURE_2D, m_compositorTexture);
    ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    ::glBindTexture(GL_TEXTURE_2D, 0);
    
    // create an FBO
    ::glGenFramebuffersEXT(1, &m_fbo);
    ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_fbo);
    
    m_boundFBO = m_fbo;
    if (!m_attrs.antialias && (m_attrs.stencil || m_attrs.depth))
        ::glGenRenderbuffersEXT(1, &m_depthStencilBuffer);

    // create an multisample FBO
    if (m_attrs.antialias) {
        ::glGenFramebuffersEXT(1, &m_multisampleFBO);
        ::glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_multisampleFBO);
        m_boundFBO = m_multisampleFBO;
        ::glGenRenderbuffersEXT(1, &m_multisampleColorBuffer);
        if (m_attrs.stencil || m_attrs.depth)
            ::glGenRenderbuffersEXT(1, &m_multisampleDepthStencilBuffer);
    }
    
    // ANGLE initialization.

    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;
    
    m_compiler.setResources(ANGLEResources);
    
    ::glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
    ::glEnable(GL_POINT_SPRITE);

    ::glClearColor(0, 0, 0, 0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    if (m_contextObj) {
        CGLSetCurrentContext(m_contextObj);
        ::glDeleteTextures(1, &m_texture);
        ::glDeleteTextures(1, &m_compositorTexture);
        if (m_attrs.antialias) {
            ::glDeleteRenderbuffersEXT(1, &m_multisampleColorBuffer);
            if (m_attrs.stencil || m_attrs.depth)
                ::glDeleteRenderbuffersEXT(1, &m_multisampleDepthStencilBuffer);
            ::glDeleteFramebuffersEXT(1, &m_multisampleFBO);
        } else {
            if (m_attrs.stencil || m_attrs.depth)
                ::glDeleteRenderbuffersEXT(1, &m_depthStencilBuffer);
        }
        ::glDeleteFramebuffersEXT(1, &m_fbo);
        CGLSetCurrentContext(0);
        CGLDestroyContext(m_contextObj);
    }
}

void GraphicsContext3D::makeContextCurrent()
{
    if (!m_contextObj)
        return;

    CGLContextObj currentContext = CGLGetCurrentContext();
    if (currentContext != m_contextObj)
        CGLSetCurrentContext(m_contextObj);
}

bool GraphicsContext3D::isGLES2Compliant() const
{
    return false;
}

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<ContextLostCallback>)
{
}

}

#endif // ENABLE(WEBGL)
