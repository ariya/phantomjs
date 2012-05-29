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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
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

#if ENABLE(ACCELERATED_2D_CANVAS) || ENABLE(WEBGL)

#include "DrawingBuffer.h"

#include "Extensions3D.h"
#include "WebGLLayer.h"

#import "BlockExceptions.h"

namespace WebCore {

DrawingBuffer::DrawingBuffer(GraphicsContext3D* context,
                             const IntSize& size,
                             bool multisampleExtensionSupported,
                             bool packedDepthStencilExtensionSupported)
    : m_context(context)
    , m_size(-1, -1)
    , m_multisampleExtensionSupported(multisampleExtensionSupported)
    , m_packedDepthStencilExtensionSupported(packedDepthStencilExtensionSupported)
    , m_fbo(context->createFramebuffer())
    , m_colorBuffer(0)
    , m_depthStencilBuffer(0)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
    , m_multisampleFBO(0)
    , m_multisampleColorBuffer(0)
{
    ASSERT(m_fbo);
    if (!m_fbo) {
        clear();
        return;
    }
        
    // Create the WebGLLayer
    BEGIN_BLOCK_OBJC_EXCEPTIONS
        m_platformLayer.adoptNS([[WebGLLayer alloc] initWithGraphicsContext3D:m_context.get()]);
#ifndef NDEBUG
        [m_platformLayer.get() setName:@"DrawingBuffer Layer"];
#endif    
    END_BLOCK_OBJC_EXCEPTIONS

    // create a texture to render into
    m_colorBuffer = context->createTexture();
    context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_colorBuffer);
    context->texParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    context->texParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);
    
    // Create the FBO
    m_fbo = context->createFramebuffer();
    ASSERT(m_fbo);
    if (!m_fbo) {
        clear();
        return;
    }
        
    createSecondaryBuffers();
    reset(size);
}

DrawingBuffer::~DrawingBuffer()
{
    clear();
}

void DrawingBuffer::didReset()
{
}

PlatformLayer* DrawingBuffer::platformLayer()
{
    return m_platformLayer.get();
}

Platform3DObject DrawingBuffer::platformColorBuffer() const
{
    return m_colorBuffer;
}

}

#endif
