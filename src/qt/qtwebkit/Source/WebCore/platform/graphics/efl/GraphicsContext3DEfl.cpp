/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "GraphicsContext3D.h"

#if USE(3D_GRAPHICS) || USE(ACCELERATED_COMPOSITING)

#include "GLDefs.h"
#include "GraphicsContext3DPrivate.h"
#include "Image.h"
#include "ImageSource.h"
#include "NotImplemented.h"
#include "PlatformContextCairo.h"

namespace WebCore {

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, RenderStyle renderStyle)
{
    RefPtr<GraphicsContext3D> context = adoptRef(new GraphicsContext3D(attrs, hostWindow, renderStyle));
    return context->m_private ? context.release() : 0;
}

GraphicsContext3D::GraphicsContext3D(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : m_currentWidth(0)
    , m_currentHeight(0)
    , m_compiler(isGLES2Compliant() ? SH_ESSL_OUTPUT : SH_GLSL_OUTPUT)
    , m_attrs(attrs)
    , m_renderStyle(renderStyle)
    , m_texture(0)
    , m_compositorTexture(0)
    , m_fbo(0)
#if USE(OPENGL_ES_2)
    , m_depthBuffer(0)
    , m_stencilBuffer(0)
#endif
    , m_depthStencilBuffer(0)
    , m_layerComposited(false)
    , m_internalColorFormat(0)
    , m_multisampleFBO(0)
    , m_multisampleDepthStencilBuffer(0)
    , m_multisampleColorBuffer(0)
    , m_private(GraphicsContext3DPrivate::create(this, hostWindow))
{
    if (!m_private)
        return;

    validateAttributes();

    if (renderStyle == RenderOffscreen) {
        // Create buffers for the canvas FBO.
        glGenFramebuffers(1, &m_fbo);

        // Create a texture to render into.
        glGenTextures(1, &m_texture);
        glBindTexture(GraphicsContext3D::TEXTURE_2D, m_texture);
        glTexParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
        glTexParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
        glTexParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
        glTexParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
        glBindTexture(GraphicsContext3D::TEXTURE_2D, 0);

        // Create a multisample FBO.
        if (m_attrs.antialias) {
            glGenFramebuffers(1, &m_multisampleFBO);
            glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_multisampleFBO);
            m_state.boundFBO = m_multisampleFBO;
            glGenRenderbuffers(1, &m_multisampleColorBuffer);
            if (m_attrs.stencil || m_attrs.depth)
                glGenRenderbuffers(1, &m_multisampleDepthStencilBuffer);
        } else {
            // Bind canvas FBO.
            glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_fbo);
            m_state.boundFBO = m_fbo;
#if USE(OPENGL_ES_2)
            if (m_attrs.depth)
                glGenRenderbuffers(1, &m_depthBuffer);
            if (m_attrs.stencil)
                glGenRenderbuffers(1, &m_stencilBuffer);
#endif
            if (m_attrs.stencil || m_attrs.depth)
                glGenRenderbuffers(1, &m_depthStencilBuffer);
        }
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

    GC3Dint range[2], precision;
    getShaderPrecisionFormat(GraphicsContext3D::FRAGMENT_SHADER, GraphicsContext3D::HIGH_FLOAT, range, &precision);
    ANGLEResources.FragmentPrecisionHigh = (range[0] || range[1] || precision);

    m_compiler.setResources(ANGLEResources);

#if !USE(OPENGL_ES_2)
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif
    if (renderStyle != RenderToCurrentGLContext)
        glClearColor(0.0, 0.0, 0.0, 0.0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    if (!m_private || (m_renderStyle == RenderToCurrentGLContext) || !makeContextCurrent())
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (m_texture)
        glDeleteTextures(1, &m_texture);

    if (m_fbo)
        glDeleteFramebuffers(1, &m_fbo);

    if (m_attrs.antialias) {
        if (m_multisampleColorBuffer)
            glDeleteRenderbuffers(1, &m_multisampleColorBuffer);

        if (m_multisampleDepthStencilBuffer)
            glDeleteRenderbuffers(1, &m_multisampleDepthStencilBuffer);

        if (m_multisampleFBO)
            glDeleteFramebuffers(1, &m_multisampleFBO);
    } else if (m_attrs.stencil || m_attrs.depth) {
#if USE(OPENGL_ES_2)
        if (m_depthBuffer)
            glDeleteRenderbuffers(1, &m_depthBuffer);

        if (m_stencilBuffer)
            glDeleteRenderbuffers(1, &m_stencilBuffer);
#endif
        if (m_depthStencilBuffer)
            glDeleteRenderbuffers(1, &m_depthStencilBuffer);
    }
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D()
{
    return m_private->platformGraphicsContext3D();
}

Platform3DObject GraphicsContext3D::platformTexture() const
{
    return m_texture;
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* GraphicsContext3D::platformLayer() const
{
#if USE(TEXTURE_MAPPER_GL)
    return m_private.get();
#else
    notImplemented();
    return 0;
#endif
}
#endif

bool GraphicsContext3D::makeContextCurrent()
{
    return m_private->makeContextCurrent();
}

bool GraphicsContext3D::isGLES2Compliant() const
{
#if USE(OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<ContextLostCallback> callBack)
{
    m_private->setContextLostCallback(callBack);
}

void GraphicsContext3D::setErrorMessageCallback(PassOwnPtr<ErrorMessageCallback>) 
{
    notImplemented();
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight, PlatformContextCairo* context)
{
    if (!imagePixels || imageWidth <= 0 || imageHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0 || !context)
        return;

    cairo_t* cr = context->cr();
    context->save(); 

    RefPtr<cairo_surface_t> imageSurface = adoptRef(cairo_image_surface_create_for_data(
        const_cast<unsigned char*>(imagePixels), CAIRO_FORMAT_ARGB32, imageWidth, imageHeight, imageWidth * 4));

    cairo_rectangle(cr, 0, 0, canvasWidth, canvasHeight);

    // OpenGL keeps the pixels stored bottom up, so we need to flip the image here.
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, 1.0, 0.0, 0.0, -1.0, 0.0, imageHeight);
    cairo_set_matrix(cr, &matrix);
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_set_source_surface(cr, imageSurface.get(), 0, 0);
    cairo_fill(cr);
    context->restore();
}

#if USE(GRAPHICS_SURFACE)
void GraphicsContext3D::createGraphicsSurfaces(const IntSize& size)
{
    m_private->didResizeCanvas(size);
}
#endif

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
    delete m_decoder;
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    // This implementation is taken from GraphicsContext3DCairo.

    if (!m_image)
        return false;

    // We need this to stay in scope because the native image is just a shallow copy of the data.
    m_decoder = new ImageSource(premultiplyAlpha ? ImageSource::AlphaPremultiplied : ImageSource::AlphaNotPremultiplied, ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied);

    if (!m_decoder)
        return false;

    ImageSource& decoder = *m_decoder;
    m_alphaOp = AlphaDoNothing;

    if (m_image->data()) {
        decoder.setData(m_image->data(), true);

        if (!decoder.frameCount())
            return false;

        m_imageSurface = decoder.createFrameAtIndex(0);
        if (!m_imageSurface || !decoder.frameIsCompleteAtIndex(0))
            return false;
    } else {
        m_imageSurface = m_image->nativeImageForCurrentFrame();
        // 1. For texImage2D with HTMLVideoElment input, assume no PremultiplyAlpha had been applied and the alpha value is 0xFF for each pixel,
        // which is true at present and may be changed in the future and needs adjustment accordingly.
        // 2. For texImage2D with HTMLCanvasElement input in which Alpha is already Premultiplied in this port,
        // do AlphaDoUnmultiply if UNPACK_PREMULTIPLY_ALPHA_WEBGL is set to false.
        if (!premultiplyAlpha && m_imageHtmlDomSource != HtmlDomVideo)
            m_alphaOp = AlphaDoUnmultiply;
    }

    if (!m_imageSurface)
        return false;

    m_imageWidth = cairo_image_surface_get_width(m_imageSurface.get());
    m_imageHeight = cairo_image_surface_get_height(m_imageSurface.get());

    if (!m_imageWidth || !m_imageHeight)
        return false;

    if (cairo_image_surface_get_format(m_imageSurface.get()) != CAIRO_FORMAT_ARGB32)
        return false;

    uint srcUnpackAlignment = 1;
    size_t bytesPerRow = cairo_image_surface_get_stride(m_imageSurface.get());
    size_t bitsPerPixel = 32;
    unsigned padding = bytesPerRow - bitsPerPixel / 8 * m_imageWidth;

    if (padding) {
        srcUnpackAlignment = padding + 1;
        while (bytesPerRow % srcUnpackAlignment)
            ++srcUnpackAlignment;
    }

    m_imagePixelData = cairo_image_surface_get_data(m_imageSurface.get());
    m_imageSourceFormat = DataFormatBGRA8;
    m_imageSourceUnpackAlignment = srcUnpackAlignment;

    return true;
}

} // namespace WebCore

#endif // USE(3D_GRAPHICS)
