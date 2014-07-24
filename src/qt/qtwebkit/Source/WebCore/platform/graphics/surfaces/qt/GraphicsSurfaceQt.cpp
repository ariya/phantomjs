/*
 Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies)

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
#include "GraphicsSurface.h"

#if USE(GRAPHICS_SURFACE) && PLATFORM(QT)

#include "BitmapImage.h"
#include <QImage>
#include <QPainter>
#include <QPixmap>

namespace WebCore {
void GraphicsSurface::didReleaseImage(void* data)
{
    GraphicsSurface* surface = static_cast<GraphicsSurface*>(data);
    surface->platformUnlock();
}

PassOwnPtr<GraphicsContext> GraphicsSurface::platformBeginPaint(const IntSize& size, char* bits, int stride)
{
    QImage::Format format = (flags() & SupportsAlpha) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;

    // The image and painter will be released when the returned GraphicsContext is released.
    QImage* image = new QImage(reinterpret_cast<uchar*>(bits), size.width(), size.height(), stride, format, didReleaseImage, this);
    QPainter* painter = new QPainter(image);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);
    OwnPtr<GraphicsContext> graphicsContext = adoptPtr(new GraphicsContext(painter));
    graphicsContext->takeOwnershipOfPlatformContext();
    return graphicsContext.release();
}

PassRefPtr<Image> GraphicsSurface::createReadOnlyImage(const IntRect& rect)
{
    int stride;
    QImage::Format format = (flags() & SupportsAlpha) ? QImage::Format_ARGB32_Premultiplied : QImage::Format_RGB32;
    char* data = platformLock(rect, &stride, RetainPixels | ReadOnly);
    QImage image(reinterpret_cast<uchar*>(data), rect.width(), rect.height(), stride, format, didReleaseImage, this);
    return BitmapImage::create(new QPixmap(QPixmap::fromImage(image)));
}

}
#endif
