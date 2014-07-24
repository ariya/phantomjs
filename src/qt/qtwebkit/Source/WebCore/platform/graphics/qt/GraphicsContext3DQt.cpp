/*
    Copyright (C) 2010 Tieto Corporation.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.
     
    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.
     
    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "config.h"
#include "GraphicsContext3D.h"

#if USE(OPENGL_ES_2)
#include "Extensions3DOpenGLES.h"
#else
#include "Extensions3DOpenGL.h"
#endif
#include "GraphicsContext.h"
#include "GraphicsSurface.h"
#include "HostWindow.h"
#include "ImageBuffer.h"
#include "ImageData.h"
#include "NativeImageQt.h"
#include "NotImplemented.h"
#include "OpenGLShims.h"
#include "QWebPageClient.h"
#include "SharedBuffer.h"
#include "TextureMapperPlatformLayer.h"
#include <qpa/qplatformpixmap.h>
#include <wtf/text/CString.h>

#if QT_VERSION >= 0x050100
#include <QOffscreenSurface>
#else
#include <QWindow>
#endif

#if USE(TEXTURE_MAPPER_GL)
#include <texmap/TextureMapperGL.h>
#endif

#if USE(3D_GRAPHICS)

namespace WebCore {

#if !defined(GLchar)
typedef char GLchar;
#endif

#if !defined(GL_DEPTH24_STENCIL8)
#define GL_DEPTH24_STENCIL8 0x88F0
#endif

class GraphicsContext3DPrivate
#if USE(ACCELERATED_COMPOSITING)
        : public TextureMapperPlatformLayer
#endif
{
public:
    GraphicsContext3DPrivate(GraphicsContext3D*, HostWindow*, GraphicsContext3D::RenderStyle);
    ~GraphicsContext3DPrivate();

#if USE(ACCELERATED_COMPOSITING)
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect& target, const TransformationMatrix&, float opacity);
#endif
#if USE(GRAPHICS_SURFACE)
    virtual IntSize platformLayerSize() const;
    virtual uint32_t copyToGraphicsSurface();
    virtual GraphicsSurfaceToken graphicsSurfaceToken() const;
#endif

    QRectF boundingRect() const;
    void blitMultisampleFramebuffer() const;
    void blitMultisampleFramebufferAndRestoreContext() const;
    bool makeCurrentIfNeeded() const;
    void createOffscreenBuffers();
    void initializeANGLE();
    void createGraphicsSurfaces(const IntSize&);

    GraphicsContext3D* m_context;
    HostWindow* m_hostWindow;
    PlatformGraphicsSurface3D m_surface;
    PlatformGraphicsContext3D m_platformContext;
    QObject* m_surfaceOwner;
#if USE(GRAPHICS_SURFACE)
    GraphicsSurface::Flags m_surfaceFlags;
    RefPtr<GraphicsSurface> m_graphicsSurface;
#endif
};

bool GraphicsContext3D::isGLES2Compliant() const
{
#if USE(OPENGL_ES_2)
    return true;
#else
    return false;
#endif
}

GraphicsContext3DPrivate::GraphicsContext3DPrivate(GraphicsContext3D* context, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
    : m_context(context)
    , m_hostWindow(hostWindow)
    , m_surface(0)
    , m_platformContext(0)
    , m_surfaceOwner(0)
{
    if (renderStyle == GraphicsContext3D::RenderToCurrentGLContext) {
        m_platformContext = QOpenGLContext::currentContext();
        if (m_platformContext)
            m_surface = m_platformContext->surface();
        return;
    }

    QOpenGLContext* shareContext = 0;
    if (hostWindow && hostWindow->platformPageClient() && hostWindow->platformPageClient()->makeOpenGLContextCurrentIfAvailable())
        shareContext = QOpenGLContext::currentContext();

#if QT_VERSION >= 0x050100
    QOffscreenSurface* surface = new QOffscreenSurface;
    surface->create();
    m_surface = surface;
    m_surfaceOwner = surface;
#else
    QWindow* window = new QWindow;
    window->setSurfaceType(QSurface::OpenGLSurface);
    window->setGeometry(-10, -10, 1, 1);
    window->create();
    m_surface = window;
    m_surfaceOwner = window;
#endif

    m_platformContext = new QOpenGLContext(m_surfaceOwner);
    if (shareContext)
        m_platformContext->setShareContext(shareContext);

    if (!m_platformContext->create())
        return;

    makeCurrentIfNeeded();

#if USE(GRAPHICS_SURFACE)
    IntSize surfaceSize(m_context->m_currentWidth, m_context->m_currentHeight);
    m_surfaceFlags = GraphicsSurface::SupportsTextureTarget
                    | GraphicsSurface::SupportsSharing;

    if (!surfaceSize.isEmpty())
        m_graphicsSurface = GraphicsSurface::create(surfaceSize, m_surfaceFlags, m_platformContext);
#endif
}

void GraphicsContext3DPrivate::createOffscreenBuffers()
{
    glGenFramebuffers(/* count */ 1, &m_context->m_fbo);

    glGenTextures(1, &m_context->m_texture);
    glBindTexture(GraphicsContext3D::TEXTURE_2D, m_context->m_texture);
    glTexParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MAG_FILTER, GraphicsContext3D::LINEAR);
    glTexParameterf(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_MIN_FILTER, GraphicsContext3D::LINEAR);
    glTexParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_S, GraphicsContext3D::CLAMP_TO_EDGE);
    glTexParameteri(GraphicsContext3D::TEXTURE_2D, GraphicsContext3D::TEXTURE_WRAP_T, GraphicsContext3D::CLAMP_TO_EDGE);
    glBindTexture(GraphicsContext3D::TEXTURE_2D, 0);

    // Create a multisample FBO.
    if (m_context->m_attrs.antialias) {
        glGenFramebuffers(1, &m_context->m_multisampleFBO);
        glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_multisampleFBO);
        m_context->m_state.boundFBO = m_context->m_multisampleFBO;
        glGenRenderbuffers(1, &m_context->m_multisampleColorBuffer);
        if (m_context->m_attrs.stencil || m_context->m_attrs.depth)
            glGenRenderbuffers(1, &m_context->m_multisampleDepthStencilBuffer);
    } else {
        // Bind canvas FBO.
        glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_fbo);
        m_context->m_state.boundFBO = m_context->m_fbo;
