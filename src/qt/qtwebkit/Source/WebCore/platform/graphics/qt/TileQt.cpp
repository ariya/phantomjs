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
#include "TileQt.h"

#if USE(TILED_BACKING_STORE)

#include "GraphicsContext.h"
#include "TiledBackingStore.h"
#include "TiledBackingStoreClient.h"
#include <QObject>
#include <QPainter>
#include <QRegion>

namespace WebCore {
    
static const unsigned checkerSize = 16;
static const unsigned checkerColor1 = 0xff555555;
static const unsigned checkerColor2 = 0xffaaaaaa;
    
static QPixmap& checkeredPixmap()
{
    static QPixmap* pixmap;
    if (!pixmap) {
        pixmap = new QPixmap(checkerSize, checkerSize);
        QPainter painter(pixmap);
        QColor color1(checkerColor1);
        QColor color2(checkerColor2);
        for (unsigned y = 0; y < checkerSize; y += checkerSize / 2) {
            bool alternate = y % checkerSize;
            for (unsigned x = 0; x < checkerSize; x += checkerSize / 2) {
                painter.fillRect(x, y, checkerSize / 2, checkerSize / 2, alternate ? color1 : color2);
                alternate = !alternate;
            }
        }
    }
    return *pixmap;
}
    
TileQt::TileQt(TiledBackingStore* backingStore, const Coordinate& tileCoordinate)
    : m_backingStore(backingStore)
    , m_coordinate(tileCoordinate)
    , m_rect(m_backingStore->tileRectForCoordinate(tileCoordinate))
    , m_buffer(0)
    , m_backBuffer(0)
    , m_dirtyRegion(new QRegion(m_rect))
{
}

TileQt::~TileQt()
{
    delete m_buffer;
    delete m_backBuffer;
    delete m_dirtyRegion;
}

bool TileQt::isDirty() const
{ 
    return !m_dirtyRegion->isEmpty(); 
}

bool TileQt::isReadyToPaint() const
{ 
    return m_buffer; 
}

void TileQt::invalidate(const IntRect& dirtyRect)
{
    IntRect tileDirtyRect = intersection(dirtyRect, m_rect);
    if (tileDirtyRect.isEmpty())
        return;

    *m_dirtyRegion += tileDirtyRect;
}
    
Vector<IntRect> TileQt::updateBackBuffer()
{
    if (m_buffer && !isDirty())
        return Vector<IntRect>();

    if (!m_backBuffer) {
        if (!m_buffer) {
            m_backBuffer = new QPixmap(m_backingStore->tileSize().width(), m_backingStore->tileSize().height());
            m_backBuffer->fill(m_backingStore->client()->tiledBackingStoreBackgroundColor());
        } else {
            // Currently all buffers are updated synchronously at the same time so there is no real need
            // to have separate back and front buffers. Just use the existing buffer.
            m_backBuffer = m_buffer;
            m_buffer = 0;
        }
    }

    QVector<QRect> dirtyRects = m_dirtyRegion->rects();
    *m_dirtyRegion = QRegion();
    
    QPainter painter(m_backBuffer);
    GraphicsContext context(&painter);
    context.translate(-m_rect.x(), -m_rect.y());

    Vector<IntRect> updatedRects;
    int size = dirtyRects.size();
    for (int n = 0; n < size; ++n)  {
        context.save();
        IntRect rect = dirtyRects[n];
        updatedRects.append(rect);
        context.clip(FloatRect(rect));
        context.scale(FloatSize(m_backingStore->contentsScale(), m_backingStore->contentsScale()));
        m_backingStore->client()->tiledBackingStorePaint(&context, m_backingStore->mapToContents(rect));
        context.restore();
    }

    return updatedRects;
}

void TileQt::swapBackBufferToFront()
{
    if (!m_backBuffer)
        return;
    delete m_buffer;
    m_buffer = m_backBuffer;
    m_backBuffer = 0;
}

void TileQt::paint(GraphicsContext* context, const IntRect& rect)
{
    if (!m_buffer)
        return;
    
    IntRect target = intersection(rect, m_rect);
    IntRect source((target.x() - m_rect.x()),
                   (target.y() - m_rect.y()),
                   target.width(),
                   target.height());
    
    context->platformContext()->drawPixmap(target, *m_buffer, source);
}
    
void TileQt::resize(const IntSize& newSize)
{
    IntRect oldRect = m_rect;
    m_rect = IntRect(m_rect.location(), newSize);
    if (m_rect.maxX() > oldRect.maxX())
        invalidate(IntRect(oldRect.maxX(), oldRect.y(), m_rect.maxX() - oldRect.maxX(), m_rect.height()));
    if (m_rect.maxY() > oldRect.maxY())
        invalidate(IntRect(oldRect.x(), oldRect.maxY(), m_rect.width(), m_rect.maxY() - oldRect.maxY()));
}

void TiledBackingStoreBackend::paintCheckerPattern(GraphicsContext* context, const FloatRect& target)
{
    QPainter* painter = context->platformContext();
    QTransform worldTransform = painter->worldTransform();
    qreal scaleX = worldTransform.m11();
    qreal scaleY = worldTransform.m22();
    
    QRect targetViewRect = QRectF(target.x() * scaleX,
                                  target.y() * scaleY,
                                  target.width() * scaleX,
                                  target.height() * scaleY).toAlignedRect();
    
    QTransform adjustedTransform(1., worldTransform.m12(), worldTransform.m13(),
                  worldTransform.m21(), 1., worldTransform.m23(),
                  worldTransform.m31(), worldTransform.m32(), worldTransform.m33());
    painter->setWorldTransform(adjustedTransform);
    
    painter->drawTiledPixmap(targetViewRect,
                             checkeredPixmap(),
                             QPoint(targetViewRect.left() % checkerSize,
                                    targetViewRect.top() % checkerSize));
    
    painter->setWorldTransform(worldTransform);
}

PassRefPtr<Tile> TiledBackingStoreBackend::createTile(TiledBackingStore* backingStore, const Tile::Coordinate& tileCoordinate)
{
    return TileQt::create(backingStore, tileCoordinate);
}

}

#endif
