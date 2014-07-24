/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

#if USE(GRAPHICS_SURFACE) && OS(DARWIN)
#include "TextureMapperGL.h"
#include <CoreFoundation/CFNumber.h>
#include <CGLContext.h>
#include <CGLCurrent.h>
#include <CGLIOSurface.h>
#include <IOSurface/IOSurface.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <mach/mach.h>

#if PLATFORM(QT)
#include <QGuiApplication>
#include <QOpenGLContext>
#include <qpa/qplatformnativeinterface.h>
#endif

namespace WebCore {

static uint32_t createTexture(IOSurfaceRef handle)
{
    GLuint texture = 0;
    GLuint format = GL_BGRA;
    GLuint type = GL_UNSIGNED_INT_8_8_8_8_REV;
    GLuint internalFormat = GL_RGBA;
    CGLContextObj context = CGLGetCurrentContext();
    if (!context)
        return 0;

    GLint prevTexture;
    glGetIntegerv(GL_TEXTURE_RECTANGLE_ARB, &prevTexture);

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texture);
    CGLError error = CGLTexImageIOSurface2D(context, GL_TEXTURE_RECTANGLE_ARB, internalFormat, IOSurfaceGetWidth(handle), IOSurfaceGetHeight(handle), format, type, handle, 0);
    if (error) {
        glDeleteTextures(1, &texture);
        texture = 0;
    }

    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, prevTexture);

    return texture;
}

struct GraphicsSurfacePrivate {
public:
    GraphicsSurfacePrivate(const GraphicsSurfaceToken& token, const IntSize& size)
        : m_context(0)
        , m_size(size)
        , m_token(token)
        , m_frontBufferTexture(0)
        , m_frontBufferReadTexture(0)
        , m_backBufferTexture(0)
        , m_backBufferReadTexture(0)
        , m_readFbo(0)
        , m_drawFbo(0)
    {
        m_frontBuffer = IOSurfaceLookupFromMachPort(m_token.frontBufferHandle);
        m_backBuffer = IOSurfaceLookupFromMachPort(m_token.backBufferHandle);
    }