#if USE(OPENGL_ES_2)
        if (m_context->m_attrs.depth)
            glGenRenderbuffers(1, &m_context->m_depthBuffer);
        if (m_context->m_attrs.stencil)
            glGenRenderbuffers(1, &m_context->m_stencilBuffer);
#endif
        if (m_context->m_attrs.stencil || m_context->m_attrs.depth)
            glGenRenderbuffers(1, &m_context->m_depthStencilBuffer);
    }
}

void GraphicsContext3DPrivate::initializeANGLE()
{
    ShBuiltInResources ANGLEResources;
    ShInitBuiltInResources(&ANGLEResources);

    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_ATTRIBS, &ANGLEResources.MaxVertexAttribs);
    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_UNIFORM_VECTORS, &ANGLEResources.MaxVertexUniformVectors);
    m_context->getIntegerv(GraphicsContext3D::MAX_VARYING_VECTORS, &ANGLEResources.MaxVaryingVectors);
    m_context->getIntegerv(GraphicsContext3D::MAX_VERTEX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxVertexTextureImageUnits);
    m_context->getIntegerv(GraphicsContext3D::MAX_COMBINED_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxCombinedTextureImageUnits);
    m_context->getIntegerv(GraphicsContext3D::MAX_TEXTURE_IMAGE_UNITS, &ANGLEResources.MaxTextureImageUnits);
    m_context->getIntegerv(GraphicsContext3D::MAX_FRAGMENT_UNIFORM_VECTORS, &ANGLEResources.MaxFragmentUniformVectors);

    // Always set to 1 for OpenGL ES.
    ANGLEResources.MaxDrawBuffers = 1;

    Extensions3D* extensions = m_context->getExtensions();
    if (extensions->supports("GL_ARB_texture_rectangle"))
        ANGLEResources.ARB_texture_rectangle = 1;

    GC3Dint range[2], precision;
    m_context->getShaderPrecisionFormat(GraphicsContext3D::FRAGMENT_SHADER, GraphicsContext3D::HIGH_FLOAT, range, &precision);
    ANGLEResources.FragmentPrecisionHigh = (range[0] || range[1] || precision);

    m_context->m_compiler.setResources(ANGLEResources);
}

