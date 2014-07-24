/*
 * Copyright (C) 2009 Torch Mobile, Inc. All rights reserved.
 * Copyright (C) 2010 Torch Mobile (Beijing) Co. Ltd. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "ImageBuffer.h"

#include "GraphicsContext.h"
#include "Image.h"
#include "ImageData.h"
#include "NotImplemented.h"
#include "SharedBitmap.h"

namespace WebCore {

class BufferedImage: public Image {

public:
    BufferedImage(const ImageBufferData* data)
        : m_data(data)
    {
    }

    virtual bool currentFrameKnownToBeOpaque() { return !m_data->m_bitmap->hasAlpha() ; }
    virtual IntSize size() const { return IntSize(m_data->m_bitmap->width(), m_data->m_bitmap->height()); }
    virtual void destroyDecodedData(bool destroyAll = true) {}
    virtual unsigned decodedSize() const { return 0; }
    virtual void draw(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator, BlendMode);
    virtual void drawPattern(GraphicsContext*, const FloatRect& srcRect, const AffineTransform& patternTransform,
                             const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator, const FloatRect& destRect);

    const ImageBufferData* m_data;
};

void BufferedImage::draw(GraphicsContext* ctxt, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode)
{
    IntRect intDstRect = enclosingIntRect(dstRect);
    IntRect intSrcRect(srcRect);
    m_data->m_bitmap->draw(ctxt, intDstRect, intSrcRect, styleColorSpace, compositeOp, blendMode);
}

void BufferedImage::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                             const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    m_data->m_bitmap->drawPattern(ctxt, tileRectIn, patternTransform, phase, styleColorSpace, op, destRect, size());
}

ImageBufferData::ImageBufferData(const IntSize& size)
    : m_bitmap(SharedBitmap::create(size, BitmapInfo::BitCount32, false))
{
    // http://www.w3.org/TR/2009/WD-html5-20090212/the-canvas-element.html#canvaspixelarray
    // "When the canvas is initialized it must be set to fully transparent black."
    m_bitmap->resetPixels(true);
    m_bitmap->setHasAlpha(true);
}

ImageBuffer::ImageBuffer(const IntSize& size, float resolutionScale, ColorSpace colorSpace, RenderingMode, bool& success)
    : m_data(size)
    , m_size(size)
    , m_logicalSize(size)
{
    // FIXME: Respect resoutionScale to support high-DPI canvas.
    UNUSED_PARAM(resolutionScale);
    // FIXME: colorSpace is not used
    UNUSED_PARAM(colorSpace);

    m_context = adoptPtr(new GraphicsContext(0));
    m_context->setBitmap(m_data.m_bitmap);
    success = true;
}

ImageBuffer::~ImageBuffer()
{
}

GraphicsContext* ImageBuffer::context() const
{
    return m_context.get();
}

PassRefPtr<Image> ImageBuffer::copyImage(BackingStoreCopy copyBehavior, ScaleBehavior) const
{
    ASSERT(copyBehavior == CopyBackingStore);
    return adoptRef(new BufferedImage(&m_data));
}

BackingStoreCopy ImageBuffer::fastCopyImageMode()
{
    return CopyBackingStore;
}

void ImageBuffer::clip(GraphicsContext*, const FloatRect&) const
{
    notImplemented();
}

void ImageBuffer::draw(GraphicsContext* context, ColorSpace styleColorSpace, const FloatRect& destRect, const FloatRect& srcRect,
                       CompositeOperator op, BlendMode blendMode, bool useLowQualityScale)
{
    RefPtr<Image> imageCopy = copyImage(CopyBackingStore);
    context->drawImage(imageCopy.get(), styleColorSpace, destRect, srcRect, op, blendMode, DoNotRespectImageOrientation, useLowQualityScale);
}

void ImageBuffer::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransform,
                              const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<Image> imageCopy = copyImage(CopyBackingStore);
    imageCopy->drawPattern(context, srcRect, patternTransform, phase, styleColorSpace, op, destRect);
}

template <bool premultiplied>
static PassRefPtr<Uint8ClampedArray> getImageData(const IntRect& rect, const SharedBitmap* bitmap)
{
    RefPtr<Uint8ClampedArray> imageData = Uint8ClampedArray::createUninitialized(rect.width() * rect.height() * 4);

    const unsigned char* src = static_cast<const unsigned char*>(bitmap->bytes());
    if (!src)
        return imageData.release();

    IntRect sourceRect(0, 0, bitmap->width(), bitmap->height());
    sourceRect.intersect(rect);
    if (sourceRect.isEmpty())
        return imageData.release();

    unsigned char* dst = imageData->data();
    imageData->zeroFill();
    src += (sourceRect.y() * bitmap->width() + sourceRect.x()) * 4;
    dst += ((sourceRect.y() - rect.y()) * rect.width() + sourceRect.x() - rect.x()) * 4;
    int bytesToCopy = sourceRect.width() * 4;
    int srcSkip = (bitmap->width() - sourceRect.width()) * 4;
    int dstSkip = (rect.width() - sourceRect.width()) * 4;
    const unsigned char* dstEnd = dst + sourceRect.height() * rect.width() * 4;
    while (dst < dstEnd) {
        const unsigned char* dstRowEnd = dst + bytesToCopy;
        while (dst < dstRowEnd) {
            // Convert ARGB little endian to RGBA big endian
            int blue = *src++;
            int green = *src++;
            int red = *src++;
            int alpha = *src++;
            if (premultiplied) {
                *dst++ = static_cast<unsigned char>((red * alpha + 254) / 255);
                *dst++ = static_cast<unsigned char>((green * alpha + 254) / 255);
                *dst++ = static_cast<unsigned char>((blue * alpha + 254) / 255);
                *dst++ = static_cast<unsigned char>(alpha);
            } else {
                *dst++ = static_cast<unsigned char>(red);
                *dst++ = static_cast<unsigned char>(green);
                *dst++ = static_cast<unsigned char>(blue);
                *dst++ = static_cast<unsigned char>(alpha);
                ++src;
            }
        }
        src += srcSkip;
        dst += dstSkip;
    }

    return imageData.release();
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getUnmultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<false>(rect, m_data.m_bitmap.get());
}

PassRefPtr<Uint8ClampedArray> ImageBuffer::getPremultipliedImageData(const IntRect& rect, CoordinateSystem) const
{
    return getImageData<true>(rect, m_data.m_bitmap.get());
}

void ImageBuffer::putByteArray(Multiply multiplied, Uint8ClampedArray* source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, CoordinateSystem)
{
    SharedBitmap* bitmap = m_data.m_bitmap.get();
    unsigned char* dst = (unsigned char*)bitmap->bytes();
    if (!dst)
        return;

    IntRect destRect(destPoint, sourceRect.size());
    destRect.intersect(IntRect(0, 0, bitmap->width(), bitmap->height()));

    if (destRect.isEmpty())
        return;

    const unsigned char* src = source->data();
    dst += (destRect.y() * bitmap->width() + destRect.x()) * 4;
    src += (sourceRect.y() * sourceSize.width() + sourceRect.x()) * 4;
    int bytesToCopy = destRect.width() * 4;
    int dstSkip = (bitmap->width() - destRect.width()) * 4;
    int srcSkip = (sourceSize.width() - destRect.width()) * 4;
    const unsigned char* dstEnd = dst + destRect.height() * bitmap->width() * 4;
    while (dst < dstEnd) {
        const unsigned char* dstRowEnd = dst + bytesToCopy;
        while (dst < dstRowEnd) {
            // Convert RGBA big endian to ARGB little endian
            int red = *src++;
            int green = *src++;
            int blue = *src++;
            int alpha = *src++;
            if (multiplied == Premultiplied) {
                *dst++ = static_cast<unsigned char>(blue * 255 / alpha);
                *dst++ = static_cast<unsigned char>(green * 255 / alpha);
                *dst++ = static_cast<unsigned char>(red * 255 / alpha);
                *dst++ = static_cast<unsigned char>(alpha);
            } else {
                *dst++ = static_cast<unsigned char>(blue);
                *dst++ = static_cast<unsigned char>(green);
                *dst++ = static_cast<unsigned char>(red);
                *dst++ = static_cast<unsigned char>(alpha);
            }
        }
        src += srcSkip;
        dst += dstSkip;
    }
}

void ImageBuffer::platformTransformColorSpace(const Vector<int>& lookUpTable)
{
    UNUSED_PARAM(lookUpTable);
    notImplemented();
}

String ImageBuffer::toDataURL(const String& mimeType, const double*, CoordinateSystem) const
{
    if (!m_data.m_bitmap->bytes())
        return "data:,";

    notImplemented();
    return String();
}

} // namespace WebCore
