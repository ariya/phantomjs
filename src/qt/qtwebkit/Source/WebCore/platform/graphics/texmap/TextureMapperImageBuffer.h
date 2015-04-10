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

#ifndef TextureMapperImageBuffer_h
#define TextureMapperImageBuffer_h

#include "ImageBuffer.h"
#include "TextureMapper.h"

#if USE(TEXTURE_MAPPER)
namespace WebCore {

class BitmapTextureImageBuffer : public BitmapTexture {
    friend class TextureMapperImageBuffer;
public:
    static PassRefPtr<BitmapTexture> create() { return adoptRef(new BitmapTextureImageBuffer); }
    virtual IntSize size() const { return m_image->internalSize(); }
    virtual void didReset();
    virtual bool isValid() const { return m_image; }
    inline GraphicsContext* graphicsContext() { return m_image ? m_image->context() : 0; }
    virtual void updateContents(Image*, const IntRect&, const IntPoint&, UpdateContentsFlag);
    virtual void updateContents(TextureMapper*, GraphicsLayer*, const IntRect& target, const IntPoint& offset, UpdateContentsFlag);
    virtual void updateContents(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag);
#if ENABLE(CSS_FILTERS)
    PassRefPtr<BitmapTexture> applyFilters(TextureMapper*, const FilterOperations&);
#endif

private:
    BitmapTextureImageBuffer() { }
    OwnPtr<ImageBuffer> m_image;
};


class TextureMapperImageBuffer : public TextureMapper {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<TextureMapper> create() { return adoptPtr(new TextureMapperImageBuffer); }

    // TextureMapper implementation
    virtual void drawBorder(const Color&, float borderWidth, const FloatRect&, const TransformationMatrix&) OVERRIDE;
    virtual void drawNumber(int number, const Color&, const FloatPoint&, const TransformationMatrix&) OVERRIDE;
    virtual void drawTexture(const BitmapTexture&, const FloatRect& targetRect, const TransformationMatrix&, float opacity, unsigned exposedEdges) OVERRIDE;
    virtual void drawSolidColor(const FloatRect&, const TransformationMatrix&, const Color&) OVERRIDE;
    virtual void beginClip(const TransformationMatrix&, const FloatRect&) OVERRIDE;
    virtual void bindSurface(BitmapTexture* surface) OVERRIDE { m_currentSurface = surface;}
    virtual void endClip() OVERRIDE;
    virtual IntRect clipBounds() OVERRIDE { return currentContext()->clipBounds(); }
    virtual IntSize maxTextureSize() const;
    virtual PassRefPtr<BitmapTexture> createTexture() OVERRIDE { return BitmapTextureImageBuffer::create(); }

    inline GraphicsContext* currentContext()
    {
        return m_currentSurface ? static_cast<BitmapTextureImageBuffer*>(m_currentSurface.get())->graphicsContext() : graphicsContext();
    }

private:
    TextureMapperImageBuffer()
        : TextureMapper(SoftwareMode)
    { }
    RefPtr<BitmapTexture> m_currentSurface;
};

}
#endif // USE(TEXTURE_MAPPER)

#endif // TextureMapperImageBuffer_h
