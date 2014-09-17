/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp.toker@collabora.co.uk>
 * Copyright (C) 2008, Google Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc
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
#include "ImageSource.h"

#if PLATFORM(QT)
#include "ImageDecoderQt.h"
#else
#include "ImageDecoder.h"
#endif

namespace WebCore {

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
unsigned ImageSource::s_maxPixelsPerDecodedImage = 1024 * 1024;
#endif

ImageSource::ImageSource(ImageSource::AlphaOption alphaOption, ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : m_decoder(0)
    , m_alphaOption(alphaOption)
    , m_gammaAndColorProfileOption(gammaAndColorProfileOption)
{
}

ImageSource::~ImageSource()
{
    clear(true);
}

void ImageSource::clear(bool destroyAll, size_t clearBeforeFrame, SharedBuffer* data, bool allDataReceived)
{
    if (!destroyAll) {
        if (m_decoder)
            m_decoder->clearFrameBufferCache(clearBeforeFrame);
        return;
    }

    delete m_decoder;
    m_decoder = 0;
    if (data)
        setData(data, allDataReceived);
}

bool ImageSource::initialized() const
{
    return m_decoder;
}

void ImageSource::setData(SharedBuffer* data, bool allDataReceived)
{
    // Make the decoder by sniffing the bytes.
    // This method will examine the data and instantiate an instance of the appropriate decoder plugin.
    // If insufficient bytes are available to determine the image type, no decoder plugin will be
    // made.
    if (!m_decoder) {
        m_decoder = static_cast<NativeImageSourcePtr>(ImageDecoder::create(*data, m_alphaOption, m_gammaAndColorProfileOption));
#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
        if (m_decoder && s_maxPixelsPerDecodedImage)
            m_decoder->setMaxNumPixels(s_maxPixelsPerDecodedImage);
#endif
    }

    if (m_decoder)
        m_decoder->setData(data, allDataReceived);
}

String ImageSource::filenameExtension() const
{
    return m_decoder ? m_decoder->filenameExtension() : String();
}

bool ImageSource::isSizeAvailable()
{
    return m_decoder && m_decoder->isSizeAvailable();
}

IntSize ImageSource::size() const
{
    return m_decoder ? m_decoder->size() : IntSize();
}

IntSize ImageSource::frameSizeAtIndex(size_t index) const
{
    return m_decoder ? m_decoder->frameSizeAtIndex(index) : IntSize();
}

bool ImageSource::getHotSpot(IntPoint&) const
{
    return false;
}

size_t ImageSource::bytesDecodedToDetermineProperties() const
{
    return 0;
}

int ImageSource::repetitionCount()
{
    return m_decoder ? m_decoder->repetitionCount() : cAnimationNone;
}

size_t ImageSource::frameCount() const
{
    return m_decoder ? m_decoder->frameCount() : 0;
}

NativeImagePtr ImageSource::createFrameAtIndex(size_t index)
{
    if (!m_decoder)
        return 0;

    ImageFrame* buffer = m_decoder->frameBufferAtIndex(index);
    if (!buffer || buffer->status() == ImageFrame::FrameEmpty)
        return 0;

    // Zero-height images can cause problems for some ports.  If we have an
    // empty image dimension, just bail.
    if (size().isEmpty())
        return 0;

    // Return the buffer contents as a native image.  For some ports, the data
    // is already in a native container, and this just increments its refcount.
    return buffer->asNewNativeImage();
}

float ImageSource::frameDurationAtIndex(size_t index)
{
    if (!m_decoder)
        return 0;

    ImageFrame* buffer = m_decoder->frameBufferAtIndex(index);
    if (!buffer || buffer->status() == ImageFrame::FrameEmpty)
        return 0;

    // Many annoying ads specify a 0 duration to make an image flash as quickly as possible.
    // We follow Firefox's behavior and use a duration of 100 ms for any frames that specify
    // a duration of <= 10 ms. See <rdar://problem/7689300> and <http://webkit.org/b/36082>
    // for more information.
    const float duration = buffer->duration() / 1000.0f;
    if (duration < 0.011f)
        return 0.100f;
    return duration;
}

bool ImageSource::frameHasAlphaAtIndex(size_t index)
{
    // When a frame has not finished decoding, always mark it as having alpha.
    // Ports that check the result of this function to determine their
    // compositing op need this in order to not draw the undecoded portion as
    // black.
    // TODO: Perhaps we should ensure that each individual decoder returns true
    // in this case.
    return !frameIsCompleteAtIndex(index)
        || m_decoder->frameBufferAtIndex(index)->hasAlpha();
}

bool ImageSource::frameIsCompleteAtIndex(size_t index)
{
    if (!m_decoder)
        return false;

    ImageFrame* buffer = m_decoder->frameBufferAtIndex(index);
    return buffer && buffer->status() == ImageFrame::FrameComplete;
}

}
