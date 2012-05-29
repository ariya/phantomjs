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

#include "config.h"
#include "TiledImageOpenVG.h"

#include "FloatRect.h"
#include "IntRect.h"
#include "VGUtils.h"

namespace WebCore {

TiledImageOpenVG::TiledImageOpenVG(const IntSize& size, const IntSize& tileSize)
    : SharedResourceOpenVG()
    , m_size(size)
    , m_maxTileSize(tileSize)
    , m_numColumns((m_size.width() - 1) / m_maxTileSize.width() + 1)
    , m_tiles(((m_size.height() - 1) / m_maxTileSize.height() + 1) * m_numColumns, VG_INVALID_HANDLE)
{
}

TiledImageOpenVG::TiledImageOpenVG(const TiledImageOpenVG& other)
    : SharedResourceOpenVG()
    , m_size(other.m_size)
    , m_maxTileSize(other.m_maxTileSize)
    , m_numColumns(other.m_numColumns)
    , m_tiles(other.m_tiles)
{
    detachTiles();
}

TiledImageOpenVG& TiledImageOpenVG::operator=(const TiledImageOpenVG& other)
{
    if (&other != this) {
        destroyTiles();

        m_size = other.m_size;
        m_maxTileSize = other.m_maxTileSize;
        m_numColumns = other.m_numColumns;
        m_tiles = other.m_tiles;

        detachTiles();
    }
    return *this;
}

TiledImageOpenVG::~TiledImageOpenVG()
{
    destroyTiles();
}

int TiledImageOpenVG::numTiles() const
{
    return m_tiles.size();
}

int TiledImageOpenVG::numColumns() const
{
    return m_numColumns;
}

int TiledImageOpenVG::numRows() const
{
    return m_tiles.size() / m_numColumns;
}

void TiledImageOpenVG::setTile(int xIndex, int yIndex, VGImage image)
{
    ASSERT(xIndex < m_numColumns);
    int i = (yIndex * m_numColumns) + xIndex;
    ASSERT(i < m_tiles.size());
    m_tiles.at(i) = image;
}

IntRect TiledImageOpenVG::tilesInRect(const FloatRect& rect) const
{
    int leftIndex = static_cast<int>(rect.x()) / m_maxTileSize.width();
    int topIndex = static_cast<int>(rect.y()) / m_maxTileSize.height();

    if (leftIndex < 0)
        leftIndex = 0;
    if (topIndex < 0)
        topIndex = 0;

    // Round rect edges up to get the outer pixel boundaries.
    int rightIndex = (static_cast<int>(ceil(rect.right())) - 1) / m_maxTileSize.width();
    int bottomIndex = (static_cast<int>(ceil(rect.bottom())) - 1) / m_maxTileSize.height();
    int columns = (rightIndex - leftIndex) + 1;
    int rows = (bottomIndex - topIndex) + 1;

    return IntRect(leftIndex, topIndex,
        (columns <= m_numColumns) ? columns : m_numColumns,
        (rows <= (m_tiles.size() / m_numColumns)) ? rows : (m_tiles.size() / m_numColumns));
}

VGImage TiledImageOpenVG::tile(int xIndex, int yIndex) const
{
    ASSERT(xIndex < m_numColumns);
    int i = (yIndex * m_numColumns) + xIndex;
    ASSERT(i < m_tiles.size());
    return m_tiles.at(i);
}

IntRect TiledImageOpenVG::tileRect(int xIndex, int yIndex) const
{
    ASSERT(xIndex < m_numColumns);
    ASSERT((yIndex * m_numColumns) + xIndex < m_tiles.size());

    int x = xIndex * m_maxTileSize.width();
    int y = yIndex * m_maxTileSize.height();

    return IntRect(x, y,
        ((m_maxTileSize.width() < m_size.width() - x) ? m_maxTileSize.width() : (m_size.width() - x)),
        ((m_maxTileSize.height() < m_size.height() - y) ? m_maxTileSize.height() : (m_size.height() - y)));
}

void TiledImageOpenVG::detachTiles()
{
    makeSharedContextCurrent(); // because we create new images

    int numTiles = m_tiles.size();
    VGImage newTile, originalTile;

    for (int i = 0; i < numTiles; ++i) {
        originalTile = m_tiles.at(i);

        if (originalTile == VG_INVALID_HANDLE)
            continue;

        VGImageFormat format = (VGImageFormat) vgGetParameteri(originalTile, VG_IMAGE_FORMAT);
        VGint width = vgGetParameteri(originalTile, VG_IMAGE_WIDTH);
        VGint height = vgGetParameteri(originalTile, VG_IMAGE_HEIGHT);
        ASSERT_VG_NO_ERROR();

        newTile = vgCreateImage(format, width, height, VG_IMAGE_QUALITY_FASTER);
        ASSERT_VG_NO_ERROR();

        vgCopyImage(newTile, 0, 0, originalTile, 0, 0, width, height, VG_FALSE /* dither */);
        ASSERT_VG_NO_ERROR();

        m_tiles.at(i) = newTile;
    }
}

void TiledImageOpenVG::destroyTiles()
{
    makeCompatibleContextCurrent();

    Vector<VGImage>::const_iterator it = m_tiles.begin();
    Vector<VGImage>::const_iterator end = m_tiles.end();

    for (; it != end; ++it) {
        if (*it != VG_INVALID_HANDLE)
            vgDestroyImage(*it);
    }
    ASSERT_VG_NO_ERROR();

    m_tiles.fill(VG_INVALID_HANDLE);
}

}
