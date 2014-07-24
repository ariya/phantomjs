/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 * Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
#include "FECustomFilter.h"

#include "CustomFilterCompiledProgram.h"
#include "CustomFilterRenderer.h"
#include "CustomFilterValidatedProgram.h"
#include "Extensions3D.h"
#include "GraphicsContext3D.h"
#include "RenderTreeAsText.h"
#include "TextStream.h"

#include <wtf/Uint8ClampedArray.h>

namespace WebCore {

FECustomFilter::FECustomFilter(Filter* filter, PassRefPtr<GraphicsContext3D> context, PassRefPtr<CustomFilterValidatedProgram> validatedProgram, const CustomFilterParameterList& parameters,
    unsigned meshRows, unsigned meshColumns, CustomFilterMeshType meshType)
    : FilterEffect(filter)
    , m_context(context)
    , m_validatedProgram(validatedProgram)
    , m_inputTexture(0)
    , m_frameBuffer(0)
    , m_depthBuffer(0)
    , m_destTexture(0)
    , m_triedMultisampleBuffer(false)
    , m_multisampleFrameBuffer(0)
    , m_multisampleRenderBuffer(0)
    , m_multisampleDepthBuffer(0)
{
    // We will not pass a CustomFilterCompiledProgram here, as we only want to compile it when we actually need it in the first paint.
    m_customFilterRenderer = CustomFilterRenderer::create(m_context, m_validatedProgram->programInfo().programType(), parameters, meshRows, meshColumns, meshType);
}

PassRefPtr<FECustomFilter> FECustomFilter::create(Filter* filter, PassRefPtr<GraphicsContext3D> context, PassRefPtr<CustomFilterValidatedProgram> validatedProgram, const CustomFilterParameterList& parameters,
    unsigned meshRows, unsigned meshColumns, CustomFilterMeshType meshType)
{
    return adoptRef(new FECustomFilter(filter, context, validatedProgram, parameters, meshRows, meshColumns, meshType));
}

FECustomFilter::~FECustomFilter()
{
    deleteRenderBuffers();
}

void FECustomFilter::deleteRenderBuffers()
{
    ASSERT(m_context);
    m_context->makeContextCurrent();
    if (m_inputTexture) {
        m_context->deleteTexture(m_inputTexture);
        m_inputTexture = 0;
    }
    if (m_frameBuffer) {
        // Make sure to unbind any framebuffer from the context first, otherwise
        // some platforms might refuse to bind the same buffer id again.
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0);
        m_context->deleteFramebuffer(m_frameBuffer);
        m_frameBuffer = 0;
    }
    if (m_depthBuffer) {
        m_context->deleteRenderbuffer(m_depthBuffer);
        m_depthBuffer = 0;
    }
    if (m_destTexture) {
        m_context->deleteTexture(m_destTexture);
        m_destTexture = 0;
    }
    deleteMultisampleRenderBuffers();
}

void FECustomFilter::deleteMultisampleRenderBuffers()
{
    if (m_multisampleFrameBuffer) {
        // Make sure to unbind any framebuffer from the context first, otherwise
        // some platforms might refuse to bind the same buffer id again.
        m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, 0);
        m_context->deleteFramebuffer(m_multisampleFrameBuffer);
        m_multisampleFrameBuffer = 0;
    }
    if (m_multisampleRenderBuffer) {
        m_context->deleteRenderbuffer(m_multisampleRenderBuffer);
        m_multisampleRenderBuffer = 0;
    }
    if (m_multisampleDepthBuffer) {
        m_context->deleteRenderbuffer(m_multisampleDepthBuffer);
        m_multisampleDepthBuffer = 0;
    }
}

void FECustomFilter::platformApplySoftware()
{
    if (!applyShader())
        clearShaderResult();
}

void FECustomFilter::clearShaderResult()
{
    clearResult();
    Uint8ClampedArray* dstPixelArray = createUnmultipliedImageResult();
    if (!dstPixelArray)
        return;

    FilterEffect* in = inputEffect(0);
    setIsAlphaImage(in->isAlphaImage());
    IntRect effectDrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    in->copyUnmultipliedImage(dstPixelArray, effectDrawingRect);
}

void FECustomFilter::drawFilterMesh(Platform3DObject inputTexture)
{
    bool multisample = canUseMultisampleBuffers();
    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, multisample ? m_multisampleFrameBuffer : m_frameBuffer);
    m_context->viewport(0, 0, m_contextSize.width(), m_contextSize.height());

    m_context->clearColor(0, 0, 0, 0);
    m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT | GraphicsContext3D::DEPTH_BUFFER_BIT);

    m_customFilterRenderer->draw(inputTexture, m_contextSize);

    if (multisample)
        resolveMultisampleBuffer();
}

