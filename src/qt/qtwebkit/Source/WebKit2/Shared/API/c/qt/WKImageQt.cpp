/*
 * Copyright (C) 2011 University of Szeged. All rights reserved.
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "WKImageQt.h"

#include "ShareableBitmap.h"
#include "WKSharedAPICast.h"
#include "WebImage.h"
#include <QPainter>
#include <WebCore/GraphicsContext.h>
#include <WebCore/IntSize.h>

using namespace WebCore;
using namespace WebKit;

QImage WKImageCreateQImage(WKImageRef imageRef)
{
    return toImpl(imageRef)->bitmap()->createQImage().copy();
}

WKImageRef WKImageCreateFromQImage(const QImage& image)
{
    if (image.isNull())
        return 0;

    ASSERT(image.bytesPerLine() == image.width() * 4);

    RefPtr<WebImage> webImage = WebImage::create(image.size(), static_cast<ImageOptions>(0));
    if (!webImage->bitmap())
        return 0;
    OwnPtr<GraphicsContext> graphicsContext = webImage->bitmap()->createGraphicsContext();
    QPainter* painter = graphicsContext->platformContext();
    painter->drawImage(QPoint(0, 0), image);
    return toAPI(webImage.release().leakRef());
}
