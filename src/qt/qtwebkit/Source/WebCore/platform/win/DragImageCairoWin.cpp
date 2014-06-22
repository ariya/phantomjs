/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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
#include "DragImage.h"

#include "BitmapInfo.h"
#include "CachedImage.h"
#include "GraphicsContext.h"
#include "GraphicsContextPlatformPrivateCairo.h"
#include "Image.h"
#include <wtf/RetainPtr.h>
#include <cairo-win32.h>
#include <windows.h>

namespace WebCore {

void deallocContext(PlatformContextCairo* target)
{
    delete target;
}

HBITMAP allocImage(HDC dc, IntSize size, PlatformContextCairo** targetRef)
{
    BitmapInfo bmpInfo = BitmapInfo::create(size);

    LPVOID bits;
    HBITMAP hbmp = CreateDIBSection(dc, &bmpInfo, DIB_RGB_COLORS, &bits, 0, 0);

    // At this point, we have a Cairo surface that points to a Windows DIB.  The DIB interprets
    // with the opposite meaning of positive Y axis, so everything we draw into this cairo
    // context is going to be upside down.
    if (!targetRef)
        return hbmp;

    cairo_surface_t* bitmapContext = cairo_image_surface_create_for_data((unsigned char*)bits,
                                               CAIRO_FORMAT_ARGB32,
                                               bmpInfo.bmiHeader.biWidth,
                                               bmpInfo.bmiHeader.biHeight,
                                               bmpInfo.bmiHeader.biWidth * 4);

    if (!bitmapContext) {
        DeleteObject(hbmp);
        return 0;
    }

    cairo_t* cr = cairo_create(bitmapContext);
    cairo_surface_destroy(bitmapContext);

    // At this point, we have a Cairo surface that points to a Windows DIB.  The DIB interprets
    // with the opposite meaning of positive Y axis, so everything we draw into this cairo
    // context is going to be upside down.
    //
    // So, we must invert the CTM for the context so that drawing commands will be flipped
    // before they get written to the internal buffer.
    cairo_matrix_t matrix;
    cairo_matrix_init(&matrix, 1.0, 0.0, 0.0, -1.0, 0.0, size.height());
    cairo_set_matrix(cr, &matrix);

    *targetRef = new PlatformGraphicsContext(cr);
    cairo_destroy(cr);

    return hbmp;
}

static cairo_surface_t* createCairoContextFromBitmap(HBITMAP bitmap)
{
    BITMAP info;
    GetObject(bitmap, sizeof(info), &info);
    ASSERT(info.bmBitsPixel == 32);

    // At this point, we have a Cairo surface that points to a Windows BITMAP.  The BITMAP
    // has the opposite meaning of positive Y axis, so everything we draw into this cairo
    // context is going to be upside down.
    return cairo_image_surface_create_for_data((unsigned char*)info.bmBits,
                                               CAIRO_FORMAT_ARGB32,
                                               info.bmWidth,
                                               info.bmHeight,
                                               info.bmWidthBytes);
}

DragImageRef scaleDragImage(DragImageRef image, FloatSize scale)
{
    // FIXME: due to the way drag images are done on windows we need 
    // to preprocess the alpha channel <rdar://problem/5015946>
    if (!image)
        return 0;

    IntSize srcSize = dragImageSize(image);
    IntSize dstSize(static_cast<int>(srcSize.width() * scale.width()), static_cast<int>(srcSize.height() * scale.height()));

    HBITMAP hbmp = 0;
    HDC dc = GetDC(0);
    HDC dstDC = CreateCompatibleDC(dc);

    if (!dstDC)
        goto exit;

    PlatformContextCairo* targetContext;
    hbmp = allocImage(dstDC, dstSize, &targetContext);
    if (!hbmp)
        goto exit;

    cairo_surface_t* srcImage = createCairoContextFromBitmap(image);

    // Scale the target surface to the new image size, and flip it
    // so that when we set the srcImage as the surface it will draw
    // right-side-up.
    cairo_t* cr = targetContext->cr();
    cairo_translate(cr, 0, dstSize.height());
    cairo_scale(cr, scale.width(), -scale.height());
    cairo_set_source_surface(cr, srcImage, 0.0, 0.0);

    // Now we can paint and get the correct result
    cairo_paint(cr);

    cairo_surface_destroy(srcImage);
    deallocContext(targetContext);
    ::DeleteObject(image);
    image = 0;

exit:
    if (!hbmp)
        hbmp = image;
    if (dstDC)
        DeleteDC(dstDC);
    ReleaseDC(0, dc);
    return hbmp;
}
    
DragImageRef createDragImageFromImage(Image* img, RespectImageOrientationEnum)
{
    HBITMAP hbmp = 0;
    HDC dc = GetDC(0);
    HDC workingDC = CreateCompatibleDC(dc);
    if (!workingDC)
        goto exit;

    PlatformContextCairo* drawContext = 0;
    hbmp = allocImage(workingDC, img->size(), &drawContext);
    if (!hbmp)
        goto exit;

    if (!drawContext) {
        ::DeleteObject(hbmp);
        hbmp = 0;
    }

    { // This block is required due to the msvc compiler error C2362.
        cairo_t* cr = drawContext->cr();
        cairo_set_source_rgb(cr, 1.0, 0.0, 1.0);
        cairo_fill_preserve(cr);

        RefPtr<cairo_surface_t> surface = img->nativeImageForCurrentFrame();
        if (surface) {
            // Draw the image.
            cairo_set_source_surface(cr, surface.get(), 0.0, 0.0);
            cairo_paint(cr);
        }
    }

    deallocContext(drawContext);

exit:
    if (workingDC)
        DeleteDC(workingDC);
    ReleaseDC(0, dc);
    return hbmp;
}
    
}
