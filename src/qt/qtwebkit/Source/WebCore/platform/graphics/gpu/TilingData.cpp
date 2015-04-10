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

#if USE(ACCELERATED_COMPOSITING) || ENABLE(ACCELERATED_2D_CANVAS)

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

TilingData::TilingData(const IntSize& maxTextureSize, const IntSize& totalSize, bool hasBorderTexels)
    : m_maxTextureSize(maxTextureSize)
    , m_totalSize(totalSize)
    , m_borderTexels(hasBorderTexels ? 1 : 0)
{
    recomputeNumTiles();
}

void TilingData::setTotalSize(const IntSize& totalSize)
{
    m_totalSize = totalSize;
    recomputeNumTiles();
}

void TilingData::setMaxTextureSize(const IntSize& maxTextureSize)
{
    m_maxTextureSize = maxTextureSize;
    recomputeNumTiles();
}

void TilingData::setHasBorderTexels(bool hasBorderTexels)
{
    m_borderTexels = hasBorderTexels ? 1 : 0;
    recomputeNumTiles();
}

int TilingData::tileXIndexFromSrcCoord(int srcPos) const
{
    if (numTilesX() <= 1)
        return 0;

    ASSERT(m_maxTextureSize.width() - 2 * m_borderTexels);
    int x = (srcPos - m_borderTexels) / (m_maxTextureSize.width() - 2 * m_borderTexels);
    return min(max(x, 0), numTilesX() - 1);
}

int TilingData::tileYIndexFromSrcCoord(int srcPos) const
{
    if (numTilesY() <= 1)
        return 0;

    ASSERT(m_maxTextureSize.height() - 2 * m_borderTexels);
    int y = (srcPos - m_borderTexels) / (m_maxTextureSize.height() - 2 * m_borderTexels);
    return min(max(y, 0), numTilesY() - 1);
}

IntRect TilingData::tileBounds(int i, int j) const
{
    assertTile(i, j);
    int x = tilePositionX(i);
    int y = tilePositionY(j);
    int width = tileSizeX(i);
    int height = tileSizeY(j);
    ASSERT(x >= 0 && y >= 0 && width >= 0 && height >= 0);
    ASSERT(x <= m_totalSize.width() && y <= m_totalSize.height());
    return IntRect(x, y, width, height);
}

IntRect TilingData::tileBoundsWithBorder(int i, int j) const
{
    IntRect bounds = tileBounds(i, j);

    if (m_borderTexels) {
        int x1 = bounds.x();
        int x2 = bounds.maxX();
        int y1 = bounds.y();
        int y2 = bounds.maxY();

        if (i > 0)
            x1--;
        if (i < (numTilesX() - 1))
            x2++;
        if (j > 0)
            y1--;
        if (j < (numTilesY() - 1))
            y2++;

        bounds = IntRect(x1, y1, x2 - x1, y2 - y1);
    }

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
        return m_totalSize.width();
    if (!xIndex && m_numTilesX > 1)
        return m_maxTextureSize.width() - m_borderTexels;
    if (xIndex < numTilesX() - 1)
        return m_maxTextureSize.width() - 2 * m_borderTexels;
    if (xIndex == numTilesX() - 1)
        return m_totalSize.width() - tilePositionX(xIndex);

    ASSERT_NOT_REACHED();
    return 0;
}

int TilingData::tileSizeY(int yIndex) const
{
    ASSERT(yIndex >= 0 && yIndex < numTilesY());

    if (!yIndex && m_numTilesY == 1)
        return m_totalSize.height();
    if (!yIndex && m_numTilesY > 1)
        return m_maxTextureSize.height() - m_borderTexels;
    if (yIndex < numTilesY() - 1)
        return m_maxTextureSize.height() - 2 * m_borderTexels;
    if (yIndex == numTilesY() - 1)
        return m_totalSize.height() - tilePositionY(yIndex);

    ASSERT_NOT_REACHED();
    return 0;
}

IntPoint TilingData::textureOffset(int xIndex, int yIndex) const
{
    int left = (!xIndex || m_numTilesX == 1) ? 0 : m_borderTexels;
    int top = (!yIndex || m_numTilesY == 1) ? 0 : m_borderTexels;

    return IntPoint(left, top);
}

void TilingData::recomputeNumTiles()
{
    m_numTilesX = computeNumTiles(m_maxTextureSize.width(), m_totalSize.width(), m_borderTexels);
    m_numTilesY = computeNumTiles(m_maxTextureSize.height(), m_totalSize.height(), m_borderTexels);
}

}

#endif
