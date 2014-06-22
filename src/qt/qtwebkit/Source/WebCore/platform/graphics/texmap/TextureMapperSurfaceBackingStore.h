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

#ifndef TextureMapperSurfaceBackingStore_h
#define TextureMapperSurfaceBackingStore_h

#if USE(ACCELERATED_COMPOSITING) && USE(GRAPHICS_SURFACE)

#include "GraphicsSurface.h"
#include "TextureMapperBackingStore.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class TextureMapper;
class FloatRect;

class TextureMapperSurfaceBackingStore : public TextureMapperBackingStore {
public:
    static PassRefPtr<TextureMapperSurfaceBackingStore> create() { return adoptRef(new TextureMapperSurfaceBackingStore); }
    void setGraphicsSurface(PassRefPtr<GraphicsSurface>);
    void swapBuffersIfNeeded(uint32_t frontBuffer);
    virtual PassRefPtr<BitmapTexture> texture() const;
    virtual void paintToTextureMapper(TextureMapper*, const FloatRect&, const TransformationMatrix&, float);
    virtual ~TextureMapperSurfaceBackingStore() { }

private:
    TextureMapperSurfaceBackingStore()
        : TextureMapperBackingStore()
        { }

    RefPtr<GraphicsSurface> m_graphicsSurface;
};

}

#endif

#endif
