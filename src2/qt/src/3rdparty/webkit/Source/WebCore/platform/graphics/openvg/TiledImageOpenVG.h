/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef TiledImageOpenVG_h
#define TiledImageOpenVG_h

#include "IntRect.h"
#include "IntSize.h"
#include "SharedResourceOpenVG.h"

#include <openvg.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatRect;

class TiledImageOpenVG : public SharedResourceOpenVG {
public:
    TiledImageOpenVG(const IntSize& size, const IntSize& tileSize);
    TiledImageOpenVG(const TiledImageOpenVG&);
    ~TiledImageOpenVG();

    TiledImageOpenVG& operator=(const TiledImageOpenVG&);

    const IntSize& size() const { return m_size; }
    const IntSize& maxTileSize() const { return m_maxTileSize; }

    int numTiles() const;
    int numColumns() const;
    int numRows() const;

    IntRect tilesInRect(const FloatRect&) const;

    void setTile(int xIndex, int yIndex, VGImage);
    VGImage tile(int xIndex, int yIndex) const;
    IntRect tileRect(int xIndex, int yIndex) const;

private:
    void detachTiles();
    void destroyTiles();

    IntSize m_size;
    IntSize m_maxTileSize;

    int m_numColumns;

    Vector<VGImage> m_tiles;
};

}

#endif
