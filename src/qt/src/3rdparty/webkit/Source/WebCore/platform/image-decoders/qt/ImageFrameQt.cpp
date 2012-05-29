/*
 * Copyright (C) 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2008, 2009 Google, Inc.
 * Copyright (C) 2009 Holger Hans Peter Freyther
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
#include "ImageDecoder.h"

#include "NotImplemented.h"

#include <QPixmap>
#include <stdio.h>

namespace WebCore {

ImageFrame::ImageFrame()
    : m_hasAlpha(false) 
    , m_size()
    , m_status(FrameEmpty)
    , m_duration(0)
    , m_disposalMethod(DisposeNotSpecified)
{
}

ImageFrame& ImageFrame::operator=(const ImageFrame& other)
{
    if (this == &other)
        return *this;

    copyBitmapData(other);
    setOriginalFrameRect(other.originalFrameRect());
    setStatus(other.status());
    setDuration(other.duration());
    setDisposalMethod(other.disposalMethod());
    return *this;
}

void ImageFrame::clearPixelData()
{
    m_pixmap = QPixmap();
    m_image = QImage();
    m_status = FrameEmpty;
    // NOTE: Do not reset other members here; clearFrameBufferCache()
    // calls this to free the bitmap data, but other functions like
    // initFrameBuffer() and frameComplete() may still need to read
    // other metadata out of this frame later.
}

void ImageFrame::zeroFillPixelData()
{
    if (m_pixmap.isNull() && !m_image.isNull()) {
        m_pixmap = QPixmap(m_image.width(), m_image.height());
        m_image = QImage();
    }
    m_pixmap.fill(QColor(0, 0, 0, 0));
}

bool ImageFrame::copyBitmapData(const ImageFrame& other)
{
    if (this == &other)
        return true;

    m_image = other.m_image;
    m_pixmap = other.m_pixmap;
    m_size = other.m_size;
    m_hasAlpha = other.m_hasAlpha;
    return true;
}

bool ImageFrame::setSize(int newWidth, int newHeight)
{
    // This function should only be called once, it will leak memory
    // otherwise.
    ASSERT(width() == 0 && height() == 0);

    m_size = IntSize(newWidth, newHeight);
    m_image = QImage();
    m_pixmap = QPixmap(newWidth, newHeight);
    if (m_pixmap.isNull())
        return false;

    zeroFillPixelData();

    return true;
}

QPixmap* ImageFrame::asNewNativeImage() const
{
    if (m_pixmap.isNull() && !m_image.isNull()) {
        m_pixmap = QPixmap::fromImage(m_image);
        m_image = QImage();
    }
    return new QPixmap(m_pixmap);
}

bool ImageFrame::hasAlpha() const
{
    return m_hasAlpha;
}

void ImageFrame::setHasAlpha(bool alpha)
{
    m_hasAlpha = alpha;
}

void ImageFrame::setColorProfile(const ColorProfile& colorProfile)
{
    notImplemented();
}

void ImageFrame::setStatus(FrameStatus status)
{
    m_status = status;
}

// The image must not have format 8888 pre multiplied...
void ImageFrame::setPixmap(const QPixmap& pixmap)
{
    m_pixmap = pixmap;
    m_image = QImage();
    m_size = pixmap.size();
    m_hasAlpha = pixmap.hasAlphaChannel();
}

int ImageFrame::width() const
{
    return m_size.width();
}

int ImageFrame::height() const
{
    return m_size.height();
}

}
