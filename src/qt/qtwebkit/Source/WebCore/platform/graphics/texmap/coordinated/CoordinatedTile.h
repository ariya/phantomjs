/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef CoordinatedTile_h
#define CoordinatedTile_h

#if USE(TILED_BACKING_STORE)

#include "CoordinatedSurface.h"
#include "IntRect.h"
#include "Tile.h"
#include "TiledBackingStore.h"

namespace WebCore {

class CoordinatedTileClient;
class ImageBuffer;
class SurfaceUpdateInfo;
class TiledBackingStore;

class CoordinatedTile : public Tile, public CoordinatedSurface::Client {
public:
    static PassRefPtr<Tile> create(CoordinatedTileClient* client, TiledBackingStore* tiledBackingStore, const Coordinate& tileCoordinate) { return adoptRef(new CoordinatedTile(client, tiledBackingStore, tileCoordinate)); }
    ~CoordinatedTile();

    bool isDirty() const;
    void invalidate(const IntRect&);
    Vector<IntRect> updateBackBuffer();
    void swapBackBufferToFront();
    bool isReadyToPaint() const;
    void paint(GraphicsContext*, const IntRect&);

    const Coordinate& coordinate() const { return m_coordinate; }
    const IntRect& rect() const { return m_rect; }
    void resize(const IntSize&);

    virtual void paintToSurfaceContext(GraphicsContext*) OVERRIDE;

private:
    CoordinatedTile(CoordinatedTileClient*, TiledBackingStore*, const Coordinate&);

    CoordinatedTileClient* m_client;
    TiledBackingStore* m_tiledBackingStore;
    Coordinate m_coordinate;
    IntRect m_rect;

    uint32_t m_ID;
    IntRect m_dirtyRect;

    OwnPtr<ImageBuffer> m_localBuffer;
};

class CoordinatedTileClient {
public:
    virtual ~CoordinatedTileClient() { }
    virtual void createTile(uint32_t tileID, const SurfaceUpdateInfo&, const IntRect&) = 0;
    virtual void updateTile(uint32_t tileID, const SurfaceUpdateInfo&, const IntRect&) = 0;
    virtual void removeTile(uint32_t tileID) = 0;
    virtual bool paintToSurface(const IntSize&, uint32_t& atlasID, IntPoint&, CoordinatedSurface::Client*) = 0;
};

class CoordinatedTileBackend : public TiledBackingStoreBackend {
public:
    static PassOwnPtr<TiledBackingStoreBackend> create(CoordinatedTileClient* client) { return adoptPtr(new CoordinatedTileBackend(client)); }
    PassRefPtr<Tile> createTile(TiledBackingStore*, const Tile::Coordinate&);
    void paintCheckerPattern(GraphicsContext*, const FloatRect&);

private:
    explicit CoordinatedTileBackend(CoordinatedTileClient*);
    CoordinatedTileClient* m_client;
};

} // namespace WebCore

#endif // USE(TILED_BACKING_STORE)

#endif // CoordinatedTile_h
