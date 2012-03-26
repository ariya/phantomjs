/*
 * Copyright (C) Research In Motion Limited 2009-2010. All rights reserved.
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
#include "ImageDecoder.h"

#include "IntRect.h"
#include "IntSize.h"
#include "SurfaceOpenVG.h"
#include "TiledImageOpenVG.h"
#include "VGUtils.h"

#if PLATFORM(EGL)
#include "EGLDisplayOpenVG.h"
#endif

#include <openvg.h>

namespace WebCore {

NativeImagePtr ImageFrame::asNewNativeImage() const
{
    static const VGImageFormat bufferFormat = VG_sARGB_8888_PRE;
    // Save memory by using 16-bit images for fully opaque images.
    const VGImageFormat imageFormat = hasAlpha() ? bufferFormat : VG_sRGB_565;

#if PLATFORM(EGL)
    EGLDisplayOpenVG::current()->sharedPlatformSurface()->makeCurrent();
#endif

    const IntSize vgMaxImageSize(vgGeti(VG_MAX_IMAGE_WIDTH), vgGeti(VG_MAX_IMAGE_HEIGHT));
    ASSERT_VG_NO_ERROR();

    TiledImageOpenVG* tiledImage = new TiledImageOpenVG(IntSize(width(), height()), vgMaxImageSize);

    const int numColumns = tiledImage->numColumns();
    const int numRows = tiledImage->numRows();

    for (int yIndex = 0; yIndex < numRows; ++yIndex) {
        for (int xIndex = 0; xIndex < numColumns; ++xIndex) {
            IntRect tileRect = tiledImage->tileRect(xIndex, yIndex);
            VGImage image = vgCreateImage(imageFormat,
                tileRect.width(), tileRect.height(), VG_IMAGE_QUALITY_FASTER);
            ASSERT_VG_NO_ERROR();

            PixelData* pixelData = const_cast<PixelData*>(m_bytes);
            pixelData += (tileRect.y() * width()) + tileRect.x();

            vgImageSubData(image, reinterpret_cast<unsigned char*>(pixelData),
                width() * sizeof(PixelData), bufferFormat,
                0, 0, tileRect.width(), tileRect.height());
            ASSERT_VG_NO_ERROR();

            tiledImage->setTile(xIndex, yIndex, image);
        }
    }

    return tiledImage;
}

}
