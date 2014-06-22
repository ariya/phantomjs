/*
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

// Implementation notes:
// Current implementation provides the source image size without doing a full
// decode. It seems that libimg does not support partial decoding.

#include "config.h"
#include "JPEGImageDecoder.h"

#include <errno.h>
#include <img/img.h>
#include <string.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

static img_lib_t s_ilib;

static inline int libInit()
{
    static bool s_initialized;
    if (s_initialized)
        return 0;

    if (img_lib_attach(&s_ilib) != IMG_ERR_OK) {
        LOG_ERROR("Unable to attach to libimg.");
        return -1;
    }
    s_initialized = true;
    return 0;
}

class ImageReader {
public:
    ImageReader(const char* data, size_t dataSize)
        : m_data(data)
        , m_size(dataSize)
        , m_width(0)
        , m_height(0)
    {
        libInit();
    }

    void updateData(const char* data, size_t dataSize);
    int setSize(size_t width, size_t height);
    int sizeExtract(size_t& width, size_t& height);
    int decode(size_t width, size_t height, ImageFrame*);

private:
    const char* m_data;
    size_t m_size;
    size_t m_width;
    size_t m_height;
};

// Function to get the original size of an image
static int imgDecodeSetup(_Uintptrt data, img_t* img, unsigned flags)
{
    ImageReader* reader = reinterpret_cast<ImageReader*>(data);

    if ((img->flags & (IMG_W | IMG_H)) == (IMG_W | IMG_H))
        reader->setSize(img->w, img->h);

    // Always: want to stop processing whether get a size or not.
    return IMG_ERR_INTR;
}

void ImageReader::updateData(const char* data, size_t dataSize)
{
    m_data = data;
    m_size = dataSize;
}

int ImageReader::setSize(size_t width, size_t height)
{
    m_width = width;
    m_height = height;
    return 0;
}

int ImageReader::sizeExtract(size_t& width, size_t& height)
{
    img_decode_callouts_t callouts;
    img_t img;
    io_stream_t* iostream = io_open(IO_MEM, IO_READ, m_size, m_data);
    if (!iostream)
        return -1;

    memset(&img, 0, sizeof(img));
    memset(&callouts, 0, sizeof(callouts));
    callouts.setup_f = imgDecodeSetup;
    callouts.data = reinterpret_cast<_Uintptrt>(this);

    int rc = img_load(s_ilib, iostream, &callouts, &img);
    io_close(iostream);
    if (rc != IMG_ERR_INTR)
        return -1;
    if (!m_width || !m_height)
        return -1;

    width = m_width;
    height = m_height;

    return 0;
}

int ImageReader::decode(size_t width, size_t height, ImageFrame* aFrame)
{
    if (libInit() == -1)
        return -1;

    img_t img;
    memset(&img, 0, sizeof(img));
    img.format = IMG_FMT_RGB888;
    img.w = width;
    img.h = height;

    const int ColorComponents = 3;
    // Use a multiple of 2 bytes to improve performance
    int stride = (ColorComponents * width + 3) & ~3;
    OwnArrayPtr<_uint8> buffer = adoptArrayPtr(new _uint8[stride * height]);
    if (!buffer)
        return -1;
    img.access.direct.data = buffer.get();
    img.access.direct.stride = stride;
    img.flags = IMG_W | IMG_H | IMG_DIRECT | IMG_FORMAT;

    io_stream_t* iostream = io_open(IO_MEM, IO_READ, m_size, m_data);
    if (!iostream)
        return -1;

    int rc = img_load_resize(s_ilib, iostream, 0, &img);
    io_close(iostream);
    if (rc != IMG_ERR_OK)
        return -1;

    for (unsigned j = 0; j < height; j++) {
        _uint8* curPtr = buffer.get() + j * stride;
        for (unsigned i = 0; i < width; i++) {
            aFrame->setRGBA(i, j, curPtr[0], curPtr[1], curPtr[2], 255);
            curPtr += 3;
        }
    }
    return 0;
}

JPEGImageDecoder::JPEGImageDecoder(ImageSource::AlphaOption alphaOption,
    ImageSource::GammaAndColorProfileOption gammaAndColorProfileOption)
    : ImageDecoder(alphaOption, gammaAndColorProfileOption)
{
}

void JPEGImageDecoder::setData(SharedBuffer* data, bool allDataReceived)
{
    ImageDecoder::setData(data, allDataReceived);

    if (m_reader)
        m_reader->updateData(m_data->data(), m_data->size());
}

bool JPEGImageDecoder::isSizeAvailable()
{
    if (!ImageDecoder::isSizeAvailable()) {
        if (!m_reader) {
            if (m_data)
                m_reader = adoptPtr(new ImageReader(m_data->data(), m_data->size()));
            if (!m_reader)
                return false;
        }
        size_t width, height;

        if (m_reader->sizeExtract(width, height) == -1)
            return false;
        if (!setSize(width, height))
            return false;
    }

    return ImageDecoder::isSizeAvailable();
}

ImageFrame* JPEGImageDecoder::frameBufferAtIndex(size_t index)
{
    if (!isAllDataReceived())
        return 0;

    if (index)
        return 0;

    if (m_frameBufferCache.isEmpty())
        m_frameBufferCache.resize(1);

    ImageFrame& frame = m_frameBufferCache[0];

    // Check to see if it's already decoded.
    // FIXME: Could size change between calls to this method?
    if (frame.status() == ImageFrame::FrameComplete)
        return &frame;

    if (frame.status() == ImageFrame::FrameEmpty) {
        if (!size().width() || !size().height()) {
            if (!isSizeAvailable())
                return 0;
        }
        if (!frame.setSize(size().width(), size().height())) {
            setFailed();
            return 0;
        }
        frame.setStatus(ImageFrame::FramePartial);
        // For JPEGs, the frame always fills the entire image.
        frame.setOriginalFrameRect(IntRect(IntPoint(), size()));
    }

    if (!m_reader || m_reader->decode(size().width(), size().height(), &frame) == -1) {
        setFailed();
        return 0;
    }

    frame.setStatus(ImageFrame::FrameComplete);
    return &frame;
}

} // namespace WebCore

