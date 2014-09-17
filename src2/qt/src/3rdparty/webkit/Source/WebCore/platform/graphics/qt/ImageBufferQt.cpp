/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Holger Hans Peter Freyther
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
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
#include "ImageBuffer.h"

#include "GraphicsContext.h"
#include "ImageData.h"
#include "MIMETypeRegistry.h"
#include "StillImageQt.h"
#include "TransparencyLayer.h"
#include <wtf/text/CString.h>
#include <wtf/text/StringConcatenate.h>

#include <QBuffer>
#include <QColor>
#include <QImage>
#include <QImageWriter>
#include <QPainter>
#include <QPixmap>
#include <math.h>

namespace WebCore {

ImageBufferData::ImageBufferData(const IntSize& size)
    : m_pixmap(size)
{
    if (m_pixmap.isNull())
        return;

    m_pixmap.fill(QColor(Qt::transparent));

    QPainter* painter = new QPainter;
    m_painter = adoptPtr(painter);

    if (!painter->begin(&m_pixmap))
        return;

    // Since ImageBuffer is used mainly for Canvas, explicitly initialize
    // its painter's pen and brush with the corresponding canvas defaults
    // NOTE: keep in sync with CanvasRenderingContext2D::State
    QPen pen = painter->pen();
    pen.setColor(Qt::black);
    pen.setWidth(1);
    pen.setCapStyle(Qt::FlatCap);
    pen.setJoinStyle(Qt::SvgMiterJoin);
    pen.setMiterLimit(10);
    painter->setPen(pen);
    QBrush brush = painter->brush();
    brush.setColor(Qt::black);
    painter->setBrush(brush);
    painter->setCompositionMode(QPainter::CompositionMode_SourceOver);
    
    m_image = StillImage::createForRendering(&m_pixmap);
}

QImage ImageBufferData::toQImage() const
{
    QPaintEngine* paintEngine = m_pixmap.paintEngine();
    if (!paintEngine || paintEngine->type() != QPaintEngine::Raster)
        return m_pixmap.toImage();

    // QRasterPixmapData::toImage() will deep-copy the backing QImage if there's an active QPainter on it.
    // For performance reasons, we don't want that here, so we temporarily redirect the paint engine.
    QPaintDevice* currentPaintDevice = paintEngine->paintDevice();
    paintEngine->setPaintDevice(0);
    QImage image = m_pixmap.toImage();
    paintEngine->setPaintDevice(currentPaintDevice);
    return image;
}

ImageBuffer::ImageBuffer(const IntSize& size, ColorSpace, RenderingMode, bool& success)
    : m_data(size)
    , m_size(size)
{
    success = m_data.m_painter && m_data.m_painter->isActive();
    if (!success)
        return;

    m_context = adoptPtr(new GraphicsContext(m_data.m_painter.get()));
}

ImageBuffer::~ImageBuffer()
{
}

size_t ImageBuffer::dataSize() const
{
    return m_size.width() * m_size.height() * 4;
}

GraphicsContext* ImageBuffer::context() const
{
    ASSERT(m_data.m_painter->isActive());

    return m_context.get();
}

bool ImageBuffer::drawsUsingCopy() const
{
    return false;
}

PassRefPtr<Image> ImageBuffer::copyImage() const
{
    return StillImage::create(m_data.m_pixmap);
}

void ImageBuffer::draw(GraphicsContext* destContext, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
                       CompositeOperator op, bool useLowQualityScale)
{
    if (destContext == context()) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage();
        destContext->drawImage(copy.get(), ColorSpaceDeviceRGB, destRect, srcRect, op, useLowQualityScale);
    } else
        destContext->drawImage(m_data.m_image.get(), styleColorSpace, destRect, srcRect, op, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* destContext, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    if (destContext == context()) {
        // We're drawing into our own buffer.  In order for this to work, we need to copy the source buffer first.
        RefPtr<Image> copy = copyImage();
        copy->drawPattern(destContext, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
    } else
        m_data.m_image->drawPattern(destContext, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

void ImageBuffer::clip(GraphicsContext* context, const FloatRect& floatRect) const
{
    QPixmap* nativeImage = m_data.m_image->nativeImageForCurrentFrame();
    if (!nativeImage)
        return;

    IntRect rect = enclosingIntRect(floatRect);
    QPixmap alphaMask = *nativeImage;
    if (alphaMask.width() != rect.width() || alphaMask.height() != rect.height())
        alphaMask = alphaMask.scaled(rect.width(), rect.height());

    context->pushTransparencyLayerInternal(rect, 1.0, alphaMask);
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    bool isPainting = m_data.m_painter->isActive();
    if (isPainting)
        m_data.m_painter->end();

    QImage image = m_data.toQImage().convertToFormat(QImage::Format_ARGB32);
    ASSERT(!image.isNull());

    uchar* bits = image.bits();
    const int bytesPerLine = image.bytesPerLine();

    for (int y = 0; y < m_size.height(); ++y) {
        quint32* scanLine = reinterpret_cast_ptr<quint32*>(bits + y * bytesPerLine);
        for (int x = 0; x < m_size.width(); ++x) {
            QRgb& pixel = scanLine[x];
            pixel = qRgba(lookUpTable[qRed(pixel)],
                          lookUpTable[qGreen(pixel)],
                          lookUpTable[qBlue(pixel)],
                          qAlpha(pixel));
        }
    }

    m_data.m_pixmap = QPixmap::fromImage(image);

    if (isPainting)
        m_data.m_painter->begin(&m_data.m_pixmap);
}

template <Multiply multiplied>
PassRefPtr<ByteArray> getImageData(const IntRect& rect, const ImageBufferData& imageData, const IntSize& size)
{
    RefPtr<ByteArray> result = ByteArray::create(rect.width() * rect.height() * 4);
    unsigned char* data = result->data();

    if (rect.x() < 0 || rect.y() < 0 || rect.maxX() > size.width() || rect.maxY() > size.height())
        memset(data, 0, result->length());

    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();
    int numColumns = endx - originx;

    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();
    int numRows = endy - originy;

    // NOTE: For unmultiplied data, we undo the premultiplication below.
    QImage image = imageData.toQImage().convertToFormat(QImage::Format_ARGB32_Premultiplied);

    ASSERT(!image.isNull());

    const int bytesPerLine = image.bytesPerLine();
    const uchar* bits = image.constBits();

    quint32* destRows = reinterpret_cast_ptr<quint32*>(&data[desty * rect.width() * 4 + destx * 4]);

    if (multiplied == Unmultiplied) {
        for (int y = 0; y < numRows; ++y) {
            const quint32* scanLine = reinterpret_cast_ptr<const quint32*>(bits + (y + originy) * bytesPerLine);
            for (int x = 0; x < numColumns; x++) {
                QRgb pixel = scanLine[x + originx];
                int alpha = qAlpha(pixel);
                // Un-premultiply and convert RGB to BGR.
                if (alpha == 255)
                    destRows[x] = (0xFF000000
                                | (qBlue(pixel) << 16)
                                | (qGreen(pixel) << 8)
                                | (qRed(pixel)));
                else if (alpha > 0)
                    destRows[x] = ((alpha << 24)
                                | (((255 * qBlue(pixel)) / alpha)) << 16)
                                | (((255 * qGreen(pixel)) / alpha) << 8)
                                | ((255 * qRed(pixel)) / alpha);
                else
                    destRows[x] = 0;
            }
            destRows += rect.width();
        }
    } else {
        for (int y = 0; y < numRows; ++y) {
            const quint32* scanLine = reinterpret_cast_ptr<const quint32*>(bits + (y + originy) * bytesPerLine);
            for (int x = 0; x < numColumns; x++) {
                QRgb pixel = scanLine[x + originx];
                // Convert RGB to BGR.
                destRows[x] = ((pixel << 16) & 0xff0000) | ((pixel >> 16) & 0xff) | (pixel & 0xff00ff00);

            }
            destRows += rect.width();
        }
    }

    return result.release();
}

PassRefPtr<ByteArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect) const
{
    return getImageData<Unmultiplied>(rect, m_data, m_size);
}

PassRefPtr<ByteArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect) const
{
    return getImageData<Premultiplied>(rect, m_data, m_size);
}

static inline unsigned int premultiplyABGRtoARGB(unsigned int x)
{
    unsigned int a = x >> 24;
    if (a == 255)
        return (x << 16) | ((x >> 16) & 0xff) | (x & 0xff00ff00);
    unsigned int t = (x & 0xff00ff) * a;
    t = (t + ((t >> 8) & 0xff00ff) + 0x800080) >> 8;
    t = ((t << 16) | (t >> 16)) & 0xff00ff;

    x = ((x >> 8) & 0xff) * a;
    x = (x + ((x >> 8) & 0xff) + 0x80);
    x &= 0xff00;
    x |= t | (a << 24);
    return x;
}

template <Multiply multiplied>
void putImageData(ByteArray*& source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, ImageBufferData& data, const IntSize& size)
{
    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);

    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.maxX());

    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= size.width());

    int numColumns = endx - destx;

    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.maxY());

    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= size.height());
    int numRows = endy - desty;

    unsigned srcBytesPerRow = 4 * sourceSize.width();

    // NOTE: For unmultiplied input data, we do the premultiplication below.
    QImage image(numColumns, numRows, QImage::Format_ARGB32_Premultiplied);
    uchar* bits = image.bits();
    const int bytesPerLine = image.bytesPerLine();

    const quint32* srcScanLine = reinterpret_cast_ptr<const quint32*>(source->data() + originy * srcBytesPerRow + originx * 4);

    if (multiplied == Unmultiplied) {
        for (int y = 0; y < numRows; ++y) {
            quint32* destScanLine = reinterpret_cast_ptr<quint32*>(bits + y * bytesPerLine);
            for (int x = 0; x < numColumns; x++) {
                // Premultiply and convert BGR to RGB.
                quint32 pixel = srcScanLine[x];
                destScanLine[x] = premultiplyABGRtoARGB(pixel);
            }
            srcScanLine += sourceSize.width();
        }
    } else {
        for (int y = 0; y < numRows; ++y) {
            quint32* destScanLine = reinterpret_cast_ptr<quint32*>(bits + y * bytesPerLine);
            for (int x = 0; x < numColumns; x++) {
                // Convert BGR to RGB.
                quint32 pixel = srcScanLine[x];
                destScanLine[x] = ((pixel << 16) & 0xff0000) | ((pixel >> 16) & 0xff) | (pixel & 0xff00ff00);
            }
            srcScanLine += sourceSize.width();
        }
    }

    bool isPainting = data.m_painter->isActive();
    if (!isPainting)
        data.m_painter->begin(&data.m_pixmap);
    else {
        data.m_painter->save();

        // putImageData() should be unaffected by painter state
        data.m_painter->resetTransform();
        data.m_painter->setOpacity(1.0);
        data.m_painter->setClipping(false);
    }

    data.m_painter->setCompositionMode(QPainter::CompositionMode_Source);
    data.m_painter->drawImage(destx, desty, image);

    if (!isPainting)
        data.m_painter->end();
    else
        data.m_painter->restore();
}

