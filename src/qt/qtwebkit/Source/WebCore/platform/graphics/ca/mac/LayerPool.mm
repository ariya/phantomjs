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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "LayerPool.h"

#include "Logging.h"
#include "WebTileLayer.h"
#include <wtf/CurrentTime.h>

namespace WebCore {
    
static const double capacityDecayTime = 5;
    
LayerPool::LayerPool()
    : m_totalBytes(0)
    , m_maxBytesForPool(48 * 1024 * 1024)
    , m_pruneTimer(this, &LayerPool::pruneTimerFired)
    , m_lastAddTime(0)
{
}

LayerPool* LayerPool::sharedPool()
{
    static LayerPool* sharedPool = new LayerPool;
    return sharedPool;
}

unsigned LayerPool::backingStoreBytesForSize(const IntSize& size)
{
    return size.width() * size.height() * 4;
}

LayerPool::LayerList& LayerPool::listOfLayersWithSize(const IntSize& size, AccessType accessType)
{
    HashMap<IntSize, LayerList>::iterator it = m_reuseLists.find(size);
    if (it == m_reuseLists.end()) {
        it = m_reuseLists.add(size, LayerList()).iterator;
        m_sizesInPruneOrder.append(size);
    } else if (accessType == MarkAsUsed) {
        m_sizesInPruneOrder.remove(m_sizesInPruneOrder.reverseFind(size));
        m_sizesInPruneOrder.append(size);
    }
    return it->value;
}

void LayerPool::addLayer(const RetainPtr<WebTileLayer>& layer)
{
    IntSize layerSize([layer.get() bounds].size);
    if (!canReuseLayerWithSize(layerSize))
        return;

    listOfLayersWithSize(layerSize).prepend(layer);
    m_totalBytes += backingStoreBytesForSize(layerSize);
    
    m_lastAddTime = currentTime();
    schedulePrune();
}

RetainPtr<WebTileLayer> LayerPool::takeLayerWithSize(const IntSize& size)
{
    if (!canReuseLayerWithSize(size))
        return nil;
    LayerList& reuseList = listOfLayersWithSize(size, MarkAsUsed);
    if (reuseList.isEmpty())
        return nil;
    m_totalBytes -= backingStoreBytesForSize(size);
    return reuseList.takeFirst();
}
    
unsigned LayerPool::decayedCapacity() const
{
    // Decay to one quarter over capacityDecayTime
    double timeSinceLastAdd = currentTime() - m_lastAddTime;
    if (timeSinceLastAdd > capacityDecayTime)
        return m_maxBytesForPool / 4;
    float decayProgess = float(timeSinceLastAdd / capacityDecayTime);
    return m_maxBytesForPool / 4 + m_maxBytesForPool * 3 / 4 * (1 - decayProgess);
}

void LayerPool::schedulePrune()
{
    if (m_pruneTimer.isActive())
        return;
    m_pruneTimer.startOneShot(1);
}

void LayerPool::pruneTimerFired(Timer<LayerPool>*)
{
    unsigned shrinkTo = decayedCapacity();
    while (m_totalBytes > shrinkTo) {
        ASSERT(!m_sizesInPruneOrder.isEmpty());
        IntSize sizeToDrop = m_sizesInPruneOrder.first();
        LayerList& oldestReuseList = m_reuseLists.find(sizeToDrop)->value;
        if (oldestReuseList.isEmpty()) {
            m_reuseLists.remove(sizeToDrop);
            m_sizesInPruneOrder.remove(0);
            continue;
        }

        m_totalBytes -= backingStoreBytesForSize(sizeToDrop);
        // The last element in the list is the oldest, hence most likely not to
        // still have a backing store.
        oldestReuseList.remove(--oldestReuseList.end());
    }
    if (currentTime() - m_lastAddTime <= capacityDecayTime)
        schedulePrune();
}

void LayerPool::drain()
{
    m_reuseLists.clear();
    m_sizesInPruneOrder.clear();
    m_totalBytes = 0;
}

}
