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
    : WebGLObject(ctx)
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

void WebGLVertexArrayObjectOES::deleteObjectImpl(Platform3DObject object)
{
    Extensions3D* extensions = context()->graphicsContext3D()->getExtensions();
    switch (m_type) {
    case VaoTypeDefault:
        break;
    default:
        extensions->deleteVertexArrayOES(object);
        break;
    }
}

}

#endif // ENABLE(WEBGL)