    GraphicsSurfacePrivate(const PlatformGraphicsContext3D shareContext, const IntSize& size, GraphicsSurface::Flags flags)
        : m_context(0)
        , m_size(size)
        , m_frontBufferTexture(0)
        , m_frontBufferReadTexture(0)
        , m_backBufferTexture(0)
        , m_backBufferReadTexture(0)
        , m_readFbo(0)
        , m_drawFbo(0)
    {
#if PLATFORM(QT)
        QPlatformNativeInterface* nativeInterface = QGuiApplication::platformNativeInterface();
        CGLContextObj shareContextObject = static_cast<CGLContextObj>(nativeInterface->nativeResourceForContext(QByteArrayLiteral("cglContextObj"), shareContext));
        if (!shareContextObject)
            return;

        CGLPixelFormatObj pixelFormatObject = CGLGetPixelFormat(shareContextObject);
        if (kCGLNoError != CGLCreateContext(pixelFormatObject, shareContextObject, &m_context))
            return;

        CGLRetainContext(m_context);
#endif

        unsigned pixelFormat = 'BGRA';
        unsigned bytesPerElement = 4;
        int width = m_size.width();
        int height = m_size.height();

        unsigned long bytesPerRow = IOSurfaceAlignProperty(kIOSurfaceBytesPerRow, width * bytesPerElement);
        if (!bytesPerRow)
            return;

        unsigned long allocSize = IOSurfaceAlignProperty(kIOSurfaceAllocSize, height * bytesPerRow);
        if (!allocSize)
            return;

        const void *keys[6];
        const void *values[6];
        keys[0] = kIOSurfaceWidth;
        values[0] = CFNumberCreate(0, kCFNumberIntType, &width);
        keys[1] = kIOSurfaceHeight;
        values[1] = CFNumberCreate(0, kCFNumberIntType, &height);
        keys[2] = kIOSurfacePixelFormat;
        values[2] = CFNumberCreate(0, kCFNumberIntType, &pixelFormat);
        keys[3] = kIOSurfaceBytesPerElement;
        values[3] = CFNumberCreate(0, kCFNumberIntType, &bytesPerElement);
        keys[4] = kIOSurfaceBytesPerRow;
        values[4] = CFNumberCreate(0, kCFNumberLongType, &bytesPerRow);
        keys[5] = kIOSurfaceAllocSize;
        values[5] = CFNumberCreate(0, kCFNumberLongType, &allocSize);

        CFDictionaryRef dict = CFDictionaryCreate(0, keys, values, 6, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        for (unsigned i = 0; i < 6; i++)
            CFRelease(values[i]);

        m_frontBuffer = IOSurfaceCreate(dict);
        m_backBuffer = IOSurfaceCreate(dict);

        if (!(flags & GraphicsSurface::SupportsSharing))
            return;

        m_token = GraphicsSurfaceToken(IOSurfaceCreateMachPort(m_frontBuffer), IOSurfaceCreateMachPort(m_backBuffer));
    }

    ~GraphicsSurfacePrivate()
    {
        if (m_frontBufferTexture)
            glDeleteTextures(1, &m_frontBufferTexture);

        if (m_frontBufferReadTexture)
            glDeleteTextures(1, &m_frontBufferReadTexture);

        if (m_backBufferTexture)
            glDeleteTextures(1, &m_backBufferTexture);

        if (m_backBufferReadTexture)
            glDeleteTextures(1, &m_backBufferReadTexture);

        if (m_frontBuffer)
            CFRelease(IOSurfaceRef(m_frontBuffer));

        if (m_backBuffer)
            CFRelease(IOSurfaceRef(m_backBuffer));

        if (m_readFbo)
            glDeleteFramebuffers(1, &m_readFbo);

        if (m_drawFbo)
            glDeleteFramebuffers(1, &m_drawFbo);

        if (m_context)
            CGLReleaseContext(m_context);

        if (m_token.frontBufferHandle)
            mach_port_deallocate(mach_task_self(), m_token.frontBufferHandle);
        if (m_token.backBufferHandle)
            mach_port_deallocate(mach_task_self(), m_token.backBufferHandle);

    }

    uint32_t swapBuffers()
    {
        std::swap(m_frontBuffer, m_backBuffer);
        std::swap(m_frontBufferTexture, m_backBufferTexture);
        std::swap(m_frontBufferReadTexture, m_backBufferReadTexture);

        return IOSurfaceGetID(m_frontBuffer);
    }

    void makeCurrent()
    {
        m_detachedContext = CGLGetCurrentContext();

        if (m_context)
            CGLSetCurrentContext(m_context);
    }

    void doneCurrent()
    {
        CGLSetCurrentContext(m_detachedContext);
        m_detachedContext = 0;
    }

    void copyFromTexture(uint32_t texture, const IntRect& sourceRect)
    {
        // FIXME: The following glFlush can possibly be replaced by using the GL_ARB_sync extension.
        glFlush(); // Make sure the texture has actually been completely written in the original context.

        makeCurrent();

        int x = sourceRect.x();
        int y = sourceRect.y();
        int width = sourceRect.width();
        int height = sourceRect.height();

        glPushAttrib(GL_ALL_ATTRIB_BITS);
        GLint previousFBO;
        glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFBO);

        if (!m_drawFbo)
            glGenFramebuffers(1, &m_drawFbo);

        if (!m_readFbo)
            glGenFramebuffers(1, &m_readFbo);

        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_readFbo);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_drawFbo);
        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, backBufferTextureID(), 0);
        glBlitFramebuffer(x, y, width, height, x, x+height, y+width, y, GL_COLOR_BUFFER_BIT, GL_LINEAR); // Flip the texture upside down.

        glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, 0, 0);
        glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, 0, 0);
        glBindFramebuffer(GL_FRAMEBUFFER, previousFBO);
        glPopAttrib();

        // Flushing the gl command buffer is necessary to ensure the texture has correctly been bound to the IOSurface.
        glFlush();

        swapBuffers();
        doneCurrent();
    }

    GraphicsSurfaceToken token() const
    {
        return m_token;
    }

    uint32_t frontBufferTextureID()
    {
        if (!m_frontBufferReadTexture)
            m_frontBufferReadTexture = createTexture(m_frontBuffer);

        return m_frontBufferReadTexture;
    }

    uint32_t backBufferTextureID()
    {
        if (!m_backBufferTexture)
            m_backBufferTexture = createTexture(m_backBuffer);

        return m_backBufferTexture;
    }

    PlatformGraphicsSurface frontBuffer() const
    {
        return m_frontBuffer;
    }

    PlatformGraphicsSurface backBuffer() const
    {
        return m_backBuffer;
    }

    IntSize size() const
    {
        return m_size;
    }

private:
    CGLContextObj m_context;
    IntSize m_size;
    CGLContextObj m_detachedContext;
    PlatformGraphicsSurface m_frontBuffer;
    PlatformGraphicsSurface m_backBuffer;
    uint32_t m_frontBufferTexture;
    uint32_t m_frontBufferReadTexture;
    uint32_t m_backBufferTexture;
    uint32_t m_backBufferReadTexture;
    uint32_t m_readFbo;
    uint32_t m_drawFbo;
    GraphicsSurfaceToken m_token;
};

GraphicsSurfaceToken GraphicsSurface::platformExport()
{
    return m_private->token();
}

uint32_t GraphicsSurface::platformGetTextureID()
{
    return m_private->frontBufferTextureID();
}

