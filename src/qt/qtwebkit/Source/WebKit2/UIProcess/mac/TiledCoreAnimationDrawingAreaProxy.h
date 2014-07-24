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

#ifndef TiledCoreAnimationDrawingAreaProxy_h
#define TiledCoreAnimationDrawingAreaProxy_h

#if ENABLE(THREADED_SCROLLING)

#include "DrawingAreaProxy.h"
#include <wtf/PassOwnPtr.h>

namespace WebKit {

class TiledCoreAnimationDrawingAreaProxy : public DrawingAreaProxy {
public:
    static PassOwnPtr<TiledCoreAnimationDrawingAreaProxy> create(WebPageProxy*);
    virtual ~TiledCoreAnimationDrawingAreaProxy();

private:
    explicit TiledCoreAnimationDrawingAreaProxy(WebPageProxy*);

    // DrawingAreaProxy
    virtual void deviceScaleFactorDidChange() OVERRIDE;
    virtual void layerHostingModeDidChange() OVERRIDE;
    virtual void visibilityDidChange() OVERRIDE;
    virtual void sizeDidChange() OVERRIDE;
    virtual void waitForPossibleGeometryUpdate(double timeout = didUpdateBackingStoreStateTimeout) OVERRIDE;
    virtual void colorSpaceDidChange() OVERRIDE;
    virtual void minimumLayoutSizeDidChange() OVERRIDE;

    virtual void enterAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&) OVERRIDE;
    virtual void exitAcceleratedCompositingMode(uint64_t backingStoreStateID, const UpdateInfo&) OVERRIDE;
    virtual void updateAcceleratedCompositingMode(uint64_t backingStoreStateID, const LayerTreeContext&) OVERRIDE;

    // Message handlers.
    virtual void didUpdateGeometry() OVERRIDE;
    virtual void intrinsicContentSizeDidChange(const WebCore::IntSize& newIntrinsicContentSize) OVERRIDE;

    void sendUpdateGeometry();

    // Whether we're waiting for a DidUpdateGeometry message from the web process.
    bool m_isWaitingForDidUpdateGeometry;

    // The last size we sent to the web process.
    WebCore::IntSize m_lastSentSize;
    WebCore::IntSize m_lastSentLayerPosition;

    // The last minimum layout size we sent to the web process.
    WebCore::IntSize m_lastSentMinimumLayoutSize;
};

} // namespace WebKit

#endif // ENABLE(THREADED_SCROLLING)

#endif // TiledCoreAnimationDrawingAreaProxy_h
