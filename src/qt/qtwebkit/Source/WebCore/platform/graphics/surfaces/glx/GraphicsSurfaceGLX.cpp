/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)
 Copyright (C) 2013 Intel Corporation.

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
#include "GraphicsSurface.h"

#if USE(GRAPHICS_SURFACE)

#include "NotImplemented.h"
#include "TextureMapperGL.h"

#if PLATFORM(QT)
// Qt headers must be included before glx headers.
#include <QGuiApplication>
#include <QOpenGLContext>
#include <qpa/qplatformnativeinterface.h>
#include <GL/glext.h>
#include <GL/glx.h>
#else
#include <opengl/GLDefs.h>
#endif

#include "GLXConfigSelector.h"

namespace WebCore {

static PFNGLXBINDTEXIMAGEEXTPROC pGlXBindTexImageEXT = 0;
static PFNGLXRELEASETEXIMAGEEXTPROC pGlXReleaseTexImageEXT = 0;
static PFNGLBINDFRAMEBUFFERPROC pGlBindFramebuffer = 0;
static PFNGLBLITFRAMEBUFFERPROC pGlBlitFramebuffer = 0;
static PFNGLGENFRAMEBUFFERSPROC pGlGenFramebuffers = 0;
static PFNGLDELETEFRAMEBUFFERSPROC pGlDeleteFramebuffers = 0;
static PFNGLFRAMEBUFFERTEXTURE2DPROC pGlFramebufferTexture2D = 0;

static int glxAttributes[] = {
    GLX_TEXTURE_FORMAT_EXT,
    GLX_TEXTURE_FORMAT_RGBA_EXT,
    GLX_TEXTURE_TARGET_EXT,
    GLX_TEXTURE_2D_EXT,
    0
};

static bool isMesaGLX()
{
    static bool isMesa = !!strstr(glXGetClientString(X11Helper::nativeDisplay(), GLX_VENDOR), "Mesa");
    return isMesa;
}

struct GraphicsSurfacePrivate {
    GraphicsSurfacePrivate(const PlatformGraphicsContext3D shareContext = 0)
        : m_xPixmap(0)
        , m_glxPixmap(0)
        , m_surface(0)
        , m_glxSurface(0)
        , m_glContext(0)
        , m_detachedContext(0)
        , m_detachedSurface(0)
        , m_isReceiver(false)
        , m_texture(0)
    {
        GLXContext shareContextObject = 0;

#if PLATFORM(QT)
        if (shareContext) {
            QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
            shareContextObject = static_cast<GLXContext>(nativeInterface->nativeResourceForContext(QByteArrayLiteral("glxcontext"), shareContext));
            if (!shareContextObject)
                return;
        }
#else
        UNUSED_PARAM(shareContext);
#endif

        GLPlatformSurface::SurfaceAttributes sharedSurfaceAttributes = GLPlatformSurface::DoubleBuffered |
            GLPlatformSurface::SupportAlpha;
        m_configSelector = adoptPtr(new GLXConfigSelector(sharedSurfaceAttributes));

        if (!m_configSelector->surfaceContextConfig()) {
            clear();
            return;
        }

        // Create a GLX context for OpenGL rendering
        m_glContext = glXCreateNewContext(display(), m_configSelector->surfaceContextConfig(), GLX_RGBA_TYPE, shareContextObject, true);
    }

    GraphicsSurfacePrivate(uint32_t winId)
        : m_xPixmap(0)
        , m_glxPixmap(0)
        , m_surface(winId)
        , m_glxSurface(0)
        , m_glContext(0)
        , m_detachedContext(0)
        , m_detachedSurface(0)
        , m_isReceiver(true)
        , m_texture(0)
    {
    }

    ~GraphicsSurfacePrivate()
    {
        clear();
    }

