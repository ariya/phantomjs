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
#include "ImageBufferData.h"

#include <CoreGraphics/CoreGraphics.h>
#include <wtf/Assertions.h>

#if USE(ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if USE(IOSURFACE_CANVAS_BACKING_STORE)
#include <IOSurface/IOSurface.h>
#include <dispatch/dispatch.h>
#endif

using namespace std;

#if USE(ACCELERATE)
struct ScanlineData {
    vImagePixelCount scanlineWidth;
    unsigned char* srcData;
    size_t srcRowBytes;
    unsigned char* destData;
    size_t destRowBytes;
};
#endif

namespace WebCore {

ImageBufferData::ImageBufferData(const IntSize&)
: m_data(0)
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
, m_surface(0)
#endif
{
}

#if USE(ACCELERATE)

#if __MAC_OS_X_VERSION_MAX_ALLOWED >= 1070
static bool haveVImageRoundingErrorFix() { return true; }
#else
// The vImage unpremultiply routine had a rounding bug before 10.6.7 <rdar://problem/8631548>
static bool haveVImageRoundingErrorFix()
{
    SInt32 version;
    static bool result = (Gestalt(gestaltSystemVersion, &version) == noErr && version > 0x1066);
    return result;
}
#endif // __MAC_OS_X_VERSION_MAX_ALLOWED >= 1070

#if USE(IOSURFACE_CANVAS_BACKING_STORE)
static void convertScanline(void* data, size_t tileNumber, bool premultiply)
{
    ScanlineData* scanlineData = static_cast<ScanlineData*>(data);

    vImage_Buffer src;
    src.data = scanlineData->srcData + tileNumber * scanlineData->srcRowBytes;
    src.height = 1;
    src.width = scanlineData->scanlineWidth;
    src.rowBytes = scanlineData->srcRowBytes;

    vImage_Buffer dest;
    dest.data = scanlineData->destData + tileNumber * scanlineData->destRowBytes;
    dest.height = 1;
    dest.width = scanlineData->scanlineWidth;
    dest.rowBytes = scanlineData->destRowBytes;

    if (premultiply) {
        if (kvImageNoError != vImagePremultiplyData_RGBA8888(&src, &dest, kvImageDoNotTile))
            return;
    } else {
        if (kvImageNoError != vImageUnpremultiplyData_RGBA8888(&src, &dest, kvImageDoNotTile))
            return;
    }

    // Swap channels 1 and 3, to convert BGRA<->RGBA. IOSurfaces is BGRA, ImageData expects RGBA.
    const uint8_t map[4] = { 2, 1, 0, 3 };
    vImagePermuteChannels_ARGB8888(&dest, &dest, map, kvImageDoNotTile);
}

static void unpremultitplyScanline(void* data, size_t tileNumber)
{
    convertScanline(data, tileNumber, false);
}

static void premultitplyScanline(void* data, size_t tileNumber)
{
    convertScanline(data, tileNumber, true);
}
#endif // USE(IOSURFACE_CANVAS_BACKING_STORE)
#endif // USE(ACCELERATE)

PassRefPtr<Uint8ClampedArray> ImageBufferData::getData(const IntRect& rect, const IntSize& size, bool accelerateRendering, bool unmultiplied, float resolutionScale) const
{
    Checked<unsigned, RecordOverflow> area = 4;
    area *= rect.width();
    area *= rect.height();
    if (area.hasOverflowed())
        return 0;

    RefPtr<Uint8ClampedArray> result = Uint8ClampedArray::createUninitialized(area.unsafeGet());
    unsigned char* data = result->data();
    
    Checked<int> endx = rect.maxX();
    endx *= ceilf(resolutionScale);
    Checked<int> endy = rect.maxY();
    endy *= resolutionScale;
    if (rect.x() < 0 || rect.y() < 0 || endx.unsafeGet() > size.width() || endy.unsafeGet() > size.height())
        result->zeroFill();
    
    int originx = rect.x();
    int destx = 0;
    int destw = rect.width();
    if (originx < 0) {
        destw += originx;
        destx = -originx;
        originx = 0;
    }
    destw = min<int>(destw, ceilf(size.width() / resolutionScale) - originx);
    originx *= resolutionScale;
    if (endx.unsafeGet() > size.width())
        endx = size.width();
    Checked<int> width = endx - originx;
    
    int originy = rect.y();
    int desty = 0;
    int desth = rect.height();
    if (originy < 0) {
        desth += originy;
        desty = -originy;
        originy = 0;
    }
    desth = min<int>(desth, ceilf(size.height() / resolutionScale) - originy);
    originy *= resolutionScale;
    if (endy.unsafeGet() > size.height())
        endy = size.height();
    Checked<int> height = endy - originy;
    
    if (width.unsafeGet() <= 0 || height.unsafeGet() <= 0)
        return result.release();
    
    unsigned destBytesPerRow = 4 * rect.width();
    unsigned char* destRows = data + desty * destBytesPerRow + destx * 4;
    
    unsigned srcBytesPerRow;
    unsigned char* srcRows;
    
    if (!accelerateRendering) {
        srcBytesPerRow = 4 * size.width();
        srcRows = reinterpret_cast<unsigned char*>(m_data) + originy * srcBytesPerRow + originx * 4;
        
#if USE(ACCELERATE)
        if (unmultiplied && haveVImageRoundingErrorFix()) {
            vImage_Buffer src;
            src.height = height.unsafeGet();
            src.width = width.unsafeGet();
            src.rowBytes = srcBytesPerRow;
            src.data = srcRows;
            
            vImage_Buffer dst;
            dst.height = desth;
            dst.width = destw;
            dst.rowBytes = destBytesPerRow;
            dst.data = destRows;

            if (resolutionScale != 1) {
                vImage_AffineTransform scaleTransform = { 1 / resolutionScale, 0, 0, 1 / resolutionScale, 0, 0 }; // FIXME: Add subpixel translation.
                Pixel_8888 backgroundColor;
                vImageAffineWarp_ARGB8888(&src, &dst, 0, &scaleTransform, backgroundColor, kvImageEdgeExtend);
                // The unpremultiplying will be done in-place.
                src = dst;
            }

            vImageUnpremultiplyData_RGBA8888(&src, &dst, kvImageNoFlags);
            return result.release();
        }
#endif
        if (resolutionScale != 1) {
            RetainPtr<CGContextRef> sourceContext = adoptCF(CGBitmapContextCreate(srcRows, width.unsafeGet(), height.unsafeGet(), 8, srcBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            RetainPtr<CGImageRef> sourceImage = adoptCF(CGBitmapContextCreateImage(sourceContext.get()));
            RetainPtr<CGContextRef> destinationContext = adoptCF(CGBitmapContextCreate(destRows, destw, desth, 8, destBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            CGContextSetBlendMode(destinationContext.get(), kCGBlendModeCopy);
            CGContextDrawImage(destinationContext.get(), CGRectMake(0, 0, width.unsafeGet() / resolutionScale, height.unsafeGet() / resolutionScale), sourceImage.get()); // FIXME: Add subpixel translation.
            if (!unmultiplied)
                return result.release();

            srcRows = destRows;
            srcBytesPerRow = destBytesPerRow;
            width = destw;
            height = desth;
        }
        if (unmultiplied) {
            if ((width * 4).hasOverflowed())
                CRASH();
            for (int y = 0; y < height.unsafeGet(); ++y) {
                for (int x = 0; x < width.unsafeGet(); x++) {
                    int basex = x * 4;
                    unsigned char alpha = srcRows[basex + 3];
                    if (alpha) {
                        destRows[basex] = (srcRows[basex] * 255) / alpha;
                        destRows[basex + 1] = (srcRows[basex + 1] * 255) / alpha;
                        destRows[basex + 2] = (srcRows[basex + 2] * 255) / alpha;
                        destRows[basex + 3] = alpha;
                    } else
                        reinterpret_cast<uint32_t*>(destRows + basex)[0] = reinterpret_cast<uint32_t*>(srcRows + basex)[0];
                }
                srcRows += srcBytesPerRow;
                destRows += destBytesPerRow;
            }
        } else {
            for (int y = 0; y < height.unsafeGet(); ++y) {
                for (int x = 0; x < (width * 4).unsafeGet(); x += 4)
                    reinterpret_cast<uint32_t*>(destRows + x)[0] = reinterpret_cast<uint32_t*>(srcRows + x)[0];
                srcRows += srcBytesPerRow;
                destRows += destBytesPerRow;
            }
        }
    } else {
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
        IOSurfaceRef surface = m_surface.get();
        IOSurfaceLock(surface, kIOSurfaceLockReadOnly, 0);
        srcBytesPerRow = IOSurfaceGetBytesPerRow(surface);
        srcRows = (unsigned char*)(IOSurfaceGetBaseAddress(surface)) + originy * srcBytesPerRow + originx * 4;

#if USE(ACCELERATE)
        vImage_Buffer src;
        src.height = height.unsafeGet();
        src.width = width.unsafeGet();
        src.rowBytes = srcBytesPerRow;
        src.data = srcRows;

        vImage_Buffer dest;
        dest.height = desth;
        dest.width = destw;
        dest.rowBytes = destBytesPerRow;
        dest.data = destRows;

        if (resolutionScale != 1) {
            vImage_AffineTransform scaleTransform = { 1 / resolutionScale, 0, 0, 1 / resolutionScale, 0, 0 }; // FIXME: Add subpixel translation.
            Pixel_8888 backgroundColor;
            vImageAffineWarp_ARGB8888(&src, &dest, 0, &scaleTransform, backgroundColor, kvImageEdgeExtend);
            // The unpremultiplying and channel-swapping will be done in-place.
            if (unmultiplied) {
                srcRows = destRows;
                width = destw;
                height = desth;
                srcBytesPerRow = destBytesPerRow;
            } else
                src = dest;
        }

        if (unmultiplied) {
            ScanlineData scanlineData;
            scanlineData.scanlineWidth = width.unsafeGet();
            scanlineData.srcData = srcRows;
            scanlineData.srcRowBytes = srcBytesPerRow;
            scanlineData.destData = destRows;
            scanlineData.destRowBytes = destBytesPerRow;

            dispatch_apply_f(height.unsafeGet(), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), &scanlineData, unpremultitplyScanline);
        } else {
            // Swap pixel channels from BGRA to RGBA.
            const uint8_t map[4] = { 2, 1, 0, 3 };
            vImagePermuteChannels_ARGB8888(&src, &dest, map, kvImageNoFlags);
        }
#else
        if (resolutionScale != 1) {
            RetainPtr<CGContextRef> sourceContext = adoptCF(CGBitmapContextCreate(srcRows, width.unsafeGet(), height.unsafeGet(), 8, srcBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            RetainPtr<CGImageRef> sourceImage = adoptCF(CGBitmapContextCreateImage(sourceContext.get()));
            RetainPtr<CGContextRef> destinationContext = adoptCF(CGBitmapContextCreate(destRows, destw, desth, 8, destBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            CGContextSetBlendMode(destinationContext.get(), kCGBlendModeCopy);
            CGContextDrawImage(destinationContext.get(), CGRectMake(0, 0, width.unsafeGet() / resolutionScale, height.unsafeGet() / resolutionScale), sourceImage.get()); // FIXME: Add subpixel translation.

            srcRows = destRows;
            srcBytesPerRow = destBytesPerRow;
            width = destw;
            height = desth;
        }
        
        if ((width * 4).hasOverflowed())
            CRASH();

        if (unmultiplied) {
            for (int y = 0; y < height.unsafeGet(); ++y) {
                for (int x = 0; x < width.unsafeGet(); x++) {
                    int basex = x * 4;
                    unsigned char b = srcRows[basex];
                    unsigned char alpha = srcRows[basex + 3];
                    if (alpha) {
                        destRows[basex] = (srcRows[basex + 2] * 255) / alpha;
                        destRows[basex + 1] = (srcRows[basex + 1] * 255) / alpha;
                        destRows[basex + 2] = (b * 255) / alpha;
                        destRows[basex + 3] = alpha;
                    } else {
                        destRows[basex] = srcRows[basex + 2];
                        destRows[basex + 1] = srcRows[basex + 1];
                        destRows[basex + 2] = b;
                        destRows[basex + 3] = srcRows[basex + 3];
                    }
                }
                srcRows += srcBytesPerRow;
                destRows += destBytesPerRow;
            }
        } else {
            for (int y = 0; y < height.unsafeGet(); ++y) {
                for (int x = 0; x < width.unsafeGet(); x++) {
                    int basex = x * 4;
                    unsigned char b = srcRows[basex];
                    destRows[basex] = srcRows[basex + 2];
                    destRows[basex + 1] = srcRows[basex + 1];
                    destRows[basex + 2] = b;
                    destRows[basex + 3] = srcRows[basex + 3];
                }
                srcRows += srcBytesPerRow;
                destRows += destBytesPerRow;
            }
        }
#endif // USE(ACCELERATE)
        IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, 0);
#else
        ASSERT_NOT_REACHED();
#endif // USE(IOSURFACE_CANVAS_BACKING_STORE)
    }
    
    return result.release();
}

void ImageBufferData::putData(Uint8ClampedArray*& source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, const IntSize& size, bool accelerateRendering, bool unmultiplied, float resolutionScale)
{
    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);
    
    Checked<int> originx = sourceRect.x();
    Checked<int> destx = (Checked<int>(destPoint.x()) + sourceRect.x());
    destx *= resolutionScale;
    ASSERT(destx.unsafeGet() >= 0);
    ASSERT(destx.unsafeGet() < size.width());
    ASSERT(originx.unsafeGet() >= 0);
    ASSERT(originx.unsafeGet() <= sourceRect.maxX());
    
    Checked<int> endx = (Checked<int>(destPoint.x()) + sourceRect.maxX());
    endx *= resolutionScale;
    ASSERT(endx.unsafeGet() <= size.width());
    
    Checked<int> width = sourceRect.width();
    Checked<int> destw = endx - destx;

    Checked<int> originy = sourceRect.y();
    Checked<int> desty = (Checked<int>(destPoint.y()) + sourceRect.y());
    desty *= resolutionScale;
    ASSERT(desty.unsafeGet() >= 0);
    ASSERT(desty.unsafeGet() < size.height());
    ASSERT(originy.unsafeGet() >= 0);
    ASSERT(originy.unsafeGet() <= sourceRect.maxY());
    
    Checked<int> endy = (Checked<int>(destPoint.y()) + sourceRect.maxY());
    endy *= resolutionScale;
    ASSERT(endy.unsafeGet() <= size.height());

    Checked<int> height = sourceRect.height();
    Checked<int> desth = endy - desty;
    
    if (width <= 0 || height <= 0)
        return;
    
    unsigned srcBytesPerRow = 4 * sourceSize.width();
    unsigned char* srcRows = source->data() + (originy * srcBytesPerRow + originx * 4).unsafeGet();
    unsigned destBytesPerRow;
    unsigned char* destRows;
    
    if (!accelerateRendering) {
        destBytesPerRow = 4 * size.width();
        destRows = reinterpret_cast<unsigned char*>(m_data) + (desty * destBytesPerRow + destx * 4).unsafeGet();
        
#if  USE(ACCELERATE)
        if (haveVImageRoundingErrorFix() && unmultiplied) {
            vImage_Buffer src;
            src.height = height.unsafeGet();
            src.width = width.unsafeGet();
            src.rowBytes = srcBytesPerRow;
            src.data = srcRows;
            
            vImage_Buffer dst;
            dst.height = desth.unsafeGet();
            dst.width = destw.unsafeGet();
            dst.rowBytes = destBytesPerRow;
            dst.data = destRows;

            if (resolutionScale != 1) {
                vImage_AffineTransform scaleTransform = { resolutionScale, 0, 0, resolutionScale, 0, 0 }; // FIXME: Add subpixel translation.
                Pixel_8888 backgroundColor;
                vImageAffineWarp_ARGB8888(&src, &dst, 0, &scaleTransform, backgroundColor, kvImageEdgeExtend);
                // The premultiplying will be done in-place.
                src = dst;
            }

            vImagePremultiplyData_RGBA8888(&src, &dst, kvImageNoFlags);
            return;
        }
#endif
        if (resolutionScale != 1) {
            RetainPtr<CGContextRef> sourceContext = adoptCF(CGBitmapContextCreate(srcRows, width.unsafeGet(), height.unsafeGet(), 8, srcBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            RetainPtr<CGImageRef> sourceImage = adoptCF(CGBitmapContextCreateImage(sourceContext.get()));
            RetainPtr<CGContextRef> destinationContext = adoptCF(CGBitmapContextCreate(destRows, destw.unsafeGet(), desth.unsafeGet(), 8, destBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            CGContextSetBlendMode(destinationContext.get(), kCGBlendModeCopy);
            CGContextDrawImage(destinationContext.get(), CGRectMake(0, 0, width.unsafeGet() / resolutionScale, height.unsafeGet() / resolutionScale), sourceImage.get()); // FIXME: Add subpixel translation.
            if (!unmultiplied)
                return;

            srcRows = destRows;
            srcBytesPerRow = destBytesPerRow;
            width = destw;
            height = desth;
        }

        for (int y = 0; y < height.unsafeGet(); ++y) {
            for (int x = 0; x < width.unsafeGet(); x++) {
                int basex = x * 4;
                unsigned char alpha = srcRows[basex + 3];
                if (unmultiplied && alpha != 255) {
                    destRows[basex] = (srcRows[basex] * alpha + 254) / 255;
                    destRows[basex + 1] = (srcRows[basex + 1] * alpha + 254) / 255;
                    destRows[basex + 2] = (srcRows[basex + 2] * alpha + 254) / 255;
                    destRows[basex + 3] = alpha;
                } else
                    reinterpret_cast<uint32_t*>(destRows + basex)[0] = reinterpret_cast<uint32_t*>(srcRows + basex)[0];
            }
            destRows += destBytesPerRow;
            srcRows += srcBytesPerRow;
        }
    } else {
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
        IOSurfaceRef surface = m_surface.get();
        IOSurfaceLock(surface, 0, 0);
        destBytesPerRow = IOSurfaceGetBytesPerRow(surface);
        destRows = (unsigned char*)(IOSurfaceGetBaseAddress(surface)) + (desty * destBytesPerRow + destx * 4).unsafeGet();

#if USE(ACCELERATE)
        vImage_Buffer src;
        src.height = height.unsafeGet();
        src.width = width.unsafeGet();
        src.rowBytes = srcBytesPerRow;
        src.data = srcRows;

        vImage_Buffer dest;
        dest.height = desth.unsafeGet();
        dest.width = destw.unsafeGet();
        dest.rowBytes = destBytesPerRow;
        dest.data = destRows;

        if (resolutionScale != 1) {
            vImage_AffineTransform scaleTransform = { resolutionScale, 0, 0, resolutionScale, 0, 0 }; // FIXME: Add subpixel translation.
            Pixel_8888 backgroundColor;
            vImageAffineWarp_ARGB8888(&src, &dest, 0, &scaleTransform, backgroundColor, kvImageEdgeExtend);
            // The unpremultiplying and channel-swapping will be done in-place.
            if (unmultiplied) {
                srcRows = destRows;
                width = destw;
                height = desth;
                srcBytesPerRow = destBytesPerRow;
            } else
                src = dest;
        }

        if (unmultiplied) {
            ScanlineData scanlineData;
            scanlineData.scanlineWidth = width.unsafeGet();
            scanlineData.srcData = srcRows;
            scanlineData.srcRowBytes = srcBytesPerRow;
            scanlineData.destData = destRows;
            scanlineData.destRowBytes = destBytesPerRow;

            dispatch_apply_f(height.unsafeGet(), dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), &scanlineData, premultitplyScanline);
        } else {
            // Swap pixel channels from RGBA to BGRA.
            const uint8_t map[4] = { 2, 1, 0, 3 };
            vImagePermuteChannels_ARGB8888(&src, &dest, map, kvImageNoFlags);
        }
#else
        if (resolutionScale != 1) {
            RetainPtr<CGContextRef> sourceContext = adoptCF(CGBitmapContextCreate(srcRows, width.unsafeGet(), height.unsafeGet(), 8, srcBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            RetainPtr<CGImageRef> sourceImage = adoptCF(CGBitmapContextCreateImage(sourceContext.get()));
            RetainPtr<CGContextRef> destinationContext = adoptCF(CGBitmapContextCreate(destRows, destw.unsafeGet(), desth.unsafeGet(), 8, destBytesPerRow, m_colorSpace, kCGImageAlphaPremultipliedLast));
            CGContextSetBlendMode(destinationContext.get(), kCGBlendModeCopy);
            CGContextDrawImage(destinationContext.get(), CGRectMake(0, 0, width.unsafeGet() / resolutionScale, height.unsafeGet() / resolutionScale), sourceImage.get()); // FIXME: Add subpixel translation.

            srcRows = destRows;
            srcBytesPerRow = destBytesPerRow;
            width = destw;
            height = desth;
        }

        for (int y = 0; y < height.unsafeGet(); ++y) {
            for (int x = 0; x < width.unsafeGet(); x++) {
                int basex = x * 4;
                unsigned char b = srcRows[basex];
                unsigned char alpha = srcRows[basex + 3];
                if (unmultiplied && alpha != 255) {
                    destRows[basex] = (srcRows[basex + 2] * alpha + 254) / 255;
                    destRows[basex + 1] = (srcRows[basex + 1] * alpha + 254) / 255;
                    destRows[basex + 2] = (b * alpha + 254) / 255;
                    destRows[basex + 3] = alpha;
                } else {
                    destRows[basex] = srcRows[basex + 2];
                    destRows[basex + 1] = srcRows[basex + 1];
                    destRows[basex + 2] = b;
                    destRows[basex + 3] = alpha;
                }
            }
            destRows += destBytesPerRow;
            srcRows += srcBytesPerRow;
        }
#endif // USE(ACCELERATE)

        IOSurfaceUnlock(surface, 0, 0);
#else
        ASSERT_NOT_REACHED();
#endif // USE(IOSURFACE_CANVAS_BACKING_STORE)
    }
}

} // namespace WebCore
