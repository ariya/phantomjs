/*
 * Copyright (C) 2010 Sencha, Inc.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ContextShadow.h"

#include "AffineTransform.h"
#include "GraphicsContext.h"
#include <QPainter>
#include <QTimerEvent>

namespace WebCore {

// ContextShadow needs a scratch image as the buffer for the blur filter.
// Instead of creating and destroying the buffer for every operation,
// we create a buffer which will be automatically purged via a timer.

class ShadowBuffer: public QObject {
public:
    ShadowBuffer(QObject* parent = 0);

    QImage* scratchImage(const QSize& size);

    void schedulePurge();

protected:
    void timerEvent(QTimerEvent* event);

private:
    QImage image;
    int timerId;
};

ShadowBuffer::ShadowBuffer(QObject* parent)
    : QObject(parent)
    , timerId(-1)
{
}

QImage* ShadowBuffer::scratchImage(const QSize& size)
{
    int width = size.width();
    int height = size.height();

    // We do not need to recreate the buffer if the buffer is reasonably
    // larger than the requested size. However, if the requested size is
    // much smaller than our buffer, reduce our buffer so that we will not
    // keep too many allocated pixels for too long.
    if (!image.isNull() && (image.width() > width) && (image.height() > height))
        if (((2 * width) > image.width()) && ((2 * height) > image.height())) {
            image.fill(0);
            return &image;
        }

    // Round to the nearest 32 pixels so we do not grow the buffer everytime
    // there is larger request by 1 pixel.
    width = (1 + (width >> 5)) << 5;
    height = (1 + (height >> 5)) << 5;

    image = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    return &image;
}

void ShadowBuffer::schedulePurge()
{
    static const double BufferPurgeDelay = 2; // seconds
    if (timerId >= 0)
        killTimer(timerId);
    timerId = startTimer(BufferPurgeDelay * 1000);
}

void ShadowBuffer::timerEvent(QTimerEvent* event)
{
    if (event->timerId() == timerId) {
        killTimer(timerId);
        image = QImage();
    }
    QObject::timerEvent(event);
}

Q_GLOBAL_STATIC(ShadowBuffer, scratchShadowBuffer)

PlatformContext ContextShadow::beginShadowLayer(GraphicsContext* context, const FloatRect& layerArea)
{
    // Set m_blurDistance.
    adjustBlurDistance(context);

    PlatformContext p = context->platformContext();

    QRect clipRect;
    if (p->hasClipping())
#if QT_VERSION >= QT_VERSION_CHECK(4, 8, 0)
        clipRect = p->clipBoundingRect().toAlignedRect();
#else
        clipRect = p->clipRegion().boundingRect();
#endif
    else
        clipRect = p->transform().inverted().mapRect(p->window());

    // Set m_layerOrigin, m_layerContextTranslation, m_sourceRect.
    IntRect clip(clipRect.x(), clipRect.y(), clipRect.width(), clipRect.height());
    IntRect layerRect = calculateLayerBoundingRect(context, layerArea, clip);

    // Don't paint if we are totally outside the clip region.
    if (layerRect.isEmpty())
        return 0;

    ShadowBuffer* shadowBuffer = scratchShadowBuffer();
    QImage* shadowImage = shadowBuffer->scratchImage(layerRect.size());
    m_layerImage = QImage(*shadowImage);

    m_layerContext = new QPainter;
    m_layerContext->begin(&m_layerImage);
    m_layerContext->setFont(p->font());
    m_layerContext->translate(m_layerContextTranslation);
    return m_layerContext;
}

void ContextShadow::endShadowLayer(GraphicsContext* context)
{
    m_layerContext->end();
    delete m_layerContext;
    m_layerContext = 0;

    if (m_type == BlurShadow) {
        blurLayerImage(m_layerImage.bits(), IntSize(m_layerImage.width(), m_layerImage.height()),
                       m_layerImage.bytesPerLine());
    }

    if (m_type != NoShadow) {
        // "Colorize" with the right shadow color.
        QPainter p(&m_layerImage);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(m_layerImage.rect(), m_color.rgb());
        p.end();
    }

    context->platformContext()->drawImage(m_layerOrigin, m_layerImage, m_sourceRect);

    scratchShadowBuffer()->schedulePurge();
}

}