    uint32_t createSurface(const IntSize& size)
    {
        if (!display() || !m_configSelector)
            return 0;

        GLXFBConfig config = m_configSelector->surfaceContextConfig();
        OwnPtrX11<XVisualInfo> visInfo(m_configSelector->visualInfo(config));

        if (!visInfo.get()) {
            clear();
            return 0;
        }

        X11Helper::createOffScreenWindow(&m_surface, *visInfo.get(), size);

        if (!m_surface) {
            clear();
            return 0;
        }

        m_glxSurface = glXCreateWindow(display(), config, m_surface, 0);
        return m_surface;
    }

    void createPixmap(uint32_t winId)
    {
        XWindowAttributes attr;
        if (!XGetWindowAttributes(display(), winId, &attr))
            return;

        // Ensure that the window is mapped.
        if (attr.map_state == IsUnmapped || attr.map_state == IsUnviewable)
            return;

        ScopedXPixmapCreationErrorHandler handler;
        m_size = IntSize(attr.width, attr.height);

        XRenderPictFormat* format = XRenderFindVisualFormat(display(), attr.visual);
        bool hasAlpha = (format->type == PictTypeDirect && format->direct.alphaMask);
        m_xPixmap = XCompositeNameWindowPixmap(display(), winId);
        glxAttributes[1] = (format->depth == 32 && hasAlpha) ? GLX_TEXTURE_FORMAT_RGBA_EXT : GLX_TEXTURE_FORMAT_RGB_EXT;

        GLPlatformSurface::SurfaceAttributes sharedSurfaceAttributes = GLPlatformSurface::Default;
        if (hasAlpha)
            sharedSurfaceAttributes = GLPlatformSurface::SupportAlpha;

        if (!m_configSelector)
            m_configSelector = adoptPtr(new GLXConfigSelector(sharedSurfaceAttributes));

        m_glxPixmap = glXCreatePixmap(display(), m_configSelector->surfaceClientConfig(format->depth, XVisualIDFromVisual(attr.visual)), m_xPixmap, glxAttributes);

        if (!handler.isValidOperation())
            clear();
        else {
            uint inverted = 0;
            glXQueryDrawable(display(), m_glxPixmap, GLX_Y_INVERTED_EXT, &inverted);
            m_flags = !!inverted ? TextureMapperGL::ShouldFlipTexture : 0;

            if (hasAlpha)
                m_flags |= TextureMapperGL::ShouldBlend;
        }
    }

    void makeCurrent()
    {
        m_detachedContext = glXGetCurrentContext();
        m_detachedSurface = glXGetCurrentDrawable();
        if (m_surface && m_glContext)
            glXMakeCurrent(display(), m_surface, m_glContext);
    }

    void doneCurrent()
    {
        if (m_detachedContext)
            glXMakeCurrent(display(), m_detachedSurface, m_detachedContext);
        m_detachedContext = 0;
    }

    void swapBuffers()
    {
        if (isReceiver()) {
            if (isMesaGLX() && textureID()) {
                glBindTexture(GL_TEXTURE_2D, textureID());
                // Mesa doesn't re-bind texture to the front buffer on glXSwapBufer
                // Manually release previous lock and rebind texture to surface to ensure frame updates.
                pGlXReleaseTexImageEXT(display(), glxPixmap(), GLX_FRONT_EXT);
                pGlXBindTexImageEXT(display(), glxPixmap(), GLX_FRONT_EXT, 0);
            }

            return;
        }

        GLXContext glContext = glXGetCurrentContext();

        if (m_surface && glContext) {
            GLint oldFBO;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &oldFBO);
            pGlBindFramebuffer(GL_FRAMEBUFFER, 0);
            glXSwapBuffers(display(), m_surface);
            pGlBindFramebuffer(GL_FRAMEBUFFER, oldFBO);
        }
    }

