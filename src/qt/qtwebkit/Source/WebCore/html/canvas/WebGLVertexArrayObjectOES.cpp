/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "WebGLVertexArrayObjectOES.h"

#include "Extensions3D.h"
#include "WebGLRenderingContext.h"

namespace WebCore {

PassRefPtr<WebGLVertexArrayObjectOES> WebGLVertexArrayObjectOES::create(WebGLRenderingContext* ctx, VaoType type)
{
    return adoptRef(new WebGLVertexArrayObjectOES(ctx, type));
}

WebGLVertexArrayObjectOES::WebGLVertexArrayObjectOES(WebGLRenderingContext* ctx, VaoType type)
    : WebGLContextObject(ctx)
    , m_type(type)
    , m_hasEverBeenBound(false)
    , m_boundElementArrayBuffer(0)
{
    m_vertexAttribState.resize(ctx->getMaxVertexAttribs());
    
    Extensions3D* extensions = context()->graphicsContext3D()->getExtensions();
    switch (m_type) {
    case VaoTypeDefault:
        break;
    default:
        setObject(extensions->createVertexArrayOES());
        break;
    }
}

WebGLVertexArrayObjectOES::~WebGLVertexArrayObjectOES()
{
    deleteObject(0);
}

void WebGLVertexArrayObjectOES::deleteObjectImpl(GraphicsContext3D* context3d, Platform3DObject object)
{
    Extensions3D* extensions = context3d->getExtensions();
    switch (m_type) {
    case VaoTypeDefault:
        break;
    default:
        extensions->deleteVertexArrayOES(object);
        break;
    }

    if (m_boundElementArrayBuffer)
        m_boundElementArrayBuffer->onDetached(context3d);

    for (size_t i = 0; i < m_vertexAttribState.size(); ++i) {
        VertexAttribState& state = m_vertexAttribState[i];
        if (state.bufferBinding)
            state.bufferBinding->onDetached(context3d);
    }
}

void WebGLVertexArrayObjectOES::setElementArrayBuffer(PassRefPtr<WebGLBuffer> buffer)
{
    if (buffer)
        buffer->onAttached();
    if (m_boundElementArrayBuffer)
        m_boundElementArrayBuffer->onDetached(context()->graphicsContext3D());
    m_boundElementArrayBuffer = buffer;
    
}

void WebGLVertexArrayObjectOES::setVertexAttribState(
    GC3Duint index, GC3Dsizei bytesPerElement, GC3Dint size, GC3Denum type, GC3Dboolean normalized, GC3Dsizei stride, GC3Dintptr offset, PassRefPtr<WebGLBuffer> buffer)
{
    GC3Dsizei validatedStride = stride ? stride : bytesPerElement;

    VertexAttribState& state = m_vertexAttribState[index];

    if (buffer)
        buffer->onAttached();
    if (state.bufferBinding)
        state.bufferBinding->onDetached(context()->graphicsContext3D());

    state.bufferBinding = buffer;
    state.bytesPerElement = bytesPerElement;
    state.size = size;
    state.type = type;
    state.normalized = normalized;
    state.stride = validatedStride;
    state.originalStride = stride;
    state.offset = offset;
}

void WebGLVertexArrayObjectOES::unbindBuffer(PassRefPtr<WebGLBuffer> buffer)
{
    if (m_boundElementArrayBuffer == buffer) {
        m_boundElementArrayBuffer->onDetached(context()->graphicsContext3D());
        m_boundElementArrayBuffer = 0;
    }

    for (size_t i = 0; i < m_vertexAttribState.size(); ++i) {
        VertexAttribState& state = m_vertexAttribState[i];
        if (state.bufferBinding == buffer) {
            buffer->onDetached(context()->graphicsContext3D());

            if (!i && !context()->isGLES2Compliant()) {
                state.bufferBinding = context()->m_vertexAttrib0Buffer;
                state.bufferBinding->onAttached();
                state.bytesPerElement = 0;
                state.size = 4;
                state.type = GraphicsContext3D::FLOAT;
                state.normalized = false;
                state.stride = 16;
                state.originalStride = 0;
                state.offset = 0;
            } else
                state.bufferBinding = 0;
        }
    }
}

}

#endif // ENABLE(WEBGL)
