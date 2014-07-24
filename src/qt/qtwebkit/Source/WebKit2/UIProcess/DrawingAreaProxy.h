/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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

#ifndef DrawingAreaProxy_h
#define DrawingAreaProxy_h

#include "BackingStore.h"
#include "DrawingAreaInfo.h"
#include "MessageReceiver.h"
#include <WebCore/FloatPoint.h>
#include <WebCore/IntRect.h>
#include <WebCore/IntSize.h>
#include <stdint.h>
#include <wtf/Noncopyable.h>

namespace WebKit {

class LayerTreeContext;
class CoordinatedLayerTreeHostProxy;
class UpdateInfo;
class WebPageProxy;

class DrawingAreaProxy : public CoreIPC::MessageReceiver {
    WTF_MAKE_NONCOPYABLE(DrawingAreaProxy);

public:
    virtual ~DrawingAreaProxy();

    DrawingAreaType type() const { return m_type; }

    virtual void deviceScaleFactorDidChange() = 0;

    // FIXME: These should be pure virtual.
    virtual void visibilityDidChange() { }
    virtual void layerHostingModeDidChange() { }

    virtual void setBackingStoreIsDiscardable(bool) { }

    virtual void waitForBackingStoreUpdateOnNextPaint() { }

    const WebCore::IntSize& size() const { return m_size; }
    void setSize(const WebCore::IntSize&, const WebCore::IntSize&, const WebCore::IntSize& scrollOffset);

    // The timeout, in seconds, we use when waiting for a DidUpdateGeometry message.
    static const double didUpdateBackingStoreStateTimeout;

    virtual void waitForPossibleGeometryUpdate(double = didUpdateBackingStoreStateTimeout) { }

    virtual void colorSpaceDidChange() { }
    virtual void minimumLayoutSizeDidChange() { }

#if USE(COORDINATED_GRAPHICS)
    virtual void updateViewport();
    virtual WebCore::IntRect viewportVisibleRect() const { return contentsRect(); }
    virtual WebCore::IntRect contentsRect() const;
    CoordinatedLayerTreeHostProxy* coordinatedLayerTreeHostProxy() const { return m_coordinatedLayerTreeHostProxy.get(); }
    virtual void setVisibleContentsRect(const WebCore::FloatRect& /* visibleContentsRect */, const WebCore::FloatPoint& /* trajectoryVector */) { }

    WebPageProxy* page() { return m_webPageProxy; }
#endif
protected:
    explicit DrawingAreaProxy(DrawingAreaType, WebPageProxy*);

    DrawingAreaType m_type;
    WebPageProxy* m_webPageProxy;

    WebCore::IntSize m_size;
    WebCore::IntSize m_layerPosition;
    WebCore::IntSize m_scrollOffset;

#if USE(COORDINATED_GRAPHICS)
    OwnPtr<CoordinatedLayerTreeHostProxy> m_coordinatedLayerTreeHostProxy;
#endif

private:
    virtual void sizeDidChange() = 0;

    // CoreIPC::MessageReceiver
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;

    // CoreIPC message handlers.
    // FIXME: These should be pure virtual.
    virtual void update(uint64_t /* backingStoreStateID */, const UpdateInfo&) { }
    virtual void didUpdateBackingStoreState(uint64_t /* backingStoreStateID */, const UpdateInfo&, const LayerTreeContext&) { }
#if USE(ACCELERATED_COMPOSITING)
    virtual void enterAcceleratedCompositingMode(uint64_t /* backingStoreStateID */, const LayerTreeContext&) { }
    virtual void exitAcceleratedCompositingMode(uint64_t /* backingStoreStateID */, const UpdateInfo&) { }
    virtual void updateAcceleratedCompositingMode(uint64_t /* backingStoreStateID */, const LayerTreeContext&) { }
#endif
#if PLATFORM(MAC)
    virtual void didUpdateGeometry() { }
    virtual void intrinsicContentSizeDidChange(const WebCore::IntSize& newIntrinsicContentSize) { }
#endif
};

} // namespace WebKit

#endif // DrawingAreaProxy_h
