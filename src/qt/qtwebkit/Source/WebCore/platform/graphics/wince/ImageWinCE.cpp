/*
 * Copyright (C) 2006, 2007 Apple Inc.  All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile Inc.
 * Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "Image.h"

#include "BitmapImage.h"
#include "GraphicsContext.h"
#include "ImageDecoder.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"
#include "TransformationMatrix.h"
#include "WinceGraphicsExtras.h"
#include <wtf/OwnPtr.h>
#include <wtf/text/WTFString.h>

#include <windows.h>

namespace WebCore {

PassNativeImagePtr ImageFrame::asNewNativeImage() const
{
    return SharedBitmap::create(m_backingStore, m_size, hasAlpha());
}

bool FrameData::clear(bool clearMetaData)
{
    if (clearMetaData)
        m_haveMetadata = false;

    if (m_frame) {
        m_frame = 0;
        return true;
    }

    return false;
}

bool BitmapImage::getHBITMAPOfSize(HBITMAP bmp, LPSIZE size)
{
    if (!bmp)
        return false;

    BITMAP bmpInfo;
    GetObject(bmp, sizeof(BITMAP), &bmpInfo);

    ASSERT(bmpInfo.bmBitsPixel == 32);
    int bufferSize = bmpInfo.bmWidthBytes * bmpInfo.bmHeight;

    OwnPtr<HDC> hdc = adoptPtr(CreateCompatibleDC(0));
    HGDIOBJ hOldBmp = SelectObject(hdc.get(), bmp);

    {
        GraphicsContext gc(hdc.get());

        IntSize imageSize = BitmapImage::size();
        if (size)
            drawFrameMatchingSourceSize(&gc, FloatRect(0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight), IntSize(*size), ColorSpaceDeviceRGB, CompositeCopy);
        else
            draw(&gc, FloatRect(0, 0, bmpInfo.bmWidth, bmpInfo.bmHeight), FloatRect(0, 0, imageSize.width(), imageSize.height()), ColorSpaceDeviceRGB, CompositeCopy, BlendModeNormal);
    }

    SelectObject(hdc.get(), hOldBmp);

    return true;
}

void BitmapImage::drawFrameMatchingSourceSize(GraphicsContext* ctxt, const FloatRect& dstRect, const IntSize& srcSize, ColorSpace styleColorSpace, CompositeOperator compositeOp)
{
    int frames = frameCount();
    for (int i = 0; i < frames; ++i) {
        RefPtr<SharedBitmap> bmp = frameAtIndex(i);
        if (!bmp || bmp->height() != static_cast<unsigned>(srcSize.height()) || bmp->width() != static_cast<unsigned>(srcSize.width()))
            continue;

        size_t currentFrame = m_currentFrame;
        m_currentFrame = i;
        draw(ctxt, dstRect, FloatRect(0, 0, srcSize.width(), srcSize.height()), styleColorSpace, compositeOp, BlendModeNormal);
        m_currentFrame = currentFrame;
        return;
    }

    // No image of the correct size was found, fallback to drawing the current frame
    IntSize imageSize = BitmapImage::size();
    draw(ctxt, dstRect, FloatRect(0, 0, imageSize.width(), imageSize.height()), styleColorSpace, compositeOp, BlendModeNormal);
}

void BitmapImage::draw(GraphicsContext* ctxt, const FloatRect& dstRect, const FloatRect& srcRectIn, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode)
{
    if (!m_source.initialized())
        return;

    if (mayFillWithSolidColor())
        fillWithSolidColor(ctxt, dstRect, solidColor(), styleColorSpace, compositeOp);
    else {
        IntRect intSrcRect(srcRectIn);
        RefPtr<SharedBitmap> bmp = frameAtIndex(m_currentFrame);
        if (bmp) {
            if (bmp->width() != m_source.size().width()) {
                double scaleFactor = static_cast<double>(bmp->width()) / m_source.size().width();

                intSrcRect.setX(stableRound(srcRectIn.x() * scaleFactor));
                intSrcRect.setWidth(stableRound(srcRectIn.width() * scaleFactor));
                intSrcRect.setY(stableRound(srcRectIn.y() * scaleFactor));
                intSrcRect.setHeight(stableRound(srcRectIn.height() * scaleFactor));
            }
            bmp->draw(ctxt, enclosingIntRect(dstRect), intSrcRect, styleColorSpace, compositeOp, blendMode);
        }
    }

    startAnimation();
}

void Image::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, BlendMode blendMode)
{
    notImplemented();
}

void BitmapImage::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                        const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect)
{
    RefPtr<SharedBitmap> bmp = nativeImageForCurrentFrame();
    if (!bmp)
        return;

    bmp->drawPattern(ctxt, tileRectIn, patternTransform, phase, styleColorSpace, op, destRect, m_source.size());
}

void BitmapImage::checkForSolidColor()
{
    if (m_checkedForSolidColor)
        return;

    if (frameCount() != 1) {
        m_isSolidColor = false;
        m_checkedForSolidColor = true;
        return;
    }

    RefPtr<SharedBitmap> bmp = frameAtIndex(0);
    if (!bmp || !bmp->validHeight()) {
        m_isSolidColor = false;
        return;
    }

    if (bmp->width() != 1 || bmp->validHeight() != 1) {
        m_isSolidColor = false;
        m_checkedForSolidColor = true;
        return;
    }

    m_isSolidColor = true;

    if (bmp->is16bit()) {
        unsigned short c = ((unsigned short *)bmp->bytes())[0];
        int r = (c >> 7) & 0xF8;
        int g = (c >> 2) & 0xF8;
        int b = (c << 3) & 0xF8;
        if (bmp->usesTransparentColor() && bmp->transparentColor() == RGB(r, g, b))
            m_solidColor = Color(r, g, b, 0);
        else
            m_solidColor = Color(r, g, b);
    } else {
        unsigned c = ((unsigned *)bmp->bytes())[0];
        m_solidColor = Color(c);
    }

    if (bmp->validHeight() == bmp->height())
        m_checkedForSolidColor = true;
}

} // namespace WebCore