bool FECustomFilter::prepareForDrawing()
{
    m_context->makeContextCurrent();

    if (!m_customFilterRenderer->compiledProgram()) {
        RefPtr<CustomFilterCompiledProgram> compiledProgram = m_validatedProgram->compiledProgram();
        if (!compiledProgram) {
            // Lazily create a compiled program and let CustomFilterValidatedProgram hold on to it.
            compiledProgram = CustomFilterCompiledProgram::create(m_context, m_validatedProgram->validatedVertexShader(), m_validatedProgram->validatedFragmentShader(), m_validatedProgram->programInfo().programType());
            m_validatedProgram->setCompiledProgram(compiledProgram);
        }
        // Lazily inject the compiled program into the CustomFilterRenderer.
        m_customFilterRenderer->setCompiledProgram(compiledProgram.release());
    }

    if (!m_customFilterRenderer->prepareForDrawing())
        return false;

    // Only allocate a texture if the program needs one and the caller doesn't allocate one by itself.
    if ((m_customFilterRenderer->programNeedsInputTexture() && !ensureInputTexture()) || !ensureFrameBuffer())
        return false;

    return true;
}

bool FECustomFilter::applyShader()
{
    Uint8ClampedArray* dstPixelArray = m_customFilterRenderer->premultipliedAlpha() ? createPremultipliedImageResult() : createUnmultipliedImageResult();
    if (!dstPixelArray)
        return false;

    if (!prepareForDrawing())
        return false;

    FilterEffect* in = inputEffect(0);
    IntRect effectDrawingRect = requestedRegionOfInputImageData(in->absolutePaintRect());
    IntSize newContextSize(effectDrawingRect.size());
    if (!resizeContextIfNeeded(newContextSize))
        return false;

    bool needsInputTexture = m_customFilterRenderer->programNeedsInputTexture();
    if (needsInputTexture) {
        RefPtr<Uint8ClampedArray> srcPixelArray = in->asUnmultipliedImage(effectDrawingRect);
        uploadInputTexture(srcPixelArray.get());
    }
    drawFilterMesh(needsInputTexture ? m_inputTexture : 0);

    ASSERT(static_cast<size_t>(newContextSize.width() * newContextSize.height() * 4) == dstPixelArray->length());
    m_context->readPixels(0, 0, newContextSize.width(), newContextSize.height(), GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, dstPixelArray->data());

    return true;
}

bool FECustomFilter::ensureInputTexture()
{
    if (!m_inputTexture)
        m_inputTexture = m_context->createTexture();
    return m_inputTexture;
}

void FECustomFilter::uploadInputTexture(Uint8ClampedArray* srcPixelArray)
{
    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_inputTexture);
    m_context->texImage2D(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, m_contextSize.width(), m_contextSize.height(), 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, srcPixelArray->data());
}

bool FECustomFilter::ensureFrameBuffer()
{
    if (!m_frameBuffer)
        m_frameBuffer = m_context->createFramebuffer();
    if (!m_depthBuffer)
        m_depthBuffer = m_context->createRenderbuffer();
    if (!m_destTexture)
        m_destTexture = m_context->createTexture();
    return m_frameBuffer && m_depthBuffer && m_destTexture;
}

bool FECustomFilter::createMultisampleBuffer()
{
    ASSERT(!m_triedMultisampleBuffer);
    m_triedMultisampleBuffer = true;

    Extensions3D* extensions = m_context->getExtensions();
    if (!extensions
        || !extensions->maySupportMultisampling()
        || !extensions->supports("GL_ANGLE_framebuffer_multisample")
        || !extensions->supports("GL_ANGLE_framebuffer_blit")
        || !extensions->supports("GL_OES_rgb8_rgba8"))
        return false;

    extensions->ensureEnabled("GL_ANGLE_framebuffer_blit");
    extensions->ensureEnabled("GL_ANGLE_framebuffer_multisample");
    extensions->ensureEnabled("GL_OES_rgb8_rgba8");

    if (!m_multisampleFrameBuffer)
        m_multisampleFrameBuffer = m_context->createFramebuffer();
    if (!m_multisampleRenderBuffer)
        m_multisampleRenderBuffer = m_context->createRenderbuffer();
    if (!m_multisampleDepthBuffer)
        m_multisampleDepthBuffer = m_context->createRenderbuffer();

    return true;
}

void FECustomFilter::resolveMultisampleBuffer()
{
    ASSERT(m_triedMultisampleBuffer && m_multisampleFrameBuffer && m_multisampleRenderBuffer && m_multisampleDepthBuffer);
    m_context->bindFramebuffer(Extensions3D::READ_FRAMEBUFFER, m_multisampleFrameBuffer);
    m_context->bindFramebuffer(Extensions3D::DRAW_FRAMEBUFFER, m_frameBuffer);

    ASSERT(m_context->getExtensions());
    m_context->getExtensions()->blitFramebuffer(0, 0, m_contextSize.width(), m_contextSize.height(), 0, 0, m_contextSize.width(), m_contextSize.height(), GraphicsContext3D::COLOR_BUFFER_BIT, GraphicsContext3D::NEAREST);

    m_context->bindFramebuffer(Extensions3D::READ_FRAMEBUFFER, 0);
    m_context->bindFramebuffer(Extensions3D::DRAW_FRAMEBUFFER, 0);

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_frameBuffer);
}

