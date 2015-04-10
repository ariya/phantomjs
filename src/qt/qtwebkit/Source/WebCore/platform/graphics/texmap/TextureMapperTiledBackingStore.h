/*
 Copyright (C) 2013 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TextureMapperTiledBackingStore_h
#define TextureMapperTiledBackingStore_h

#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)

#include "FloatRect.h"
#include "Image.h"
#include "TextureMapperBackingStore.h"
#include "TextureMapperTile.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class TextureMapper;

class TextureMapperTiledBackingStore : public TextureMapperBackingStore {
public:
    static PassRefPtr<TextureMapperTiledBackingStore> create() { return adoptRef(new TextureMapperTiledBackingStore); }
    virtual ~TextureMapperTiledBackingStore() { }

    virtual PassRefPtr<BitmapTexture> texture() const OVERRIDE;
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float) OVERRIDE;
    virtual void drawBorder(TextureMapper*, const Color&, float borderWidth, const FloatRect&, const TransformationMatrix&) OVERRIDE;
    virtual void drawRepaintCounter(TextureMapper*, int repaintCount, const Color&, const FloatRect&, const TransformationMatrix&) OVERRIDE;
    void updateContents(TextureMapper*, Image*, const FloatSize&, const IntRect&, BitmapTexture::UpdateContentsFlag);
    void updateContents(TextureMapper*, GraphicsLayer*, const FloatSize&, const IntRect&, BitmapTexture::UpdateContentsFlag);

    void setContentsToImage(Image* image) { m_image = image; }

private:
    TextureMapperTiledBackingStore();
    void createOrDestroyTilesIfNeeded(const FloatSize& backingStoreSize, const IntSize& tileSize, bool hasAlpha, IntRect&);
    void updateContentsFromImageIfNeeded(TextureMapper*);
    TransformationMatrix adjustedTransformForRect(const FloatRect&);
    inline FloatRect rect() const { return FloatRect(FloatPoint::zero(), m_size); }

    Vector<TextureMapperTile> m_tiles;
    FloatSize m_size;
    RefPtr<Image> m_image;
};

} // namespace WebCore
#endif

#endif
