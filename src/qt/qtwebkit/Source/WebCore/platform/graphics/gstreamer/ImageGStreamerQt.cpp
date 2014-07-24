/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "ImageGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)
#include <gst/gst.h>
#include <gst/video/video.h>

#ifdef GST_API_VERSION_1
#include <gst/video/gstvideometa.h>
#endif

#include <wtf/gobject/GOwnPtr.h>

using namespace std;
using namespace WebCore;

ImageGStreamer::ImageGStreamer(GstBuffer* buffer, GstCaps* caps)
    : m_image(0)
{
    GstVideoFormat format;
    IntSize size;
    int pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride;
    getVideoSizeAndFormatFromCaps(caps, size, format, pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride);

#ifdef GST_API_VERSION_1
    GstMapInfo info;
    gst_buffer_map(buffer, &info, GST_MAP_READ);
    uchar* bufferData = reinterpret_cast<uchar*>(info.data);
#else
    uchar* bufferData = reinterpret_cast<uchar*>(GST_BUFFER_DATA(buffer));
#endif
    QImage::Format imageFormat;
    QImage::InvertMode invertMode;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    if (format == GST_VIDEO_FORMAT_BGRA) {
        imageFormat = QImage::Format_ARGB32;
        invertMode = QImage::InvertRgba;
    } else {
        imageFormat = QImage::Format_RGB32;
        invertMode = QImage::InvertRgb;
    }
#else
    imageFormat = (format == GST_VIDEO_FORMAT_ARGB) ? QImage::Format_ARGB32 : QImage::Format_RGB32;
#endif

    QImage image(bufferData, size.width(), size.height(), imageFormat);

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    image.invertPixels(invertMode);
#endif
    QPixmap* surface = new QPixmap;
    surface->convertFromImage(image);
    m_image = BitmapImage::create(surface);

#ifdef GST_API_VERSION_1
    if (GstVideoCropMeta* cropMeta = gst_buffer_get_video_crop_meta(buffer))
        setCropRect(FloatRect(cropMeta->x, cropMeta->y, cropMeta->width, cropMeta->height));

    gst_buffer_unmap(buffer, &info);
#endif
}

ImageGStreamer::~ImageGStreamer()
{
    if (m_image)
        m_image.clear();

    m_image = 0;
}
#endif // USE(GSTREAMER)