GraphicsContext3DPrivate::~GraphicsContext3DPrivate()
{
#if USE(ACCELERATED_COMPOSITING)
    if (TextureMapperPlatformLayer::Client* client = TextureMapperPlatformLayer::client())
        client->platformLayerWasDestroyed();
#endif
    delete m_surfaceOwner;
    m_surfaceOwner = 0;
}

static inline quint32 swapBgrToRgb(quint32 pixel)
{
    return (((pixel << 16) | (pixel >> 16)) & 0x00ff00ff) | (pixel & 0xff00ff00);
}

#if USE(ACCELERATED_COMPOSITING)
void GraphicsContext3DPrivate::paintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& matrix, float opacity)
{
    m_context->markLayerComposited();
    blitMultisampleFramebufferAndRestoreContext();

    if (textureMapper->accelerationMode() == TextureMapper::OpenGLMode) {
        TextureMapperGL* texmapGL = static_cast<TextureMapperGL*>(textureMapper);
#if USE(GRAPHICS_SURFACE)
        ASSERT(m_graphicsSurface);
        // CGL only provides us the context, but not the view the context is currently bound to.
        // To make sure the context is bound the the right surface we have to do a makeCurrent through QOpenGL again.
        // FIXME: Remove this code as soon as GraphicsSurfaceMac makes use of NSOpenGL.
        QOpenGLContext* currentContext = QOpenGLContext::currentContext();
        QSurface* currentSurface = currentContext->surface();
        makeCurrentIfNeeded();

        m_graphicsSurface->copyFromTexture(m_context->m_texture, IntRect(0, 0, m_context->m_currentWidth, m_context->m_currentHeight));

        // CGL only provides us the context, but not the view the context is currently bound to.
        // To make sure the context is bound the the right surface we have to do a makeCurrent through QOpenGL again.
        // FIXME: Remove this code as soon as GraphicsSurfaceMac makes use of NSOpenGL.
        currentContext->makeCurrent(currentSurface);

        m_graphicsSurface->paintToTextureMapper(texmapGL, targetRect, matrix, opacity);
#else
        TextureMapperGL::Flags flags = TextureMapperGL::ShouldFlipTexture | (m_context->m_attrs.alpha ? TextureMapperGL::ShouldBlend : 0);
        IntSize textureSize(m_context->m_currentWidth, m_context->m_currentHeight);
        texmapGL->drawTexture(m_context->m_texture, flags, textureSize, targetRect, matrix, opacity);
#endif
        return;
    }

    GraphicsContext* context = textureMapper->graphicsContext();
    QPainter* painter = context->platformContext();
    painter->save();
    painter->setTransform(matrix);
    painter->setOpacity(opacity);

    const int height = m_context->m_currentHeight;
    const int width = m_context->m_currentWidth;

    // Alternatively read pixels to a memory buffer.
    QImage offscreenImage(width, height, QImage::Format_ARGB32);
    quint32* imagePixels = reinterpret_cast<quint32*>(offscreenImage.bits());

    makeCurrentIfNeeded();
    glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_fbo);
    glReadPixels(/* x */ 0, /* y */ 0, width, height, GraphicsContext3D::RGBA, GraphicsContext3D::UNSIGNED_BYTE, imagePixels);
    glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_state.boundFBO);

    // OpenGL gives us ABGR on 32 bits, and with the origin at the bottom left
    // We need RGB32 or ARGB32_PM, with the origin at the top left.
    quint32* pixelsSrc = imagePixels;
    const int halfHeight = height / 2;
    for (int row = 0; row < halfHeight; ++row) {
        const int targetIdx = (height - 1 - row) * width;
        quint32* pixelsDst = imagePixels + targetIdx;
        for (int column = 0; column < width; ++column) {
            quint32 tempPixel = *pixelsSrc;
            *pixelsSrc = swapBgrToRgb(*pixelsDst);
            *pixelsDst = swapBgrToRgb(tempPixel);
            ++pixelsSrc;
            ++pixelsDst;
        }
    }
    if (static_cast<int>(height) % 2) {
        for (int column = 0; column < width; ++column) {
            *pixelsSrc = swapBgrToRgb(*pixelsSrc);
            ++pixelsSrc;
        }
    }

    painter->drawImage(targetRect, offscreenImage);
    painter->restore();
}
#endif // USE(ACCELERATED_COMPOSITING)

