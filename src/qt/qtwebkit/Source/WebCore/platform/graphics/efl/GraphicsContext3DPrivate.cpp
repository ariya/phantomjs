/*
    Copyright (C) 2012 Samsung Electronics
    Copyright (C) 2012 Intel Corporation. All rights reserved.

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
#include "GraphicsContext3DPrivate.h"

#if USE(3D_GRAPHICS) || USE(ACCELERATED_COMPOSITING)

#include "HostWindow.h"
#include "NotImplemented.h"

namespace WebCore {

PassOwnPtr<GraphicsContext3DPrivate> GraphicsContext3DPrivate::create(GraphicsContext3D* context, HostWindow* hostWindow)
{
    OwnPtr<GraphicsContext3DPrivate> platformLayer = adoptPtr(new GraphicsContext3DPrivate(context, hostWindow));

    if (platformLayer && platformLayer->initialize())
        return platformLayer.release();

    return nullptr;
}

GraphicsContext3DPrivate::GraphicsContext3DPrivate(GraphicsContext3D* context, HostWindow* hostWindow)
    : m_context(context)
    , m_hostWindow(hostWindow)
{
}

bool GraphicsContext3DPrivate::initialize()
{
    if (m_context->m_renderStyle == GraphicsContext3D::RenderDirectlyToHostWindow)
        return false;

    if (m_hostWindow && m_hostWindow->platformPageClient()) {
        // FIXME: Implement this code path for WebKit1.
        // Get Evas object from platformPageClient and set EvasGL related members.
        return false;
    }

    m_offScreenContext = GLPlatformContext::createContext(m_context->m_renderStyle);
    if (!m_offScreenContext)
        return false;

    if (m_context->m_renderStyle == GraphicsContext3D::RenderOffscreen) {
        m_offScreenSurface = GLPlatformSurface::createOffScreenSurface();

        if (!m_offScreenSurface)
            return false;

        if (!m_offScreenContext->initialize(m_offScreenSurface.get()))
            return false;

        if (!makeContextCurrent())
            return false;
#if USE(GRAPHICS_SURFACE)
        m_surfaceOperation = CreateSurface;
#endif
    }

    return true;
}

GraphicsContext3DPrivate::~GraphicsContext3DPrivate()
{
    releaseResources();
}

void GraphicsContext3DPrivate::releaseResources()
{
    if (m_context->m_renderStyle == GraphicsContext3D::RenderToCurrentGLContext)
        return;

    // Release the current context and drawable only after destroying any associated gl resources.
#if USE(GRAPHICS_SURFACE)
    if (m_previousGraphicsSurface)
        m_previousGraphicsSurface = nullptr;

    if (m_graphicsSurface)
        m_graphicsSurface = nullptr;

    m_surfaceHandle = GraphicsSurfaceToken();
#endif
    if (m_offScreenSurface)
        m_offScreenSurface->destroy();

    if (m_offScreenContext) {
        m_offScreenContext->destroy();
        m_offScreenContext->releaseCurrent();
    }
}

bool GraphicsContext3DPrivate::createSurface(PageClientEfl*, bool)
{
    notImplemented();
    return false;
}

void GraphicsContext3DPrivate::setContextLostCallback(PassOwnPtr<GraphicsContext3D::ContextLostCallback> callBack)
{
    m_contextLostCallback = callBack;
}

PlatformGraphicsContext3D GraphicsContext3DPrivate::platformGraphicsContext3D() const
{
    return m_offScreenContext->handle();
}

bool GraphicsContext3DPrivate::makeContextCurrent() const
{
    bool success = m_offScreenContext->makeCurrent(m_offScreenSurface.get());

    if (!m_offScreenContext->isValid()) {
        // FIXME: Restore context
        if (m_contextLostCallback)
            m_contextLostCallback->onContextLost();

        return false;
    }

    return success;
}

bool GraphicsContext3DPrivate::prepareBuffer() const
{
    if (!makeContextCurrent())
        return false;

    m_context->markLayerComposited();

    if (m_context->m_attrs.antialias) {
        bool enableScissorTest = false;
        int width = m_context->m_currentWidth;
        int height = m_context->m_currentHeight;
        // We should copy the full buffer, and not respect the current scissor bounds.
        // FIXME: It would be more efficient to track the state of the scissor test.
        if (m_context->isEnabled(GraphicsContext3D::SCISSOR_TEST)) {
            enableScissorTest = true;
            m_context->disable(GraphicsContext3D::SCISSOR_TEST);
        }

        glBindFramebuffer(Extensions3D::READ_FRAMEBUFFER, m_context->m_multisampleFBO);
        glBindFramebuffer(Extensions3D::DRAW_FRAMEBUFFER, m_context->m_fbo);

        // Use NEAREST as no scale is performed during the blit.
        m_context->getExtensions()->blitFramebuffer(0, 0, width, height, 0, 0, width, height, GraphicsContext3D::COLOR_BUFFER_BIT, GraphicsContext3D::NEAREST);

        if (enableScissorTest)
            m_context->enable(GraphicsContext3D::SCISSOR_TEST);

        glBindFramebuffer(GL_FRAMEBUFFER, m_context->m_state.boundFBO);
    }

    return true;
}

#if USE(TEXTURE_MAPPER_GL)
void GraphicsContext3DPrivate::paintToTextureMapper(TextureMapper*, const FloatRect& /* target */, const TransformationMatrix&, float /* opacity */)
{
    notImplemented();
}
#endif

