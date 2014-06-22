/*
 * Copyright (C) 2011 Samsung Electronics
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TileCairo_h
#define TileCairo_h

#if USE(TILED_BACKING_STORE) && USE(CAIRO)

#include "IntPoint.h"
#include "IntRect.h"
#include "RefPtrCairo.h"
#include "Tile.h"
#include <cairo.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class TiledBackingStore;

class TileCairo : public Tile {
public:
    static PassRefPtr<Tile> create(TiledBackingStore* backingStore, const Coordinate& tileCoordinate)
    {
        return adoptRef(new TileCairo(backingStore, tileCoordinate));
    }
    virtual ~TileCairo();

    virtual bool isDirty() const;
    virtual void invalidate(const IntRect&);
    virtual Vector<IntRect> updateBackBuffer();
    virtual void swapBackBufferToFront();
    virtual bool isReadyToPaint() const;
    virtual void paint(GraphicsContext*, const IntRect&);

    virtual const Tile::Coordinate& coordinate() const { return m_coordinate; }
    virtual const IntRect& rect() const { return m_rect; }
    virtual void resize(const WebCore::IntSize&);

protected:
    TileCairo(TiledBackingStore*, const Coordinate&);

    TiledBackingStore* m_backingStore;
    Coordinate m_coordinate;
    IntRect m_rect;

    RefPtr<cairo_surface_t> m_buffer;
    RefPtr<cairo_region_t> m_dirtyRegion;
};

}
#endif
#endif

