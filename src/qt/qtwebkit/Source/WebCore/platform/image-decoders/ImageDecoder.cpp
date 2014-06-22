/*
 * Copyright (C) 2008-2009 Torch Mobile, Inc.
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
 *
 */

#include "config.h"
#include "ImageDecoder.h"

#include "BMPImageDecoder.h"
#include "GIFImageDecoder.h"
#include "ICOImageDecoder.h"
#if PLATFORM(QT)
#include "ImageDecoderQt.h"
#endif
#if !PLATFORM(QT) || USE(LIBJPEG)
#include "JPEGImageDecoder.h"
#endif
#include "PNGImageDecoder.h"
#include "SharedBuffer.h"
#if USE(WEBP)
#include "WEBPImageDecoder.h"
#endif

#include <algorithm>
#include <cmath>

using namespace std;

namespace WebCore {

namespace {

unsigned copyFromSharedBuffer(char* buffer, unsigned bufferLength, const SharedBuffer& sharedBuffer, unsigned offset)
{
    unsigned bytesExtracted = 0;
    const char* moreData;
    while (unsigned moreDataLength = sharedBuffer.getSomeData(moreData, offset)) {
        unsigned bytesToCopy = min(bufferLength - bytesExtracted, moreDataLength);
        memcpy(buffer + bytesExtracted, moreData, bytesToCopy);
        bytesExtracted += bytesToCopy;
        if (bytesExtracted == bufferLength)
            break;
        offset += bytesToCopy;
    }
    return bytesExtracted;
}

bool matchesGIFSignature(char* contents)
{
    return !memcmp(contents, "GIF87a", 6) || !memcmp(contents, "GIF89a", 6);
}

bool matchesPNGSignature(char* contents)
{
    return !memcmp(contents, "\x89\x50\x4E\x47\x0D\x0A\x1A\x0A", 8);
}

bool matchesJPEGSignature(char* contents)
{
    return !memcmp(contents, "\xFF\xD8\xFF", 3);
}

#if USE(WEBP)
bool matchesWebPSignature(char* contents)
{
    return !memcmp(contents, "RIFF", 4) && !memcmp(contents + 8, "WEBPVP", 6);
}
#endif

bool matchesBMPSignature(char* contents)
{
    return !memcmp(contents, "BM", 2);
}

bool matchesICOSignature(char* contents)
{
    return !memcmp(contents, "\x00\x00\x01\x00", 4);
}

bool matchesCURSignature(char* contents)
{
    return !memcmp(contents, "\x00\x00\x02\x00", 4);
}

}

ImageDecoder* ImageDecoder::create(const SharedBuffer& data, ImageSource::AlphaOption alphaOption, ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
{
    static const unsigned lengthOfLongestSignature = 14; // To wit: "RIFF????WEBPVP"
    char contents[lengthOfLongestSignature];
    unsigned length = copyFromSharedBuffer(contents, lengthOfLongestSignature, data, 0);
    if (length < lengthOfLongestSignature)
        return 0;

    if (matchesGIFSignature(contents))
        return new GIFImageDecoder(alphaOption, gammaAndColorProfileOption);

#if !PLATFORM(QT) || (PLATFORM(QT) && USE(LIBPNG))
    if (matchesPNGSignature(contents))
        return new PNGImageDecoder(alphaOption, gammaAndColorProfileOption);

    if (matchesICOSignature(contents) || matchesCURSignature(contents))
        return new ICOImageDecoder(alphaOption, gammaAndColorProfileOption);
#endif

#if !PLATFORM(QT) || (PLATFORM(QT) && USE(LIBJPEG))
    if (matchesJPEGSignature(contents))
        return new JPEGImageDecoder(alphaOption, gammaAndColorProfileOption);
#endif

#if USE(WEBP)
    if (matchesWebPSignature(contents))
        return new WEBPImageDecoder(alphaOption, gammaAndColorProfileOption);
#endif

    if (matchesBMPSignature(contents))
        return new BMPImageDecoder(alphaOption, gammaAndColorProfileOption);

#if PLATFORM(QT)
    return new ImageDecoderQt(alphaOption, gammaAndColorProfileOption);
#endif
    return 0;
}

ImageFrame::ImageFrame()
    : m_hasAlpha(false)
    , m_status(FrameEmpty)
    , m_duration(0)
    , m_disposalMethod(DisposeNotSpecified)
    , m_premultiplyAlpha(true)
{
} 

ImageFrame& ImageFrame::operator=(const ImageFrame& other)
{
    if (this == &other)
        return *this;

    copyBitmapData(other);
    setOriginalFrameRect(other.originalFrameRect());
    setStatus(other.status());
    setDuration(other.duration());
    setDisposalMethod(other.disposalMethod());
    setPremultiplyAlpha(other.premultiplyAlpha());
    return *this;
}

void ImageFrame::clearPixelData()
{
    m_backingStore.clear();
    m_bytes = 0;
    m_status = FrameEmpty;
    // NOTE: Do not reset other members here; clearFrameBufferCache() calls this
    // to free the bitmap data, but other functions like initFrameBuffer() and
    // frameComplete() may still need to read other metadata out of this frame
    // later.
}

void ImageFrame::zeroFillPixelData()
{
    memset(m_bytes, 0, m_size.width() * m_size.height() * sizeof(PixelData));
    m_hasAlpha = true;
}

void ImageFrame::zeroFillFrameRect(const IntRect& rect)
{
    ASSERT(IntRect(IntPoint(), m_size).contains(rect));

    if (rect.isEmpty())
        return;

    size_t rectWidthInBytes = rect.width() * sizeof(PixelData);
    PixelData* start = m_bytes + (rect.y() * width()) + rect.x();
    for (int i = 0; i < rect.height(); ++i) {
        memset(start, 0, rectWidthInBytes);
        start += width();
    }

    setHasAlpha(true);
}

bool ImageFrame::copyBitmapData(const ImageFrame& other)
{
    if (this == &other)
        return true;

    m_backingStore = other.m_backingStore;
    m_bytes = m_backingStore.data();
    m_size = other.m_size;
    setHasAlpha(other.m_hasAlpha);
    return true;
}

bool ImageFrame::setSize(int newWidth, int newHeight)
{
    ASSERT(!width() && !height());
    size_t backingStoreSize = newWidth * newHeight;
    if (!m_backingStore.tryReserveCapacity(backingStoreSize))
        return false;
    m_backingStore.resize(backingStoreSize);
    m_bytes = m_backingStore.data();
    m_size = IntSize(newWidth, newHeight);

    zeroFillPixelData();
    return true;
}

bool ImageFrame::hasAlpha() const
{
    return m_hasAlpha;
}

void ImageFrame::setHasAlpha(bool alpha)
{
    m_hasAlpha = alpha;
}

void ImageFrame::setColorProfile(const ColorProfile& colorProfile)
{
    m_colorProfile = colorProfile;
}

void ImageFrame::setStatus(FrameStatus status)
{
    m_status = status;
}

namespace {

enum MatchType {
    Exact,
    UpperBound,
    LowerBound
};

inline void fillScaledValues(Vector<int>& scaledValues, double scaleRate, int length)
{
    double inflateRate = 1. / scaleRate;
    scaledValues.reserveCapacity(static_cast<int>(length * scaleRate + 0.5));
    for (int scaledIndex = 0; ; ++scaledIndex) {
        int index = static_cast<int>(scaledIndex * inflateRate + 0.5);
        if (index >= length)
            break;
        scaledValues.append(index);
    }
}

template <MatchType type> int getScaledValue(const Vector<int>& scaledValues, int valueToMatch, int searchStart)
{
    if (scaledValues.isEmpty())
        return valueToMatch;

    const int* dataStart = scaledValues.data();
    const int* dataEnd = dataStart + scaledValues.size();
    const int* matched = std::lower_bound(dataStart + searchStart, dataEnd, valueToMatch);
    switch (type) {
    case Exact:
        return matched != dataEnd && *matched == valueToMatch ? matched - dataStart : -1;
    case LowerBound:
        return matched != dataEnd && *matched == valueToMatch ? matched - dataStart : matched - dataStart - 1;
    case UpperBound:
    default:
        return matched != dataEnd ? matched - dataStart : -1;
    }
}

}

bool ImageDecoder::frameHasAlphaAtIndex(size_t index) const
{
    return !frameIsCompleteAtIndex(index) || m_frameBufferCache[index].hasAlpha();
}

bool ImageDecoder::frameIsCompleteAtIndex(size_t index) const
{
    return (index < m_frameBufferCache.size()) && (m_frameBufferCache[index].status() == ImageFrame::FrameComplete);
}

unsigned ImageDecoder::frameBytesAtIndex(size_t index) const
{
    if (m_frameBufferCache.size() <= index)
        return 0;
    // FIXME: Use the dimension of the requested frame.
    return m_size.area() * sizeof(ImageFrame::PixelData);
}

void ImageDecoder::prepareScaleDataIfNecessary()
{
    m_scaled = false;
    m_scaledColumns.clear();
    m_scaledRows.clear();

    int width = size().width();
    int height = size().height();
    int numPixels = height * width;
    if (m_maxNumPixels <= 0 || numPixels <= m_maxNumPixels)
        return;

    m_scaled = true;
    double scale = sqrt(m_maxNumPixels / (double)numPixels);
    fillScaledValues(m_scaledColumns, scale, width);
    fillScaledValues(m_scaledRows, scale, height);
}

int ImageDecoder::upperBoundScaledX(int origX, int searchStart)
{
    return getScaledValue<UpperBound>(m_scaledColumns, origX, searchStart);
}

int ImageDecoder::lowerBoundScaledX(int origX, int searchStart)
{
    return getScaledValue<LowerBound>(m_scaledColumns, origX, searchStart);
}

int ImageDecoder::upperBoundScaledY(int origY, int searchStart)
{
    return getScaledValue<UpperBound>(m_scaledRows, origY, searchStart);
}

int ImageDecoder::lowerBoundScaledY(int origY, int searchStart)
{
    return getScaledValue<LowerBound>(m_scaledRows, origY, searchStart);
}

int ImageDecoder::scaledY(int origY, int searchStart)
{
    return getScaledValue<Exact>(m_scaledRows, origY, searchStart);
}

}
