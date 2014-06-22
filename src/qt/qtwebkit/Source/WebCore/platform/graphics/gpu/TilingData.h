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

#include "IntSize.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class FloatRect;
class IntRect;
class IntPoint;

class TilingData {
public:
    TilingData(const IntSize& maxTextureSize, const IntSize& totalSize, bool hasBorderTexels);

    void setTotalSize(const IntSize&);
    const IntSize& totalSize() const { return m_totalSize; }
    void setMaxTextureSize(const IntSize&);
    const IntSize& maxTextureSize() const { return m_maxTextureSize; }
    void setHasBorderTexels(bool);
    int borderTexels() const { return m_borderTexels; }

    bool hasEmptyBounds() const { return !numTilesX() || !numTilesY(); }
    int numTilesX() const { return m_numTilesX; }
    int numTilesY() const { return m_numTilesY; }
    int tileXIndexFromSrcCoord(int) const;
    int tileYIndexFromSrcCoord(int) const;

    IntRect tileBounds(int, int) const;
    IntRect tileBoundsWithBorder(int, int) const;
    int tilePositionX(int xIndex) const;
    int tilePositionY(int yIndex) const;
    int tileSizeX(int xIndex) const;
    int tileSizeY(int yIndex) const;

    // Difference between tileBound's and tileBoundWithBorder's location().
    IntPoint textureOffset(int xIndex, int yIndex) const;

private:
    TilingData() : m_borderTexels(0) { }
    void assertTile(int i, int j) const { ASSERT_UNUSED(i, i >= 0 && i < numTilesX()); ASSERT_UNUSED(j, j >= 0 && j < numTilesY()); }
    void recomputeNumTiles();

    IntSize m_maxTextureSize;
    IntSize m_totalSize;
    int m_borderTexels; // 0 or 1

    // computed values:
    int m_numTilesX;
    int m_numTilesY;
};

}

#endif // TilingData_h
