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

#ifndef ImageGStreamer_h
#define ImageGStreamer_h

#if USE(GSTREAMER)

#include "BitmapImage.h"
#include <gst/gst.h>
#include <gst/video/video.h>
#include <wtf/PassRefPtr.h>

#if USE(CAIRO)
#include <cairo.h>
#endif

namespace WebCore {
class IntSize;

class ImageGStreamer : public RefCounted<ImageGStreamer> {
    public:
        static PassRefPtr<ImageGStreamer> createImage(GstBuffer*);
        ~ImageGStreamer();

        PassRefPtr<BitmapImage> image()
        {
            ASSERT(m_image);
            return m_image.get();
        }

    private:
        RefPtr<BitmapImage> m_image;

#if USE(CAIRO)
        ImageGStreamer(GstBuffer*&, IntSize, cairo_format_t&);
#endif

#if PLATFORM(QT)
        ImageGStreamer(GstBuffer*&, IntSize, QImage::Format);
#endif

#if PLATFORM(MAC)
        ImageGStreamer(GstBuffer*&, IntSize);
#endif

    };
}

#endif // USE(GSTREAMER)
#endif
