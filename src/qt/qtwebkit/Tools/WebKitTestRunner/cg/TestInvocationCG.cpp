/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestInvocation.h"

#include "PixelDumpSupport.h"
#include "PlatformWebView.h"
#include "TestController.h"
#include <ImageIO/CGImageDestination.h>
#include <WebKit2/WKImageCG.h>
#include <wtf/MD5.h>
#include <wtf/RetainPtr.h>
#include <wtf/StringExtras.h>

#if PLATFORM(MAC)
#include <LaunchServices/UTCoreTypes.h>
#endif

#if PLATFORM(WIN)
static const CFStringRef kUTTypePNG = CFSTR("public.png");
#endif

namespace WTR {

enum FlipGraphicsContextOrNot {
    DontFlipGraphicsContext,
    FlipGraphicsContext
};

static CGContextRef createCGContextFromImage(WKImageRef wkImage, FlipGraphicsContextOrNot flip = DontFlipGraphicsContext)
{
    RetainPtr<CGImageRef> image = adoptCF(WKImageCreateCGImage(wkImage));

    size_t pixelsWide = CGImageGetWidth(image.get());
    size_t pixelsHigh = CGImageGetHeight(image.get());
    size_t rowBytes = (4 * pixelsWide + 63) & ~63;

    // Creating this bitmap in the device color space should prevent any color conversion when the image of the web view is drawn into it.
    RetainPtr<CGColorSpaceRef> colorSpace = adoptCF(CGColorSpaceCreateDeviceRGB());
    CGContextRef context = CGBitmapContextCreate(0, pixelsWide, pixelsHigh, 8, rowBytes, colorSpace.get(), kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Host);
    
    if (flip == FlipGraphicsContext) {
        CGContextSaveGState(context);
        CGContextScaleCTM(context, 1, -1);
        CGContextTranslateCTM(context, 0, -static_cast<CGFloat>(pixelsHigh));
    }
    
    CGContextDrawImage(context, CGRectMake(0, 0, pixelsWide, pixelsHigh), image.get());
    if (flip == FlipGraphicsContext)
        CGContextRestoreGState(context);

    return context;
}

void computeMD5HashStringForContext(CGContextRef bitmapContext, char hashString[33])
{
    ASSERT(CGBitmapContextGetBitsPerPixel(bitmapContext) == 32); // ImageDiff assumes 32 bit RGBA, we must as well.
    size_t pixelsHigh = CGBitmapContextGetHeight(bitmapContext);
    size_t pixelsWide = CGBitmapContextGetWidth(bitmapContext);
    size_t bytesPerRow = CGBitmapContextGetBytesPerRow(bitmapContext);
    
    // We need to swap the bytes to ensure consistent hashes independently of endianness
    MD5 md5;
    unsigned char* bitmapData = static_cast<unsigned char*>(CGBitmapContextGetData(bitmapContext));
#if PLATFORM(MAC)
    if ((CGBitmapContextGetBitmapInfo(bitmapContext) & kCGBitmapByteOrderMask) == kCGBitmapByteOrder32Big) {
        for (unsigned row = 0; row < pixelsHigh; row++) {
            Vector<uint8_t> buffer(4 * pixelsWide);
            for (unsigned column = 0; column < pixelsWide; column++)
                buffer[column] = OSReadLittleInt32(bitmapData, 4 * column);
            md5.addBytes(buffer);
            bitmapData += bytesPerRow;
        }
    } else
#endif
    {
        for (unsigned row = 0; row < pixelsHigh; row++) {
            md5.addBytes(bitmapData, 4 * pixelsWide);
            bitmapData += bytesPerRow;
        }
    }

    Vector<uint8_t, 16> hash;
    md5.checksum(hash);
    
    hashString[0] = '\0';
    for (int i = 0; i < 16; i++)
        snprintf(hashString, 33, "%s%02x", hashString, hash[i]);
}

static void dumpBitmap(CGContextRef bitmapContext, const char* checksum)
{
    RetainPtr<CGImageRef> image = adoptCF(CGBitmapContextCreateImage(bitmapContext));
    RetainPtr<CFMutableDataRef> imageData = adoptCF(CFDataCreateMutable(0, 0));
    RetainPtr<CGImageDestinationRef> imageDest = adoptCF(CGImageDestinationCreateWithData(imageData.get(), kUTTypePNG, 1, 0));
    CGImageDestinationAddImage(imageDest.get(), image.get(), 0);
    CGImageDestinationFinalize(imageDest.get());

    const unsigned char* data = CFDataGetBytePtr(imageData.get());
    const size_t dataLength = CFDataGetLength(imageData.get());

    printPNG(data, dataLength, checksum);
}

static void paintRepaintRectOverlay(CGContextRef context, WKImageRef image, WKArrayRef repaintRects)
{
    WKSize imageSize = WKImageGetSize(image);

    CGContextSaveGState(context);

    // Using a transparency layer is easier than futzing with clipping.
    CGContextBeginTransparencyLayer(context, 0);
    
    // Flip the context.
    CGContextScaleCTM(context, 1, -1);
    CGContextTranslateCTM(context, 0, -imageSize.height);
    
    CGContextSetRGBFillColor(context, 0, 0, 0, static_cast<CGFloat>(0.66));
    CGContextFillRect(context, CGRectMake(0, 0, imageSize.width, imageSize.height));

    // Clear the repaint rects.
    size_t count = WKArrayGetSize(repaintRects);
    for (size_t i = 0; i < count; ++i) {
        WKRect rect = WKRectGetValue(static_cast<WKRectRef>(WKArrayGetItemAtIndex(repaintRects, i)));
        CGRect cgRect = CGRectMake(rect.origin.x, rect.origin.y, rect.size.width, rect.size.height);
        CGContextClearRect(context, cgRect);
    }
    
    CGContextEndTransparencyLayer(context);
    CGContextRestoreGState(context);
}

void TestInvocation::dumpPixelsAndCompareWithExpected(WKImageRef image, WKArrayRef repaintRects)
{
    PlatformWebView* webView = TestController::shared().mainWebView();
    WKRetainPtr<WKImageRef> windowSnapshot = webView->windowSnapshotImage();

    // There is no way at this time to fake a window's scale factor, so we need to avoid the window
    // snapshots for HiDPI tests.
    if (WKPageGetBackingScaleFactor(webView->page()) != 1)
        windowSnapshot = 0;

    RetainPtr<CGContextRef> context;
    if (windowSnapshot)
        context = adoptCF(createCGContextFromImage(windowSnapshot.get(), DontFlipGraphicsContext));
    else
        context = adoptCF(createCGContextFromImage(image));

    // A non-null repaintRects array means we're doing a repaint test.
    if (repaintRects)
        paintRepaintRectOverlay(context.get(), image, repaintRects);

    char actualHash[33];
    computeMD5HashStringForContext(context.get(), actualHash);
    if (!compareActualHashToExpectedAndDumpResults(actualHash))
        dumpBitmap(context.get(), actualHash);
}

} // namespace WTR