#if USE(GRAPHICS_SURFACE)
IntSize GraphicsContext3DPrivate::platformLayerSize() const
{
    return IntSize(m_context->m_currentWidth, m_context->m_currentHeight);
}

uint32_t GraphicsContext3DPrivate::copyToGraphicsSurface()
{
    if (!m_graphicsSurface)
        return 0;

    m_context->markLayerComposited();
    blitMultisampleFramebufferAndRestoreContext();
    m_graphicsSurface->copyFromTexture(m_context->m_texture, IntRect(0, 0, m_context->m_currentWidth, m_context->m_currentHeight));
    return m_graphicsSurface->frontBuffer();
}

GraphicsSurfaceToken GraphicsContext3DPrivate::graphicsSurfaceToken() const
{
    return m_graphicsSurface->exportToken();
}
#endif

QRectF GraphicsContext3DPrivate::boundingRect() const
{
    return QRectF(QPointF(0, 0), QSizeF(m_context->m_currentWidth, m_context->m_currentHeight));
}

void GraphicsContext3DPrivate::blitMultisampleFramebuffer() const
{
    if (!m_context->m_attrs.antialias)
        return;
#if !USE(OPENGL_ES_2)
    glBindFramebuffer(GL_READ_FRAMEBUFFER_EXT, m_context->m_multisampleFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER_EXT, m_context->m_fbo);
    glBlitFramebuffer(0, 0, m_context->m_currentWidth, m_context->m_currentHeight, 0, 0, m_context->m_currentWidth, m_context->m_currentHeight, GL_COLOR_BUFFER_BIT, GL_LINEAR);
#endif
    glBindFramebuffer(GraphicsContext3D::FRAMEBUFFER, m_context->m_state.boundFBO);
}

void GraphicsContext3DPrivate::blitMultisampleFramebufferAndRestoreContext() const
{
    const QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    QSurface* currentSurface = 0;
    if (currentContext && currentContext != m_platformContext) {
        currentSurface = currentContext->surface();
        m_platformContext->makeCurrent(m_surface);
    }

    if (m_context->m_attrs.antialias)
        blitMultisampleFramebuffer();

    // While the context is still bound, make sure all the Framebuffer content is in finished state.
    // This is necessary as we are doing our own double buffering instead of using a drawable that provides swapBuffers.
    glFinish();

    if (currentContext && currentContext != m_platformContext)
        const_cast<QOpenGLContext*>(currentContext)->makeCurrent(currentSurface);
}

bool GraphicsContext3DPrivate::makeCurrentIfNeeded() const
{
    const QOpenGLContext* currentContext = QOpenGLContext::currentContext();
    if (currentContext == m_platformContext)
        return true;
    return m_platformContext->makeCurrent(m_surface);
}

void GraphicsContext3DPrivate::createGraphicsSurfaces(const IntSize& size)
{
#if USE(GRAPHICS_SURFACE)
    if (size.isEmpty())
        m_graphicsSurface.clear();
    else
        m_graphicsSurface = GraphicsSurface::create(size, m_surfaceFlags, m_platformContext);
#endif
}

PassRefPtr<GraphicsContext3D> GraphicsContext3D::create(GraphicsContext3D::Attributes attrs, HostWindow* hostWindow, GraphicsContext3D::RenderStyle renderStyle)
{
    // This implementation doesn't currently support rendering directly to the HostWindow.
    if (renderStyle == RenderDirectlyToHostWindow)
        return 0;
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
    , m_private(adoptPtr(new GraphicsContext3DPrivate(this, hostWindow, renderStyle)))
{
    validateAttributes();

    if (!m_private->m_surface || !m_private->m_platformContext) {
        LOG_ERROR("GraphicsContext3D: GL context creation failed.");
        m_private = nullptr;
        return;
    }

    static bool initialized = false;
    static bool success = true;
    if (!initialized) {
        success = initializeOpenGLShims();
        initialized = true;
    }
    if (!success) {
        m_private = nullptr;
        return;
    }

    if (renderStyle == RenderOffscreen)
        m_private->createOffscreenBuffers();

    m_private->initializeANGLE();

#if !USE(OPENGL_ES_2)
    glEnable(GL_POINT_SPRITE);
    glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);
#endif

    if (renderStyle != RenderToCurrentGLContext)
        glClearColor(0.0, 0.0, 0.0, 0.0);
}

