/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef WebTiledLayer_h
#define WebTiledLayer_h

#if USE(ACCELERATED_COMPOSITING)

#include "WebLayer.h"

namespace WebCore {

class WebTiledLayer : public WebLayer {
public:
    static PassRefPtr<WebTiledLayer> create(const CGSize& tileSize, GraphicsLayer* owner);

    virtual ~WebTiledLayer();

    virtual void setBounds(const CGRect&);
    virtual void setFrame(const CGRect&);

protected:
    WebTiledLayer(const CGSize& tileSize, GraphicsLayer* owner);

    // Overridden from WKCACFLayer
    virtual WKCACFLayer* internalSublayerAtIndex(int) const;
    virtual int internalIndexOfSublayer(const WKCACFLayer*);

    virtual size_t internalSublayerCount() const;
    virtual void internalInsertSublayer(PassRefPtr<WKCACFLayer>, size_t index);

    virtual void internalRemoveAllSublayers();
    virtual void internalSetSublayers(const Vector<RefPtr<WKCACFLayer> >&);

    virtual void internalSetNeedsDisplay(const CGRect* dirtyRect);

#ifndef NDEBUG
    virtual void internalCheckLayerConsistency();
#endif

private:
    static void tileDisplayCallback(CACFLayerRef, CGContextRef);
    void drawTile(CACFLayerRef, CGContextRef);

    CGSize constrainedSize(const CGSize& size) const;

    void addTile();
    void removeTile();
    CACFLayerRef tileAtIndex(int);
    int tileCount() const;

    void updateTiles();

    CGSize m_tileSize;
    CGSize m_constrainedSize;
    RetainPtr<CACFLayerRef> m_tileParent;
};

}

#endif // USE(ACCELERATED_COMPOSITING)

#endif // WebTiledLayer_h
