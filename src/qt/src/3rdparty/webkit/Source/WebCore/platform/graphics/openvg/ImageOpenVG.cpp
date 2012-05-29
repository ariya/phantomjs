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
#include "Image.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageDecoder.h"
#include "ImageObserver.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include "PainterOpenVG.h"
#include "SurfaceOpenVG.h"
#include "TiledImageOpenVG.h"
#include "VGUtils.h"

#include <wtf/MathExtras.h>

namespace WebCore {

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        delete m_frame;
        m_frame = 0;
        return true;
    }
    return false;
}

BitmapImage::BitmapImage(TiledImageOpenVG* tiledImage, ImageObserver* observer)
    : Image(observer)
    , m_size(tiledImage->size())
    , m_currentFrame(0)
    , m_frames(1)
    , m_frameTimer(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_desiredFrameStartTime(0)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(false)
    , m_allDataReceived(false)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_hasUniformFrameSize(true)
    , m_haveFrameCount(true)
    , m_frameCount(1)
{
    initPlatformData();

    ASSERT(m_size.width() > 0);
    ASSERT(m_size.height() > 0);

    m_decodedSize = m_size.width() * m_size.height() * 4;

    m_frames[0].m_frame = tiledImage;
    m_frames[0].m_hasAlpha = true;
    m_frames[0].m_isComplete = true;
    m_frames[0].m_haveMetadata = true;
    checkForSolidColor();
}

void BitmapImage::checkForSolidColor()
{
    TiledImageOpenVG* tiledImage = 0;

    if (m_frameCount == 1 && m_size.width() == 1 && m_size.height() == 1)
        tiledImage = nativeImageForCurrentFrame();

    if (tiledImage) {
        m_isSolidColor = true;
        RGBA32 pixel;
        vgGetImageSubData(tiledImage->tile(0, 0), &pixel, 0, VG_sARGB_8888, 0, 0, 1, 1);
        ASSERT_VG_NO_ERROR();
        m_solidColor.setRGB(pixel);
    } else
        m_isSolidColor = false;

    m_checkedForSolidColor = true;
}

void BitmapImage::initPlatformData()
{
}

void BitmapImage::invalidatePlatformData()
{
}

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
static void adjustSourceRectForDownSampling(FloatRect& srcRect, const IntSize& origSize, const IntSize& scaledSize)
{
    // We assume down-sampling zoom rates in X direction and in Y direction are same.
    if (origSize.width() == scaledSize.width())
        return;

    // Image has been down sampled.
    double rate = static_cast<double>(scaledSize.width()) / origSize.width();
    double temp = srcRect.right() * rate;
    srcRect.setX(srcRect.x() * rate);
    srcRect.setWidth(temp - srcRect.x());
    temp = srcRect.bottom() * rate;
    srcRect.setY(srcRect.y() * rate);
    srcRect.setHeight(temp - srcRect.y());
}
#endif

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op)
{
    if (dst.isEmpty() || src.isEmpty())
        return;

    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    startAnimation();

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(context, dst, solidColor(), styleColorSpace, op);
        return;
    }

    context->save();

    // Set the compositing operation.
    if (op == CompositeSourceOver && !frameHasAlphaAtIndex(m_currentFrame))
        context->setCompositeOperation(CompositeCopy);
    else
        context->setCompositeOperation(op);

    FloatRect srcRectLocal(src);
#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    adjustSourceRectForDownSampling(srcRectLocal, size(), image->size());
#endif

    context->platformContext()->activePainter()->drawImage(image, dst, srcRectLocal);
    context->restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

void Image::drawPattern(GraphicsContext* context, const FloatRect& src,
                        const AffineTransform& patternTransformation,
                        const FloatPoint& phase, ColorSpace styleColorSpace,
                        CompositeOperator op, const FloatRect& dst)
{
    if (dst.isEmpty() || src.isEmpty())
        return;

    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    startAnimation();

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(context, dst, solidColor(), styleColorSpace, op);
        return;
    }

    notImplemented();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

PassRefPtr<Image> Image::loadPlatformResource(char const* name)
{
    notImplemented();
    return 0;
}

}
