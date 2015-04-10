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
#include "RemoteLayerTreeHost.h"

#include "RemoteLayerTreeHostMessages.h"
#include "RemoteLayerTreeTransaction.h"
#include "WebPageProxy.h"
#include "WebProcessProxy.h"
#include <WebCore/GraphicsLayer.h>

using namespace WebCore;

namespace WebKit {

RemoteLayerTreeHost::RemoteLayerTreeHost(WebPageProxy* webPageProxy)
    : m_webPageProxy(webPageProxy)
    , m_rootLayer(nullptr)
{
    m_webPageProxy->process()->addMessageReceiver(Messages::RemoteLayerTreeHost::messageReceiverName(), m_webPageProxy->pageID(), this);
}

RemoteLayerTreeHost::~RemoteLayerTreeHost()
{
    m_webPageProxy->process()->removeMessageReceiver(Messages::RemoteLayerTreeHost::messageReceiverName(), m_webPageProxy->pageID());
}

void RemoteLayerTreeHost::notifyAnimationStarted(const GraphicsLayer*, double time)
{
}

void RemoteLayerTreeHost::notifyFlushRequired(const GraphicsLayer*)
{
}

void RemoteLayerTreeHost::paintContents(const GraphicsLayer*, GraphicsContext&, GraphicsLayerPaintingPhase, const IntRect&)
{
}

void RemoteLayerTreeHost::commit(const RemoteLayerTreeTransaction& transaction)
{
    GraphicsLayer* rootLayer = getOrCreateLayer(transaction.rootLayerID());
    if (m_rootLayer != rootLayer) {
        m_rootLayer = rootLayer;
        m_webPageProxy->setAcceleratedCompositingRootLayer(m_rootLayer);
    }

#ifndef NDEBUG
    // FIXME: Apply the transaction instead of dumping it to stderr.
    transaction.dump();
#endif
}

GraphicsLayer* RemoteLayerTreeHost::getOrCreateLayer(uint64_t layerID)
{
    auto addResult = m_layers.add(layerID, nullptr);
    if (addResult.isNewEntry)
        addResult.iterator->value = GraphicsLayer::create(this);

    return addResult.iterator->value.get();
}

} // namespace WebKit