    void copyFromTexture(uint32_t texture, const IntRect& sourceRect)
    {
        makeCurrent();
        int x = sourceRect.x();
        int y = sourceRect.y();
        int width = sourceRect.width();
        int height = sourceRect.height();

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        GLint previousFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

        GLuint originFBO;
        pGlGenFramebuffers(1, &originFBO);
        pGlBindFramebuffer(GL_READ_FRAMEBUFFER, originFBO);
        glBindTexture(GL_TEXTURE_2D, texture);
        pGlFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        pGlBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
        pGlBlitFramebuffer(x, y, width, height, x, y, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);

        pGlFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindTexture(GL_TEXTURE_2D, 0);
        pGlBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
        pGlDeleteFramebuffers(1, &originFBO);

        glPopAttrib();

        swapBuffers();
        doneCurrent();
    }

    Display* display() const { return X11Helper::nativeDisplay(); }

    GLXPixmap glxPixmap() const
    {
        if (!m_glxPixmap && m_surface)
            const_cast<GraphicsSurfacePrivate*>(this)->createPixmap(m_surface);
        return m_glxPixmap;
    }

    IntSize size() const
    {
        if (m_size.isEmpty()) {
            XWindowAttributes attr;
            if (XGetWindowAttributes(display(), m_surface, &attr))
                const_cast<GraphicsSurfacePrivate*>(this)->m_size = IntSize(attr.width, attr.height);
        }
        return m_size;
    }

    bool isReceiver() const { return m_isReceiver; }

    TextureMapperGL::Flags flags() const { return m_flags; }

    Window surface() const { return m_surface; }

    GLuint textureID() const
    {
        if (m_texture) 
            return m_texture;

        GLXPixmap pixmap = glxPixmap();
        if (!pixmap)
            return 0;

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        pGlXBindTexImageEXT(display(), pixmap, GLX_FRONT_EXT, 0);
        const_cast<GraphicsSurfacePrivate*>(this)->m_texture = texture;

        return texture;
    }
private:
    void clear()
    {
        if (m_texture) {
            pGlXReleaseTexImageEXT(display(), glxPixmap(), GLX_FRONT_EXT);
            glDeleteTextures(1, &m_texture);
        }

        if (m_glxPixmap) {
            glXDestroyPixmap(display(), m_glxPixmap);
            m_glxPixmap = 0;
        }

        if (m_xPixmap) {
            XFreePixmap(display(), m_xPixmap);
            m_xPixmap = 0;
        }

        // Client doesn't own the window. Delete surface only on writing side.
        if (!m_isReceiver && m_surface) {
            XDestroyWindow(display(), m_surface);
            m_surface = 0;
        }

        if (m_glContext) {
            glXDestroyContext(display(), m_glContext);
            m_glContext = 0;
        }

        if (m_configSelector)
            m_configSelector = nullptr;
    }

    IntSize m_size;
    Pixmap m_xPixmap;
    GLXPixmap m_glxPixmap;
    uint32_t m_surface;
    Window m_glxSurface;
    GLXContext m_glContext;
    GLXContext m_detachedContext;
    GLXDrawable m_detachedSurface;
    OwnPtr<GLXConfigSelector> m_configSelector;
    bool m_isReceiver;
    TextureMapperGL::Flags m_flags;
    GLuint m_texture;
};

static bool resolveGLMethods()
{
    static bool resolved = false;
    if (resolved)
        return true;
    pGlXBindTexImageEXT = reinterpret_cast<PFNGLXBINDTEXIMAGEEXTPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXBindTexImageEXT")));
    pGlXReleaseTexImageEXT = reinterpret_cast<PFNGLXRELEASETEXIMAGEEXTPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glXReleaseTexImageEXT")));
    pGlBindFramebuffer = reinterpret_cast<PFNGLBINDFRAMEBUFFERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBindFramebuffer")));
    pGlBlitFramebuffer = reinterpret_cast<PFNGLBLITFRAMEBUFFERPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glBlitFramebuffer")));

    pGlGenFramebuffers = reinterpret_cast<PFNGLGENFRAMEBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glGenFramebuffers")));
    pGlDeleteFramebuffers = reinterpret_cast<PFNGLDELETEFRAMEBUFFERSPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glDeleteFramebuffers")));
    pGlFramebufferTexture2D = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DPROC>(glXGetProcAddress(reinterpret_cast<const GLubyte*>("glFramebufferTexture2D")));
    resolved = pGlBlitFramebuffer && pGlBindFramebuffer && pGlXBindTexImageEXT && pGlXReleaseTexImageEXT;

    return resolved;
}

