/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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

#if USE(3D_GRAPHICS)

#include "GraphicsContext3D.h"

#include "Extensions3DOpenGLES.h"
#include "GraphicsContext.h"
#include "OpenGLESShims.h"
#include "WebGLLayerWebKitThread.h"

#include <BlackBerryPlatformGraphics.h>
#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformLog.h>
#include <TiledImage.h>

namespace WebCore {

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(Attributes attribs, HostWindow* hostWindow, RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to a window.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;

    return adoptRef(new GraphicsContext3D(attribs, hostWindow, renderStyle));
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attrs, HostWindow*, GraphicsContext3D::RenderStyle renderStyle)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_context(BlackBerry::Platform::Graphics::createWebGLContext())
    , m_compiler(SH_ESSL_OUTPUT)
    , m_attrs(attrs)
    , m_texture(0)
    , m_fbo(0)
    , m_depthStencilBuffer(0)
    , m_layerComposited(false)
    , m_internalColorFormat(GL_RGBA)
    , m_isImaginationHardware(0)
{
    if (renderStyle != RenderDirectlyToHostWindow) {
#if USE(ACCELERATED_COMPOSITING)
        m_compositingLayer = WebGLLayerWebKitThread::create();
#endif
        makeContextCurrent();

        // Create a texture to render into.
        ::glGenTextures(1, &m_texture);
        ::glBindTexture(GL_TEXTURE_2D, m_texture);
        ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        ::glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        ::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        ::glBindTexture(GL_TEXTURE_2D, 0);

        // Create an FBO.
        ::glGenFramebuffers(1, &m_fbo);
        ::glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
        if (m_attrs.stencil || m_attrs.depth)
            ::glGenRenderbuffers(1, &m_depthStencilBuffer);
        m_state.boundFBO = m_fbo;

#if USE(ACCELERATED_COMPOSITING)
        static_cast<WebGLLayerWebKitThread*>(m_compositingLayer.get())->setWebGLContext(this);
#endif
    }

    makeContextCurrent();

    const char* renderer = reinterpret_cast<const char*>(::glGetString(GL_RENDERER));
    m_isImaginationHardware = std::strstr(renderer, "PowerVR SGX");

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

    GC3Dint range[2], precision;
    getShaderPrecisionFormat(GraphicsContext3D::FRAGMENT_SHADER, GraphicsContext3D::HIGH_FLOAT, range, &precision);
    ANGLEResources.FragmentPrecisionHigh = (range[0] || range[1] || precision);

    m_compiler.setResources(ANGLEResources);

    ::glClearColor(0, 0, 0, 0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    if (m_texture) {
        makeContextCurrent();
        ::glDeleteTextures(1, &m_texture);
        if (m_attrs.stencil || m_attrs.depth)
            ::glDeleteRenderbuffers(1, &m_depthStencilBuffer);
        ::glDeleteFramebuffers(1, &m_fbo);
    }

    static_cast<WebGLLayerWebKitThread *>(m_compositingLayer.get())->webGLContextDestroyed(); // Must release compositing layer before destroying the context.
    BlackBerry::Platform::Graphics::destroyWebGLContext(m_context);
}

void GraphicsContext3D::prepareTexture()
{
    if (m_layerComposited)
        return;

    makeContextCurrent();
    if (m_attrs.antialias)
        resolveMultisamplingIfNecessary();

    m_layerComposited = true;
}

bool GraphicsContext3D::reshapeFBOs(const IntSize& size)
{
    // A BlackBerry-specific implementation of reshapeFBOs is necessary because it contains:
    //  - an Imagination-specific implementation of anti-aliasing
    //  - an Imagination-specific fix for FBOs of size less than (16,16)

    int fboWidth = size.width();
    int fboHeight = size.height();

    // Imagination-specific fix
    if (m_isImaginationHardware) {
        fboWidth = std::max(fboWidth, 16);
        fboHeight = std::max(fboHeight, 16);
    }

    GLuint internalColorFormat, colorFormat, internalDepthStencilFormat = 0;
    if (m_attrs.alpha) {
        internalColorFormat = GL_RGBA;
        colorFormat = GL_RGBA;
    } else {
        internalColorFormat = GL_RGB;
        colorFormat = GL_RGB;
    }
    if (m_attrs.stencil || m_attrs.depth) {
        // We don't allow the logic where stencil is required and depth is not.
        // See GraphicsContext3D constructor.
        if (m_attrs.stencil && m_attrs.depth)
            internalDepthStencilFormat = GL_DEPTH24_STENCIL8_EXT;
        else
            internalDepthStencilFormat = GL_DEPTH_COMPONENT16;
    }

    GLint sampleCount = 8;
    if (m_attrs.antialias) {
        GLint maxSampleCount;
        // Hardcode the maximum number of samples due to header issue (PR132183)
        // ::glGetIntegerv(GL_MAX_SAMPLES_IMG, &maxSampleCount);
        maxSampleCount = 4;
        sampleCount = std::min(8, maxSampleCount);
    }

    bool mustRestoreFBO = false;
    if (m_state.boundFBO != m_fbo) {
        mustRestoreFBO = true;
        ::glBindFramebufferEXT(GraphicsContext3D::FRAMEBUFFER, m_fbo);
    }

    ::glBindTexture(GL_TEXTURE_2D, m_texture);
    ::glTexImage2D(GL_TEXTURE_2D, 0, internalColorFormat, fboWidth, fboHeight, 0, colorFormat, GL_UNSIGNED_BYTE, 0);

    Extensions3D* extensions = getExtensions();
    if (m_attrs.antialias) {
        extensions->framebufferTexture2DMultisampleIMG(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture, 0, sampleCount);

        if (m_attrs.stencil || m_attrs.depth) {
            ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
            extensions->renderbufferStorageMultisampleIMG(GL_RENDERBUFFER_EXT, sampleCount, internalDepthStencilFormat, fboWidth, fboHeight);

            if (m_attrs.stencil)
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
            if (m_attrs.depth)
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
            ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        }
    } else {
        ::glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_texture, 0);

        if (m_attrs.stencil || m_attrs.depth) {
            ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
            ::glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, internalDepthStencilFormat, fboWidth, fboHeight);

            if (m_attrs.stencil)
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_STENCIL_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
            if (m_attrs.depth)
                ::glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, m_depthStencilBuffer);
            ::glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, 0);
        }
    }
    ::glBindTexture(GL_TEXTURE_2D, 0);


    logFrameBufferStatus(__LINE__);

    return mustRestoreFBO;
}

