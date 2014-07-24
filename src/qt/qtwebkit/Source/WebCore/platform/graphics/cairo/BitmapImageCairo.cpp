/*
 * Copyright (C) 2004, 2005, 2006 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
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
#include "BitmapImage.h"

#include "ImageObserver.h"
#include "PlatformContextCairo.h"
#include <cairo.h>

namespace WebCore {

BitmapImage::BitmapImage(PassRefPtr<cairo_surface_t> nativeImage, ImageObserver* observer)
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
    int width = cairo_image_surface_get_width(nativeImage.get());
    int height = cairo_image_surface_get_height(nativeImage.get());
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_hasAlpha = cairo_surface_get_content(nativeImage.get()) != CAIRO_CONTENT_COLOR;
    m_frames[0].m_frame = nativeImage;
    m_frames[0].m_haveMetadata = true;
    checkForSolidColor();
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode)
{
    draw(context, dst, src, styleColorSpace, op, blendMode, DoNotRespectImageOrientation);
}

void BitmapImage::draw(GraphicsContext* context, const FloatRect& dst, const FloatRect& src, ColorSpace styleColorSpace, CompositeOperator op,
    BlendMode blendMode, RespectImageOrientationEnum shouldRespectImageOrientation)
{
    if (!dst.width() || !dst.height() || !src.width() || !src.height())
        return;

    startAnimation();

    RefPtr<cairo_surface_t> surface = frameAtIndex(m_currentFrame);
    if (!surface) // If it's too early we won't have an image yet.
        return;

    if (mayFillWithSolidColor()) {
        fillWithSolidColor(context, dst, solidColor(), styleColorSpace, op);
        return;
    }

    context->save();

    // Set the compositing operation.
    if (op == CompositeSourceOver && blendMode == BlendModeNormal && !frameHasAlphaAtIndex(m_currentFrame))
        context->setCompositeOperation(CompositeCopy);
    else
        context->setCompositeOperation(op, blendMode);

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    IntSize scaledSize(cairo_image_surface_get_width(surface.get()), cairo_image_surface_get_height(surface.get()));
    FloatRect adjustedSrcRect = adjustSourceRectForDownSampling(src, scaledSize);
#else
    FloatRect adjustedSrcRect(src);
#endif

    ImageOrientation orientation = DefaultImageOrientation;
    if (shouldRespectImageOrientation == RespectImageOrientation)
        orientation = frameOrientationAtIndex(m_currentFrame);

    FloatRect dstRect = dst;

    if (orientation != DefaultImageOrientation) {
        // ImageOrientation expects the origin to be at (0, 0).
        context->translate(dstRect.x(), dstRect.y());
        dstRect.setLocation(FloatPoint());
        context->concatCTM(orientation.transformFromDefault(dstRect.size()));
        if (orientation.usesWidthAsHeight()) {
            // The destination rectangle will have it's width and height already reversed for the orientation of
            // the image, as it was needed for page layout, so we need to reverse it back here.
            dstRect = FloatRect(dstRect.x(), dstRect.y(), dstRect.height(), dstRect.width());
        }
    }

    context->platformContext()->drawSurfaceToContext(surface.get(), dstRect, adjustedSrcRect, context);

    context->restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

void BitmapImage::checkForSolidColor()
{
    m_isSolidColor = false;
    m_checkedForSolidColor = true;

    if (frameCount() > 1)
        return;

    RefPtr<cairo_surface_t> surface = frameAtIndex(m_currentFrame);
    if (!surface) // If it's too early we won't have an image yet.
        return;

    ASSERT(cairo_surface_get_type(surface.get()) == CAIRO_SURFACE_TYPE_IMAGE);

    int width = cairo_image_surface_get_width(surface.get());
    int height = cairo_image_surface_get_height(surface.get());

    if (width != 1 || height != 1)
        return;

    unsigned* pixelColor = reinterpret_cast_ptr<unsigned*>(cairo_image_surface_get_data(surface.get()));
    m_solidColor = colorFromPremultipliedARGB(*pixelColor);

    m_isSolidColor = true;
}

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    if (m_frame) {
        m_frame.clear();
        return true;
    }
    return false;
}

} // namespace WebCore

