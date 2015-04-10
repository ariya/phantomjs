/*
 * Copyright (C) 2013 Intel Corporation. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GraphicsSurface.h"

#if USE(GRAPHICS_SURFACE)

#include "GLPlatformContext.h"
#include "GLTransportSurface.h"
#include "NotImplemented.h"
#include "TextureMapperGL.h"

namespace WebCore {

struct GraphicsSurfacePrivate {

    GraphicsSurfacePrivate()
    {
    }

    GraphicsSurfacePrivate(PlatformBufferHandle winId, const IntSize& size, GraphicsSurface::Flags flags)
        : m_flags(0)
        , m_rect(FloatPoint::zero(), size)
        , m_size(size)
        , m_sharedHandle(winId)
    {
        if (flags & GraphicsSurface::SupportsAlpha)
            m_flags |= TextureMapperGL::ShouldBlend;
    }

    ~GraphicsSurfacePrivate()
    {
    }

    void destroy()
    {
        if (m_client)
            m_client->destroy();

        if (m_sharedContext && m_sharedContext->handle() && m_sharedSurface)
            makeContextCurrent();

        if (m_sharedSurface)
            m_sharedSurface->destroy();

        if (m_sharedContext) {
            m_sharedContext->destroy();
            m_sharedContext->releaseCurrent();
        }
    }

    bool initializeTransportSurface(const IntSize& size, GraphicsSurface::Flags flags, const PlatformGraphicsContext3D shareContext)
    {
        GLPlatformSurface::SurfaceAttributes sharedSurfaceAttributes = GLPlatformSurface::Default;
        m_size = size;

        if (flags & GraphicsSurface::SupportsAlpha)
            sharedSurfaceAttributes = GLPlatformSurface::SupportAlpha;

        m_sharedSurface = GLTransportSurface::createTransportSurface(size, sharedSurfaceAttributes);
        if (!m_sharedSurface)
            return false;

        m_sharedContext = GLPlatformContext::createContext(GraphicsContext3D::RenderOffscreen);
        if (!m_sharedContext)
            return false;

        if (!m_sharedContext->initialize(m_sharedSurface.get(), static_cast<PlatformContext>(shareContext)))
            return false;

        if (!makeContextCurrent())
            return false;

        return true;
    }

    bool makeContextCurrent() const
    {
        return m_sharedContext->makeCurrent(m_sharedSurface.get());
    }

    void copyFromTexture(GLuint textureId)
    {
        if (!makeContextCurrent())
            return;

        m_sharedSurface->updateContents(textureId);
    }

    PlatformBufferHandle handle() const
    {
        return m_sharedSurface->handle();
    }

    // Client
    void updateClientBuffer()
    {
        if (!m_client)
            return;

        m_client->prepareTexture();
    }

    TextureMapperGL::Flags flags() const { return m_flags; }

    const FloatRect& rect() const { return m_rect; }
    const IntSize& size() const { return m_size; }

    GLuint textureId() const
    {
        if (!m_client)
            const_cast<GraphicsSurfacePrivate*>(this)->initializeClient();

        return m_client ? m_client->texture() : 0;
    }

private:
    void initializeClient()
    {
        m_client = GLTransportSurfaceClient::createTransportSurfaceClient(m_sharedHandle, m_size, m_flags & TextureMapperGL::ShouldBlend);

        if (!m_client)
            return;
    }

    TextureMapperGL::Flags m_flags;
    FloatRect m_rect;
    IntSize m_size;
    PlatformBufferHandle m_sharedHandle;
    OwnPtr<GLTransportSurfaceClient> m_client;
    OwnPtr<GLPlatformContext> m_sharedContext;
    OwnPtr<GLTransportSurface> m_sharedSurface;
};

GraphicsSurfaceToken GraphicsSurface::platformExport()
{
    return m_private->handle();
}

uint32_t GraphicsSurface::platformGetTextureID()
{
    return m_private->textureId();
}

void GraphicsSurface::platformCopyToGLTexture(uint32_t /*target*/, uint32_t /*id*/, const IntRect& /*targetRect*/, const IntPoint& /*offset*/)
{
    notImplemented();
}

void GraphicsSurface::platformCopyFromTexture(uint32_t textureId, const IntRect&)
{
    if (!m_private)
        return;

    m_private->copyFromTexture(textureId);
}

void GraphicsSurface::platformPaintToTextureMapper(TextureMapper* textureMapper, const FloatRect& targetRect, const TransformationMatrix& transform, float opacity)
{
    uint32_t texture = platformGetTextureID();
    if (!texture)
        return;

    TransformationMatrix adjustedTransform = transform;
    adjustedTransform.multiply(TransformationMatrix::rectToRect(m_private->rect(), targetRect));
    static_cast<TextureMapperGL*>(textureMapper)->drawTexture(texture, m_private->flags(), m_private->size(), m_private->rect(), adjustedTransform, opacity);
}

uint32_t GraphicsSurface::platformFrontBuffer() const
{
    return 0;
}

uint32_t GraphicsSurface::platformSwapBuffers()
{
    m_private->updateClientBuffer();
    return 0;
}

IntSize GraphicsSurface::platformSize() const
{
    return m_private->size();
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformCreate(const IntSize& size, Flags flags, const PlatformGraphicsContext3D shareContext)
{
    // GraphicsSurface doesn't yet support copyToTexture or singlebuffered surface.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate();

    if (surface->m_private->initializeTransportSurface(size, flags, shareContext))
        return surface;

    return PassRefPtr<GraphicsSurface>();
}

PassRefPtr<GraphicsSurface> GraphicsSurface::platformImport(const IntSize& size, Flags flags, const GraphicsSurfaceToken& token)
{
    // GraphicsSurface doesn't yet support copyToTexture or singlebuffered surface.
    if (flags & SupportsCopyToTexture || flags & SupportsSingleBuffered)
        return PassRefPtr<GraphicsSurface>();

    RefPtr<GraphicsSurface> surface = adoptRef(new GraphicsSurface(size, flags));
    surface->m_private = new GraphicsSurfacePrivate(token.frontBufferHandle, size, flags);
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
    if (!m_private)
        return;

    m_private->destroy();

    delete m_private;
    m_private = 0;
}

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

}

#endif
