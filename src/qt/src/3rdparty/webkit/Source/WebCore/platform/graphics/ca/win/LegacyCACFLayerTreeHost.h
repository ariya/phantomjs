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

#ifndef LegacyCACFLayerTreeHost_h
#define LegacyCACFLayerTreeHost_h

#if USE(ACCELERATED_COMPOSITING)

#include "CACFLayerTreeHost.h"

namespace WebCore {

// FIXME: Currently there is a LegacyCACFLayerTreeHost for each WebView and each
// has its own WKCACFContext and Direct3DDevice9, which is inefficient.
// (https://bugs.webkit.org/show_bug.cgi?id=31855)
class LegacyCACFLayerTreeHost : public CACFLayerTreeHost {
public:
    static PassRefPtr<LegacyCACFLayerTreeHost> create();
    virtual ~LegacyCACFLayerTreeHost();

private:
    LegacyCACFLayerTreeHost();

    void initD3DGeometry();

    // Call this when the device window has changed size or when IDirect3DDevice9::Present returns
    // D3DERR_DEVICELOST. Returns true if the device was recovered, false if rendering must be
    // aborted and reattempted soon.
    enum ResetReason { ChangedWindowSize, LostDevice };
    bool resetDevice(ResetReason);

    void renderSoon();
    void renderTimerFired(Timer<LegacyCACFLayerTreeHost>*);

    virtual void initializeContext(void* userData, PlatformCALayer*);
    virtual void resize();
    virtual bool createRenderer();
    virtual void destroyRenderer();
    virtual CFTimeInterval lastCommitTime() const;
    virtual void flushContext();
    virtual void contextDidChange();
    virtual void paint();
    virtual void render(const Vector<CGRect>& dirtyRects = Vector<CGRect>());

    Timer<LegacyCACFLayerTreeHost> m_renderTimer;
    COMPtr<IDirect3DDevice9> m_d3dDevice;
    WKCACFContext* m_context;
    bool m_mightBeAbleToCreateDeviceLater;
    bool m_mustResetLostDeviceBeforeRendering;

#ifndef NDEBUG
    bool m_printTree;
#endif
};

} // namespace WebCore

#endif // USE(ACCELERATED_COMPOSITING)

#endif // LegacyCACFLayerTreeHost_h
