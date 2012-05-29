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

#ifndef TextureMapper_h
#define TextureMapper_h

#if USE(ACCELERATED_COMPOSITING)
#if (defined(QT_OPENGL_LIB))
    #if defined(QT_OPENGL_ES_2) && !defined(TEXMAP_OPENGL_ES_2)
        #define TEXMAP_OPENGL_ES_2
    #endif
#endif

#include "GraphicsContext.h"
#include "IntRect.h"
#include "IntSize.h"
#include "TransformationMatrix.h"
#include <wtf/UnusedParam.h>

/*
    TextureMapper is a mechanism that enables hardware acceleration of CSS animations (accelerated compositing) without
    a need for a platform specific scene-graph library like CoreAnimations or QGraphicsView.
*/

namespace WebCore {

class TextureMapper;

// A 2D texture that can be the target of software or GL rendering.
class BitmapTexture  : public RefCounted<BitmapTexture> {
public:
    BitmapTexture() : m_lockCount(0) {}
    virtual ~BitmapTexture() { }

    virtual bool allowOfflineTextureUpload() const { return false; }
    virtual void destroy() = 0;
    virtual IntSize size() const = 0;
    virtual bool isValid() const = 0;
    virtual void reset(const IntSize& size, bool opaque = false)
    {
        m_isOpaque = opaque;
        m_contentSize = size;
    }

    virtual void pack() { }
    virtual void unpack() { }
    virtual bool isPacked() const { return false; }

    virtual PlatformGraphicsContext* beginPaint(const IntRect& dirtyRect) = 0;
    virtual void endPaint() = 0;
    virtual PlatformGraphicsContext* beginPaintMedia()
    {
        return beginPaint(IntRect(0, 0, size().width(), size().height()));
    }
    virtual void setContentsToImage(Image*) = 0;
    virtual bool save(const String&) { return false; }

    inline void lock() { ++m_lockCount; }
    inline void unlock() { --m_lockCount; }
    inline bool isLocked() { return m_lockCount; }
    inline IntSize contentSize() const { return m_contentSize; }
    inline void setOffset(const IntPoint& o) { m_offset = o; }
    inline IntPoint offset() const { return m_offset; }

protected:

private:
    int m_lockCount;
    IntSize m_contentSize;
    bool m_isOpaque;
    IntPoint m_offset;
};

// A "context" class used to encapsulate accelerated texture mapping functions: i.e. drawing a texture
// onto the screen or into another texture with a specified transform, opacity and mask.
class TextureMapper {
    friend class BitmapTexture;

public:
    static PassOwnPtr<TextureMapper> create(GraphicsContext* graphicsContext = 0);
    virtual ~TextureMapper() { }

    virtual void drawTexture(const BitmapTexture& texture, const IntRect& target, const TransformationMatrix& matrix = TransformationMatrix(), float opacity = 1.0f, const BitmapTexture* maskTexture = 0) = 0;

    // makes a surface the target for the following drawTexture calls.
    virtual void bindSurface(BitmapTexture* surface) = 0;
    virtual void paintToTarget(const BitmapTexture& texture, const IntSize&, const TransformationMatrix& matrix, float opacity, const IntRect& visibleRect)
    {
        UNUSED_PARAM(visibleRect);
        drawTexture(texture, IntRect(0, 0, texture.contentSize().width(), texture.contentSize().height()), matrix, opacity, 0);
    }

    virtual void setGraphicsContext(GraphicsContext*) { }
    virtual void setClip(const IntRect&) = 0;
    virtual bool allowSurfaceForRoot() const = 0;
    virtual PassRefPtr<BitmapTexture> createTexture() = 0;

    void setImageInterpolationQuality(InterpolationQuality quality) { m_interpolationQuality = quality; }
    void setTextDrawingMode(TextDrawingModeFlags mode) { m_textDrawingMode = mode; }

    InterpolationQuality imageInterpolationQuality() const { return m_interpolationQuality; }
    TextDrawingModeFlags textDrawingMode() const { return m_textDrawingMode; }

    void setViewportSize(const IntSize&);

protected:
    TextureMapper()
        : m_interpolationQuality(InterpolationDefault)
        , m_textDrawingMode(TextModeFill)
    {}

private:
    InterpolationQuality m_interpolationQuality;
    TextDrawingModeFlags m_textDrawingMode;
};

};

#endif

#endif
