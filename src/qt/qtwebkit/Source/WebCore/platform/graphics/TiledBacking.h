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

#ifndef TiledBacking_h
#define TiledBacking_h

#if PLATFORM(MAC)
OBJC_CLASS CALayer;
#endif

namespace WebCore {

class IntRect;

enum ScrollingModeIndication {
    MainThreadScrollingBecauseOfStyleIndication,
    MainThreadScrollingBecauseOfEventHandlersIndication,
    ThreadedScrollingIndication
};

class TiledBacking {
public:
    virtual ~TiledBacking() { }

    virtual void setVisibleRect(const FloatRect&) = 0;
    virtual FloatRect visibleRect() const = 0;
    virtual bool tilesWouldChangeForVisibleRect(const FloatRect&) const = 0;

    virtual void setExposedRect(const FloatRect&) = 0;
    virtual void setClipsToExposedRect(bool) = 0;
    virtual bool clipsToExposedRect() = 0;

    virtual void prepopulateRect(const FloatRect&) = 0;

    virtual void setIsInWindow(bool) = 0;

    enum {
        CoverageForVisibleArea = 0,
        CoverageForVerticalScrolling = 1 << 0,
        CoverageForHorizontalScrolling = 1 << 1,
        CoverageForSlowScrolling = 1 << 2, // Indicates that we expect to paint a lot on scrolling.
        CoverageForScrolling = CoverageForVerticalScrolling | CoverageForHorizontalScrolling
    };
    typedef unsigned TileCoverage;

    virtual void setTileCoverage(TileCoverage) = 0;
    virtual TileCoverage tileCoverage() const = 0;

    virtual IntSize tileSize() const = 0;

    virtual void forceRepaint() = 0;

    virtual void setScrollingPerformanceLoggingEnabled(bool) = 0;
    virtual bool scrollingPerformanceLoggingEnabled() const = 0;
    
    virtual void setAggressivelyRetainsTiles(bool) = 0;
    virtual bool aggressivelyRetainsTiles() const = 0;
    
    virtual void setUnparentsOffscreenTiles(bool) = 0;
    virtual bool unparentsOffscreenTiles() const = 0;
    
    virtual double retainedTileBackingStoreMemory() const = 0;

    // Exposed for testing
    virtual IntRect tileCoverageRect() const = 0;
    virtual IntRect tileGridExtent() const = 0;
    virtual void setScrollingModeIndication(ScrollingModeIndication) = 0;

#if PLATFORM(MAC)
    virtual CALayer *tiledScrollingIndicatorLayer() = 0;
#endif
};

} // namespace WebCore

#endif // TiledBacking_h
