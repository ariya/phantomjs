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

#import "config.h"
#import "RemoteLayerTreeContext.h"

#import "RemoteGraphicsLayer.h"
#import "RemoteLayerTreeTransaction.h"
#import "RemoteLayerTreeHostMessages.h"
#import "WebPage.h"
#import <WebCore/Frame.h>
#import <WebCore/FrameView.h>
#import <WebCore/Page.h>
#import <wtf/PassOwnPtr.h>
#import <wtf/TemporaryChange.h>

using namespace WebCore;

namespace WebKit {

PassOwnPtr<RemoteLayerTreeContext> RemoteLayerTreeContext::create(WebPage* webPage)
{
    return adoptPtr(new RemoteLayerTreeContext(webPage));
}

RemoteLayerTreeContext::RemoteLayerTreeContext(WebPage* webPage)
    : m_webPage(webPage)
    , m_layerFlushTimer(this, &RemoteLayerTreeContext::layerFlushTimerFired)
    , m_rootLayerID(0)
    , m_currentTransaction(nullptr)
{
}

RemoteLayerTreeContext::~RemoteLayerTreeContext()
{
}

void RemoteLayerTreeContext::setRootLayer(GraphicsLayer* rootLayer)
{
    ASSERT(rootLayer);

    m_rootLayerID = static_cast<RemoteGraphicsLayer*>(rootLayer)->layerID();
}

void RemoteLayerTreeContext::layerWillBeDestroyed(RemoteGraphicsLayer* graphicsLayer)
{
    ASSERT(!m_destroyedLayers.contains(graphicsLayer->layerID()));

    m_destroyedLayers.append(graphicsLayer->layerID());
}

void RemoteLayerTreeContext::scheduleLayerFlush()
{
    if (m_layerFlushTimer.isActive())
        return;

    m_layerFlushTimer.startOneShot(0);
}

RemoteLayerTreeTransaction& RemoteLayerTreeContext::currentTransaction()
{
    ASSERT(m_currentTransaction);

    return *m_currentTransaction;
}

PassOwnPtr<GraphicsLayer> RemoteLayerTreeContext::createGraphicsLayer(GraphicsLayerClient* client)
{
    return RemoteGraphicsLayer::create(client, this);
}

void RemoteLayerTreeContext::layerFlushTimerFired(WebCore::Timer<RemoteLayerTreeContext>*)
{
    flushLayers();
}

void RemoteLayerTreeContext::flushLayers()
{
    ASSERT(!m_currentTransaction);

    RemoteLayerTreeTransaction transaction;
    transaction.setRootLayerID(m_rootLayerID);
    transaction.setDestroyedLayerIDs(std::move(m_destroyedLayers));

    TemporaryChange<RemoteLayerTreeTransaction*> transactionChange(m_currentTransaction, &transaction);

    m_webPage->layoutIfNeeded();
    m_webPage->corePage()->mainFrame()->view()->flushCompositingStateIncludingSubframes();

    m_webPage->send(Messages::RemoteLayerTreeHost::Commit(transaction));
}

} // namespace WebKit