void ImageBuffer::putUnmultipliedImageData(ByteArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint)
{
    putImageData<Unmultiplied>(source, sourceSize, sourceRect, destPoint, m_data, m_size);
}

void ImageBuffer::putPremultipliedImageData(ByteArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint)
{
    putImageData<Premultiplied>(source, sourceSize, sourceRect, destPoint, m_data, m_size);
}

// We get a mimeType here but QImageWriter does not support mimetypes but
// only formats (png, gif, jpeg..., xpm). So assume we get image/ as image
// mimetypes and then remove the image/ to get the Qt format.
String ImageBuffer::toDataURL(const String& mimeType, const double* quality) const
{
    ASSERT(MIMETypeRegistry::isSupportedImageMIMETypeForEncoding(mimeType));

    if (!mimeType.startsWith("image/"))
        return "data:,";

    // prepare our target
    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QBuffer::WriteOnly);

    if (quality && *quality >= 0.0 && *quality <= 1.0) {
        if (!m_data.m_pixmap.save(&buffer, mimeType.substring(sizeof "image").utf8().data(), *quality * 100 + 0.5)) {
            buffer.close();
            return "data:,";
        }
    } else {
        if (!m_data.m_pixmap.save(&buffer, mimeType.substring(sizeof "image").utf8().data(), 100)) {
            buffer.close();
            return "data:,";
        }
    }

    buffer.close();

    return makeString("data:", mimeType, ";base64,", data.toBase64().data());
}

}
