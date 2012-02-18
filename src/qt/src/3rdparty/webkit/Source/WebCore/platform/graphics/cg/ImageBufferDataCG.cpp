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

#include <wtf/Assertions.h>

#if USE(ACCELERATE)
#include <Accelerate/Accelerate.h>
#endif

#if USE(IOSURFACE_CANVAS_BACKING_STORE)
#include <IOSurface/IOSurface.h>
#include <dispatch/dispatch.h>
#endif

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
// The vImage unpremultiply routine had a rounding bug before 10.6.7 <rdar://problem/8631548>
static bool haveVImageRoundingErrorFix()
{
    SInt32 version;
    static bool result = (Gestalt(gestaltSystemVersion, &version) == noErr && version > 0x1066);
    return result;
}

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

PassRefPtr<ByteArray> ImageBufferData::getData(const IntRect& rect, const IntSize& size, bool accelerateRendering, bool unmultiplied) const
{
    float area = 4.0f * rect.width() * rect.height();
    if (area > static_cast<float>(std::numeric_limits<int>::max()))
        return 0;

    RefPtr<ByteArray> result = ByteArray::create(rect.width() * rect.height() * 4);
    unsigned char* data = result->data();
    
    if (rect.x() < 0 || rect.y() < 0 || rect.maxX() > size.width() || rect.maxY() > size.height())
        memset(data, 0, result->length());
    
    int originx = rect.x();
    int destx = 0;
    if (originx < 0) {
        destx = -originx;
        originx = 0;
    }
    int endx = rect.maxX();
    if (endx > size.width())
        endx = size.width();
    int width = endx - originx;
    
    int originy = rect.y();
    int desty = 0;
    if (originy < 0) {
        desty = -originy;
        originy = 0;
    }
    int endy = rect.maxY();
    if (endy > size.height())
        endy = size.height();
    int height = endy - originy;
    
    if (width <= 0 || height <= 0)
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
            src.height = height;
            src.width = width;
            src.rowBytes = srcBytesPerRow;
            src.data = srcRows;
            
            vImage_Buffer dst;
            dst.height = height;
            dst.width = width;
            dst.rowBytes = destBytesPerRow;
            dst.data = destRows;
            
            vImageUnpremultiplyData_RGBA8888(&src, &dst, kvImageNoFlags);
            return result.release();
        }
#endif
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; x++) {
                int basex = x * 4;
                unsigned char alpha = srcRows[basex + 3];
                if (unmultiplied && alpha) {
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
#if USE(IOSURFACE_CANVAS_BACKING_STORE)
        IOSurfaceRef surface = m_surface.get();
        IOSurfaceLock(surface, kIOSurfaceLockReadOnly, 0);
        srcBytesPerRow = IOSurfaceGetBytesPerRow(surface);
        srcRows = (unsigned char*)(IOSurfaceGetBaseAddress(surface)) + originy * srcBytesPerRow + originx * 4;

#if USE(ACCELERATE)
        if (unmultiplied) {
            ScanlineData scanlineData;
            scanlineData.scanlineWidth = width;
            scanlineData.srcData = srcRows;
            scanlineData.srcRowBytes = srcBytesPerRow;
            scanlineData.destData = destRows;
            scanlineData.destRowBytes = destBytesPerRow;

            dispatch_apply_f(height, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), &scanlineData, unpremultitplyScanline);
        } else {
            vImage_Buffer src;
            src.height = height;
            src.width = width;
            src.rowBytes = srcBytesPerRow;
            src.data = srcRows;

            vImage_Buffer dest;
            dest.height = height;
            dest.width = width;
            dest.rowBytes = destBytesPerRow;
            dest.data = destRows;

            // Swap pixel channels from BGRA to RGBA.
            const uint8_t map[4] = { 2, 1, 0, 3 };
            vImagePermuteChannels_ARGB8888(&src, &dest, map, kvImageNoFlags);
        }
#else
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; x++) {
                int basex = x * 4;
                unsigned char alpha = srcRows[basex + 3];
                if (unmultiplied && alpha) {
                    destRows[basex] = (srcRows[basex + 2] * 255) / alpha;
                    destRows[basex + 1] = (srcRows[basex + 1] * 255) / alpha;
                    destRows[basex + 2] = (srcRows[basex] * 255) / alpha;
                    destRows[basex + 3] = alpha;
                } else {
                    destRows[basex] = srcRows[basex + 2];
                    destRows[basex + 1] = srcRows[basex + 1];
                    destRows[basex + 2] = srcRows[basex];
                    destRows[basex + 3] = alpha;
                }
            }
            srcRows += srcBytesPerRow;
            destRows += destBytesPerRow;
        }
