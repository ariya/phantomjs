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

#if USE(ACCELERATED_COMPOSITING)

#include "SelectionOverlay.h"

#include "Frame.h"
#include "GraphicsContext.h"
#include "LayerMessage.h"
#include "LayerWebKitThread.h"
#include "Path.h"
#include "RenderTheme.h"
#include "RenderView.h"
#include "WebPage_p.h"

#include <BlackBerryPlatformMessageClient.h>

using namespace WebCore;

namespace BlackBerry {
namespace WebKit {

SelectionOverlay::SelectionOverlay(WebPagePrivate* page)
    : m_page(page)
{
}

SelectionOverlay::~SelectionOverlay()
{
}

void SelectionOverlay::draw(const Selection& selection)
{
    ASSERT(BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread());

    m_selection = selection;

    while (m_layers.size() < static_cast<size_t>(m_selection.size()))
        m_layers.append(GraphicsLayer::create(this));

    m_layers.resize(m_selection.size());

    size_t i = 0;
    for (Selection::iterator it = m_selection.begin(); it != m_selection.end(); ++it, ++i) {
        GraphicsLayer* parent = it->key;
        GraphicsLayer* overlay = m_layers[i].get();

        parent->platformLayer()->addOverlay(overlay->platformLayer());
        overlay->setPosition(FloatPoint::zero());
        if (parent == m_page->m_overlayLayer)
            overlay->setSize(m_page->contentsSize());
        else
            overlay->setSize(parent->size());
        overlay->setAnchorPoint(FloatPoint3D(0, 0, 0));
        overlay->setDrawsContent(true);
        overlay->setNeedsDisplay();
    }
}

void SelectionOverlay::hide()
{
    ASSERT(BlackBerry::Platform::webKitThreadMessageClient()->isCurrentThread());

    for (size_t i = 0; i < m_layers.size(); ++i)
        m_layers[i]->platformLayer()->removeFromSuperlayer();
    m_selection.clear();
}

void SelectionOverlay::notifyFlushRequired(const GraphicsLayer* layer)
{
    m_page->notifyFlushRequired(layer);
}

void SelectionOverlay::paintContents(const GraphicsLayer* layer, GraphicsContext& c, GraphicsLayerPaintingPhase, const IntRect& inClip)
{
    if (!layer->platformLayer()->superlayer())
        return;

    Selection::iterator it = m_selection.find(layer->platformLayer()->superlayer()->owner());
    if (it == m_selection.end())
        return;

    const Vector<WebCore::FloatQuad>& quads = it->value;
    GraphicsLayer* parent = it->key;

    IntRect clip(inClip);
    clip.move(parent->offsetFromRenderer().width(), parent->offsetFromRenderer().height());

    c.save();

    c.translate(-parent->offsetFromRenderer().width(), -parent->offsetFromRenderer().height());

    Color color = RenderTheme::defaultTheme()->activeSelectionBackgroundColor();
    c.setFillColor(color, ColorSpaceDeviceRGB);

    for (unsigned i = 0; i < quads.size(); ++i) {
        FloatRect rectToPaint = quads[i].boundingBox();

        // Selection on non-composited sub-frames need to be adjusted.
        if (!m_page->focusedOrMainFrame()->contentRenderer()->isComposited()) {
            WebCore::IntPoint framePosition = m_page->frameOffset(m_page->focusedOrMainFrame());
            rectToPaint.move(framePosition.x(), framePosition.y());
        }

        rectToPaint.intersect(clip);
        if (rectToPaint.isEmpty())
            continue;

        c.fillRect(rectToPaint);
    }

    c.restore();
}

} // namespace WebKit
} // namespace BlackBerry

#endif // USE(ACCELERATED_COMPOSITING)
