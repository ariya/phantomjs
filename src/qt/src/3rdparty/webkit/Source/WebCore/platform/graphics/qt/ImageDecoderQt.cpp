/*
 * Copyright (C) 2006 Friedemann Kleint <fkleint@trolltech.com>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "ImageDecoderQt.h"

#include <QtCore/QByteArray>
#include <QtCore/QBuffer>

#include <QtGui/QImageReader>
#include <qdebug.h>

namespace WebCore {

ImageDecoder* ImageDecoder::create(const SharedBuffer& data, ImageSource::AlphaOption alphaOption, ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
{
    // We need at least 4 bytes to figure out what kind of image we're dealing with.
    if (data.size() < 4)
        return 0;

    return new ImageDecoderQt(alphaOption, gammaAndColorProfileOption);
}

ImageDecoderQt::ImageDecoderQt(ImageSource::AlphaOption alphaOption, ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : ImageDecoder(alphaOption, gammaAndColorProfileOption)
    , m_repetitionCount(cAnimationNone)
{
}

ImageDecoderQt::~ImageDecoderQt()
{
}

void ImageDecoderQt::setData(SharedBuffer* data, bool allDataReceived)
{
    if (failed())
        return;

    // No progressive loading possible
    if (!allDataReceived)
        return;

    // Cache our own new data.
    ImageDecoder::setData(data, allDataReceived);

    // We expect to be only called once with allDataReceived
    ASSERT(!m_buffer);
    ASSERT(!m_reader);

    // Attempt to load the data
    QByteArray imageData = QByteArray::fromRawData(m_data->data(), m_data->size());
    m_buffer = adoptPtr(new QBuffer);
    m_buffer->setData(imageData);
    m_buffer->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    m_reader = adoptPtr(new QImageReader(m_buffer.get(), m_format));

    // This will force the JPEG decoder to use JDCT_IFAST
    m_reader->setQuality(49);

    // QImageReader only allows retrieving the format before reading the image
    m_format = m_reader->format();
}

bool ImageDecoderQt::isSizeAvailable()
{
    if (!ImageDecoder::isSizeAvailable() && m_reader)
        internalDecodeSize();

    return ImageDecoder::isSizeAvailable();
}

size_t ImageDecoderQt::frameCount()
{
    if (m_frameBufferCache.isEmpty() && m_reader) {
        if (m_reader->supportsAnimation()) {
            int imageCount = m_reader->imageCount();

            // Fixup for Qt decoders... imageCount() is wrong
            // and jumpToNextImage does not work either... so
            // we will have to parse everything...
            if (!imageCount)
                forceLoadEverything();
            else {
                m_frameBufferCache.resize(imageCount);
                for (size_t i = 0; i < m_frameBufferCache.size(); ++i)
                    m_frameBufferCache[i].setPremultiplyAlpha(m_premultiplyAlpha);
            }
        } else {
            m_frameBufferCache.resize(1);
            m_frameBufferCache[0].setPremultiplyAlpha(m_premultiplyAlpha);
        }
    }

    return m_frameBufferCache.size();
}

int ImageDecoderQt::repetitionCount() const
{
    if (m_reader && m_reader->supportsAnimation())
        m_repetitionCount = m_reader->loopCount();
    return m_repetitionCount;
}

String ImageDecoderQt::filenameExtension() const
{
    return String(m_format.constData(), m_format.length());
};

ImageFrame* ImageDecoderQt::frameBufferAtIndex(size_t index)
{
    // In case the ImageDecoderQt got recreated we don't know
    // yet how many images we are going to have and need to
    // find that out now.
    size_t count = m_frameBufferCache.size();
    if (!failed() && !count) {
        internalDecodeSize();
        count = frameCount();
    }

    if (index >= count)
        return 0;

    ImageFrame& frame = m_frameBufferCache[index];
    if (frame.status() != ImageFrame::FrameComplete && m_reader)
        internalReadImage(index);
    return &frame;
}

void ImageDecoderQt::clearFrameBufferCache(size_t /*index*/)
{
}

void ImageDecoderQt::internalDecodeSize()
{
    ASSERT(m_reader);

    // If we have a QSize() something failed
    QSize size = m_reader->size();
    if (size.isEmpty()) {
        setFailed();
        return clearPointers();
    }

    setSize(size.width(), size.height());
}

void ImageDecoderQt::internalReadImage(size_t frameIndex)
{
    ASSERT(m_reader);

    if (m_reader->supportsAnimation())
        m_reader->jumpToImage(frameIndex);
    else if (frameIndex) {
        setFailed();
        return clearPointers();
    }

    if (!internalHandleCurrentImage(frameIndex))
      setFailed();

    // Attempt to return some memory
    for (int i = 0; i < m_frameBufferCache.size(); ++i) {
        if (m_frameBufferCache[i].status() != ImageFrame::FrameComplete)
            return;
    }

    clearPointers();
}

bool ImageDecoderQt::internalHandleCurrentImage(size_t frameIndex)
{
    QPixmap pixmap = QPixmap::fromImageReader(m_reader.get());

    if (pixmap.isNull()) {
        frameCount();
        repetitionCount();
        clearPointers();
        return false;
    }

    // now into the ImageFrame - even if the image is not
    ImageFrame* const buffer = &m_frameBufferCache[frameIndex];
    buffer->setOriginalFrameRect(m_reader->currentImageRect());
    buffer->setStatus(ImageFrame::FrameComplete);
    buffer->setDuration(m_reader->nextImageDelay());
    buffer->setPixmap(pixmap);
    return true;
}

// The QImageIOHandler is not able to tell us how many frames
// we have and we need to parse every image. We do this by
// increasing the m_frameBufferCache by one and try to parse
// the image. We stop when QImage::read fails and then need
// to resize the m_frameBufferCache to the final size and update
// the failed bit. If we failed to decode the first image
// then we truly failed to decode, otherwise we're OK.

// TODO: Do not increment the m_frameBufferCache.size() by one but more than one
void ImageDecoderQt::forceLoadEverything()
{
    int imageCount = 0;

    do {
        m_frameBufferCache.resize(++imageCount);
    } while (internalHandleCurrentImage(imageCount - 1));

    // If we failed decoding the first image we actually
    // have no images and need to set the failed bit.
    // Otherwise, we want to forget about
    // the last attempt to decode a image.
    m_frameBufferCache.resize(imageCount - 1);
    for (size_t i = 0; i < m_frameBufferCache.size(); ++i)
        m_frameBufferCache[i].setPremultiplyAlpha(m_premultiplyAlpha);
    if (imageCount == 1)
      setFailed();
}

void ImageDecoderQt::clearPointers()
{
    m_reader.clear();
    m_buffer.clear();
}
}

// vim: ts=4 sw=4 et
