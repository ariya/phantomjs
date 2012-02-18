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

#include "GraphicsContextCG.h"
#include "ImageGStreamer.h"
#if USE(GSTREAMER)

using namespace WebCore;

PassRefPtr<ImageGStreamer> ImageGStreamer::createImage(GstBuffer* buffer)
{
    int width = 0, height = 0;
    GstCaps* caps = gst_buffer_get_caps(buffer);
    GstVideoFormat format;
    if (!gst_video_format_parse_caps(caps, &format, &width, &height)) {
        gst_caps_unref(caps);
        return NULL;
    }

    return adoptRef(new ImageGStreamer(buffer, IntSize(width, height)));
}

ImageGStreamer::ImageGStreamer(GstBuffer*& buffer, IntSize size)
    : m_image(0)
{
    ASSERT(GST_BUFFER_SIZE(buffer));

    RetainPtr<CFDataRef> data(AdoptCF, CFDataCreateWithBytesNoCopy(0, static_cast<UInt8*>(GST_BUFFER_DATA(buffer)), GST_BUFFER_SIZE(buffer), kCFAllocatorNull));
    RetainPtr<CGDataProviderRef> provider(AdoptCF, CGDataProviderCreateWithCFData(data.get()));
    CGImageRef frameImage = CGImageCreate(size.width(), size.height(), 8, 32, size.width()*4, deviceRGBColorSpaceRef(),
        kCGBitmapByteOrder32Little | kCGImageAlphaFirst, provider.get(), 0, false, kCGRenderingIntentDefault);
    m_image = BitmapImage::create(frameImage);
}

ImageGStreamer::~ImageGStreamer()
{
    if (m_image)
        m_image.clear();

    m_image = 0;
}

#endif // USE(GSTREAMER)
