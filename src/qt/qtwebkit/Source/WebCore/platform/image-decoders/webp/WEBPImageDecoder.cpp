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

#include "PlatformInstrumentation.h"

#ifdef QCMS_WEBP_COLOR_CORRECTION
#include "qcms.h"
#include "webp/demux.h"
#else
#undef ICCP_FLAG
#define ICCP_FLAG 0
#endif

// Backward emulation for earlier versions than 0.1.99.
#if (WEBP_DECODER_ABI_VERSION < 0x0163)
#define MODE_rgbA MODE_RGBA
#define MODE_bgrA MODE_BGRA
#endif

#if CPU(BIG_ENDIAN) || CPU(MIDDLE_ENDIAN)
inline WEBP_CSP_MODE outputMode(bool hasAlpha) { return hasAlpha ? MODE_rgbA : MODE_RGBA; }
#else // LITTLE_ENDIAN, output BGRA pixels.
inline WEBP_CSP_MODE outputMode(bool hasAlpha) { return hasAlpha ? MODE_bgrA : MODE_BGRA; }
#endif

namespace WebCore {

WEBPImageDecoder::WEBPImageDecoder(ImageSource::AlphaOption alphaOption,
                                   ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : ImageDecoder(alphaOption, gammaAndColorProfileOption)
    , m_decoder(0)
    , m_hasAlpha(false)
    , m_formatFlags(0)
#ifdef QCMS_WEBP_COLOR_CORRECTION
    , m_haveReadProfile(false)
    , m_transform(0)
    , m_decodedHeight(0)
#endif
{
}

WEBPImageDecoder::~WEBPImageDecoder()
{
    clear();
}

void WEBPImageDecoder::clear()
{
#ifdef QCMS_WEBP_COLOR_CORRECTION
    if (m_transform)
        qcms_transform_release(m_transform);
    m_transform = 0;
#endif
    if (m_decoder)
        WebPIDelete(m_decoder);
    m_decoder = 0;
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
    if (frame.status() != ImageFrame::FrameComplete) {
        PlatformInstrumentation::willDecodeImage("WEBP");
        decode(false);
        PlatformInstrumentation::didDecodeImage();
    }
    return &frame;
}

#ifdef QCMS_WEBP_COLOR_CORRECTION

void WEBPImageDecoder::createColorTransform(const char* data, size_t size)
{
    if (m_transform)
        qcms_transform_release(m_transform);
    m_transform = 0;

    qcms_profile* deviceProfile = ImageDecoder::qcmsOutputDeviceProfile();
    if (!deviceProfile)
        return;
    qcms_profile* inputProfile = qcms_profile_from_memory(data, size);
    if (!inputProfile)
        return;

    // We currently only support color profiles for RGB profiled images.
    ASSERT(icSigRgbData == qcms_profile_get_color_space(inputProfile));
    // The input image pixels are RGBA format.
    qcms_data_type format = QCMS_DATA_RGBA_8;
    // FIXME: Don't force perceptual intent if the image profile contains an intent.
    m_transform = qcms_transform_create(inputProfile, format, deviceProfile, QCMS_DATA_RGBA_8, QCMS_INTENT_PERCEPTUAL);

    qcms_profile_release(inputProfile);
}

void WEBPImageDecoder::readColorProfile(const uint8_t* data, size_t size)
{
    WebPChunkIterator chunkIterator;
    WebPData inputData = { data, size };
    WebPDemuxState state;

    WebPDemuxer* demuxer = WebPDemuxPartial(&inputData, &state);
    if (!WebPDemuxGetChunk(demuxer, "ICCP", 1, &chunkIterator)) {
        WebPDemuxReleaseChunkIterator(&chunkIterator);
        WebPDemuxDelete(demuxer);
        return;
    }

    const char* profileData = reinterpret_cast<const char*>(chunkIterator.chunk.bytes);
    size_t profileSize = chunkIterator.chunk.size;

    // Only accept RGB color profiles from input class devices.
    bool ignoreProfile = false;
    if (profileSize < ImageDecoder::iccColorProfileHeaderLength)
        ignoreProfile = true;
    else if (!ImageDecoder::rgbColorProfile(profileData, profileSize))
        ignoreProfile = true;
    else if (!ImageDecoder::inputDeviceColorProfile(profileData, profileSize))
        ignoreProfile = true;

    if (!ignoreProfile)
        createColorTransform(profileData, profileSize);

    WebPDemuxReleaseChunkIterator(&chunkIterator);
    WebPDemuxDelete(demuxer);
}

void WEBPImageDecoder::applyColorProfile(const uint8_t* data, size_t size, ImageFrame& buffer)
{
    int width;
    int decodedHeight;
    if (!WebPIDecGetRGB(m_decoder, &decodedHeight, &width, 0, 0))
        return; // See also https://bugs.webkit.org/show_bug.cgi?id=74062
    if (decodedHeight <= 0)
        return;

    if (!m_haveReadProfile) {
        readColorProfile(data, size);
        m_haveReadProfile = true;
    }

    ASSERT(width == scaledSize().width());
    ASSERT(decodedHeight <= scaledSize().height());

    for (int y = m_decodedHeight; y < decodedHeight; ++y) {
        uint8_t* row = reinterpret_cast<uint8_t*>(buffer.getAddr(0, y));
        if (qcms_transform* transform = colorTransform())
            qcms_transform_data_type(transform, row, row, width, QCMS_OUTPUT_RGBX);
        uint8_t* pixel = row;
        for (int x = 0; x < width; ++x, pixel += 4)
            buffer.setRGBA(x, y, pixel[0], pixel[1], pixel[2], pixel[3]);
    }

    m_decodedHeight = decodedHeight;
}

#endif // QCMS_WEBP_COLOR_CORRECTION

bool WEBPImageDecoder::decode(bool onlySize)
{
    if (failed())
        return false;

    const uint8_t* dataBytes = reinterpret_cast<const uint8_t*>(m_data->data());
    const size_t dataSize = m_data->size();

    if (!ImageDecoder::isSizeAvailable()) {
        static const size_t imageHeaderSize = 30;
        if (dataSize < imageHeaderSize)
            return false;
        int width, height;
#ifdef QCMS_WEBP_COLOR_CORRECTION
        WebPData inputData = { dataBytes, dataSize };
        WebPDemuxState state;
        WebPDemuxer* demuxer = WebPDemuxPartial(&inputData, &state);
        if (!demuxer)
            return setFailed();

        width = WebPDemuxGetI(demuxer, WEBP_FF_CANVAS_WIDTH);
        height = WebPDemuxGetI(demuxer, WEBP_FF_CANVAS_HEIGHT);
        m_formatFlags = WebPDemuxGetI(demuxer, WEBP_FF_FORMAT_FLAGS);
        m_hasAlpha = !!(m_formatFlags & ALPHA_FLAG);

        WebPDemuxDelete(demuxer);
        if (state <= WEBP_DEMUX_PARSING_HEADER)
            return false;
#elif (WEBP_DECODER_ABI_VERSION >= 0x0163)
        WebPBitstreamFeatures features;
        if (WebPGetFeatures(dataBytes, dataSize, &features) != VP8_STATUS_OK)
            return setFailed();
        width = features.width;
        height = features.height;
        m_hasAlpha = features.has_alpha;
#else
        // Earlier version won't be able to display WebP files with alpha.
        if (!WebPGetInfo(dataBytes, dataSize, &width, &height))
            return setFailed();
        m_hasAlpha = false;
#endif
        if (!setSize(width, height))
            return setFailed();
    }

    ASSERT(ImageDecoder::isSizeAvailable());
    if (onlySize)
        return true;

    ASSERT(!m_frameBufferCache.isEmpty());
    ImageFrame& buffer = m_frameBufferCache[0];
    ASSERT(buffer.status() != ImageFrame::FrameComplete);

    if (buffer.status() == ImageFrame::FrameEmpty) {
        if (!buffer.setSize(size().width(), size().height()))
            return setFailed();
        buffer.setStatus(ImageFrame::FramePartial);
        buffer.setHasAlpha(m_hasAlpha);
        buffer.setOriginalFrameRect(IntRect(IntPoint(), size()));
    }

    if (!m_decoder) {
        WEBP_CSP_MODE mode = outputMode(m_hasAlpha);
        if (!m_premultiplyAlpha)
            mode = outputMode(false);
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile())
            mode = MODE_RGBA; // Decode to RGBA for input to libqcms.
        int rowStride = size().width() * sizeof(ImageFrame::PixelData);
        uint8_t* output = reinterpret_cast<uint8_t*>(buffer.getAddr(0, 0));
        int outputSize = size().height() * rowStride;
        m_decoder = WebPINewRGB(mode, output, outputSize, rowStride);
        if (!m_decoder)
            return setFailed();
    }

    switch (WebPIUpdate(m_decoder, dataBytes, dataSize)) {
    case VP8_STATUS_OK:
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile()) 
            applyColorProfile(dataBytes, dataSize, buffer);
        buffer.setStatus(ImageFrame::FrameComplete);
        clear();
        return true;
    case VP8_STATUS_SUSPENDED:
        if ((m_formatFlags & ICCP_FLAG) && !ignoresGammaAndColorProfile()) 
            applyColorProfile(dataBytes, dataSize, buffer);
        return false;
    default:
        clear();                         
        return setFailed();
    }
}

} // namespace WebCore

#endif
