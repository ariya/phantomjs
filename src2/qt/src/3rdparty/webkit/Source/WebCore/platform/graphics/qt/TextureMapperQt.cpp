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

#include "config.h"
#include "TextureMapperQt.h"

#include <QtCore/qdebug.h>
#include <QtGui/qpaintengine.h>
#include <QtGui/qpixmap.h>

#ifdef QT_OPENGL_LIB
# include "opengl/TextureMapperGL.h"
#endif

namespace WebCore {

void BitmapTextureQt::destroy()
{
    if (m_pixmap.paintingActive())
        qFatal("Destroying an active pixmap");
    m_pixmap = QPixmap();
}

void BitmapTextureQt::reset(const IntSize& size, bool isOpaque)
{
    BitmapTexture::reset(size, isOpaque);

    if (size.width() > m_pixmap.size().width() || size.height() > m_pixmap.size().height() || m_pixmap.isNull())
        m_pixmap = QPixmap(size.width(), size.height());
    if (!isOpaque)
        m_pixmap.fill(Qt::transparent);
}

PlatformGraphicsContext* BitmapTextureQt::beginPaint(const IntRect& dirtyRect)
{
    m_painter.begin(&m_pixmap);
    TextureMapperQt::initialize(&m_painter);
    m_painter.setCompositionMode(QPainter::CompositionMode_Clear);
    m_painter.fillRect(QRect(dirtyRect), Qt::transparent);
    m_painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    return &m_painter;
}

void BitmapTextureQt::endPaint()
{
    m_painter.end();
}

bool BitmapTextureQt::save(const String& path)
{
    return m_pixmap.save(path, "PNG");
}

void BitmapTextureQt::setContentsToImage(Image* image)
{
    if (!image)
        return;
    const QPixmap* pixmap = image->nativeImageForCurrentFrame();
    if (!pixmap)
        return;
    BitmapTexture::reset(pixmap->size(), !pixmap->hasAlphaChannel());
    m_pixmap = *pixmap;
}

void BitmapTextureQt::pack()
{
    if (m_pixmap.isNull())
        return;

    m_image = m_pixmap.toImage();
    m_pixmap = QPixmap();
    m_isPacked = true;
}

void BitmapTextureQt::unpack()
{
    m_isPacked = false;
    if (m_image.isNull())
        return;

    m_pixmap = QPixmap::fromImage(m_image);
    m_image = QImage();
}

void TextureMapperQt::setClip(const IntRect& rect)
{
     QPainter* painter = m_currentSurface ? &m_currentSurface->m_painter : m_painter;
     painter->setClipRect(rect);
}

TextureMapperQt::TextureMapperQt()
    : m_currentSurface(0)
{
}

void TextureMapperQt::setGraphicsContext(GraphicsContext* context)
{
    m_painter = context->platformContext();
    initialize(m_painter);
}

void TextureMapperQt::bindSurface(BitmapTexture* surface)
{
    if (m_currentSurface == surface)
        return;
    if (m_currentSurface)
        m_currentSurface->m_painter.end();
    if (!surface) {
        m_currentSurface = 0;
        return;
    }
    BitmapTextureQt* surfaceQt = static_cast<BitmapTextureQt*>(surface);
    if (!surfaceQt->m_painter.isActive())
        surfaceQt->m_painter.begin(&surfaceQt->m_pixmap);
    m_currentSurface = surfaceQt;
}


void TextureMapperQt::drawTexture(const BitmapTexture& texture, const IntRect& targetRect, const TransformationMatrix& matrix, float opacity, const BitmapTexture* maskTexture)
{
    const BitmapTextureQt& textureQt = static_cast<const BitmapTextureQt&>(texture);
    QPainter* painter = m_painter;
    QPixmap pixmap = textureQt.m_pixmap;
    if (m_currentSurface)
        painter = &m_currentSurface->m_painter;

    if (maskTexture && maskTexture->isValid()) {
        const BitmapTextureQt* mask = static_cast<const BitmapTextureQt*>(maskTexture);
        QPixmap intermediatePixmap(pixmap.size());
        intermediatePixmap.fill(Qt::transparent);
        QPainter maskPainter(&intermediatePixmap);
        maskPainter.setCompositionMode(QPainter::CompositionMode_Source);
        maskPainter.drawPixmap(0, 0, pixmap);
        maskPainter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        maskPainter.drawPixmap(QRect(0, 0, pixmap.width(), pixmap.height()), mask->m_pixmap, mask->sourceRect());
        maskPainter.end();
        pixmap = intermediatePixmap;
    }

    const qreal prevOpacity = painter->opacity();
    const QTransform prevTransform = painter->transform();
    painter->setOpacity(opacity);
    painter->setTransform(matrix, true);
    painter->drawPixmap(targetRect, pixmap, textureQt.sourceRect());
    painter->setTransform(prevTransform);
    painter->setOpacity(prevOpacity);
}

PassOwnPtr<TextureMapper> TextureMapper::create(GraphicsContext* context)
{
#ifdef QT_OPENGL_LIB
    if (context && context->platformContext()->paintEngine()->type() == QPaintEngine::OpenGL2)
        return new TextureMapperGL;
#endif
    return new TextureMapperQt;
}

PassRefPtr<BitmapTexture> TextureMapperQt::createTexture()
{
    return adoptRef(new BitmapTextureQt());
}

BitmapTextureQt::BitmapTextureQt()
    : m_isPacked(false)
{

}

#ifdef QT_OPENGL_LIB
class RGBA32PremultimpliedBufferQt : public RGBA32PremultimpliedBuffer {
public:
    virtual PlatformGraphicsContext* beginPaint(const IntRect& rect, bool opaque)
    {
        // m_image is only using during paint, it's safe to override it.
        m_image = QImage(rect.size().width(), rect.size().height(), QImage::Format_ARGB32_Premultiplied);
        if (!opaque)
            m_image.fill(0);
        m_painter.begin(&m_image);
        TextureMapperQt::initialize(&m_painter);
        m_painter.translate(-rect.x(), -rect.y());
        return &m_painter;
    }

    virtual void endPaint() { m_painter.end(); }
    virtual const void* data() const { return m_image.constBits(); }

private:
    QPainter m_painter;
    QImage m_image;
};

PassRefPtr<RGBA32PremultimpliedBuffer> RGBA32PremultimpliedBuffer::create()
{
    return adoptRef(new RGBA32PremultimpliedBufferQt());
}

#endif
};
