/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
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

#if ENABLE(ACCELERATED_2D_CANVAS) || USE(3D_GRAPHICS)

#include "DrawingBuffer.h"

#include "Extensions3D.h"
#include "GraphicsContext3D.h"

namespace WebCore {

#if PLATFORM(WIN) || PLATFORM(QT) || USE(CAIRO)
DrawingBuffer::DrawingBuffer(GraphicsContext3D* context, const IntSize& size, bool multisampleExtensionSupported, bool packedDepthStencilExtensionSupported, PreserveDrawingBuffer preserveDrawingBuffer, AlphaRequirement alpha)
    : m_preserveDrawingBuffer(preserveDrawingBuffer)
    , m_alpha(alpha)
    , m_scissorEnabled(false)
    , m_texture2DBinding(0)
    , m_framebufferBinding(0)
    , m_activeTextureUnit(GraphicsContext3D::TEXTURE0)
    , m_context(context)
    , m_size(-1, -1)
    , m_multisampleExtensionSupported(multisampleExtensionSupported)
    , m_packedDepthStencilExtensionSupported(packedDepthStencilExtensionSupported)
    , m_fbo(context->createFramebuffer())
    , m_colorBuffer(0)
    , m_frontColorBuffer(0)
    , m_separateFrontTexture(false)
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

    // create a texture to render into
    m_colorBuffer = context->createTexture();
    context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_colorBuffer);
    context->texParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    context->texParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    context->texParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);

    createSecondaryBuffers();
    reset(size);
}

DrawingBuffer::~DrawingBuffer()
{
    clear();
}
#endif

// Global resource ceiling (expressed in terms of pixels) for DrawingBuffer creation and resize.
// When this limit is set, DrawingBuffer::create() and DrawingBuffer::reset() calls that would
// exceed the global cap will instead clear the buffer.
#if !PLATFORM(BLACKBERRY)
static int s_maximumResourceUsePixels = 0;
#endif
static int s_currentResourceUsePixels = 0;
static const float s_resourceAdjustedRatio = 0.5;

PassRefPtr<DrawingBuffer> DrawingBuffer::create(GraphicsContext3D* context, const IntSize& size, PreserveDrawingBuffer preserve, AlphaRequirement alpha)
{
    Extensions3D* extensions = context->getExtensions();
    bool multisampleSupported = extensions->maySupportMultisampling()
        && extensions->supports("GL_ANGLE_framebuffer_blit")
        && extensions->supports("GL_ANGLE_framebuffer_multisample")
        && extensions->supports("GL_OES_rgb8_rgba8");
    if (multisampleSupported) {
        extensions->ensureEnabled("GL_ANGLE_framebuffer_blit");
        extensions->ensureEnabled("GL_ANGLE_framebuffer_multisample");
        extensions->ensureEnabled("GL_OES_rgb8_rgba8");
    }
    bool packedDepthStencilSupported = extensions->supports("GL_OES_packed_depth_stencil");
    if (packedDepthStencilSupported)
        extensions->ensureEnabled("GL_OES_packed_depth_stencil");
    RefPtr<DrawingBuffer> drawingBuffer = adoptRef(new DrawingBuffer(context, size, multisampleSupported, packedDepthStencilSupported, preserve, alpha));
    return (drawingBuffer->m_context) ? drawingBuffer.release() : 0;
}

void DrawingBuffer::clear()
{
    if (!m_context)
        return;

    m_context->makeContextCurrent();

    if (!m_size.isEmpty()) {
        s_currentResourceUsePixels -= m_size.width() * m_size.height();
        m_size = IntSize();
    }

    if (m_colorBuffer) {
        m_context->deleteTexture(m_colorBuffer);
        m_colorBuffer = 0;
    }

    if (m_frontColorBuffer) {
        m_context->deleteTexture(m_frontColorBuffer);
        m_frontColorBuffer = 0;
    }

    if (m_multisampleColorBuffer) {
        m_context->deleteRenderbuffer(m_multisampleColorBuffer);
        m_multisampleColorBuffer = 0;
    }

    if (m_depthStencilBuffer) {
        m_context->deleteRenderbuffer(m_depthStencilBuffer);
        m_depthStencilBuffer = 0;
    }

    if (m_depthBuffer) {
        m_context->deleteRenderbuffer(m_depthBuffer);
        m_depthBuffer = 0;
    }

    if (m_stencilBuffer) {
        m_context->deleteRenderbuffer(m_stencilBuffer);
        m_stencilBuffer = 0;
    }

    if (m_multisampleFBO) {
        m_context->deleteFramebuffer(m_multisampleFBO);
        m_multisampleFBO = 0;
    }

    if (m_fbo) {
        m_context->deleteFramebuffer(m_fbo);
        m_fbo = 0;
    }
}

void DrawingBuffer::createSecondaryBuffers()
{
    // create a multisample FBO
    if (multisample()) {
        m_multisampleFBO = m_context->createFramebuffer();
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
        m_multisampleColorBuffer = m_context->createRenderbuffer();
    }
}