bool FECustomFilter::canUseMultisampleBuffers() const
{
    return m_triedMultisampleBuffer && m_multisampleFrameBuffer && m_multisampleRenderBuffer && m_multisampleDepthBuffer;
}

bool FECustomFilter::resizeMultisampleBuffers(const IntSize& newContextSize)
{
    if (!m_triedMultisampleBuffer && !createMultisampleBuffer())
        return false;

    if (!canUseMultisampleBuffers())
        return false;

    static const int kMaxSampleCount = 4;
    int maxSupportedSampleCount = 0;
    m_context->getIntegerv(Extensions3D::MAX_SAMPLES, &maxSupportedSampleCount);
    int sampleCount = std::min(kMaxSampleCount, maxSupportedSampleCount);
    if (!sampleCount) {
        deleteMultisampleRenderBuffers();
        return false;
    }

    Extensions3D* extensions = m_context->getExtensions();
    ASSERT(extensions);

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFrameBuffer);

    m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_multisampleRenderBuffer);
    extensions->renderbufferStorageMultisample(GraphicsContext3D::RENDERBUFFER, sampleCount, Extensions3D::RGBA8_OES, newContextSize.width(), newContextSize.height());
    m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::RENDERBUFFER, m_multisampleRenderBuffer);

    m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_multisampleDepthBuffer);
    extensions->renderbufferStorageMultisample(GraphicsContext3D::RENDERBUFFER, sampleCount, GraphicsContext3D::DEPTH_COMPONENT16, newContextSize.width(), newContextSize.height());
    m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_multisampleDepthBuffer);

    m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);

    if (m_context->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE) {
        deleteMultisampleRenderBuffers();
        return false;
    }

    return true;
}

bool FECustomFilter::resizeContextIfNeeded(const IntSize& newContextSize)
{
    if (newContextSize.isEmpty())
        return false;
    if (m_contextSize == newContextSize)
        return true;

    int maxTextureSize = 0;
    m_context->getIntegerv(GraphicsContext3D::MAX_TEXTURE_SIZE, &maxTextureSize);
    if (newContextSize.height() > maxTextureSize || newContextSize.width() > maxTextureSize)
        return false;

    return resizeContext(newContextSize);
}

bool FECustomFilter::resizeContext(const IntSize& newContextSize)
{
    bool multisample = resizeMultisampleBuffers(newContextSize);

    m_context->bindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_frameBuffer);
    m_context->bindTexture(GraphicsContext3D::TEXTURE_2D, m_destTexture);
    // We are going to clear the output buffer anyway, so we can safely initialize the destination texture with garbage data.
    m_context->texImage2DDirect(GraphicsContext3D::TEXTURE_2D, 0, GraphicsContext3D::RGBA, newContextSize.width(), newContextSize.height(), 0, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, 0);
    m_context->framebufferTexture2D(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::COLOR_ATTACHMENT0, GraphicsContext3D::TEXTURE_2D, m_destTexture, 0);

    // We don't need the depth buffer for the texture framebuffer, if we already
    // have a multisample buffer.
    if (!multisample) {
        m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
        m_context->renderbufferStorage(GraphicsContext3D::RENDERBUFFER, GraphicsContext3D::DEPTH_COMPONENT16, newContextSize.width(), newContextSize.height());
        m_context->framebufferRenderbuffer(GraphicsContext3D::FRAMEBUFFER, GraphicsContext3D::DEPTH_ATTACHMENT, GraphicsContext3D::RENDERBUFFER, m_depthBuffer);
    }

    if (m_context->checkFramebufferStatus(GraphicsContext3D::FRAMEBUFFER) != GraphicsContext3D::FRAMEBUFFER_COMPLETE)
        return false;

    if (multisample) {
        // Clear the framebuffer first, otherwise the first blit will fail.
        m_context->clearColor(0, 0, 0, 0);
        m_context->clear(GraphicsContext3D::COLOR_BUFFER_BIT);
    }

    m_context->bindRenderbuffer(GraphicsContext3D::RENDERBUFFER, 0);

    m_contextSize = newContextSize;
    return true;
}

void FECustomFilter::dump()
{
}

TextStream& FECustomFilter::externalRepresentation(TextStream& ts, int indent) const
{
    writeIndent(ts, indent);
    ts << "[feCustomFilter";
    FilterEffect::externalRepresentation(ts);
    ts << "]\n";
    inputEffect(0)->externalRepresentation(ts, indent + 1);
    return ts;
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS) && USE(3D_GRAPHICS)