void GraphicsContext3D::logFrameBufferStatus(int line)
{
    BBLOG(BlackBerry::Platform::LogLevelInfo, "Checking FrameBuffer status at line %d: ", line);
    switch (glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)) {
    case GL_FRAMEBUFFER_COMPLETE:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "COMPLETE | ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "INCOMPLETE ATTACHMENT | ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "MISSING ATTACHMENT | ");
        break;
    case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "INCOMPLETE DIMENSIONS | ");
        break;
    case GL_FRAMEBUFFER_UNSUPPORTED:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "UNSUPPORTED | ");
        break;
    case FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "INCOMPLETE MULTISAMPLE | ");
        break;
    }

    switch (glGetError()) {
    case GL_NO_ERROR:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "NO ERROR");
        break;
    case GL_INVALID_ENUM:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "INVALID ENUM");
        break;
    case GL_INVALID_VALUE:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "INVALID VALUE");
        break;
    case GL_INVALID_OPERATION:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "INVALID OPERATION");
        break;
    case GL_OUT_OF_MEMORY:
        BBLOG(BlackBerry::Platform::LogLevelInfo, "OUT OF MEMORY");
        break;
    }
    BBLOG(BlackBerry::Platform::LogLevelInfo, "\n");
}

void GraphicsContext3D::readPixelsIMG(GC3Dint x, GC3Dint y, GC3Dsizei width, GC3Dsizei height, GC3Denum format, GC3Denum type, void* data)
{
    // Currently only format=RGBA, type=UNSIGNED_BYTE is supported by the specification: http://www.khronos.org/registry/webgl/specs/latest/
    // If this ever changes, this code will need to be updated.

    // Calculate the strides of our data and canvas
    unsigned formatSize = 4; // RGBA UNSIGNED_BYTE
    unsigned dataStride = width * formatSize;
    unsigned canvasStride = m_currentWidth * formatSize;

    // If we are using a pack alignment of 8, then we need to align our strides to 8 byte boundaries
    // See: http://en.wikipedia.org/wiki/Data_structure_alignment (computing padding)
    int packAlignment;
    glGetIntegerv(GL_PACK_ALIGNMENT, &packAlignment);
    if (8 == packAlignment) {
        dataStride = (dataStride + 7) & ~7;
        canvasStride = (canvasStride + 7) & ~7;
    }

    unsigned char* canvasData = new unsigned char[canvasStride * m_currentHeight];
    ::glReadPixels(0, 0, m_currentWidth, m_currentHeight, format, type, canvasData);

    // If we failed to read our canvas data due to a GL error, don't continue
    int error = glGetError();
    if (GL_NO_ERROR != error) {
        synthesizeGLError(error);
        return;
    }

    // Clear our data in case some of it lies outside the bounds of our canvas
    // TODO: don't do this if all of the data lies inside the bounds of the canvas
    memset(data, 0, dataStride * height);

    // Calculate the intersection of our canvas and data bounds
    IntRect dataRect(x, y, width, height);
    IntRect canvasRect(0, 0, m_currentWidth, m_currentHeight);
    IntRect nonZeroDataRect = intersection(dataRect, canvasRect);

    unsigned xDataOffset = x < 0 ? -x * formatSize : 0;
    unsigned yDataOffset = y < 0 ? -y * dataStride : 0;
    unsigned xCanvasOffset = nonZeroDataRect.x() * formatSize;
    unsigned yCanvasOffset = nonZeroDataRect.y() * canvasStride;
    unsigned char* dst = static_cast<unsigned char*>(data) + xDataOffset + yDataOffset;
    unsigned char* src = canvasData + xCanvasOffset + yCanvasOffset;
    for (int row = 0; row < nonZeroDataRect.height(); row++) {
        memcpy(dst, src, nonZeroDataRect.width() * formatSize);
        dst += dataStride;
        src += canvasStride;
    }

    delete [] canvasData;
}

