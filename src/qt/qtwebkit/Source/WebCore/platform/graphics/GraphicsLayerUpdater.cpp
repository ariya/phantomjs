/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#if USE(ACCELERATED_COMPOSITING)

#include "GraphicsLayerUpdater.h"

#include "GraphicsLayer.h"

namespace WebCore {

GraphicsLayerUpdater::GraphicsLayerUpdater(GraphicsLayerUpdaterClient* client, PlatformDisplayID displayID)
    : m_client(client)
    , m_scheduled(false)
{
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    DisplayRefreshMonitorManager::sharedManager()->registerClient(this);
    DisplayRefreshMonitorManager::sharedManager()->windowScreenDidChange(displayID, this);
    DisplayRefreshMonitorManager::sharedManager()->scheduleAnimation(this);
#else
    UNUSED_PARAM(displayID);
#endif
}

GraphicsLayerUpdater::~GraphicsLayerUpdater()
{
    // ~DisplayRefreshMonitorClient unregisters us as a client.
}

void GraphicsLayerUpdater::scheduleUpdate()
{
    if (m_scheduled)
        return;

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    DisplayRefreshMonitorManager::sharedManager()->scheduleAnimation(this);
#endif
    m_scheduled = true;
}

void GraphicsLayerUpdater::screenDidChange(PlatformDisplayID displayID)
{
#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    DisplayRefreshMonitorManager::sharedManager()->windowScreenDidChange(displayID, this);
#else
    UNUSED_PARAM(displayID);
#endif
}

void GraphicsLayerUpdater::displayRefreshFired(double timestamp)
{
    UNUSED_PARAM(timestamp);
    m_scheduled = false;
    
    if (m_client)
        m_client->flushLayers(this);
}

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)
