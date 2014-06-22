/*
 * Copyright (C) 2004, 2005, 2006 Apple Inc. All rights reserved.
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

#if USE(CG)

#include "FloatConversion.h"
#include "GraphicsContextCG.h"
#include "ImageObserver.h"
#include "SubimageCacheWithTimer.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

bool FrameData::clear(bool clearMetadata)
{
    if (clearMetadata)
        m_haveMetadata = false;

    m_orientation = DefaultImageOrientation;

    if (m_frame) {
#if CACHE_SUBIMAGES
        subimageCache().clearImage(m_frame);
#endif
        CGImageRelease(m_frame);
        m_frame = 0;
        return true;
    }
    return false;
}

BitmapImage::BitmapImage(CGImageRef cgImage, ImageObserver* observer)
    : Image(observer)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_decodedSize(0)
    , m_decodedPropertiesSize(0)
    , m_frameCount(1)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_haveFrameCount(true)
{
    CGFloat width = CGImageGetWidth(cgImage);
    CGFloat height = CGImageGetHeight(cgImage);
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

    // Since we don't have a decoder, we can't figure out the image orientation.
    // Set m_sizeRespectingOrientation to be the same as m_size so it's not 0x0.
    m_sizeRespectingOrientation = IntSize(width, height);

    m_frames.grow(1);
    m_frames[0].m_frame = cgImage;
    m_frames[0].m_hasAlpha = true;
    m_frames[0].m_haveMetadata = true;

    checkForSolidColor();
}

// Drawing Routines

void BitmapImage::checkForSolidColor()
{
    m_checkedForSolidColor = true;
    if (frameCount() > 1) {
        m_isSolidColor = false;
        return;
    }

    CGImageRef image = frameAtIndex(0);
    
    // Currently we only check for solid color in the important special case of a 1x1 image.
    if (image && CGImageGetWidth(image) == 1 && CGImageGetHeight(image) == 1) {
        unsigned char pixel[4]; // RGBA
        RetainPtr<CGContextRef> bmap = adoptCF(CGBitmapContextCreate(pixel, 1, 1, 8, sizeof(pixel), deviceRGBColorSpaceRef(),
            kCGImageAlphaPremultipliedLast | kCGBitmapByteOrder32Big));
        if (!bmap)
            return;
        GraphicsContext(bmap.get()).setCompositeOperation(CompositeCopy);
        CGRect dst = { {0, 0}, {1, 1} };
        CGContextDrawImage(bmap.get(), dst, image);
        if (pixel[3] == 0)
            m_solidColor = Color(0, 0, 0, 0);
        else
            m_solidColor = Color(pixel[0] * 255 / pixel[3], pixel[1] * 255 / pixel[3], pixel[2] * 255 / pixel[3], pixel[3]);
        m_isSolidColor = true;
    }
}

CGImageRef BitmapImage::getCGImageRef()
{
    return frameAtIndex(0);
}

CGImageRef BitmapImage::getFirstCGImageRefOfSize(const IntSize& size)
{
    size_t count = frameCount();
    for (size_t i = 0; i < count; ++i) {
        CGImageRef cgImage = frameAtIndex(i);
        if (cgImage && IntSize(CGImageGetWidth(cgImage), CGImageGetHeight(cgImage)) == size)
            return cgImage;
    }

    // Fallback to the default CGImageRef if we can't find the right size
    return getCGImageRef();
}

RetainPtr<CFArrayRef> BitmapImage::getCGImageArray()
{
    size_t count = frameCount();
    if (!count)
        return 0;
    
    CFMutableArrayRef array = CFArrayCreateMutable(NULL, count, &kCFTypeArrayCallBacks);
    for (size_t i = 0; i < count; ++i) {
        if (CGImageRef currFrame = frameAtIndex(i))
            CFArrayAppendValue(array, currFrame);
    }
    return adoptCF(array);
}

void BitmapImage::draw(GraphicsContext* ctx, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator op, BlendMode blendMode)
{
    draw(ctx, dstRect, srcRect, styleColorSpace, op, blendMode, DoNotRespectImageOrientation);
}

void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& destRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode, RespectImageOrientationEnum shouldRespectImageOrientation)
{
    startAnimation();

    CGImageRef image = frameAtIndex(m_currentFrame);
    if (!image) // If it's too early we won't have an image yet.
        return;
    
    if (mayFillWithSolidColor()) {
        fillWithSolidColor(ctxt, destRect, solidColor(), styleColorSpace, compositeOp);
        return;
    }

    FloatSize selfSize = currentFrameSize();
    ImageOrientation orientation = DefaultImageOrientation;

    if (shouldRespectImageOrientation == RespectImageOrientation)
        orientation = frameOrientationAtIndex(m_currentFrame);

    ctxt->drawNativeImage(image, selfSize, styleColorSpace, destRect, srcRect, compositeOp, blendMode, orientation);

    if (imageObserver())
        imageObserver()->didDraw(this);
}

}

#endif // USE(CG)
