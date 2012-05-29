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

    QImage::Format imageFormat;
    if (format == GST_VIDEO_FORMAT_RGB)
        imageFormat = QImage::Format_RGB888;
    else
        imageFormat = QImage::Format_RGB32;

    return adoptRef(new ImageGStreamer(buffer, IntSize(width, height), imageFormat));
}

ImageGStreamer::ImageGStreamer(GstBuffer*& buffer, IntSize size, QImage::Format imageFormat)
    : m_image(0)
{
    QPixmap* surface = new QPixmap;
    QImage image(GST_BUFFER_DATA(buffer), size.width(), size.height(), imageFormat);
    surface->convertFromImage(image);
    m_image = BitmapImage::create(surface);
}

ImageGStreamer::~ImageGStreamer()
{
    if (m_image)
        m_image.clear();

    m_image = 0;
}
#endif // USE(GSTREAMER)