GraphicsSurfaceToken GraphicsSurface::platformExport()
{
    return GraphicsSurfaceToken(m_private->surface());
}

uint32_t GraphicsSurface::platformGetTextureID()
{
    return m_private->textureID();
}

void GraphicsSurface::platformCopyToGLTexture(uint32_t /*target*/, uint32_t /*id*/, const IntRect& /*targetRect*/, const IntPoint& /*offset*/)
{
    // This is not supported by GLX/Xcomposite.
}

void GraphicsSurface::platformCopyFromTexture(uint32_t texture, const IntRect& sourceRect)
{
    m_private->copyFromTexture(texture, sourceRect);
}

void GraphicsSurface::platformPaintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    IntSize size = m_private->size();
    if (size.isEmpty())
        return;
    uint32_t texture = platformGetTextureID();
    if (!texture)
        return;

    FloatRect rectOnContents(FloatPoint::zero(), size);
    TransformationMatrix adjustedTransform = transform;
    adjustedTransform.multiply(TransformationMatrix::rectToRect(rectOnContents, targetRect));
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(texture, m_private->flags(), size, rectOnContents, adjustedTransform, opacity);
}

uint32_t GraphicsSurface::platformFrontBuffer() const
{
    return 0;
}

uint32_t GraphicsSurface::platformSwapBuffers()
{
    m_private->swapBuffers();
    return 0;
}

IntSize GraphicsSurface::platformSize() const
{
    return m_private->size();
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(const IntSize& size, Flags flags, const PlatformGraphicsContext3D shareContext)
{
    // X11 does not support CopyToTexture, so we do not create a GraphicsSurface if this is requested.
    // GraphicsSurfaceGLX uses an XWindow as native surface. This one always has a front and a back buffer.
    // Therefore single buffered GraphicsSurfaces are not supported.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));

    surface->m_private = new GraphicsSurfacePrivate(shareContext);
    if (!resolveGLMethods())
        return PassRefPtr<GraphicsSurface>();

    surface->m_private->createSurface(size);

    return surface;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, const GraphicsSurfaceToken& token)
{
    // X11 does not support CopyToTexture, so we do not create a GraphicsSurface if this is requested.
    // GraphicsSurfaceGLX uses an XWindow as native surface. This one always has a front and a back buffer.
    // Therefore single buffered GraphicsSurfaces are not supported.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));

    surface->m_private = new GraphicsSurfacePrivate(token.frontBufferHandle);
    if (!resolveGLMethods())
        return PassRefPtr<GraphicsSurface>();

    return surface;
}

char* GraphicsSurface::platformLock(const IntRect&, int* /*outputStride*/, LockOptions)
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
    return 0;
}

void GraphicsSurface::platformUnlock()
{
    // GraphicsSurface is currently only being used for WebGL, which does not require this locking mechanism.
}

void GraphicsSurface::platformDestroy()
{
    delete m_private;
    m_private = 0;
}

#if !PLATFORM(QT)
PassOwnPtr<GraphicsContext> GraphicsSurface::platformBeginPaint(const IntSize&, char*, int)
{
    notImplemented();
    return nullptr;
}

PassRefPtr<Image> GraphicsSurface::createReadOnlyImage(const IntRect&)
{
    notImplemented();
    return 0;
}
#endif
}
#endif
