/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 * Copyright (C) 2011, 2012 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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
#include "DrawingBuffer.h"

#include "GraphicsContext3D.h"

#if USE(ACCELERATED_COMPOSITING)
#include "LayerWebKitThread.h"
#endif

namespace WebCore {

struct DrawingBufferInternal {
    unsigned offscreenColorTexture;
#if USE(ACCELERATED_COMPOSITING)
    RefPtr<LayerWebKitThread> platformLayer;
#endif
};

static unsigned generateColorTexture(GraphicsContext3D* context, const IntSize& size)
{
    unsigned offscreenColorTexture = context->createTexture();
    if (!offscreenColorTexture)
        return 0;

    context->bindTexture(GraphicsContext3D::TEXTURE_2D, offscreenColorTexture);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::NEAREST);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::NEAREST);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, size.width(), size.height(), 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, 0);
    context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, offscreenColorTexture, 0);
    return offscreenColorTexture;
}

DrawingBuffer::DrawingBuffer(GraphicsContext3D* context, const IntSize& size)
    : m_context(context)
    , m_size(size)
    , m_fbo(context->createFramebuffer())
{
    m_internal = adoptPtr(new DrawingBufferInternal);

    context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_internal->offscreenColorTexture = generateColorTexture(context, size);
}

DrawingBuffer::~DrawingBuffer()
{
    if (!m_context)
        return;

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    m_context->deleteTexture(m_internal->offscreenColorTexture);

    clear();
}

#if USE(ACCELERATED_COMPOSITING)
void DrawingBuffer::publishToPlatformLayer()
{
    if (!m_context)
        return;

    if (m_callback)
        m_callback->willPublish();

    // FIXME: We do the copy in the canvas' (child) context so that it executes in the correct order relative to
    // other commands in the child context. This ensures that the parent texture always contains a complete
    // frame and not some intermediate result. However, there is no synchronization to ensure that this copy
    // happens before the compositor draws. This means we might draw stale frames sometimes. Ideally this
    // would insert a fence into the child command stream that the compositor could wait for.
    m_context->makeContextCurrent();
    m_context->flush();
}
#endif

bool DrawingBuffer::reset(const IntSize& newSize)
{
    if (!m_context)
        return false;

    if (m_size == newSize)
        return false;

    m_size = newSize;

    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_internal->offscreenColorTexture);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, m_size.width(), m_size.height(), 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, 0);

    return true;
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* DrawingBuffer::platformLayer()
{
    return m_internal->platformLayer.get();
}
#endif

} // namespace WebCore