void DrawingBuffer::resizeDepthStencil(int sampleCount)
{
    const GraphicsContext3D::Attributes& attributes = m_context->getContextAttributes();
    if (attributes.depth && attributes.stencil && m_packedDepthStencilExtensionSupported) {
        if (!m_depthStencilBuffer)
            m_depthStencilBuffer = m_context->createRenderbuffer();
        m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_depthStencilBuffer);
        if (multisample())
            m_context->getExtensions()->renderbufferStorageMultisample(GraphicsContext3D::RENDERBUFFER, sampleCount, Extensions3D::DEPTH24_STENCIL8, m_size.width(), m_size.height());
        else
            m_context->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, Extensions3D::DEPTH24_STENCIL8, m_size.width(), m_size.height());
        m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthStencilBuffer);
        m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthStencilBuffer);
    } else {
        if (attributes.depth) {
            if (!m_depthBuffer)
                m_depthBuffer = m_context->createRenderbuffer();
            m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
            if (multisample())
                m_context->getExtensions()->renderbufferStorageMultisample(GraphicsContext3D::RENDERBUFFER, sampleCount, GraphicsContext3D::DEPTH_COMPONENT16, m_size.width(), m_size.height());
            else
                m_context->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_COMPONENT16, m_size.width(), m_size.height());
            m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
        }
        if (attributes.stencil) {
            if (!m_stencilBuffer)
                m_stencilBuffer = m_context->createRenderbuffer();
            m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_stencilBuffer);
            if (multisample())
                m_context->getExtensions()->renderbufferStorageMultisample(GraphicsContext3D::RENDERBUFFER, sampleCount, GraphicsContext3D::STENCIL_INDEX8, m_size.width(), m_size.height());
            else 
                m_context->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::STENCIL_INDEX8, m_size.width(), m_size.height());
            m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::STENCIL_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_stencilBuffer);
        }
    }
    m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);
}

void DrawingBuffer::clearFramebuffers(GC3Dbitfield clearMask)
{
    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO ? m_multisampleFBO : m_fbo);

    m_context->clear(clearMask);

    // The multisample fbo was just cleared, but we also need to clear the non-multisampled buffer too.
    if (m_multisampleFBO) {
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
        m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
    }
}

// Only way to ensure that we're not getting a bad framebuffer on some AMD/OSX devices.
// FIXME: This can be removed once renderbufferStorageMultisample starts reporting GL_OUT_OF_MEMORY properly.
bool DrawingBuffer::checkBufferIntegrity()
{
    if (!m_multisampleFBO)
        return true;

    if (m_scissorEnabled)
        m_context->disable(GraphicsContext3D::SCISSOR_TEST);

    m_context->colorMask(true, true, true, true);

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
    m_context->clearColor(1.0f, 0.0f, 1.0f, 1.0f);
    m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT);

    commit(0, 0, 1, 1);

    unsigned char pixel[4] = {0, 0, 0, 0};
    m_context->readPixels(0, 0, 1, 1, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, &pixel);

    if (m_scissorEnabled)
        m_context->enable(GraphicsContext3D::SCISSOR_TEST);

    return (pixel[0] == 0xFF && pixel[1] == 0x00 && pixel[2] == 0xFF && pixel[3] == 0xFF);
}

