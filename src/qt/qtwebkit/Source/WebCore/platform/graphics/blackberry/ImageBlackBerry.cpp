/*
 * Copyright (C) 2010, 2011, 2012, 2013 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "Image.h"

#include "AffineTransform.h"
#include "BitmapImage.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageBuffer.h"
#include "ImageDecoder.h"
#include "ImageObserver.h"
#include "IntSize.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"

#include <BlackBerryPlatformGraphicsContext.h>
#include <BlackBerryPlatformResourceStore.h>
#include <wtf/MathExtras.h>

using BlackBerry::Platform::ResourceData;
using BlackBerry::Platform::ResourceStore;

namespace WebCore {

PassRefPtr<Image> Image::loadPlatformResource(const char *name)
{
    ResourceData data = ResourceStore::instance()->requestResource(BlackBerry::Platform::String::fromUtf8(name));
    if (!data.data())
        return BitmapImage::nullImage();

    RefPtr<SharedBuffer> buffer = SharedBuffer::create(data.data(), data.len());
    if (!buffer)
        return BitmapImage::nullImage();

    RefPtr<BitmapImage> img = BitmapImage::create();
    img->setData(buffer.release(), true /* allDataReceived */);
    return img.release();
}

PassNativeImagePtr ImageFrame::asNewNativeImage() const
{
    return new BlackBerry::Platform::Graphics::TiledImage(m_size, m_bytes);
}

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

BitmapImage::BitmapImage(PassNativeImagePtr nativeImage, ImageObserver* observer)
    : Image(observer)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_decodedSize(0)
    , m_frameCount(1)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_haveFrameCount(true)
{
    int width = nativeImage->size().width();
    int height = nativeImage->size().height();
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_frame = nativeImage;
    m_frames[0].m_hasAlpha = nativeImage->hasAlpha();
    m_frames[0].m_haveMetadata = true;
    checkForSolidColor();
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = size().width() == 1 && size().height() == 1 && frameCount() == 1;
    m_checkedForSolidColor = true;
    if (m_isSolidColor) {
        unsigned buffer;
        NativeImagePtr image = nativeImageForCurrentFrame();
        image->readPixels(&buffer, 1);
        m_solidColor = Color(buffer);
    }
}

void BitmapImage::invalidatePlatformData()
{
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode)
{
    draw(context, dstRect, srcRect, styleColorSpace, op, blendMode, DoNotRespectImageOrientation);
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace, CompositeOperator op, BlendMode, RespectImageOrientationEnum shouldRespectImageOrientation)
{
    startAnimation();

    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    FloatRect normDstRect = dstRect.normalized();
    FloatRect normSrcRect = srcRect.normalized();

    if (normSrcRect.isEmpty() || normDstRect.isEmpty())
        return; // Nothing to draw.

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    normSrcRect = adjustSourceRectForDownSampling(normSrcRect, image->size());
#endif

    // use similar orientation code as ImageSkia
    ImageOrientation orientation = DefaultImageOrientation;
    if (shouldRespectImageOrientation == RespectImageOrientation)
        orientation = frameOrientationAtIndex(m_currentFrame);

    GraphicsContextStateSaver saveContext(*context, false);
    if (orientation != DefaultImageOrientation) {
        saveContext.save();

        // ImageOrientation expects the origin to be at (0, 0)
        context->translate(normDstRect.x(), normDstRect.y());
        normDstRect.setLocation(FloatPoint());

        context->concatCTM(orientation.transformFromDefault(normDstRect.size()));
        if (orientation.usesWidthAsHeight()) {
            // The destination rect will have its width and height already reversed for the orientation of
            // the image, as it was needed for page layout, so we need to reverse it back here.
            normDstRect = FloatRect(normDstRect.x(), normDstRect.y(), normDstRect.height(), normDstRect.width());
        }
    }

    CompositeOperator oldOperator = context->compositeOperation();
    context->setCompositeOperation(op);
    context->platformContext()->addImage(normDstRect, normSrcRect, image);
    context->setCompositeOperation(oldOperator);

    if (ImageObserver* observer = imageObserver())
        observer->didDraw(this);
}

void Image::drawPattern(GraphicsContext* context, const FloatRect& srcRect, const AffineTransform& patternTransformation, const FloatPoint& phase, ColorSpace, CompositeOperator op, const FloatRect& dstRect, BlendMode)
{
    NativeImagePtr image = nativeImageForCurrentFrame();
    if (!image)
        return;

    // patternTransformation contains the tile scale factor relative to
    // document coordinates. phase is given in document coordinates and
    // corresponds to a texture drawn with src starting at (0, 0).
    // Thus, dstRect starts at phase + patternTransformation.mapPoint(srcRect.location()).
    AffineTransform srcTransform;
    srcTransform.translate(phase.x(), phase.y());
    srcTransform.multiply(patternTransformation);
#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    FloatRect srcRectLocal(srcRect);
    const IntSize unscaledSize = size();
    const IntSize scaledSize = image->size();
    if (unscaledSize != scaledSize) {
        srcRectLocal = adjustSourceRectForDownSampling(srcRect, scaledSize);
        // If the image has been down sampled, the transformation should take the scale into account.
        float xscale = static_cast<float>(unscaledSize.width()) / scaledSize.width();
        float yscale = static_cast<float>(unscaledSize.height()) / scaledSize.height();
        srcTransform.scaleNonUniform(xscale, yscale);
    }
#else
    FloatRect srcRectLocal(srcRect);
#endif
    srcTransform.translate(srcRectLocal.x(), srcRectLocal.y());
    double srcTransformArray[6] = {
        srcTransform.a(), srcTransform.b(),
        srcTransform.c(), srcTransform.d(),
        srcTransform.e(), srcTransform.f()
    };

    CompositeOperator oldOperator = context->compositeOperation();
    context->setCompositeOperation(op);
    context->platformContext()->addPattern(dstRect, srcRectLocal, srcTransformArray, image);
    context->setCompositeOperation(oldOperator);
}

} // namespace WebCore
