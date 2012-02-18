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

#include "WebGLObject.h"

#include "WebGLRenderingContext.h"

namespace WebCore {

WebGLObject::WebGLObject(WebGLRenderingContext* context)
    : m_object(0)
    , m_context(context)
    , m_attachmentCount(0)
    , m_deleted(false)
{
}

WebGLObject::~WebGLObject()
{
    if (m_context)
        m_context->removeObject(this);
}

void WebGLObject::setObject(Platform3DObject object)
{
    // object==0 && m_deleted==false indicating an uninitialized state;
    ASSERT(!m_object && !m_deleted);
    m_object = object;
}

void WebGLObject::deleteObject()
{
    m_deleted = true;
    if (!m_context || !m_object)
        return;
    if (!m_attachmentCount) {
        m_context->graphicsContext3D()->makeContextCurrent();
        deleteObjectImpl(m_object);
        m_object = 0;
    }
}

}

#endif // ENABLE(WEBGL)
