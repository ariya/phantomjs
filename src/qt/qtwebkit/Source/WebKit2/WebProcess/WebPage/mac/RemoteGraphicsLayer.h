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

#ifndef RemoteGraphicsLayer_h
#define RemoteGraphicsLayer_h

#include <WebCore/GraphicsLayer.h>

namespace WebKit {

class RemoteLayerTreeContext;

class RemoteGraphicsLayer : public WebCore::GraphicsLayer {
public:
    static PassOwnPtr<WebCore::GraphicsLayer> create(WebCore::GraphicsLayerClient*, RemoteLayerTreeContext*);
    virtual ~RemoteGraphicsLayer();

    uint64_t layerID() const { return m_layerID; }

private:
    RemoteGraphicsLayer(WebCore::GraphicsLayerClient*, RemoteLayerTreeContext*);

    // WebCore::GraphicsLayer
    virtual void setName(const String&) OVERRIDE;

    virtual bool setChildren(const Vector<WebCore::GraphicsLayer*>&);
    virtual void addChild(WebCore::GraphicsLayer*);
    virtual void addChildAtIndex(WebCore::GraphicsLayer*, int index);
    virtual void addChildAbove(WebCore::GraphicsLayer* childLayer, WebCore::GraphicsLayer* sibling);
    virtual void addChildBelow(WebCore::GraphicsLayer* childLayer, WebCore::GraphicsLayer* sibling);
    virtual bool replaceChild(WebCore::GraphicsLayer* oldChild, WebCore::GraphicsLayer* newChild);

    virtual void removeFromParent() OVERRIDE;

    virtual void setPosition(const WebCore::FloatPoint&) OVERRIDE;
    virtual void setSize(const WebCore::FloatSize&) OVERRIDE;

    virtual void setNeedsDisplay() OVERRIDE;
    virtual void setNeedsDisplayInRect(const WebCore::FloatRect&) OVERRIDE;
    virtual void flushCompositingState(const WebCore::FloatRect&) OVERRIDE;
    virtual void flushCompositingStateForThisLayerOnly() OVERRIDE;

    virtual void willBeDestroyed() OVERRIDE;

    void noteLayerPropertiesChanged(unsigned layerChanges);
    void noteSublayersChanged();

    void recursiveCommitChanges();

    uint64_t m_layerID;
    RemoteLayerTreeContext* m_context;
    unsigned m_uncommittedLayerChanges;
};

} // namespace WebKit

#endif // RemoteGraphicsLayer_h
