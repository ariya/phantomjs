/*
 * Copyright (C) 2010 Igalia S.L
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
#include "ImageGStreamer.h"

#if ENABLE(VIDEO) && USE(GSTREAMER)

#include <cairo.h>
#include <gst/gst.h>
#include <gst/video/video.h>
#include <wtf/gobject/GOwnPtr.h>

#ifdef GST_API_VERSION_1
#include <gst/video/gstvideometa.h>
#endif


using namespace std;
using namespace WebCore;

ImageGStreamer::ImageGStreamer(GstBuffer* buffer, GstCaps* caps)
#ifdef GST_API_VERSION_1
    : m_buffer(buffer)
#endif
{
    GstVideoFormat format;
    IntSize size;
    int pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride;
    getVideoSizeAndFormatFromCaps(caps, size, format, pixelAspectRatioNumerator, pixelAspectRatioDenominator, stride);

#ifdef GST_API_VERSION_1
    gst_buffer_map(buffer, &m_mapInfo, GST_MAP_READ);
    unsigned char* bufferData = reinterpret_cast<unsigned char*>(m_mapInfo.data);
#else
    unsigned char* bufferData = reinterpret_cast<unsigned char*>(GST_BUFFER_DATA(buffer));
#endif

    cairo_format_t cairoFormat;
#if G_BYTE_ORDER == G_LITTLE_ENDIAN
    cairoFormat = (format == GST_VIDEO_FORMAT_BGRA) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24;
#else
    cairoFormat = (format == GST_VIDEO_FORMAT_ARGB) ? CAIRO_FORMAT_ARGB32 : CAIRO_FORMAT_RGB24;
#endif

    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(bufferData, cairoFormat, size.width(), size.height(), stride));
    ASSERT(cairo_surface_status(surface.get()) == CAIRO_STATUS_SUCCESS);
    m_image = BitmapImage::create(surface.release());

#ifdef GST_API_VERSION_1
    if (GstVideoCropMeta* cropMeta = gst_buffer_get_video_crop_meta(buffer))
        setCropRect(FloatRect(cropMeta->x, cropMeta->y, cropMeta->width, cropMeta->height));
#endif
}

ImageGStreamer::~ImageGStreamer()
{
    if (m_image)
        m_image.clear();

    m_image = 0;

#ifdef GST_API_VERSION_1
    // We keep the buffer memory mapped until the image is destroyed because the internal
    // cairo_surface_t was created using cairo_image_surface_create_for_data().
    gst_buffer_unmap(m_buffer.get(), &m_mapInfo);
#endif
}
#endif // USE(GSTREAMER)
