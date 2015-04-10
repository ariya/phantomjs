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

#ifndef PlatformCALayerWinInternal_h
#define PlatformCALayerWinInternal_h

#if USE(ACCELERATED_COMPOSITING)

#include <CoreGraphics/CGGeometry.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>

typedef struct _CACFLayer *CACFLayerRef;
typedef struct CGContext *CGContextRef;

namespace WebCore {

class FloatRect;
class PlatformCALayer;

typedef Vector<RefPtr<PlatformCALayer> > PlatformCALayerList;

class PlatformCALayerWinInternal {
public:
    PlatformCALayerWinInternal(PlatformCALayer*);
    ~PlatformCALayerWinInternal();

    void displayCallback(CACFLayerRef, CGContextRef);
    void setNeedsDisplay(const FloatRect*);
    PlatformCALayer* owner() const { return m_owner; }

    void setSublayers(const PlatformCALayerList&);
    void getSublayers(PlatformCALayerList&) const;
    void removeAllSublayers();
    void insertSublayer(PlatformCALayer*, size_t);
    size_t sublayerCount() const;
    int indexOfSublayer(const PlatformCALayer* reference);

    void setBounds(const FloatRect&);
    void setFrame(const FloatRect&);

private:
    void internalSetNeedsDisplay(const FloatRect*);
    PlatformCALayer* sublayerAtIndex(int) const;

    static void tileDisplayCallback(CACFLayerRef, CGContextRef);

    void drawTile(CACFLayerRef, CGContextRef);
    CGSize constrainedSize(const CGSize&) const;
    void addTile();
    void removeTile();
    CACFLayerRef tileAtIndex(int);
    int tileCount() const;
    void updateTiles();

    PlatformCALayer* m_owner;

    CGSize m_tileSize;
    CGSize m_constrainedSize;
    RetainPtr<CACFLayerRef> m_tileParent;
};

}

#endif // USE(ACCELERATED_COMPOSITING)

#endif // PlatformCALayerWinInternal_h
