/*
 * Copyright (C) 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"

#include "InspectorOverlayBlackBerry.h"

#include "Frame.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayer.h"
#include "WebPage_p.h"


namespace BlackBerry {
namespace WebKit {

PassOwnPtr<InspectorOverlay> InspectorOverlay::create(WebPagePrivate* page, InspectorOverlayClient* client)
{
    return adoptPtr(new InspectorOverlay(page, client));
}

InspectorOverlay::InspectorOverlay(WebPagePrivate* page, InspectorOverlayClient* client)
    : m_webPage(page)
    , m_client(client)
{
}

#if USE(ACCELERATED_COMPOSITING)
void InspectorOverlay::notifyFlushRequired(const WebCore::GraphicsLayer* layer)
{
    m_webPage->notifyFlushRequired(layer);
}

void InspectorOverlay::paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext& context, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect&)
{
    context.save();
    WebCore::IntPoint scrollPosition = m_webPage->focusedOrMainFrame()->view()->scrollPosition();
    context.translate(scrollPosition.x(), scrollPosition.y());
    m_client->paintInspectorOverlay(context);
    context.restore();
}

#endif

InspectorOverlay::~InspectorOverlay() { }

void InspectorOverlay::clear()
{
#if USE(ACCELERATED_COMPOSITING)
    if (m_overlay) {
        m_overlay->removeFromParent();
        m_overlay = nullptr;
    }
#endif
}

void InspectorOverlay::update()
{
#if USE(ACCELERATED_COMPOSITING)
    if (!m_overlay) {
        m_overlay = adoptPtr(new WebOverlay(this));
        const WebCore::IntSize size = m_webPage->contentsSize();
        m_overlay->setSize(WebCore::FloatSize(size.width(), size.height()));
        m_webPage->m_webPage->addOverlay(m_overlay.get());
    }

    m_overlay->setDrawsContent(true);
    m_overlay->setOpacity(1.0);
    m_overlay->invalidate();
#endif
}

}
}
