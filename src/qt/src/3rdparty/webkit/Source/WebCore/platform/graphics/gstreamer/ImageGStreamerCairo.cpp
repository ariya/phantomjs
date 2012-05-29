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

#if USE(GSTREAMER)

#include "GOwnPtr.h"

using namespace std;
using namespace WebCore;

PassRefPtr<ImageGStreamer> ImageGStreamer::createImage(GstBuffer* buffer)
{
    int width = 0, height = 0;
    GstCaps* caps = gst_buffer_get_caps(buffer);
    GstVideoFormat format;
    if (!gst_video_format_parse_caps(caps, &format, &width, &height)) {
        gst_caps_unref(caps);
        return 0;
    }

    gst_caps_unref(caps);

    cairo_format_t cairoFormat;
    if (format == GST_VIDEO_FORMAT_ARGB || format == GST_VIDEO_FORMAT_BGRA)
        cairoFormat = CAIRO_FORMAT_ARGB32;
    else
        cairoFormat = CAIRO_FORMAT_RGB24;

    return adoptRef(new ImageGStreamer(buffer, IntSize(width, height), cairoFormat));
}

ImageGStreamer::ImageGStreamer(GstBuffer*& buffer, IntSize size, cairo_format_t& cairoFormat)
    : m_image(0)
{
    cairo_surface_t* surface = cairo_image_surface_create_for_data(GST_BUFFER_DATA(buffer), cairoFormat,
                                                    size.width(), size.height(),
                                                    cairo_format_stride_for_width(cairoFormat, size.width()));
    ASSERT(cairo_surface_status(surface) == CAIRO_STATUS_SUCCESS);
    m_image = BitmapImage::create(surface);
}

ImageGStreamer::~ImageGStreamer()
{
    if (m_image)
        m_image.clear();

    m_image = 0;
}
#endif // USE(GSTREAMER)