bool GraphicsContext3D::paintsIntoCanvasBuffer() const
{
    return true;
}

bool GraphicsContext3D::makeContextCurrent()
{
    BlackBerry::Platform::Graphics::useWebGLContext(m_context);
    return true;
}

bool GraphicsContext3D::isGLES2Compliant() const
{
    return true;
}

Platform3DObject GraphicsContext3D::platformTexture() const
{
    return m_texture;
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D() const
{
    return m_context;
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* GraphicsContext3D::platformLayer() const
{
    return m_compositingLayer.get();
}
#endif

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight,
    GraphicsContext* context)
{
    FloatRect src(0, 0, canvasWidth, canvasHeight);
    FloatRect dst(0, 0, imageWidth, imageHeight);
    double oldTransform[6];
    double flipYTransform[6] = { 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, imageHeight };
    context->platformContext()->getTransform(oldTransform);
    context->platformContext()->setTransform(flipYTransform);
    BlackBerry::Platform::Graphics::TiledImage image(IntSize(imageWidth, imageHeight), reinterpret_cast_ptr<const unsigned*>(imagePixels));
    context->platformContext()->addImage(dst, src, &image);
    context->platformContext()->setTransform(oldTransform);
}

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<ContextLostCallback> callback)
{
    static_cast<Extensions3DOpenGLES*>(getExtensions())->setEXTContextLostCallback(callback);
}

void GraphicsContext3D::setErrorMessageCallback(PassOwnPtr<ErrorMessageCallback>)
{
}

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool)
{
    if (!m_image)
        return false;

    NativeImagePtr nativeImage = m_image->nativeImageForCurrentFrame();
    if (!nativeImage)
        return false;

    m_imageWidth = nativeImage->width();
    m_imageHeight = nativeImage->height();
    if (!m_imageWidth || !m_imageHeight)
        return false;

    unsigned imageSize = m_imageWidth * m_imageHeight;
    m_imageData.resize(imageSize);
    if (!nativeImage->readPixels(m_imageData.data(), imageSize))
        return false;

    // Raw image data is premultiplied
    m_alphaOp = AlphaDoNothing;
    if (!premultiplyAlpha)
        m_alphaOp = AlphaDoUnmultiply;

    m_imagePixelData = m_imageData.data();
    m_imageSourceFormat = DataFormatBGRA8;
    m_imageSourceUnpackAlignment = 0;

    return true;
}

} // namespace WebCore

#endif // USE(3D_GRAPHICS)

