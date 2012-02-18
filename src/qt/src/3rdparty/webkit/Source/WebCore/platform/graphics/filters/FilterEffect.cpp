/*
 * Copyright (C) 2008 Alex Mathews <possessedpenguinbob@gmail.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(FILTERS)
#include "FilterEffect.h"

#include "Filter.h"
#include "ImageBuffer.h"
#include "TextStream.h"
#include <wtf/ByteArray.h>

namespace WebCore {

FilterEffect::FilterEffect(Filter* filter)
    : m_alphaImage(false)
    , m_filter(filter)
    , m_hasX(false)
    , m_hasY(false)
    , m_hasWidth(false)
    , m_hasHeight(false)
{
    ASSERT(m_filter);
}

FilterEffect::~FilterEffect()
{
}

void FilterEffect::determineAbsolutePaintRect()
{
    m_absolutePaintRect = IntRect();
    unsigned size = m_inputEffects.size();
    for (unsigned i = 0; i < size; ++i)
        m_absolutePaintRect.unite(m_inputEffects.at(i)->absolutePaintRect());
    
    // SVG specification wants us to clip to primitive subregion.
    m_absolutePaintRect.intersect(m_maxEffectRect);
}

IntRect FilterEffect::requestedRegionOfInputImageData(const IntRect& effectRect) const
{
    ASSERT(hasResult());
    IntPoint location = m_absolutePaintRect.location();
    location.move(-effectRect.x(), -effectRect.y());
    return IntRect(location, m_absolutePaintRect.size());
}

IntRect FilterEffect::drawingRegionOfInputImage(const IntRect& srcRect) const
{
    return IntRect(IntPoint(srcRect.x() - m_absolutePaintRect.x(),
                            srcRect.y() - m_absolutePaintRect.y()), srcRect.size());
}

FilterEffect* FilterEffect::inputEffect(unsigned number) const
{
    ASSERT(number < m_inputEffects.size());
    return m_inputEffects.at(number).get();
}

void FilterEffect::clearResult()
{
    if (m_imageBufferResult)
        m_imageBufferResult.clear();
    if (m_unmultipliedImageResult)
        m_unmultipliedImageResult.clear();
    if (m_premultipliedImageResult)
        m_premultipliedImageResult.clear();
}

ImageBuffer* FilterEffect::asImageBuffer()
{
    if (!hasResult())
        return 0;
    if (m_imageBufferResult)
        return m_imageBufferResult.get();
    m_imageBufferResult = ImageBuffer::create(m_absolutePaintRect.size(), ColorSpaceLinearRGB);
    IntRect destinationRect(IntPoint(), m_absolutePaintRect.size());
    if (m_premultipliedImageResult)
        m_imageBufferResult->putPremultipliedImageData(m_premultipliedImageResult.get(), destinationRect.size(), destinationRect, IntPoint());
    else
        m_imageBufferResult->putUnmultipliedImageData(m_unmultipliedImageResult.get(), destinationRect.size(), destinationRect, IntPoint());
    return m_imageBufferResult.get();
}

PassRefPtr<ByteArray> FilterEffect::asUnmultipliedImage(const IntRect& rect)
{
    RefPtr<ByteArray> imageData = ByteArray::create(rect.width() * rect.height() * 4);
    copyUnmultipliedImage(imageData.get(), rect);
    return imageData.release();
}

PassRefPtr<ByteArray> FilterEffect::asPremultipliedImage(const IntRect& rect)
{
    RefPtr<ByteArray> imageData = ByteArray::create(rect.width() * rect.height() * 4);
    copyPremultipliedImage(imageData.get(), rect);
    return imageData.release();
}

inline void FilterEffect::copyImageBytes(ByteArray* source, ByteArray* destination, const IntRect& rect)
{
    // Initialize the destination to transparent black, if not entirely covered by the source.
    if (rect.x() < 0 || rect.y() < 0 || rect.maxX() > m_absolutePaintRect.width() || rect.maxY() > m_absolutePaintRect.height())
        memset(destination->data(), 0, destination->length());

    // Early return if the rect does not intersect with the source.
    if (rect.maxX() <= 0 || rect.maxY() <= 0 || rect.x() >= m_absolutePaintRect.width() || rect.y() >= m_absolutePaintRect.height())
        return;

    int xOrigin = rect.x();
    int xDest = 0;
    if (xOrigin < 0) {
        xDest = -xOrigin;
        xOrigin = 0;
    }
    int xEnd = rect.maxX();
    if (xEnd > m_absolutePaintRect.width())
        xEnd = m_absolutePaintRect.width();

    int yOrigin = rect.y();
    int yDest = 0;
    if (yOrigin < 0) {
        yDest = -yOrigin;
        yOrigin = 0;
    }
    int yEnd = rect.maxY();
    if (yEnd > m_absolutePaintRect.height())
        yEnd = m_absolutePaintRect.height();

    int size = (xEnd - xOrigin) * 4;
    int destinationScanline = rect.width() * 4;
    int sourceScanline = m_absolutePaintRect.width() * 4;
    unsigned char *destinationPixel = destination->data() + ((yDest * rect.width()) + xDest) * 4;
    unsigned char *sourcePixel = source->data() + ((yOrigin * m_absolutePaintRect.width()) + xOrigin) * 4;

    while (yOrigin < yEnd) {
        memcpy(destinationPixel, sourcePixel, size);
        destinationPixel += destinationScanline;
        sourcePixel += sourceScanline;
        ++yOrigin;
    }
}

void FilterEffect::copyUnmultipliedImage(ByteArray* destination, const IntRect& rect)
{
    ASSERT(hasResult());

    if (!m_unmultipliedImageResult) {
        // We prefer a conversion from the image buffer.
        if (m_imageBufferResult)
            m_unmultipliedImageResult = m_imageBufferResult->getUnmultipliedImageData(IntRect(IntPoint(), m_absolutePaintRect.size()));
        else {
            m_unmultipliedImageResult = ByteArray::create(m_absolutePaintRect.width() * m_absolutePaintRect.height() * 4);
            unsigned char* sourceComponent = m_premultipliedImageResult->data();
            unsigned char* destinationComponent = m_unmultipliedImageResult->data();
            unsigned char* end = sourceComponent + (m_absolutePaintRect.width() * m_absolutePaintRect.height() * 4);
            while (sourceComponent < end) {
                int alpha = sourceComponent[3];
                if (alpha) {
                    destinationComponent[0] = static_cast<int>(sourceComponent[0]) * 255 / alpha;
                    destinationComponent[1] = static_cast<int>(sourceComponent[1]) * 255 / alpha;
                    destinationComponent[2] = static_cast<int>(sourceComponent[2]) * 255 / alpha;
                } else {
                    destinationComponent[0] = 0;
                    destinationComponent[1] = 0;
                    destinationComponent[2] = 0;
                }
                destinationComponent[3] = alpha;
                sourceComponent += 4;
                destinationComponent += 4;
            }
        }
    }
    copyImageBytes(m_unmultipliedImageResult.get(), destination, rect);
}

void FilterEffect::copyPremultipliedImage(ByteArray* destination, const IntRect& rect)
{
    ASSERT(hasResult());

    if (!m_premultipliedImageResult) {
        // We prefer a conversion from the image buffer.
        if (m_imageBufferResult)
            m_premultipliedImageResult = m_imageBufferResult->getPremultipliedImageData(IntRect(IntPoint(), m_absolutePaintRect.size()));
        else {
            m_premultipliedImageResult = ByteArray::create(m_absolutePaintRect.width() * m_absolutePaintRect.height() * 4);
            unsigned char* sourceComponent = m_unmultipliedImageResult->data();
            unsigned char* destinationComponent = m_premultipliedImageResult->data();
            unsigned char* end = sourceComponent + (m_absolutePaintRect.width() * m_absolutePaintRect.height() * 4);
            while (sourceComponent < end) {
                int alpha = sourceComponent[3];
                destinationComponent[0] = static_cast<int>(sourceComponent[0]) * alpha / 255;
                destinationComponent[1] = static_cast<int>(sourceComponent[1]) * alpha / 255;
                destinationComponent[2] = static_cast<int>(sourceComponent[2]) * alpha / 255;
                destinationComponent[3] = alpha;
                sourceComponent += 4;
                destinationComponent += 4;
            }
        }
    }
    copyImageBytes(m_premultipliedImageResult.get(), destination, rect);
}

ImageBuffer* FilterEffect::createImageBufferResult()
{
    // Only one result type is allowed.
    ASSERT(!hasResult());
    determineAbsolutePaintRect();
    if (m_absolutePaintRect.isEmpty())
        return 0;
    m_imageBufferResult = ImageBuffer::create(m_absolutePaintRect.size(), ColorSpaceLinearRGB);
    if (!m_imageBufferResult)
        return 0;
    ASSERT(m_imageBufferResult->context());
    return m_imageBufferResult.get();
}

ByteArray* FilterEffect::createUnmultipliedImageResult()
{
    // Only one result type is allowed.
    ASSERT(!hasResult());
    determineAbsolutePaintRect();
    if (m_absolutePaintRect.isEmpty())
        return 0;
    m_unmultipliedImageResult = ByteArray::create(m_absolutePaintRect.width() * m_absolutePaintRect.height() * 4);
    return m_unmultipliedImageResult.get();
}

ByteArray* FilterEffect::createPremultipliedImageResult()
{
    // Only one result type is allowed.
    ASSERT(!hasResult());
    determineAbsolutePaintRect();
    if (m_absolutePaintRect.isEmpty())
        return 0;
    m_premultipliedImageResult = ByteArray::create(m_absolutePaintRect.width() * m_absolutePaintRect.height() * 4);
    return m_premultipliedImageResult.get();
}

TextStream& FilterEffect::externalRepresentation(TextStream& ts, int) const
{
    // FIXME: We should dump the subRegions of the filter primitives here later. This isn't
    // possible at the moment, because we need more detailed informations from the target object.
    return ts;
}

} // namespace WebCore

#endif // ENABLE(FILTERS)
