/*
    Copyright (C) 2012 Samsung Electronics

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

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL)

#include "AcceleratedCompositingContextEfl.h"

#include "FrameView.h"
#include "GraphicsContext3D.h"
#include "GraphicsLayerTextureMapper.h"
#include "HostWindow.h"
#include "PageClientEfl.h"
#include "TextureMapperGL.h"
#include "TextureMapperLayer.h"
#include "ewk_view_private.h"

namespace WebCore {

PassOwnPtr<AcceleratedCompositingContext> AcceleratedCompositingContext::create(HostWindow* hostWindow)
{
    OwnPtr<AcceleratedCompositingContext> context = adoptPtr(new AcceleratedCompositingContext);
    if (!context->initialize(hostWindow))
        return nullptr;

    return context.release();
}

AcceleratedCompositingContext::AcceleratedCompositingContext()
    : m_view(0)
    , m_rootTextureMapperLayer(0)
{
}

AcceleratedCompositingContext::~AcceleratedCompositingContext()
{
}

bool AcceleratedCompositingContext::initialize(HostWindow* hostWindow)
{
    m_view = hostWindow->platformPageClient()->view();
    if (!m_view)
        return false;

    m_context3D = GraphicsContext3D::create(GraphicsContext3D::Attributes(), hostWindow, WebCore::GraphicsContext3D::RenderDirectlyToHostWindow);
    if (!m_context3D)
        return false;

    return true;
}

void AcceleratedCompositingContext::syncLayersNow()
{
    if (m_rootGraphicsLayer)
        m_rootGraphicsLayer->flushCompositingStateForThisLayerOnly();

    EWKPrivate::corePage(m_view)->mainFrame()->view()->flushCompositingStateIncludingSubframes();
}

void AcceleratedCompositingContext::renderLayers()
{
    if (!m_rootGraphicsLayer)
        return;

    if (!m_context3D->makeContextCurrent())
        return;

    int width = 0;
    int height = 0;
    evas_object_geometry_get(m_view, 0, 0, &width, &height);
    m_context3D->viewport(0, 0, width, height);

    m_textureMapper->beginPainting();
    m_rootTextureMapperLayer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get());
    m_textureMapper->endPainting();
}

void AcceleratedCompositingContext::attachRootGraphicsLayer(GraphicsLayer* rootLayer)
{
    if (!rootLayer) {
        m_rootGraphicsLayer.clear();
        m_rootTextureMapperLayer = 0;
        return;
    }

    m_rootGraphicsLayer = WebCore::GraphicsLayer::create(0);
    m_rootTextureMapperLayer = toTextureMapperLayer(m_rootGraphicsLayer.get());
    m_rootGraphicsLayer->addChild(rootLayer);
    m_rootGraphicsLayer->setDrawsContent(false);
    m_rootGraphicsLayer->setMasksToBounds(false);
    m_rootGraphicsLayer->setSize(WebCore::IntSize(1, 1));

    m_textureMapper = TextureMapperGL::create();
    m_rootTextureMapperLayer->setTextureMapper(m_textureMapper.get());

    m_rootGraphicsLayer->flushCompositingStateForThisLayerOnly();
}

GraphicsContext3D* AcceleratedCompositingContext::context()
{
    return m_context3D.get();
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER_GL)