#if USE(GRAPHICS_SURFACE)
void GraphicsContext3DPrivate::createGraphicsSurface()
{
    static PendingSurfaceOperation pendingOperation = DeletePreviousSurface | Resize | CreateSurface;
    if (!(m_surfaceOperation & pendingOperation))
        return;

    if (m_surfaceOperation & DeletePreviousSurface) {
        m_previousGraphicsSurface = nullptr;
        m_surfaceOperation &= ~DeletePreviousSurface;
    }

    if (!(m_surfaceOperation & pendingOperation))
        return;

    // Don't release current graphics surface until we have prepared surface
    // with requested size. This is to avoid flashing during resize.
    if (m_surfaceOperation & Resize) {
        m_previousGraphicsSurface = m_graphicsSurface;
        m_surfaceOperation &= ~Resize;
        m_surfaceOperation |= DeletePreviousSurface;
        m_size = IntSize(m_context->m_currentWidth, m_context->m_currentHeight);
    } else
        m_surfaceOperation &= ~CreateSurface;

    m_targetRect = IntRect(IntPoint(), m_size);

    if (m_size.isEmpty()) {
        if (m_graphicsSurface) {
            m_graphicsSurface = nullptr;
            m_surfaceHandle = GraphicsSurfaceToken();
            makeContextCurrent();
        }

        return;
    }

    m_offScreenContext->releaseCurrent();
    GraphicsSurface::Flags flags = GraphicsSurface::SupportsTextureTarget | GraphicsSurface::SupportsSharing;

    if (m_context->m_attrs.alpha)
        flags |= GraphicsSurface::SupportsAlpha;

    m_graphicsSurface = GraphicsSurface::create(m_size, flags, m_offScreenContext->handle());

    if (!m_graphicsSurface)
        m_surfaceHandle = GraphicsSurfaceToken();
    else
        m_surfaceHandle = GraphicsSurfaceToken(m_graphicsSurface->exportToken());

    makeContextCurrent();
}

void GraphicsContext3DPrivate::didResizeCanvas(const IntSize& size)
{
    if (m_surfaceOperation & CreateSurface) {
        m_size = size;
        createGraphicsSurface();
        return;
    }

    m_surfaceOperation |= Resize;
}

uint32_t GraphicsContext3DPrivate::copyToGraphicsSurface()
{
    createGraphicsSurface();

    if (!m_graphicsSurface || m_context->m_layerComposited || !prepareBuffer())
        return 0;

    m_graphicsSurface->copyFromTexture(m_context->m_texture, m_targetRect);
    makeContextCurrent();

    return m_graphicsSurface->frontBuffer();
}

GraphicsSurfaceToken GraphicsContext3DPrivate::graphicsSurfaceToken() const
{
    return m_surfaceHandle;
}

IntSize GraphicsContext3DPrivate::platformLayerSize() const
{
    return m_size;
}

GraphicsSurface::Flags GraphicsContext3DPrivate::graphicsSurfaceFlags() const
{
    if (m_graphicsSurface)
        return m_graphicsSurface->flags();

    return TextureMapperPlatformLayer::graphicsSurfaceFlags();
}
#endif

} // namespace WebCore

#endif // USE(3D_GRAPHICS) || USE(ACCELERATED_COMPOSITING)
