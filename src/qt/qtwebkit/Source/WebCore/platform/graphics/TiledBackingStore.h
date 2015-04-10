/*
 Copyright (C) 2010-2012 Nokia Corporation and/or its subsidiary(-ies)
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Library General Public
 License as published by the Free Software Foundation; either
 version 2 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Library General Public License for more details.
 
 You should have received a copy of the GNU Library General Public License
 along with this library; see the file COPYING.LIB.  If not, write to
 the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 Boston, MA 02110-1301, USA.
 */

#ifndef TiledBackingStore_h
#define TiledBackingStore_h

#if USE(TILED_BACKING_STORE)

#include "FloatPoint.h"
#include "IntPoint.h"
#include "IntRect.h"
#include "Tile.h"
#include "TiledBackingStoreBackend.h"
#include "Timer.h"
#include <wtf/Assertions.h>
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class GraphicsContext;
class TiledBackingStore;
class TiledBackingStoreClient;

class TiledBackingStore {
    WTF_MAKE_NONCOPYABLE(TiledBackingStore); WTF_MAKE_FAST_ALLOCATED;
public:
    TiledBackingStore(TiledBackingStoreClient*, PassOwnPtr<TiledBackingStoreBackend> = TiledBackingStoreBackend::create());
    ~TiledBackingStore();

    TiledBackingStoreClient* client() { return m_client; }

    // Used when class methods cannot be called asynchronously by client.
    // Updates of tiles are committed as soon as all the events in event queue have been processed.
    void setCommitTileUpdatesOnIdleEventLoop(bool enable) { m_commitTileUpdatesOnIdleEventLoop = enable; }

    void setTrajectoryVector(const FloatPoint&);
    void coverWithTilesIfNeeded();

    float contentsScale() { return m_contentsScale; }
    void setContentsScale(float);

    bool contentsFrozen() const { return m_contentsFrozen; }
    void setContentsFrozen(bool);

    void updateTileBuffers();

    void invalidate(const IntRect& dirtyRect);
    void paint(GraphicsContext*, const IntRect&);

    IntSize tileSize() { return m_tileSize; }
    void setTileSize(const IntSize&);

    IntRect mapToContents(const IntRect&) const;
    IntRect mapFromContents(const IntRect&) const;

    IntRect tileRectForCoordinate(const Tile::Coordinate&) const;
    Tile::Coordinate tileCoordinateForPoint(const IntPoint&) const;
    double tileDistance(const IntRect& viewport, const Tile::Coordinate&) const;

    IntRect coverRect() const { return m_coverRect; }
    bool visibleAreaIsCovered() const;
    void removeAllNonVisibleTiles();

    void setSupportsAlpha(bool);

private:
    void startTileBufferUpdateTimer();
    void startBackingStoreUpdateTimer(double = 0);

    void tileBufferUpdateTimerFired(Timer<TiledBackingStore>*);
    void backingStoreUpdateTimerFired(Timer<TiledBackingStore>*);

    void createTiles();
    void computeCoverAndKeepRect(const IntRect& visibleRect, IntRect& coverRect, IntRect& keepRect) const;

    bool isBackingStoreUpdatesSuspended() const;
    bool isTileBufferUpdatesSuspended() const;

    void commitScaleChange();

    bool resizeEdgeTiles();
    void setCoverRect(const IntRect& rect) { m_coverRect = rect; }
    void setKeepRect(const IntRect&);

    PassRefPtr<Tile> tileAt(const Tile::Coordinate&) const;
    void setTile(const Tile::Coordinate& coordinate, PassRefPtr<Tile> tile);
    void removeTile(const Tile::Coordinate& coordinate);

    IntRect visibleRect() const;

    float coverageRatio(const IntRect&) const;
    void adjustForContentsRect(IntRect&) const;

    void paintCheckerPattern(GraphicsContext*, const IntRect&, const Tile::Coordinate&);

private:
    TiledBackingStoreClient* m_client;
    OwnPtr<TiledBackingStoreBackend> m_backend;

    typedef HashMap<Tile::Coordinate, RefPtr<Tile> > TileMap;
    TileMap m_tiles;

    Timer<TiledBackingStore> m_tileBufferUpdateTimer;
    Timer<TiledBackingStore> m_backingStoreUpdateTimer;

    IntSize m_tileSize;
    float m_coverAreaMultiplier;

    FloatPoint m_trajectoryVector;
    FloatPoint m_pendingTrajectoryVector;
    IntRect m_visibleRect;

    IntRect m_coverRect;
    IntRect m_keepRect;
    IntRect m_rect;

    float m_contentsScale;
    float m_pendingScale;

    bool m_commitTileUpdatesOnIdleEventLoop;
    bool m_contentsFrozen;
    bool m_supportsAlpha;
    bool m_pendingTileCreation;

    friend class Tile;
};

}

#endif
#endif
