/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef DrawingAreaProxyImpl_h
#define DrawingAreaProxyImpl_h

#include "BackingStore.h"
#include "DrawingAreaProxy.h"
#include "LayerTreeContext.h"
#include <WebCore/RunLoop.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {
class Region;
}

namespace WebKit {

class CoordinatedLayerTreeHostProxy;

class DrawingAreaProxyImpl : public DrawingAreaProxy {
public:
    static PassOwnPtr<DrawingAreaProxyImpl> create(WebPageProxy*);
    virtual ~DrawingAreaProxyImpl();

    void paint(BackingStore::PlatformGraphicsContext, const WebCore::IntRect&, WebCore::Region& unpaintedRegion);

#if USE(ACCELERATED_COMPOSITING)
    bool isInAcceleratedCompositingMode() const { return !m_layerTreeContext.isEmpty(); }
#endif

    bool hasReceivedFirstUpdate() const { return m_hasReceivedFirstUpdate; }

private:
    explicit DrawingAreaProxyImpl(WebPageProxy*);

    // DrawingAreaProxy
    virtual void sizeDidChange();
    virtual void deviceScaleFactorDidChange();
    virtual void layerHostingModeDidChange() OVERRIDE;

    virtual void visibilityDidChange();
    virtual void setBackingStoreIsDiscardable(bool);
    virtual void waitForBackingStoreUpdateOnNextPaint();

    // CoreIPC message handlers
    virtual void update(uint64_t backingStoreStateID, const UpdateInfo&);
    virtual void didUpdateBackingStoreState(uint64_t backingStoreStateID, const UpdateInfo&, const LayerTreeContext&);
    virtual void enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&);
    virtual void exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo&);
    virtual void updateAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&);

    void incorporateUpdate(const UpdateInfo&);

    enum RespondImmediatelyOrNot { DoNotRespondImmediately, RespondImmediately };
    void backingStoreStateDidChange(RespondImmediatelyOrNot);
    void sendUpdateBackingStoreState(RespondImmediatelyOrNot);
    void waitForAndDispatchDidUpdateBackingStoreState();

#if USE(ACCELERATED_COMPOSITING)
    void enterAcceleratedCompositingMode(const LayerTreeContext&);
    void exitAcceleratedCompositingMode();
    void updateAcceleratedCompositingMode(const LayerTreeContext&);
#if USE(COORDINATED_GRAPHICS)
    virtual void setVisibleContentsRect(const WebCore::FloatRect& visibleContentsRect, const WebCore::FloatPoint& trajectory) OVERRIDE;
#endif
#else
    bool isInAcceleratedCompositingMode() const { return false; }
#endif

    void discardBackingStoreSoon();
    void discardBackingStore();

    // The state ID corresponding to our current backing store. Updated whenever we allocate
    // a new backing store. Any messages received that correspond to an earlier state are ignored,
    // as they don't apply to our current backing store.
    uint64_t m_currentBackingStoreStateID;

    // The next backing store state ID we will request the web process update to. Incremented
    // whenever our state changes in a way that will require a new backing store to be allocated.
    uint64_t m_nextBackingStoreStateID;

#if USE(ACCELERATED_COMPOSITING)
    // The current layer tree context.
    LayerTreeContext m_layerTreeContext;
#endif

    // Whether we've sent a UpdateBackingStoreState message and are now waiting for a DidUpdateBackingStoreState message.
    // Used to throttle UpdateBackingStoreState messages so we don't send them faster than the Web process can handle.
    bool m_isWaitingForDidUpdateBackingStoreState;
    
    // For a new Drawing Area don't draw anything until the WebProcess has sent over the first content.
    bool m_hasReceivedFirstUpdate;

    bool m_isBackingStoreDiscardable;
    OwnPtr<BackingStore> m_backingStore;

    WebCore::RunLoop::Timer<DrawingAreaProxyImpl> m_discardBackingStoreTimer;
};

} // namespace WebKit

#endif // DrawingAreaProxyImpl_h
