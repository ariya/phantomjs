/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#if ENABLE(WEBGL)

#include "GraphicsContext3D.h"

#include "BitmapImage.h"
#include "GraphicsContextCG.h"
#include "Image.h"

#include <CoreGraphics/CGBitmapContext.h>
#include <CoreGraphics/CGContext.h>
#include <CoreGraphics/CGDataProvider.h>
#include <CoreGraphics/CGImage.h>

#include <wtf/RetainPtr.h>

namespace WebCore {

enum SourceDataFormatBase {
    SourceFormatBaseR = 0,
    SourceFormatBaseA,
    SourceFormatBaseRA,
    SourceFormatBaseAR,
    SourceFormatBaseRGB,
    SourceFormatBaseRGBA,
    SourceFormatBaseARGB,
    SourceFormatBaseNumFormats
};

enum AlphaFormat {
    AlphaFormatNone = 0,
    AlphaFormatFirst,
    AlphaFormatLast,
    AlphaFormatNumFormats
};

// This returns SourceFormatNumFormats if the combination of input parameters is unsupported.
static GraphicsContext3D::SourceDataFormat getSourceDataFormat(unsigned int componentsPerPixel, AlphaFormat alphaFormat, bool is16BitFormat, bool bigEndian)
{
    const static SourceDataFormatBase formatTableBase[4][AlphaFormatNumFormats] = { // componentsPerPixel x AlphaFormat
        // AlphaFormatNone            AlphaFormatFirst            AlphaFormatLast
        { SourceFormatBaseR,          SourceFormatBaseA,          SourceFormatBaseA          }, // 1 componentsPerPixel
        { SourceFormatBaseNumFormats, SourceFormatBaseAR,         SourceFormatBaseRA         }, // 2 componentsPerPixel
        { SourceFormatBaseRGB,        SourceFormatBaseNumFormats, SourceFormatBaseNumFormats }, // 3 componentsPerPixel
        { SourceFormatBaseNumFormats, SourceFormatBaseARGB,       SourceFormatBaseRGBA        } // 4 componentsPerPixel
    };
    const static GraphicsContext3D::SourceDataFormat formatTable[SourceFormatBaseNumFormats][4] = { // SourceDataFormatBase x bitsPerComponent x endian
        // 8bits, little endian                 8bits, big endian                     16bits, little endian                        16bits, big endian
        { GraphicsContext3D::SourceFormatR8,    GraphicsContext3D::SourceFormatR8,    GraphicsContext3D::SourceFormatR16Little,    GraphicsContext3D::SourceFormatR16Big },
        { GraphicsContext3D::SourceFormatA8,    GraphicsContext3D::SourceFormatA8,    GraphicsContext3D::SourceFormatA16Little,    GraphicsContext3D::SourceFormatA16Big },
        { GraphicsContext3D::SourceFormatAR8,   GraphicsContext3D::SourceFormatRA8,   GraphicsContext3D::SourceFormatRA16Little,   GraphicsContext3D::SourceFormatRA16Big },
        { GraphicsContext3D::SourceFormatRA8,   GraphicsContext3D::SourceFormatAR8,   GraphicsContext3D::SourceFormatAR16Little,   GraphicsContext3D::SourceFormatAR16Big },
        { GraphicsContext3D::SourceFormatBGR8,  GraphicsContext3D::SourceFormatRGB8,  GraphicsContext3D::SourceFormatRGB16Little,  GraphicsContext3D::SourceFormatRGB16Big },
        { GraphicsContext3D::SourceFormatABGR8, GraphicsContext3D::SourceFormatRGBA8, GraphicsContext3D::SourceFormatRGBA16Little, GraphicsContext3D::SourceFormatRGBA16Big },
        { GraphicsContext3D::SourceFormatBGRA8, GraphicsContext3D::SourceFormatARGB8, GraphicsContext3D::SourceFormatARGB16Little, GraphicsContext3D::SourceFormatARGB16Big }
    };

    ASSERT(componentsPerPixel <= 4 && componentsPerPixel > 0);
    SourceDataFormatBase formatBase = formatTableBase[componentsPerPixel - 1][alphaFormat];
    if (formatBase == SourceFormatBaseNumFormats)
        return GraphicsContext3D::SourceFormatNumFormats;
    return formatTable[formatBase][(is16BitFormat ? 2 : 0) + (bigEndian ? 1 : 0)];
}

bool GraphicsContext3D::getImageData(Image* image,
                                     GC3Denum format,
                                     GC3Denum type,
                                     bool premultiplyAlpha,
                                     bool ignoreGammaAndColorProfile,
                                     Vector<uint8_t>& outputVector)
{
    if (!image)
        return false;
    CGImageRef cgImage;
    RetainPtr<CGImageRef> decodedImage;
    bool hasAlpha = image->isBitmapImage() ? static_cast<BitmapImage*>(image)->frameHasAlphaAtIndex(0) : true;
    if ((ignoreGammaAndColorProfile || (hasAlpha && !premultiplyAlpha)) && image->data()) {
        ImageSource decoder(ImageSource::AlphaNotPremultiplied,
                            ignoreGammaAndColorProfile ? ImageSource::GammaAndColorProfileIgnored : ImageSource::GammaAndColorProfileApplied);
        decoder.setData(image->data(), true);
        if (!decoder.frameCount())
            return false;
        decodedImage.adoptCF(decoder.createFrameAtIndex(0));
        cgImage = decodedImage.get();
    } else
        cgImage = image->nativeImageForCurrentFrame();
    if (!cgImage)
        return false;

    size_t width = CGImageGetWidth(cgImage);
    size_t height = CGImageGetHeight(cgImage);
    if (!width || !height)
        return false;

    // See whether the image is using an indexed color space, and if
    // so, re-render it into an RGB color space. The image re-packing
    // code requires color data, not color table indices, for the
    // image data.
    CGColorSpaceRef colorSpace = CGImageGetColorSpace(cgImage);
    CGColorSpaceModel model = CGColorSpaceGetModel(colorSpace);
    if (model == kCGColorSpaceModelIndexed) {
        RetainPtr<CGContextRef> bitmapContext;
        // FIXME: we should probably manually convert the image by indexing into
        // the color table, which would allow us to avoid premultiplying the
        // alpha channel. Creation of a bitmap context with an alpha channel
        // doesn't seem to work unless it's premultiplied.
        bitmapContext.adoptCF(CGBitmapContextCreate(0, width, height, 8, width * 4,
                                                    deviceRGBColorSpaceRef(),
                                                    kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host));
        if (!bitmapContext)
            return false;

        CGContextSetBlendMode(bitmapContext.get(), kCGBlendModeCopy);
        CGContextSetInterpolationQuality(bitmapContext.get(), kCGInterpolationNone);
        CGContextDrawImage(bitmapContext.get(), CGRectMake(0, 0, width, height), cgImage);

        // Now discard the original CG image and replace it with a copy from the bitmap context.
        decodedImage.adoptCF(CGBitmapContextCreateImage(bitmapContext.get()));
        cgImage = decodedImage.get();
    }

    size_t bitsPerComponent = CGImageGetBitsPerComponent(cgImage);
    size_t bitsPerPixel = CGImageGetBitsPerPixel(cgImage);
    if (bitsPerComponent != 8 && bitsPerComponent != 16)
        return false;
    if (bitsPerPixel % bitsPerComponent)
        return false;
    size_t componentsPerPixel = bitsPerPixel / bitsPerComponent;

    CGBitmapInfo bitInfo = CGImageGetBitmapInfo(cgImage);
    bool bigEndianSource = false;
    // These could technically be combined into one large switch
    // statement, but we prefer not to so that we fail fast if we
    // encounter an unexpected image configuration.
    if (bitsPerComponent == 16) {
        switch (bitInfo & kCGBitmapByteOrderMask) {
        case kCGBitmapByteOrder16Big:
            bigEndianSource = true;
            break;
        case kCGBitmapByteOrder16Little:
            bigEndianSource = false;
            break;
        case kCGBitmapByteOrderDefault:
            // This is a bug in earlier version of cg where the default endian
            // is little whereas the decoded 16-bit png image data is actually
            // Big. Later version (10.6.4) no longer returns ByteOrderDefault.
            bigEndianSource = true;
            break;
        default:
            return false;
        }
    } else {
        switch (bitInfo & kCGBitmapByteOrderMask) {
        case kCGBitmapByteOrder32Big:
            bigEndianSource = true;
            break;
        case kCGBitmapByteOrder32Little:
            bigEndianSource = false;
            break;
        case kCGBitmapByteOrderDefault:
            // It appears that the default byte order is actually big
            // endian even on little endian architectures.
            bigEndianSource = true;
            break;
        default:
            return false;
        }
    }

    AlphaOp neededAlphaOp = AlphaDoNothing;
    AlphaFormat alphaFormat = AlphaFormatNone;
    switch (CGImageGetAlphaInfo(cgImage)) {
    case kCGImageAlphaPremultipliedFirst:
        if (!premultiplyAlpha)
            neededAlphaOp = AlphaDoUnmultiply;
        alphaFormat = AlphaFormatFirst;
        break;
    case kCGImageAlphaFirst:
        // This path is only accessible for MacOS earlier than 10.6.4.
        if (premultiplyAlpha)
            neededAlphaOp = AlphaDoPremultiply;
        alphaFormat = AlphaFormatFirst;
        break;
    case kCGImageAlphaNoneSkipFirst:
        // This path is only accessible for MacOS earlier than 10.6.4.
        alphaFormat = AlphaFormatFirst;
        break;
    case kCGImageAlphaPremultipliedLast:
        if (!premultiplyAlpha)
            neededAlphaOp = AlphaDoUnmultiply;
        alphaFormat = AlphaFormatLast;
        break;
    case kCGImageAlphaLast:
        if (premultiplyAlpha)
            neededAlphaOp = AlphaDoPremultiply;
        alphaFormat = AlphaFormatLast;
        break;
    case kCGImageAlphaNoneSkipLast:
        alphaFormat = AlphaFormatLast;
        break;
    case kCGImageAlphaNone:
        alphaFormat = AlphaFormatNone;
        break;
    default:
        return false;
    }
    SourceDataFormat srcDataFormat = getSourceDataFormat(componentsPerPixel, alphaFormat, bitsPerComponent == 16, bigEndianSource);
    if (srcDataFormat == SourceFormatNumFormats)
        return false;

    RetainPtr<CFDataRef> pixelData;
    pixelData.adoptCF(CGDataProviderCopyData(CGImageGetDataProvider(cgImage)));
    if (!pixelData)
        return false;
    const UInt8* rgba = CFDataGetBytePtr(pixelData.get());
    outputVector.resize(width * height * 4);
    unsigned int srcUnpackAlignment = 0;
    size_t bytesPerRow = CGImageGetBytesPerRow(cgImage);
    unsigned int padding = bytesPerRow - bitsPerPixel / 8 * width;
    if (padding) {
        srcUnpackAlignment = padding + 1;
        while (bytesPerRow % srcUnpackAlignment)
            ++srcUnpackAlignment;
    }
    bool rt = packPixels(rgba, srcDataFormat, width, height, srcUnpackAlignment,
                         format, type, neededAlphaOp, outputVector.data());
    return rt;
}

void GraphicsContext3D::paintToCanvas(const unsigned char* imagePixels, int imageWidth, int imageHeight, int canvasWidth, int canvasHeight, CGContextRef context)
{
    if (!imagePixels || imageWidth <= 0 || imageHeight <= 0 || canvasWidth <= 0 || canvasHeight <= 0 || !context)
        return;
    int rowBytes = imageWidth * 4;
    RetainPtr<CGDataProviderRef> dataProvider(AdoptCF, CGDataProviderCreateWithData(0, imagePixels, rowBytes * imageHeight, 0));
    RetainPtr<CGImageRef> cgImage(AdoptCF, CGImageCreate(imageWidth, imageHeight, 8, 32, rowBytes, deviceRGBColorSpaceRef(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host,
        dataProvider.get(), 0, false, kCGRenderingIntentDefault));
    // CSS styling may cause the canvas's content to be resized on
    // the page. Go back to the Canvas to figure out the correct
    // width and height to draw.
    CGRect rect = CGRectMake(0, 0, canvasWidth, canvasHeight);
    // We want to completely overwrite the previous frame's
    // rendering results.
    CGContextSaveGState(context);
    CGContextSetBlendMode(context, kCGBlendModeCopy);
    CGContextSetInterpolationQuality(context, kCGInterpolationNone);
    CGContextDrawImage(context, rect, cgImage.get());
    CGContextRestoreGState(context);
}

} // namespace WebCore

#endif // ENABLE(WEBGL)