GraphicsContext3D::~GraphicsContext3D()
{
    // If GraphicsContext3D init failed in constructor, m_private set to nullptr and no buffers are allocated.
    if (!m_private)
        return;

    makeContextCurrent();
    glDeleteTextures(1, &m_texture);
    glDeleteFramebuffers(1, &m_fbo);
    if (m_attrs.antialias) {
        glDeleteRenderbuffers(1, &m_multisampleColorBuffer);
        glDeleteFramebuffers(1, &m_multisampleFBO);
        if (m_attrs.stencil || m_attrs.depth)
            glDeleteRenderbuffers(1, &m_multisampleDepthStencilBuffer);
    } else if (m_attrs.stencil || m_attrs.depth) {
#if USE(OPENGL_ES_2)
        if (m_attrs.depth)
            glDeleteRenderbuffers(1, &m_depthBuffer);
        if (m_attrs.stencil)
            glDeleteRenderbuffers(1, &m_stencilBuffer);
#endif
        glDeleteRenderbuffers(1, &m_depthStencilBuffer);
    }
}

PlatformGraphicsContext3D GraphicsContext3D::platformGraphicsContext3D()
{
    return m_private->m_platformContext;
}

Platform3DObject GraphicsContext3D::platformTexture() const
{
    return m_texture;
}

#if USE(ACCELERATED_COMPOSITING)
PlatformLayer* GraphicsContext3D::platformLayer() const
{
    return m_private.get();
}
#endif

bool GraphicsContext3D::makeContextCurrent()
{
    if (!m_private || m_renderStyle == RenderToCurrentGLContext)
        return false;
    return m_private->makeCurrentIfNeeded();
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight,
                                      int canvasWidth, int canvasHeight, QPainter* context)
{
    QImage image(imagePixels, imageWidth, imageHeight, NativeImageQt::defaultFormatForAlphaEnabledImages());
    context->save();
    context->translate(0, imageHeight);
    context->scale(1, -1);
    context->setCompositionMode(QPainter::CompositionMode_Source);
    context->drawImage(QRect(0, 0, canvasWidth, -canvasHeight), image);
    context->restore();
}

#if USE(GRAPHICS_SURFACE)
void GraphicsContext3D::createGraphicsSurfaces(const IntSize& size)
{
    m_private->createGraphicsSurfaces(size);
}
#endif

GraphicsContext3D::ImageExtractor::~ImageExtractor()
{
}

bool GraphicsContext3D::ImageExtractor::extractImage(bool premultiplyAlpha, bool ignoreGammaAndColorProfile)
{
    UNUSED_PARAM(ignoreGammaAndColorProfile);
    if (!m_image)
        return false;

    // Is image already loaded? If not, load it.
    if (m_image->data())
        m_qtImage = QImage::fromData(reinterpret_cast<const uchar*>(m_image->data()->data()), m_image->data()->size());
    else {
        QPixmap* nativePixmap = m_image->nativeImageForCurrentFrame();
        if (!nativePixmap)
            return false;

        // With QPA, we can avoid a deep copy.
        m_qtImage = *nativePixmap->handle()->buffer();
    }

    m_alphaOp = AlphaDoNothing;
    switch (m_qtImage.format()) {
    case QImage::Format_RGB32:
        // For opaque images, we should not premultiply or unmultiply alpha.
        break;
    case QImage::Format_ARGB32:
        if (premultiplyAlpha)
            m_alphaOp = AlphaDoPremultiply;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        if (!premultiplyAlpha)
            m_alphaOp = AlphaDoUnmultiply;
        break;
    default:
        // The image has a format that is not supported in packPixels. We have to convert it here.
        m_qtImage = m_qtImage.convertToFormat(premultiplyAlpha ? QImage::Format_ARGB32_Premultiplied : QImage::Format_ARGB32);
        break;
    }

    m_imageWidth = m_image->width();
    m_imageHeight = m_image->height();
    if (!m_imageWidth || !m_imageHeight)
        return false;
    m_imagePixelData = m_qtImage.constBits();
    m_imageSourceFormat = DataFormatBGRA8;
    m_imageSourceUnpackAlignment = 0;

    return true;
}

void GraphicsContext3D::setContextLostCallback(PassOwnPtr<ContextLostCallback>)
{
}

void GraphicsContext3D::setErrorMessageCallback(PassOwnPtr<ErrorMessageCallback>)
{
}

}

#endif // USE(3D_GRAPHICS)