#endif // USE(ACCELERATE)
        IOSurfaceUnlock(surface, kIOSurfaceLockReadOnly, 0);
#else
        ASSERT_NOT_REACHED();
#endif // USE(IOSURFACE_CANVAS_BACKING_STORE)
    }
    
    return result.release();
}

void ImageBufferData::putData(ByteArray*& source, const IntSize& sourceSize, const IntRect& sourceRect, const IntPoint& destPoint, const IntSize& size, bool accelerateRendering, bool unmultiplied)
{
    ASSERT(sourceRect.width() > 0);
    ASSERT(sourceRect.height() > 0);
    
    int originx = sourceRect.x();
    int destx = destPoint.x() + sourceRect.x();
    ASSERT(destx >= 0);
    ASSERT(destx < size.width());
    ASSERT(originx >= 0);
    ASSERT(originx <= sourceRect.maxX());
    
    int endx = destPoint.x() + sourceRect.maxX();
    ASSERT(endx <= size.width());
    
    int width = endx - destx;
    
    int originy = sourceRect.y();
    int desty = destPoint.y() + sourceRect.y();
    ASSERT(desty >= 0);
    ASSERT(desty < size.height());
    ASSERT(originy >= 0);
    ASSERT(originy <= sourceRect.maxY());
    
    int endy = destPoint.y() + sourceRect.maxY();
    ASSERT(endy <= size.height());
    int height = endy - desty;
    
    if (width <= 0 || height <= 0)
        return;
    
    unsigned srcBytesPerRow = 4 * sourceSize.width();
    unsigned char* srcRows = source->data() + originy * srcBytesPerRow + originx * 4;
    unsigned destBytesPerRow;
    unsigned char* destRows;
    
    if (!accelerateRendering) {
        destBytesPerRow = 4 * size.width();
        destRows = reinterpret_cast<unsigned char*>(m_data) + desty * destBytesPerRow + destx * 4;
        
#if USE(ACCELERATE)
        if (haveVImageRoundingErrorFix() && unmultiplied) {
            vImage_Buffer src;
            src.height = height;
            src.width = width;
            src.rowBytes = srcBytesPerRow;
            src.data = srcRows;
            
            vImage_Buffer dst;
            dst.height = height;
            dst.width = width;
            dst.rowBytes = destBytesPerRow;
            dst.data = destRows;
            
            vImagePremultiplyData_RGBA8888(&src, &dst, kvImageNoFlags);
            return;
        }
#endif
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; x++) {
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
        destRows = (unsigned char*)(IOSurfaceGetBaseAddress(surface)) + desty * destBytesPerRow + destx * 4;

#if USE(ACCELERATE)
        if (unmultiplied) {
            ScanlineData scanlineData;
            scanlineData.scanlineWidth = width;
            scanlineData.srcData = srcRows;
            scanlineData.srcRowBytes = srcBytesPerRow;
            scanlineData.destData = destRows;
            scanlineData.destRowBytes = destBytesPerRow;

            dispatch_apply_f(height, dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0), &scanlineData, premultitplyScanline);
        } else {
            vImage_Buffer src;
            src.height = height;
            src.width = width;
            src.rowBytes = srcBytesPerRow;
            src.data = srcRows;

            vImage_Buffer dest;
            dest.height = height;
            dest.width = width;
            dest.rowBytes = destBytesPerRow;
            dest.data = destRows;

            // Swap pixel channels from RGBA to BGRA.
            const uint8_t map[4] = { 2, 1, 0, 3 };
            vImagePermuteChannels_ARGB8888(&src, &dest, map, kvImageNoFlags);
        }
#else
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; x++) {
                int basex = x * 4;
                unsigned char alpha = srcRows[basex + 3];
                if (unmultiplied && alpha != 255) {
                    destRows[basex] = (srcRows[basex + 2] * alpha + 254) / 255;
                    destRows[basex + 1] = (srcRows[basex + 1] * alpha + 254) / 255;
                    destRows[basex + 2] = (srcRows[basex] * alpha + 254) / 255;
                    destRows[basex + 3] = alpha;
                } else {
                    destRows[basex] = srcRows[basex + 2];
                    destRows[basex + 1] = srcRows[basex + 1];
                    destRows[basex + 2] = srcRows[basex];
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
