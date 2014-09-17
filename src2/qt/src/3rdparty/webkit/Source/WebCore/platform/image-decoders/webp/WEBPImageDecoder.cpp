/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WEBPImageDecoder.h"

#if USE(WEBP)

#include "webp/decode.h"

namespace WebCore {

WEBPImageDecoder::WEBPImageDecoder(ImageSource::AlphaOption alphaOption,
                                   ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : ImageDecoder(alphaOption, gammaAndColorProfileOption)
    , m_decoder(0)
    , m_lastVisibleRow(0)
{
}

WEBPImageDecoder::~WEBPImageDecoder()
{
}

bool WEBPImageDecoder::isSizeAvailable()
{
    if (!ImageDecoder::isSizeAvailable())
         decode(true);

    return ImageDecoder::isSizeAvailable();
}

ImageFrame* WEBPImageDecoder::frameBufferAtIndex(size_t index)
{
    if (index)
        return 0;

    if (m_frameBufferCache.isEmpty()) {
        m_frameBufferCache.resize(1);
        m_frameBufferCache[0].setPremultiplyAlpha(m_premultiplyAlpha);
    }

    ImageFrame& frame = m_frameBufferCache[0];
    if (frame.status() != ImageFrame::FrameComplete)
        decode(false);
    return &frame;
}


bool WEBPImageDecoder::decode(bool onlySize)
{
    // Minimum number of bytes needed to ensure one can parse size information.
    static const size_t sizeOfHeader = 30;
    // Number of bytes per pixel.
    static const int bytesPerPixel = 3;

    if (failed())
        return false;

    const size_t dataSize = m_data->size();
    if (dataSize < sizeOfHeader)
        return true;

    int width, height;
    const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(m_data->data());
    if (!WebPGetInfo(dataBytes, dataSize, &width, &height))
        return setFailed();
    if (!ImageDecoder::isSizeAvailable() && !setSize(width, height))
        return setFailed();
    if (onlySize)
        return true;

    bool allDataReceived = isAllDataReceived();
    int stride = width * bytesPerPixel;
    ASSERT(!m_frameBufferCache.isEmpty());
    ImageFrame& buffer = m_frameBufferCache[0];
    if (buffer.status() == ImageFrame::FrameEmpty) {
        ASSERT(width == size().width());
        ASSERT(height == size().height());
        if (!buffer.setSize(width, height))
            return setFailed();
        buffer.setStatus(allDataReceived ? ImageFrame::FrameComplete : ImageFrame::FramePartial);
        // FIXME: We currently hard code false below because libwebp doesn't support alpha yet.
        buffer.setHasAlpha(false);
        buffer.setOriginalFrameRect(IntRect(IntPoint(), size()));
        m_rgbOutput.resize(height * stride);
    }
    int newLastVisibleRow = 0; // Last completed row.
    if (allDataReceived) {
        if (!WebPDecodeRGBInto(dataBytes, dataSize, m_rgbOutput.data(), m_rgbOutput.size(), stride))
            return setFailed();
        newLastVisibleRow = height;
    } else {
        if (!m_decoder) {
            m_decoder = WebPINewRGB(MODE_RGB, m_rgbOutput.data(), m_rgbOutput.size(), stride);
            if (!m_decoder)
                return setFailed();
        }
        const VP8StatusCode status = WebPIUpdate(m_decoder, dataBytes, dataSize);
        if (status != VP8_STATUS_OK && status != VP8_STATUS_SUSPENDED)
            return setFailed();
        if (!WebPIDecGetRGB(m_decoder, &newLastVisibleRow, 0, 0, 0))
            return setFailed();
        ASSERT(newLastVisibleRow >= 0);
        ASSERT(newLastVisibleRow <= height);
    }
    // FIXME: remove this data copy.
    for (int y = m_lastVisibleRow; y < newLastVisibleRow; ++y) {
        const uint8_t* const src = &m_rgbOutput[y * stride];
        for (int x = 0; x < width; ++x)
            buffer.setRGBA(x, y, src[bytesPerPixel * x + 0], src[bytesPerPixel * x + 1], src[bytesPerPixel * x + 2], 0xff);
    }
    m_lastVisibleRow = newLastVisibleRow;
    if (m_lastVisibleRow == height)
         buffer.setStatus(ImageFrame::FrameComplete);
    return m_lastVisibleRow == height;
}

}

#endif
