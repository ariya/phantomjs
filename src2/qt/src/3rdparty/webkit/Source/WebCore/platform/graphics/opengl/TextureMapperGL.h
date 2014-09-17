/*
 Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

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

#ifndef TextureMapperGL_h
#define TextureMapperGL_h

#if USE(ACCELERATED_COMPOSITING)

#include "FloatQuad.h"
#include "IntSize.h"
#include "TextureMapper.h"
#include "TransformationMatrix.h"

namespace WebCore {

class TextureMapperGLData;

// An OpenGL-ES2 implementation of TextureMapper.
class TextureMapperGL : public TextureMapper {
public:
    TextureMapperGL();
    virtual ~TextureMapperGL();

    // reimps from TextureMapper
    virtual void drawTexture(const BitmapTexture& texture, const IntRect&, const TransformationMatrix& transform, float opacity, const BitmapTexture* maskTexture);
    virtual void bindSurface(BitmapTexture* surface);
    virtual void setClip(const IntRect&);
    virtual void paintToTarget(const BitmapTexture&, const IntSize&, const TransformationMatrix&, float opacity, const IntRect& visibleRect);
    virtual bool allowSurfaceForRoot() const { return true; }
    virtual PassRefPtr<BitmapTexture> createTexture();
    virtual const char* type() const;
    void obtainCurrentContext();
    bool makeContextCurrent();
    static PassOwnPtr<TextureMapperGL> create()
    {
        return new TextureMapperGL;
    }

private:
    inline TextureMapperGLData& data() { return *m_data; }
    TextureMapperGLData* m_data;
    friend class BitmapTextureGL;
};

// An offscreen buffer to be rendered by software.
class RGBA32PremultimpliedBuffer : public RefCounted<RGBA32PremultimpliedBuffer> {
public:
    virtual ~RGBA32PremultimpliedBuffer() {}
    virtual PlatformGraphicsContext* beginPaint(const IntRect& dirtyRect, bool opaque) = 0;
    virtual void endPaint() = 0;
    virtual const void* data() const = 0;
    static PassRefPtr<RGBA32PremultimpliedBuffer> create();
};

static inline int nextPowerOfTwo(int num)
{
    for (int i = 0x10000000; i > 0; i >>= 1) {
        if (num == i)
            return num;
        if (num & i)
            return (i << 1);
    }
    return 1;
}

static inline IntSize nextPowerOfTwo(const IntSize& size)
{
    return IntSize(nextPowerOfTwo(size.width()), nextPowerOfTwo(size.height()));
}

};

#endif

#endif
