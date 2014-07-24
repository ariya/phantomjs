/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef CoordinatedBackingStore_h
#define CoordinatedBackingStore_h

#if USE(COORDINATED_GRAPHICS)

#include "TextureMapper.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperTile.h"
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>

namespace WebCore {

class CoordinatedSurface;

class CoordinatedBackingStoreTile : public TextureMapperTile {
public:
    explicit CoordinatedBackingStoreTile(float scale = 1)
        : TextureMapperTile(FloatRect())
        , m_scale(scale)
    {
    }

    inline float scale() const { return m_scale; }
    void swapBuffers(TextureMapper*);
    void setBackBuffer(const IntRect&, const IntRect&, PassRefPtr<CoordinatedSurface> buffer, const IntPoint&);

private:
    RefPtr<CoordinatedSurface> m_surface;
    IntRect m_sourceRect;
    IntRect m_tileRect;
    IntPoint m_surfaceOffset;
    float m_scale;
};

class CoordinatedBackingStore : public TextureMapperBackingStore {
public:
    void createTile(uint32_t tileID, float);
    void removeTile(uint32_t tileID);
    void removeAllTiles();
    void updateTile(uint32_t tileID, const IntRect&, const IntRect&, PassRefPtr<CoordinatedSurface>, const IntPoint&);
    static PassRefPtr<CoordinatedBackingStore> create() { return adoptRef(new CoordinatedBackingStore); }
    void commitTileOperations(TextureMapper*);
    PassRefPtr<BitmapTexture> texture() const;
    void setSize(const FloatSize&);
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float);
    virtual void drawBorder(TextureMapper*, const Color&, float borderWidth, const FloatRect&, const TransformationMatrix&) OVERRIDE;
    virtual void drawRepaintCounter(TextureMapper*, int repaintCount, const Color&, const FloatRect&, const TransformationMatrix&) OVERRIDE;

private:
    CoordinatedBackingStore()
        : m_scale(1.)
    { }
    void paintTilesToTextureMapper(Vector<TextureMapperTile*>&, TextureMapper*, const TransformationMatrix&, float, const FloatRect&);
    TransformationMatrix adjustedTransformForRect(const FloatRect&);
    FloatRect rect() const { return FloatRect(FloatPoint::zero(), m_size); }

    typedef HashMap<uint32_t, CoordinatedBackingStoreTile> CoordinatedBackingStoreTileMap;
    CoordinatedBackingStoreTileMap m_tiles;
    HashSet<uint32_t> m_tilesToRemove;
    // FIXME: m_pendingSize should be removed after the following bug is fixed: https://bugs.webkit.org/show_bug.cgi?id=108294
    FloatSize m_pendingSize;
    FloatSize m_size;
    float m_scale;
};

} // namespace WebCore

#endif // USE(COORDINATED_GRAPHICS)

#endif // CoordinatedBackingStore_h
