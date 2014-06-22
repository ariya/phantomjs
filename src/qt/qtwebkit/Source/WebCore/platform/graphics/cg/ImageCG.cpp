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
#include "Image.h"

#if USE(CG)

#include "FloatConversion.h"
#include "FloatRect.h"
#include "GraphicsContextCG.h"
#include "ImageObserver.h"
#include <ApplicationServices/ApplicationServices.h>
#include <wtf/RetainPtr.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#endif

#if PLATFORM(WIN)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

namespace WebCore {

RetainPtr<CGImageRef> Image::imageWithColorSpace(CGImageRef originalImage, ColorSpace colorSpace)
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
        return adoptCF(CGImageCreateCopyWithColorSpace(originalImage, sRGBColorSpaceRef()));
    case ColorSpaceLinearRGB:
        return adoptCF(CGImageCreateCopyWithColorSpace(originalImage, linearRGBColorSpaceRef()));
    }

    ASSERT_NOT_REACHED();
    return originalImage;
}

static void drawPatternCallback(void* info, CGContextRef context)
{
    CGImageRef image = (CGImageRef)info;
    CGContextDrawImage(context, GraphicsContext(context).roundToDevicePixels(FloatRect(0, 0, CGImageGetWidth(image), CGImageGetHeight(image))), image);
}

void Image::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode blendMode)
{
    if (!nativeImageForCurrentFrame())
        return;

    if (!patternTransform.isInvertible())
        return;

    CGContextRef context = ctxt->platformContext();
    GraphicsContextStateSaver stateSaver(*ctxt);
    CGContextClipToRect(context, destRect);
    ctxt->setCompositeOperation(op, blendMode);
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
        subImage = adoptCF(CGImageCreateWithImageInRect(tileImage, tileRect));
    }

    // Adjust the color space.
    subImage = Image::imageWithColorSpace(subImage.get(), styleColorSpace);
    
    // Leopard has an optimized call for the tiling of image patterns, but we can only use it if the image has been decoded enough that
    // its buffer is the same size as the overall image.  Because a partially decoded CGImageRef with a smaller width or height than the
    // overall image buffer needs to tile with "gaps", we can't use the optimized tiling call in that case.
    // FIXME: We cannot use CGContextDrawTiledImage with scaled tiles on Leopard, because it suffers from rounding errors.  Snow Leopard is ok.
    float scaledTileWidth = tileRect.width() * narrowPrecisionToFloat(patternTransform.a());
    float w = CGImageGetWidth(tileImage);
    if (w == size().width() && h == size().height())
        CGContextDrawTiledImage(context, FloatRect(adjustedX, adjustedY, scaledTileWidth, scaledTileHeight), subImage.get());
    else {

    // On Leopard and newer, this code now only runs for partially decoded images whose buffers do not yet match the overall size of the image.
    static const CGPatternCallbacks patternCallbacks = { 0, drawPatternCallback, NULL };
    CGAffineTransform matrix = CGAffineTransformMake(narrowPrecisionToCGFloat(patternTransform.a()), 0, 0, narrowPrecisionToCGFloat(patternTransform.d()), adjustedX, adjustedY);
    matrix = CGAffineTransformConcat(matrix, CGContextGetCTM(context));
    // The top of a partially-decoded image is drawn at the bottom of the tile. Map it to the top.
    matrix = CGAffineTransformTranslate(matrix, 0, size().height() - h);
    RetainPtr<CGPatternRef> pattern = adoptCF(CGPatternCreate(subImage.get(), CGRectMake(0, 0, tileRect.width(), tileRect.height()),
                                             matrix, tileRect.width(), tileRect.height(), 
                                             kCGPatternTilingConstantSpacing, true, &patternCallbacks));
    if (!pattern)
        return;

    RetainPtr<CGColorSpaceRef> patternSpace = adoptCF(CGColorSpaceCreatePattern(0));
    
    CGFloat alpha = 1;
    RetainPtr<CGColorRef> color = adoptCF(CGColorCreateWithPattern(patternSpace.get(), pattern.get(), &alpha));
    CGContextSetFillColorSpace(context, patternSpace.get());

    // FIXME: Really want a public API for this.  It is just CGContextSetBaseCTM(context, CGAffineTransformIdentiy).
    wkSetBaseCTM(context, CGAffineTransformIdentity);
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
