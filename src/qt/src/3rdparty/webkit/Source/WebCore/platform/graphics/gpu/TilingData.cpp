/*
 * Copyright (c) 2010, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(ACCELERATED_2D_CANVAS)

#include "TilingData.h"

#include "FloatRect.h"
#include "IntRect.h"
#include <algorithm>

using namespace std;

namespace WebCore {

static int computeNumTiles(int maxTextureSize, int totalSize, int borderTexels)
{
    if (maxTextureSize - 2 * borderTexels <= 0)
        return totalSize > 0 && maxTextureSize >= totalSize ? 1 : 0;

    int numTiles = max(1, 1 + (totalSize - 1 - 2 * borderTexels) / (maxTextureSize - 2 * borderTexels));
    return totalSize > 0 ? numTiles : 0;
}

TilingData::TilingData(int maxTextureSize, int totalSizeX, int totalSizeY, bool hasBorderTexels)
    : m_maxTextureSize(maxTextureSize)
    , m_totalSizeX(totalSizeX)
    , m_totalSizeY(totalSizeY)
    , m_borderTexels(hasBorderTexels ? 1 : 0)
{
    recomputeNumTiles();
}

void TilingData::setTotalSize(int totalSizeX, int totalSizeY)
{
    m_totalSizeX = totalSizeX;
    m_totalSizeY = totalSizeY;
    recomputeNumTiles();
}

void TilingData::setMaxTextureSize(int maxTextureSize)
{
    m_maxTextureSize = maxTextureSize;
    recomputeNumTiles();
}

int TilingData::tileXIndexFromSrcCoord(int srcPos) const
{
    int x = (srcPos - m_borderTexels) / (m_maxTextureSize - 2 * m_borderTexels);
    return min(max(x, 0), numTilesX() - 1);
}

int TilingData::tileYIndexFromSrcCoord(int srcPos) const
{
    int y = (srcPos - m_borderTexels) / (m_maxTextureSize - 2 * m_borderTexels);
    return min(max(y, 0), numTilesY() - 1);
}

IntRect TilingData::tileBounds(int tile) const
{
    assertTile(tile);
    int ix = tileXIndex(tile);
    int iy = tileYIndex(tile);
    int x = tilePositionX(ix);
    int y = tilePositionY(iy);
    int width = tileSizeX(ix);
    int height = tileSizeY(iy);
    ASSERT(x >= 0 && y >= 0 && width >= 0 && height >= 0);
    ASSERT(x <= totalSizeX() && y <= totalSizeY());
    return IntRect(x, y, width, height);
}

IntRect TilingData::tileBoundsWithBorder(int tile) const
{
    IntRect bounds = tileBounds(tile);

    if (m_borderTexels) {
        int x1 = bounds.x();
        int x2 = bounds.maxX();
        int y1 = bounds.y();
        int y2 = bounds.maxY();

        if (tileXIndex(tile) > 0)
            x1--;
        if (tileXIndex(tile) < (numTilesX() - 1))
            x2++;
        if (tileYIndex(tile) > 0)
            y1--;
        if (tileYIndex(tile) < (numTilesY() - 1))
            y2++;

        bounds = IntRect(x1, y1, x2 - x1, y2 - y1);
    }

    return bounds;
}

FloatRect TilingData::tileBoundsNormalized(int tile) const
{
    assertTile(tile);
    FloatRect bounds(tileBounds(tile));
    bounds.scale(1.0f / m_totalSizeX, 1.0f / m_totalSizeY);
    return bounds;
}

int TilingData::tilePositionX(int xIndex) const
{
    ASSERT(xIndex >= 0 && xIndex < numTilesX());

    int pos = 0;
    for (int i = 0; i < xIndex; i++)
        pos += tileSizeX(i);

    return pos;
}

int TilingData::tilePositionY(int yIndex) const
{
    ASSERT(yIndex >= 0 && yIndex < numTilesY());

    int pos = 0;
    for (int i = 0; i < yIndex; i++)
        pos += tileSizeY(i);

    return pos;
}

int TilingData::tileSizeX(int xIndex) const
{
    ASSERT(xIndex >= 0 && xIndex < numTilesX());

    if (!xIndex && m_numTilesX == 1)
        return m_totalSizeX;
    if (!xIndex && m_numTilesX > 1)
        return m_maxTextureSize - m_borderTexels;
    if (xIndex < numTilesX() - 1)
        return m_maxTextureSize - 2 * m_borderTexels;
    if (xIndex == numTilesX() - 1)
        return m_totalSizeX - tilePositionX(xIndex);

    ASSERT_NOT_REACHED();
    return 0;
}

int TilingData::tileSizeY(int yIndex) const
{
    ASSERT(yIndex >= 0 && yIndex < numTilesY());

    if (!yIndex && m_numTilesY == 1)
        return m_totalSizeY;
    if (!yIndex && m_numTilesY > 1)
        return m_maxTextureSize - m_borderTexels;
    if (yIndex < numTilesY() - 1)
        return m_maxTextureSize - 2 * m_borderTexels;
    if (yIndex == numTilesY() - 1)
        return m_totalSizeY - tilePositionY(yIndex);

    ASSERT_NOT_REACHED();
    return 0;
}

IntRect TilingData::overlappedTileIndices(const WebCore::IntRect &srcRect) const
{
    int x = tileXIndexFromSrcCoord(srcRect.x());
    int y = tileYIndexFromSrcCoord(srcRect.y());
    int r = tileXIndexFromSrcCoord(srcRect.maxX());
    int b = tileYIndexFromSrcCoord(srcRect.maxY());
    return IntRect(x, y, r - x, b - y);
}

IntRect TilingData::overlappedTileIndices(const WebCore::FloatRect &srcRect) const
{
    return overlappedTileIndices(enclosingIntRect(srcRect));
}

void TilingData::intersectDrawQuad(const FloatRect& srcRect, const FloatRect& dstRect, int tile,
                                   FloatRect* newSrc, FloatRect* newDst) const
{
    // Intersect with tile
    FloatRect tileBounds = this->tileBounds(tile);
    FloatRect srcRectIntersected = srcRect;
    srcRectIntersected.intersect(tileBounds);

    if (srcRectIntersected.isEmpty()) {
        *newSrc = *newDst = FloatRect(0, 0, 0, 0);
        return;
    }

    float srcRectIntersectedNormX = (srcRectIntersected.x() - srcRect.x()) / srcRect.width();
    float srcRectIntersectedNormY = (srcRectIntersected.y() - srcRect.y()) / srcRect.height();
    float srcRectIntersectedNormW = srcRectIntersected.width() / srcRect.width();
    float srcRectIntersectedNormH = srcRectIntersected.height() / srcRect.height();

    *newSrc = srcRectIntersected;
    newSrc->move(
        -tileBounds.x() + ((tileXIndex(tile) > 0) ? m_borderTexels : 0),
        -tileBounds.y() + ((tileYIndex(tile) > 0) ? m_borderTexels : 0));

    *newDst = FloatRect(
        srcRectIntersectedNormX * dstRect.width() + dstRect.x(),
        srcRectIntersectedNormY * dstRect.height() + dstRect.y(),
        srcRectIntersectedNormW * dstRect.width(),
        srcRectIntersectedNormH * dstRect.height());
}

IntPoint TilingData::textureOffset(int xIndex, int yIndex) const
{
    int left = (!xIndex || m_numTilesX == 1) ? 0 : m_borderTexels;
    int top = (!yIndex || m_numTilesY == 1) ? 0 : m_borderTexels;

    return IntPoint(left, top);
}

void TilingData::recomputeNumTiles()
{
    m_numTilesX = computeNumTiles(m_maxTextureSize, m_totalSizeX, m_borderTexels);
    m_numTilesY = computeNumTiles(m_maxTextureSize, m_totalSizeY, m_borderTexels);
}

}

#endif