bool DrawingBuffer::reset(const IntSize& newSize)
{
    if (!m_context)
        return false;

    m_context->makeContextCurrent();

    int maxTextureSize = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_TEXTURE_SIZE, &maxTextureSize);
    if (newSize.height() > maxTextureSize || newSize.width() > maxTextureSize) {
        clear();
        return false;
    }

    int pixelDelta = newSize.width() * newSize.height();
    int oldSize = 0;
    if (!m_size.isEmpty()) {
        oldSize = m_size.width() * m_size.height();
        pixelDelta -= oldSize;
    }

    IntSize adjustedSize = newSize;
    if (s_maximumResourceUsePixels) {
        while ((s_currentResourceUsePixels + pixelDelta) > s_maximumResourceUsePixels) {
            adjustedSize.scale(s_resourceAdjustedRatio);
            if (adjustedSize.isEmpty()) {
                clear();
                return false;
            }
            pixelDelta = adjustedSize.width() * adjustedSize.height();
            pixelDelta -= oldSize;
        }
     }

    const GraphicsContext3D::Attributes& attributes = m_context->getContextAttributes();

    if (adjustedSize != m_size) {

        unsigned internalColorFormat, colorFormat, internalRenderbufferFormat;
        if (attributes.alpha) {
            internalColorFormat = GraphicsContext3D::RGBA;
            colorFormat = GraphicsContext3D::RGBA;
            internalRenderbufferFormat = Extensions3D::RGBA8_OES;
        } else {
            internalColorFormat = GraphicsContext3D::RGB;
            colorFormat = GraphicsContext3D::RGB;
            internalRenderbufferFormat = Extensions3D::RGB8_OES;
        }

        do {
            m_size = adjustedSize;
            // resize multisample FBO
            if (multisample()) {
                int maxSampleCount = 0;

                m_context->getIntegerv(Extensions3D::MAX_SAMPLES, &maxSampleCount);
                int sampleCount = std::min(4, maxSampleCount);

                m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);

                m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_multisampleColorBuffer);
                m_context->getExtensions()->renderbufferStorageMultisample(GraphicsContext3D::RENDERBUFFER, sampleCount, internalRenderbufferFormat, m_size.width(), m_size.height());

                if (m_context->getError() == GraphicsContext3D::OUT_OF_MEMORY) {
                    adjustedSize.scale(s_resourceAdjustedRatio);
                    continue;
                }

                m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::RENDERBUFFER, m_multisampleColorBuffer);
                resizeDepthStencil(sampleCount);
                if (m_context->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
                    adjustedSize.scale(s_resourceAdjustedRatio);
                    continue;
                }
            }

            // resize regular FBO
            m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);

            m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_colorBuffer);
            m_context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, internalColorFormat, m_size.width(), m_size.height(), 0, colorFormat, GraphicsContext3D::UNSIGNED_BYTE, 0);

            m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, m_colorBuffer, 0);

            // resize the front color buffer
            if (m_separateFrontTexture) {
                m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_frontColorBuffer);
                m_context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, internalColorFormat, m_size.width(), m_size.height(), 0, colorFormat, GraphicsContext3D::UNSIGNED_BYTE, 0);
            }

            m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, 0);

            if (!multisample())
                resizeDepthStencil(0);
            if (m_context->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
                adjustedSize.scale(s_resourceAdjustedRatio);
                continue;
            }

#if OS(DARWIN)
            // FIXME: This can be removed once renderbufferStorageMultisample starts reporting GL_OUT_OF_MEMORY properly on OSX.
            if (!checkBufferIntegrity()) {
                adjustedSize.scale(s_resourceAdjustedRatio);
                continue;
            }
#endif

            break;

        } while (!adjustedSize.isEmpty());

        pixelDelta = m_size.width() * m_size.height();
        pixelDelta -= oldSize;
        s_currentResourceUsePixels += pixelDelta;

        if (!newSize.isEmpty() && adjustedSize.isEmpty()) {
            clear();
            return false;
        }
    }

    m_context->disable(GraphicsContext3D::SCISSOR_TEST);
    m_context->clearColor(0, 0, 0, 0);
    m_context->colorMask(true, true, true, true);

    GC3Dbitfield clearMask = GraphicsContext3D::COLOR_BUFFER_BIT;
    if (attributes.depth) {
        m_context->clearDepth(1.0f);
        clearMask |= GraphicsContext3D::DEPTH_BUFFER_BIT;
        m_context->depthMask(true);
    }
    if (attributes.stencil) {
        m_context->clearStencil(0);
        clearMask |= GraphicsContext3D::STENCIL_BUFFER_BIT;
        m_context->stencilMaskSeparate(GraphicsContext3D::FRONT, 0xFFFFFFFF);
    }

    clearFramebuffers(clearMask);

    return true;
}

void DrawingBuffer::commit(long x, long y, long width, long height)
{
    if (!m_context)
        return;

    if (width < 0)
        width = m_size.width();
    if (height < 0)
        height = m_size.height();

    m_context->makeContextCurrent();

    if (m_multisampleFBO) {
        m_context->bindFramebuffer(Extensions3D::READ_FRAMEBUFFER, m_multisampleFBO);
        m_context->bindFramebuffer(Extensions3D::DRAW_FRAMEBUFFER, m_fbo);

        if (m_scissorEnabled)
            m_context->disable(GraphicsContext3D::SCISSOR_TEST);

        // Use NEAREST, because there is no scale performed during the blit.
        m_context->getExtensions()->blitFramebuffer(x, y, width, height, x, y, width, height, GraphicsContext3D::COLOR_BUFFER_BIT, GraphicsContext3D::NEAREST);

        if (m_scissorEnabled)
            m_context->enable(GraphicsContext3D::SCISSOR_TEST);
    }

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
}

void DrawingBuffer::restoreFramebufferBinding()
{
    if (!m_context || !m_framebufferBinding)
        return;

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_framebufferBinding);
}

bool DrawingBuffer::multisample() const
{
    return m_context && m_context->getContextAttributes().antialias && m_multisampleExtensionSupported;
}

void DrawingBuffer::discardResources()
{
    m_colorBuffer = 0;
    m_frontColorBuffer = 0;
    m_multisampleColorBuffer = 0;

    m_depthStencilBuffer = 0;
    m_depthBuffer = 0;

    m_stencilBuffer = 0;

    m_multisampleFBO = 0;
    m_fbo = 0;
}

void DrawingBuffer::bind()
{
    if (!m_context)
        return;

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO ? m_multisampleFBO : m_fbo);
}

} // namespace WebCore

#endif
