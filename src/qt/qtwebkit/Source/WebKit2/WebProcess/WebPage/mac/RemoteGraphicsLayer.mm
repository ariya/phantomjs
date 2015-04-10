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
#include "RemoteGraphicsLayer.h"

#include "RemoteLayerTreeContext.h"
#include "RemoteLayerTreeTransaction.h"

#include <WebCore/FloatRect.h>
#include <wtf/text/CString.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateLayerID()
{
    static uint64_t layerID;
    return ++layerID;
}

PassOwnPtr<GraphicsLayer> RemoteGraphicsLayer::create(GraphicsLayerClient* client, RemoteLayerTreeContext* context)
{
    return adoptPtr(new RemoteGraphicsLayer(client, context));
}

RemoteGraphicsLayer::RemoteGraphicsLayer(GraphicsLayerClient* client, RemoteLayerTreeContext* context)
    : GraphicsLayer(client)
    , m_layerID(generateLayerID())
    , m_context(context)
    , m_uncommittedLayerChanges(RemoteLayerTreeTransaction::NoChange)
{
}

RemoteGraphicsLayer::~RemoteGraphicsLayer()
{
    willBeDestroyed();
}

void RemoteGraphicsLayer::setName(const String& name)
{
    String longName = String::format("RemoteGraphicsLayer(%p) ", this) + name;
    GraphicsLayer::setName(longName);

    noteLayerPropertiesChanged(RemoteLayerTreeTransaction::NameChanged);
}

bool RemoteGraphicsLayer::setChildren(const Vector<GraphicsLayer*>& children)
{
    if (GraphicsLayer::setChildren(children)) {
        noteSublayersChanged();
        return true;
    }

    return false;
}

void RemoteGraphicsLayer::addChild(GraphicsLayer* childLayer)
{
    GraphicsLayer::addChild(childLayer);
    noteSublayersChanged();
}

void RemoteGraphicsLayer::addChildAtIndex(GraphicsLayer* childLayer, int index)
{
    GraphicsLayer::addChildAtIndex(childLayer, index);
    noteSublayersChanged();
}

void RemoteGraphicsLayer::addChildAbove(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildAbove(childLayer, sibling);
    noteSublayersChanged();
}

void RemoteGraphicsLayer::addChildBelow(GraphicsLayer* childLayer, GraphicsLayer* sibling)
{
    GraphicsLayer::addChildBelow(childLayer, sibling);
    noteSublayersChanged();
}

bool RemoteGraphicsLayer::replaceChild(GraphicsLayer* oldChild, GraphicsLayer* newChild)
{
    if (GraphicsLayer::replaceChild(oldChild, newChild)) {
        noteSublayersChanged();
        return true;
    }

    return false;
}

void RemoteGraphicsLayer::removeFromParent()
{
    if (m_parent)
        static_cast<RemoteGraphicsLayer*>(m_parent)->noteSublayersChanged();
    GraphicsLayer::removeFromParent();
}

void RemoteGraphicsLayer::setPosition(const FloatPoint& position)
{
    if (position == m_position)
        return;

    GraphicsLayer::setPosition(position);
    noteLayerPropertiesChanged(RemoteLayerTreeTransaction::PositionChanged);
}

void RemoteGraphicsLayer::setSize(const FloatSize& size)
{
    if (size == m_size)
        return;

    GraphicsLayer::setSize(size);
    noteLayerPropertiesChanged(RemoteLayerTreeTransaction::SizeChanged);
}

void RemoteGraphicsLayer::setNeedsDisplay()
{
    FloatRect hugeRect(-std::numeric_limits<float>::max() / 2, -std::numeric_limits<float>::max() / 2,
                       std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    setNeedsDisplayInRect(hugeRect);
}

void RemoteGraphicsLayer::setNeedsDisplayInRect(const FloatRect&)
{
    // FIXME: Implement this.
}

void RemoteGraphicsLayer::flushCompositingState(const FloatRect&)
{
    recursiveCommitChanges();
}

void RemoteGraphicsLayer::flushCompositingStateForThisLayerOnly()
{
    if (!m_uncommittedLayerChanges)
        return;

    m_context->currentTransaction().layerPropertiesChanged(this, m_uncommittedLayerChanges);

    m_uncommittedLayerChanges = RemoteLayerTreeTransaction::NoChange;
}

void RemoteGraphicsLayer::willBeDestroyed()
{
    m_context->layerWillBeDestroyed(this);
    GraphicsLayer::willBeDestroyed();
}

void RemoteGraphicsLayer::noteLayerPropertiesChanged(unsigned layerChanges)
{
    if (!m_uncommittedLayerChanges && m_client)
        m_client->notifyFlushRequired(this);
    m_uncommittedLayerChanges |= layerChanges;
}

void RemoteGraphicsLayer::noteSublayersChanged()
{
    noteLayerPropertiesChanged(RemoteLayerTreeTransaction::ChildrenChanged);

    // FIXME: Handle replica layers.
}

void RemoteGraphicsLayer::recursiveCommitChanges()
{
    flushCompositingStateForThisLayerOnly();

    for (size_t i = 0; i < children().size(); ++i) {
        RemoteGraphicsLayer* graphicsLayer = static_cast<RemoteGraphicsLayer*>(children()[i]);
        graphicsLayer->recursiveCommitChanges();
    }
}

} // namespace WebKit
