/*
 Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TileQt_h
#define TileQt_h

#if USE(TILED_BACKING_STORE)

#include "IntPoint.h"
#include "IntRect.h"
#include "Tile.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

QT_BEGIN_NAMESPACE
class QPixmap;
class QRegion;
QT_END_NAMESPACE

namespace WebCore {

class TiledBackingStore;

class TileQt : public Tile {
public:
    typedef IntPoint Coordinate;

    static PassRefPtr<Tile> create(TiledBackingStore* backingStore, const Coordinate& tileCoordinate) { return adoptRef(new TileQt(backingStore, tileCoordinate)); }
    ~TileQt();

    bool isDirty() const;
    void invalidate(const IntRect&);
    Vector<IntRect> updateBackBuffer();
    void swapBackBufferToFront();
    bool isReadyToPaint() const;
    void paint(GraphicsContext*, const IntRect&);

    const Tile::Coordinate& coordinate() const { return m_coordinate; }
    const IntRect& rect() const { return m_rect; }
    void resize(const WebCore::IntSize&);

private:
    TileQt(TiledBackingStore*, const Coordinate&);

    TiledBackingStore* m_backingStore;
    Coordinate m_coordinate;
    IntRect m_rect;

    QPixmap* m_buffer;
    QPixmap* m_backBuffer;
    QRegion* m_dirtyRegion;
};

}
#endif
#endif
