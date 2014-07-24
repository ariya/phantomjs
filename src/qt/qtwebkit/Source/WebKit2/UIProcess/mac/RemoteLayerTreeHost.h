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

#ifndef RemoteLayerTreeHost_h
#define RemoteLayerTreeHost_h

#include "MessageReceiver.h"
#include <WebCore/GraphicsLayerClient.h>
#include <wtf/HashMap.h>

namespace WebKit {

class RemoteLayerTreeTransaction;
class WebPageProxy;

class RemoteLayerTreeHost : private CoreIPC::MessageReceiver, WebCore::GraphicsLayerClient {
public:
    explicit RemoteLayerTreeHost(WebPageProxy*);
    ~RemoteLayerTreeHost();

private:
    // CoreIPC::MessageReceiver.
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    // WebCore::GraphicsLayerClient.
    virtual void notifyAnimationStarted(const WebCore::GraphicsLayer*, double time) OVERRIDE;
    virtual void notifyFlushRequired(const WebCore::GraphicsLayer*) OVERRIDE;
    virtual void paintContents(const WebCore::GraphicsLayer*, WebCore::GraphicsContext&, WebCore::GraphicsLayerPaintingPhase, const WebCore::IntRect& clipRect) OVERRIDE;

    // Message handlers.
    void commit(const RemoteLayerTreeTransaction&);

    WebCore::GraphicsLayer* getOrCreateLayer(uint64_t layerID);

    WebPageProxy* m_webPageProxy;

    WebCore::GraphicsLayer* m_rootLayer;
    HashMap<uint64_t, OwnPtr<WebCore::GraphicsLayer>> m_layers;
};

} // namespace WebKit

#endif // RemoteLayerTreeHost_h
