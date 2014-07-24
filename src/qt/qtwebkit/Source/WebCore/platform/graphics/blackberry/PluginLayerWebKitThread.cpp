/*
 * Copyright (C) 2010, 2011, 2012 Research In Motion Limited. All rights reserved.
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
#include "PluginLayerWebKitThread.h"

#if USE(ACCELERATED_COMPOSITING)

#include "LayerCompositingThread.h"
#include "PluginView.h"

namespace WebCore {

PluginLayerWebKitThread::PluginLayerWebKitThread(PluginView* pluginView)
    : LayerWebKitThread(Layer, 0)
    , m_needsDisplay(false)
{
    setPluginView(pluginView);
}

PluginLayerWebKitThread::~PluginLayerWebKitThread()
{
}

void PluginLayerWebKitThread::setPluginView(PluginView* pluginView)
{
    m_pluginView = pluginView;
    setNeedsTexture(isDrawable() && pluginView);
    setLayerProgram(LayerProgramRGBA);

    if (m_pluginView)
        setNeedsDisplay();
    else {
        // We can't afford to wait until the next commit
        // to set the m_pluginView to 0 in the impl, because it is
        // about to be destroyed.
        layerCompositingThread()->setPluginView(0);
        setNeedsCommit();
    }
}

void PluginLayerWebKitThread::setHolePunchRect(const IntRect& rect)
{
    m_holePunchRect = rect;
    setNeedsCommit();
}

void PluginLayerWebKitThread::setNeedsDisplay()
{
    m_needsDisplay = true;
    setNeedsCommit();
}

void PluginLayerWebKitThread::updateTextureContentsIfNeeded()
{
    if (!m_needsDisplay)
        return;

    m_needsDisplay = false;

    if (!m_pluginView)
        return;

    m_pluginView->updateBuffer(IntRect(IntPoint::zero(), m_pluginView->size()));
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