void GraphicsSurface::platformCopyToGLTexture(uint32_t target, uint32_t id, const IntRect& targetRect, const IntPoint& offset)
{
    glPushAttrib(GL_ALL_ATTRIB_BITS);
    if (!m_fbo)
        glGenFramebuffers(1, &m_fbo);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
    glBindTexture(target, id);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, m_private->frontBufferTextureID(), 0);
    glCopyTexSubImage2D(target, 0, targetRect.x(), targetRect.y(), offset.x(), offset.y(), targetRect.width(), targetRect.height());
    glFramebufferTexture2D(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_RECTANGLE_ARB, 0, 0);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
    glPopAttrib();

    // According to IOSurface's documentation, glBindFramebuffer is the one triggering an update of the surface's cache.
    // If the web process starts rendering and unlocks the surface before this happens, we might copy contents
    // of the currently rendering frame on our texture instead of the previously completed frame.
    // Flush the command buffer to reduce the odds of this happening, this would not be necessary with double buffering.
    glFlush();
}

void GraphicsSurface::platformCopyFromTexture(uint32_t texture, const IntRect& sourceRect)
{
    m_private->copyFromTexture(texture, sourceRect);
}

void GraphicsSurface::platformPaintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    IntSize size = m_private->size();
    FloatRect rectOnContents(FloatPoint::zero(), size);
    TransformationMatrix adjustedTransform = transform;
    adjustedTransform.multiply(TransformationMatrix::rectToRect(rectOnContents, targetRect));
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(m_private->frontBufferTextureID(), TextureMapperGL::ShouldBlend | TextureMapperGL::ShouldUseARBTextureRect, size, rectOnContents, adjustedTransform, opacity);
}

uint32_t GraphicsSurface::platformFrontBuffer() const
{
    return IOSurfaceGetID(m_private->frontBuffer());
}

uint32_t GraphicsSurface::platformSwapBuffers()
{
    return m_private->swapBuffers();
}

IntSize GraphicsSurface::platformSize() const
{
    return m_private->size();
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(const IntSize& size, Flags flags, const PlatformGraphicsContext3D shareContext)
{
    // We currently disable support for CopyToTexture on Mac, because this is used for single buffered Tiles.
    // The single buffered nature of this requires a call to glFlush, as described in platformCopyToTexture.
    // This call blocks the GPU for about 40ms, which makes smooth animations impossible.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate(shareContext, size, flags);

    if (!surface->m_private->frontBuffer() || !surface->m_private->backBuffer())
        return PassRefPtr<GraphicsSurface>();

    return surface;
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, const GraphicsSurfaceToken& token)
{
    // We currently disable support for CopyToTexture on Mac, because this is used for single buffered Tiles.
    // The single buffered nature of this requires a call to glFlush, as described in platformCopyToTexture.
    // This call blocks the GPU for about 40ms, which makes smooth animations impossible.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate(token, size);

    if (!surface->m_private->frontBuffer() || !surface->m_private->backBuffer())
        return PassRefPtr<GraphicsSurface>();

    return surface;
}

static int ioSurfaceLockOptions(int lockOptions)
{
    int options = 0;
    if (lockOptions & GraphicsSurface::ReadOnly)
        options |= kIOSurfaceLockReadOnly;
    if (!(lockOptions & GraphicsSurface::RetainPixels))
        options |= kIOSurfaceLockAvoidSync;

    return options;
}

char* GraphicsSurface::platformLock(const IntRect& rect, int* outputStride, LockOptions lockOptions)
{
    // Locking is only necessary for single buffered use.
    // In this case we only have a front buffer, so we only lock this one.
    m_lockOptions = lockOptions;
    IOReturn status = IOSurfaceLock(m_private->frontBuffer(), ioSurfaceLockOptions(m_lockOptions), 0);
    if (status == kIOReturnCannotLock) {
        m_lockOptions |= RetainPixels;
        IOSurfaceLock(m_private->frontBuffer(), ioSurfaceLockOptions(m_lockOptions), 0);
    }

    int stride = IOSurfaceGetBytesPerRow(m_private->frontBuffer());
    if (outputStride)
        *outputStride = stride;

    char* base = static_cast<char*>(IOSurfaceGetBaseAddress(m_private->frontBuffer()));

    return base + stride * rect.y() + rect.x() * 4;
}

void GraphicsSurface::platformUnlock()
{
    IOSurfaceUnlock(m_private->frontBuffer(), ioSurfaceLockOptions(m_lockOptions), 0);
}

void GraphicsSurface::platformDestroy()
{
    if (m_fbo)
        glDeleteFramebuffers(1, &m_fbo);
    if (m_private)
        delete m_private;
    m_private = 0;
}

}
#endif
