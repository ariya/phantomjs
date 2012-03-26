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

#include "AffineTransform.h"
#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContextCG.h"
#include "ImageObserver.h"
#include "PDFDocumentImage.h"
#include "PlatformString.h"
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CFArray.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(MAC) || PLATFORM(CHROMIUM)
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

    if (m_frame) {
        CGImageRelease(m_frame);
        m_frame = 0;
        return true;
    }
    return false;
}

// ================================================
// Image Class
// ================================================

BitmapImage::BitmapImage(CGImageRef cgImage, ImageObserver* observer)
    : Image(observer)
    , m_currentFrame(0)
    , m_frames(0)
    , m_frameTimer(0)
    , m_repetitionCount(cAnimationNone)
    , m_repetitionCountStatus(Unknown)
    , m_repetitionsComplete(0)
    , m_isSolidColor(false)
    , m_checkedForSolidColor(false)
    , m_animationFinished(true)
    , m_allDataReceived(true)
    , m_haveSize(true)
    , m_sizeAvailable(true)
    , m_decodedSize(0)
    , m_haveFrameCount(true)
    , m_frameCount(1)
{
    initPlatformData();
    
    CGFloat width = CGImageGetWidth(cgImage);
    CGFloat height = CGImageGetHeight(cgImage);
    m_decodedSize = width * height * 4;
    m_size = IntSize(width, height);

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
        RetainPtr<CGContextRef> bmap(AdoptCF, CGBitmapContextCreate(pixel, 1, 1, 8, sizeof(pixel), deviceRGBColorSpaceRef(),
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

static RetainPtr<CGImageRef> imageWithColorSpace(CGImageRef originalImage, ColorSpace colorSpace)
{
    CGColorSpaceRef originalColorSpace = CGImageGetColorSpace(originalImage);

    // If the image already has a (non-device) color space, we don't want to
    // override it, so return.
    if (!originalColorSpace || !CFEqual(originalColorSpace, deviceRGBColorSpaceRef()))
        return originalImage;

    switch (colorSpace) {
    case ColorSpaceDeviceRGB:
        return originalImage;
    case ColorSpaceSRGB:
        return RetainPtr<CGImageRef>(AdoptCF, CGImageCreateCopyWithColorSpace(originalImage, sRGBColorSpaceRef()));
    case ColorSpaceLinearRGB:
        return RetainPtr<CGImageRef>(AdoptCF, CGImageCreateCopyWithColorSpace(originalImage, linearRGBColorSpaceRef()));
    }

    ASSERT_NOT_REACHED();
    return originalImage;
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
        if (IntSize(CGImageGetWidth(cgImage), CGImageGetHeight(cgImage)) == size)
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
    for (size_t i = 0; i < count; ++i)
        CFArrayAppendValue(array, frameAtIndex(i));
        
    return RetainPtr<CFArrayRef>(AdoptCF, array);
}

void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& destRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp)
{
    startAnimation();

    RetainPtr<CGImageRef> image = frameAtIndex(m_currentFrame);
    if (!image) // If it's too early we won't have an image yet.
        return;
    
    if (mayFillWithSolidColor()) {
        fillWithSolidColor(ctxt, destRect, solidColor(), styleColorSpace, compositeOp);
        return;
    }

    float currHeight = CGImageGetHeight(image.get());
    if (currHeight <= srcRect.y())
        return;

    CGContextRef context = ctxt->platformContext();
    GraphicsContextStateSaver stateSaver(*ctxt);

    bool shouldUseSubimage = false;

    // If the source rect is a subportion of the image, then we compute an inflated destination rect that will hold the entire image
    // and then set a clip to the portion that we want to display.
    FloatRect adjustedDestRect = destRect;
    FloatSize selfSize = currentFrameSize();
    if (srcRect.size() != selfSize) {
        CGInterpolationQuality interpolationQuality = CGContextGetInterpolationQuality(context);
        // When the image is scaled using high-quality interpolation, we create a temporary CGImage
        // containing only the portion we want to display. We need to do this because high-quality
        // interpolation smoothes sharp edges, causing pixels from outside the source rect to bleed
        // into the destination rect. See <rdar://problem/6112909>.
        shouldUseSubimage = (interpolationQuality != kCGInterpolationNone) && (srcRect.size() != destRect.size() || !ctxt->getCTM().isIdentityOrTranslationOrFlipped());
        float xScale = srcRect.width() / destRect.width();
        float yScale = srcRect.height() / destRect.height();
        if (shouldUseSubimage) {
            FloatRect subimageRect = srcRect;
            float leftPadding = srcRect.x() - floorf(srcRect.x());
            float topPadding = srcRect.y() - floorf(srcRect.y());

            subimageRect.move(-leftPadding, -topPadding);
            adjustedDestRect.move(-leftPadding / xScale, -topPadding / yScale);

            subimageRect.setWidth(ceilf(subimageRect.width() + leftPadding));
            adjustedDestRect.setWidth(subimageRect.width() / xScale);

            subimageRect.setHeight(ceilf(subimageRect.height() + topPadding));
            adjustedDestRect.setHeight(subimageRect.height() / yScale);

            image.adoptCF(CGImageCreateWithImageInRect(image.get(), subimageRect));
            if (currHeight < srcRect.maxY()) {
                ASSERT(CGImageGetHeight(image.get()) == currHeight - CGRectIntegral(srcRect).origin.y);
                adjustedDestRect.setHeight(CGImageGetHeight(image.get()) / yScale);
            }
        } else {
            adjustedDestRect.setLocation(FloatPoint(destRect.x() - srcRect.x() / xScale, destRect.y() - srcRect.y() / yScale));
            adjustedDestRect.setSize(FloatSize(selfSize.width() / xScale, selfSize.height() / yScale));
        }

        CGContextClipToRect(context, destRect);
    }

    // If the image is only partially loaded, then shrink the destination rect that we're drawing into accordingly.
    if (!shouldUseSubimage && currHeight < selfSize.height())
        adjustedDestRect.setHeight(adjustedDestRect.height() * currHeight / selfSize.height());

    ctxt->setCompositeOperation(compositeOp);

    // Flip the coords.
    CGContextScaleCTM(context, 1, -1);
    adjustedDestRect.setY(-adjustedDestRect.maxY());

    // Adjust the color space.
    image = imageWithColorSpace(image.get(), styleColorSpace);

    // Draw the image.
    CGContextDrawImage(context, adjustedDestRect, image.get());

    stateSaver.restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}

static void drawPatternCallback(void* info, CGContextRef context)
{
    CGImageRef image = (CGImageRef)info;
    CGContextDrawImage(context, GraphicsContext(context).roundToDevicePixels(FloatRect(0, 0, CGImageGetWidth(image), CGImageGetHeight(image))), image);
}

void Image::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRect, const AffineTransform& patternTransform,
                        const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    if (!nativeImageForCurrentFrame())
        return;

    ASSERT(patternTransform.isInvertible());
    if (!patternTransform.isInvertible())
        // Avoid a hang under CGContextDrawTiledImage on release builds.
        return;

    CGContextRef context = ctxt->platformContext();
    GraphicsContextStateSaver stateSaver(*ctxt);
    CGContextClipToRect(context, destRect);
    ctxt->setCompositeOperation(op);
    CGContextTranslateCTM(context, destRect.x(), destRect.y() + destRect.height());
    CGContextScaleCTM(context, 1, -1);
    
    // Compute the scaled tile size.
    float scaledTileHeight = tileRect.height() * narrowPrecisionToFloat(patternTransform.d());
    
    // We have to adjust the phase to deal with the fact we're in Cartesian space now (with the bottom left corner of destRect being
    // the origin).
    float adjustedX = phase.x() - destRect.x() + tileRect.x() * narrowPrecisionToFloat(patternTransform.a()); // We translated the context so that destRect.x() is the origin, so subtract it out.
    float adjustedY = destRect.height() - (phase.y() - destRect.y() + tileRect.y() * narrowPrecisionToFloat(patternTransform.d()) + scaledTileHeight);

    CGImageRef tileImage = nativeImageForCurrentFrame();
    float h = CGImageGetHeight(tileImage);

    RetainPtr<CGImageRef> subImage;
    if (tileRect.size() == size())
        subImage = tileImage;
    else {
        // Copying a sub-image out of a partially-decoded image stops the decoding of the original image. It should never happen
        // because sub-images are only used for border-image, which only renders when the image is fully decoded.
        ASSERT(h == height());
        subImage.adoptCF(CGImageCreateWithImageInRect(tileImage, tileRect));
    }

    // Adjust the color space.
    subImage = imageWithColorSpace(subImage.get(), styleColorSpace);
    
    // Leopard has an optimized call for the tiling of image patterns, but we can only use it if the image has been decoded enough that
    // its buffer is the same size as the overall image.  Because a partially decoded CGImageRef with a smaller width or height than the
    // overall image buffer needs to tile with "gaps", we can't use the optimized tiling call in that case.
    // FIXME: We cannot use CGContextDrawTiledImage with scaled tiles on Leopard, because it suffers from rounding errors.  Snow Leopard is ok.
    float scaledTileWidth = tileRect.width() * narrowPrecisionToFloat(patternTransform.a());
    float w = CGImageGetWidth(tileImage);
#ifdef BUILDING_ON_LEOPARD
    if (w == size().width() && h == size().height() && scaledTileWidth == tileRect.width() && scaledTileHeight == tileRect.height())
#else
    if (w == size().width() && h == size().height())
#endif
        CGContextDrawTiledImage(context, FloatRect(adjustedX, adjustedY, scaledTileWidth, scaledTileHeight), subImage.get());
    else {

    // On Leopard and newer, this code now only runs for partially decoded images whose buffers do not yet match the overall size of the image.
    static const CGPatternCallbacks patternCallbacks = { 0, drawPatternCallback, NULL };
    CGAffineTransform matrix = CGAffineTransformMake(narrowPrecisionToCGFloat(patternTransform.a()), 0, 0, narrowPrecisionToCGFloat(patternTransform.d()), adjustedX, adjustedY);
    matrix = CGAffineTransformConcat(matrix, CGContextGetCTM(context));
    // The top of a partially-decoded image is drawn at the bottom of the tile. Map it to the top.
    matrix = CGAffineTransformTranslate(matrix, 0, size().height() - h);
    RetainPtr<CGPatternRef> pattern(AdoptCF, CGPatternCreate(subImage.get(), CGRectMake(0, 0, tileRect.width(), tileRect.height()),
                                             matrix, tileRect.width(), tileRect.height(), 
                                             kCGPatternTilingConstantSpacing, true, &patternCallbacks));
    if (!pattern)
        return;

    RetainPtr<CGColorSpaceRef> patternSpace(AdoptCF, CGColorSpaceCreatePattern(0));
    
    CGFloat alpha = 1;
    RetainPtr<CGColorRef> color(AdoptCF, CGColorCreateWithPattern(patternSpace.get(), pattern.get(), &alpha));
    CGContextSetFillColorSpace(context, patternSpace.get());

    // FIXME: Really want a public API for this.  It is just CGContextSetBaseCTM(context, CGAffineTransformIdentiy).
    wkSetPatternBaseCTM(context, CGAffineTransformIdentity);
    CGContextSetPatternPhase(context, CGSizeZero);

    CGContextSetFillColorWithColor(context, color.get());
    CGContextFillRect(context, CGContextGetClipBoundingBox(context));

    }

    stateSaver.restore();

    if (imageObserver())
        imageObserver()->didDraw(this);
}


}

#endif // USE(CG)
