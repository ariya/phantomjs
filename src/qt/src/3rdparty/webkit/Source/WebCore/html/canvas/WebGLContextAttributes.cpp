/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WEBGL)

#include "WebGLContextAttributes.h"

namespace WebCore {

PassRefPtr<WebGLContextAttributes> WebGLContextAttributes::create()
{
    return adoptRef(new WebGLContextAttributes());
}

PassRefPtr<WebGLContextAttributes> WebGLContextAttributes::create(GraphicsContext3D::Attributes attributes)
{
    return adoptRef(new WebGLContextAttributes(attributes));
}

WebGLContextAttributes::WebGLContextAttributes()
    : CanvasContextAttributes()
{
}

WebGLContextAttributes::WebGLContextAttributes(GraphicsContext3D::Attributes attributes)
    : CanvasContextAttributes()
    , m_attrs(attributes)
{
}

WebGLContextAttributes::~WebGLContextAttributes()
{
}

bool WebGLContextAttributes::alpha() const
{
    return m_attrs.alpha;
}

void WebGLContextAttributes::setAlpha(bool alpha)
{
    m_attrs.alpha = alpha;
}

bool WebGLContextAttributes::depth() const
{
    return m_attrs.depth;
}

void WebGLContextAttributes::setDepth(bool depth)
{
    m_attrs.depth = depth;
}

bool WebGLContextAttributes::stencil() const
{
    return m_attrs.stencil;
}

void WebGLContextAttributes::setStencil(bool stencil)
{
    m_attrs.stencil = stencil;
}

bool WebGLContextAttributes::antialias() const
{
    return m_attrs.antialias;
}

void WebGLContextAttributes::setAntialias(bool antialias)
{
    m_attrs.antialias = antialias;
}

bool WebGLContextAttributes::premultipliedAlpha() const
{
    return m_attrs.premultipliedAlpha;
}

void WebGLContextAttributes::setPremultipliedAlpha(bool premultipliedAlpha)
{
    m_attrs.premultipliedAlpha = premultipliedAlpha;
}

bool WebGLContextAttributes::preserveDrawingBuffer() const
{
    return m_attrs.preserveDrawingBuffer;
}

void WebGLContextAttributes::setPreserveDrawingBuffer(bool preserveDrawingBuffer)
{
    m_attrs.preserveDrawingBuffer = preserveDrawingBuffer;
}

GraphicsContext3D::Attributes WebGLContextAttributes::attributes() const
{
    return m_attrs;
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
