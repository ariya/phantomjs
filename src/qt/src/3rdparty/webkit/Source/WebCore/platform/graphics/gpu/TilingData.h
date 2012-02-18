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

#ifndef TilingData_h
#define TilingData_h

#include <wtf/Noncopyable.h>

namespace WebCore {

class FloatRect;
class IntRect;
class IntPoint;

class TilingData {
    WTF_MAKE_NONCOPYABLE(TilingData);
public:
    TilingData(int maxTextureSize, int totalSizeX, int totalSizeY, bool hasBorderTexels);
    void setTotalSize(int totalSizeX, int totalSizeY);
    void setMaxTextureSize(int);
    int maxTextureSize() const { return m_maxTextureSize; }
    int totalSizeX() const { return m_totalSizeX; }
    int totalSizeY() const { return m_totalSizeY; }
    int borderTexels() const { return m_borderTexels; }

    int numTiles() const { return numTilesX() * numTilesY(); }
    int numTilesX() const { return m_numTilesX; }
    int numTilesY() const { return m_numTilesY; }
    int tileIndex(int x, int y) const { return x + y * numTilesX(); }
    int tileXIndex(int tile) const { assertTile(tile); return tile % numTilesX(); }
    int tileYIndex(int tile) const { assertTile(tile); return tile / numTilesX(); }
    int tileXIndexFromSrcCoord(int) const;
    int tileYIndexFromSrcCoord(int) const;

    IntRect tileBounds(int tile) const;
    IntRect tileBoundsWithBorder(int tile) const;
    FloatRect tileBoundsNormalized(int tile) const;
    int tilePositionX(int xIndex) const;
    int tilePositionY(int yIndex) const;
    int tileSizeX(int xIndex) const;
    int tileSizeY(int yIndex) const;
    IntRect overlappedTileIndices(const IntRect& srcRect) const;
    IntRect overlappedTileIndices(const FloatRect& srcRect) const;

    // Given a set of source and destination coordinates for a drawing quad
    // in texel units, returns adjusted data to render just the one tile.
    void intersectDrawQuad(const FloatRect& srcRect, const FloatRect& dstRect, int tile, FloatRect* newSrc, FloatRect* newDst) const;

    // Difference between tileBound's and tileBoundWithBorder's location().
    IntPoint textureOffset(int xIndex, int yIndex) const;

private:
    TilingData() : m_maxTextureSize(0), m_totalSizeX(0), m_totalSizeY(0) {}
    void assertTile(int tile) const { ASSERT(tile >= 0 && tile < numTiles()); }
    void recomputeNumTiles();

    int m_maxTextureSize;
    int m_totalSizeX;
    int m_totalSizeY;
    int m_borderTexels; // 0 or 1

    // computed values:
    int m_numTilesX;
    int m_numTilesY;
};

}

#endif // TilingData_h
