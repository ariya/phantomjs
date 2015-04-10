/*
 *  Copyright (C) 2007-2009 Torch Mobile, Inc. All rights reserved.
 *  Copyright (C) 2010 Patrick Gansterer <paroga@paroga.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "config.h"
#include "SharedBitmap.h"

#include "GDIExtras.h"
#include "GraphicsContext.h"
#include "GraphicsTypes.h"
#include "TransformationMatrix.h"
#include "WinCEGraphicsExtras.h"
#include <wtf/HashSet.h>
#include <wtf/RefCountedLeakCounter.h>
#include <wtf/PassOwnArrayPtr.h>
#include <wtf/OwnPtr.h>

#include <windows.h>

namespace WebCore {

#ifndef NDEBUG
static WTF::RefCountedLeakCounter sharedBitmapLeakCounter("SharedBitmap");
#endif


PassRefPtr<SharedBitmap> SharedBitmap::create(const IntSize& size, BitmapInfo::BitCount bitCount, bool initPixels)
{
    RefPtr<SharedBitmap> resultantBitmap = adoptRef(new SharedBitmap(size, bitCount, initPixels));
    if (resultantBitmap && !resultantBitmap->bytes())
        return 0;
    return resultantBitmap.release();
}

PassRefPtr<SharedBitmap> SharedBitmap::create(const Vector<unsigned>& data, const IntSize& size, bool hasAlpha)
{
    RefPtr<SharedBitmap> result = create(size, BitmapInfo::BitCount32, false);
    if (!result)
        return 0;
    memcpy(result->bytes(), data.data(), data.size() * sizeof(unsigned));
    result->setHasAlpha(hasAlpha);
    return result.release();
}

SharedBitmap::SharedBitmap(const IntSize& size, BitmapInfo::BitCount bitCount, bool initPixels)
    : m_bmpInfo(BitmapInfo::createBottomUp(size, bitCount))
    , m_locked(false)
    , m_usesTransparentColor(false)
    , m_transparentColor(RGB(0, 0, 0))
    , m_pixels(0)
    , m_hasAlpha(false)
    , m_validHeight(abs(size.height()))
{
#ifndef NDEBUG
    sharedBitmapLeakCounter.increment();
#endif

    unsigned bufferSize = m_bmpInfo.numPixels();
    if (bitCount == BitmapInfo::BitCount16)
        bufferSize /= 2;

    m_pixelData = adoptArrayPtr(new unsigned[bufferSize]);
    m_pixels = m_pixelData.get();

    if (initPixels)
        resetPixels();
}

SharedBitmap::~SharedBitmap()
{
#ifndef NDEBUG
    sharedBitmapLeakCounter.decrement();
#endif
}

void SharedBitmap::resetPixels(bool black)
{
    if (!m_pixels)
        return;

    unsigned bufferSize = m_bmpInfo.numPixels();
    if (black) {
        unsigned bufferSizeInBytes = bufferSize * (is16bit() ? 2 : 4);
        memset(m_pixels, 0, bufferSizeInBytes);
        return;
    }

    if (is16bit()) {
        // Fill it with white color
        wmemset(static_cast<wchar_t*>(m_pixels), 0xFFFF, bufferSize);
        return;
    }

    // Make it white but transparent
    unsigned* pixel = static_cast<unsigned*>(m_pixels);
    const unsigned* bufferEnd = pixel + bufferSize;
    while (pixel < bufferEnd)
        *pixel++ = 0x00FFFFFF;
}

static inline unsigned short convert32To16(unsigned pixel)
{
    unsigned short r = static_cast<unsigned short>((pixel & 0x00F80000) >> 8);
    unsigned short g = static_cast<unsigned short>((pixel & 0x0000FC00) >> 5);
    unsigned short b = static_cast<unsigned short>((pixel & 0x000000F8) >> 3);
    return r | g | b;
}

bool SharedBitmap::to16bit()
{
    if (m_locked)
        return false;
    if (is16bit())
        return true;

    BitmapInfo newBmpInfo = BitmapInfo::create(m_bmpInfo.size(), BitmapInfo::BitCount16);

    int width = newBmpInfo.width();
    int paddedWidth = newBmpInfo.paddedWidth();
    int bufferSize = paddedWidth * newBmpInfo.height();
    OwnArrayPtr<unsigned> newPixelData = adoptArrayPtr(new unsigned[bufferSize / 2]);
    void* newPixels = newPixelData.get();

    if (!newPixels)
        return false;

    unsigned short* p16 = static_cast<unsigned short*>(newPixels);
    const unsigned* p32 = static_cast<const unsigned*>(m_pixels);

    bool skips = paddedWidth != width;

    const unsigned short* p16end = p16 + bufferSize;
    while (p16 < p16end) {
        for (unsigned short* p16lineEnd = p16 + width; p16 < p16lineEnd; )
            *p16++ = convert32To16(*p32++);

        if (skips)
            *p16++ = 0;
    }

    if (m_hbitmap)
        m_hbitmap = nullptr;
    else
        m_pixelData = newPixelData.release();

    m_pixels = newPixels;
    m_bmpInfo = newBmpInfo;

    setHasAlpha(false);
    return true;
}

bool SharedBitmap::freeMemory()
{
    if (m_locked)
        return false;

    if (m_hbitmap) {
        m_hbitmap = nullptr;
        m_pixels = 0;
        return true;
    }

    if (m_pixels) {
        m_pixelData = nullptr;
        m_pixels = 0;
        return true;
    }

    return false;
}

PassOwnPtr<HBITMAP> SharedBitmap::createHandle(void** pixels, BitmapInfo* bmpInfo, int height, bool use16bit) const
{
    if (!m_pixels)
        return nullptr;

    if (height == -1)
        height = this->height();
    *bmpInfo = BitmapInfo::createBottomUp(IntSize(width(), height), (use16bit || is16bit()) ? BitmapInfo::BitCount16 : BitmapInfo::BitCount32);

    OwnPtr<HBITMAP> hbmp = adoptPtr(CreateDIBSection(0, bmpInfo, DIB_RGB_COLORS, pixels, 0, 0));

    if (!hbmp)
        return nullptr;

    OwnPtr<HDC> bmpDC = adoptPtr(CreateCompatibleDC(0));
    HGDIOBJ hOldBmp = SelectObject(bmpDC.get(), hbmp.get());

    StretchDIBits(bmpDC.get(), 0, 0, width(), height, 0, 0, width(), height, m_pixels, &m_bmpInfo, DIB_RGB_COLORS, SRCCOPY);

    SelectObject(bmpDC.get(), hOldBmp);

    return hbmp.release();
}

bool SharedBitmap::ensureHandle()
{
    if (m_hbitmap)
        return true;

    if (!m_pixels)
        return false;

    if (m_locked)
        return false;

    BitmapInfo bmpInfo;
    void* pixels;
    m_hbitmap = createHandle(&pixels, &bmpInfo, -1, !hasAlpha());

    if (!m_hbitmap)
        return false;

    m_pixelData = nullptr;
    m_pixels = pixels;
    m_bmpInfo = bmpInfo;

    return true;
}

void SharedBitmap::draw(GraphicsContext* ctxt, const IntRect& dstRect, const IntRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode)
{
    if (!m_pixels)
        return;
    ctxt->drawBitmap(this, dstRect, srcRect, styleColorSpace, compositeOp, blendMode);
}

void SharedBitmap::draw(HDC hdc, const IntRect& dstRect, const IntRect& srcRect, CompositeOperator compositeOp, BlendMode blendMode)
{
    if (!m_pixels)
        return;

    if (dstRect.isEmpty() || srcRect.isEmpty())
        return;

    HBITMAP hbitmap = 0;
    OwnPtr<HBITMAP> hTempBitmap;
    bool usingHandle = compositeOp == CompositeSourceOver && (hasAlpha() && hasAlphaBlendSupport() || usesTransparentColor());

    if (usingHandle) {
        if (ensureHandle())
            hbitmap = m_hbitmap.get();
        else {
            void* pixels;
            BitmapInfo bmpInfo;
            hTempBitmap = createHandle(&pixels, &bmpInfo, -1, usesTransparentColor());
            hbitmap = hTempBitmap.get();
        }
    }
    if (!hbitmap) {
        // FIXME: handle other composite operation types?
        DWORD rop = compositeOp == CompositeCopy ? SRCCOPY
            : compositeOp == CompositeXOR ? PATINVERT
            : compositeOp == CompositeClear ? WHITENESS
            : SRCCOPY;

        StretchDIBits(hdc, dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(),
            srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), m_pixels, &m_bmpInfo, DIB_RGB_COLORS, rop);
        return;
    }

    OwnPtr<HDC> hmemdc = adoptPtr(CreateCompatibleDC(hdc));
    HGDIOBJ hOldBmp = SelectObject(hmemdc.get(), hbitmap);

    if (!usesTransparentColor() && hasAlphaBlendSupport()) {
        static const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        bool success = alphaBlendIfSupported(hdc, dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(), hmemdc.get(),
            srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), blend);
        ASSERT_UNUSED(success, success);
    } else {
        TransparentBlt(hdc, dstRect.x(), dstRect.y(), dstRect.width(), dstRect.height(), hmemdc.get(),
            srcRect.x(), srcRect.y(), srcRect.width(), srcRect.height(), transparentColor());
    }

    SelectObject(hmemdc.get(), hOldBmp);
}

PassOwnPtr<HBITMAP> SharedBitmap::clipBitmap(const IntRect& rect, bool useAlpha, BitmapInfo& bmpInfo, void*& pixels)
{
    if (!bytes())
        return nullptr;

    int oldWidth = width();
    int oldHeight = height();
    int copyWidth = std::min<int>(rect.width(), oldWidth - rect.x());
    int copyHeight = std::min<int>(rect.height(), oldHeight - rect.y());
    if (!copyWidth || !copyHeight)
        return nullptr;

    bmpInfo = BitmapInfo::createBottomUp(IntSize(copyWidth, copyHeight), (useAlpha && is32bit()) ? BitmapInfo::BitCount32 : BitmapInfo::BitCount16);
    OwnPtr<HBITMAP> newBmp = adoptPtr(CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, &pixels, 0, 0));

    if (!newBmp)
        return nullptr;

    OwnPtr<HDC> dcNew = adoptPtr(CreateCompatibleDC(0));
    HGDIOBJ tmpNew = SelectObject(dcNew.get(), newBmp.get());

    StretchDIBits(dcNew.get(), 0, 0, copyWidth, copyHeight, rect.x(), rect.y(), copyWidth, copyHeight,
        bytes(), &bitmapInfo(), DIB_RGB_COLORS, SRCCOPY);

    SelectObject(dcNew.get(), tmpNew);
    return newBmp.release();
}

PassRefPtr<SharedBitmap> SharedBitmap::clipBitmap(const IntRect& rect, bool useAlpha)
{
    int oldWidth = width();
    int oldHeight = height();
    int copyWidth = std::min<int>(rect.width(), oldWidth - rect.x());
    int copyHeight = std::min<int>(rect.height(), oldHeight - rect.y());
    if (!copyWidth || !copyHeight)
        return 0;

    RefPtr<SharedBitmap> newBmp = create(IntSize(copyWidth, copyHeight), useAlpha && is32bit() ? BitmapInfo::BitCount32 : BitmapInfo::BitCount16, false);

    if (!newBmp || !newBmp->bytes())
        return 0;

    DCHolder dcNew(newBmp.get());

    StretchDIBits(dcNew.get(), 0, 0, copyWidth, copyHeight, rect.x(), rect.y(), copyWidth, copyHeight,
        bytes(), &bitmapInfo(), DIB_RGB_COLORS, SRCCOPY);

    return newBmp;
}

static void drawPatternSimple(HDC hdc, const RECT& destRect, HBITMAP hbmp, const POINT& phase)
{
    OwnPtr<HBRUSH> hBrush = adoptPtr(CreatePatternBrush(hbmp));
    if (!hBrush)
        return;

    POINT oldOrg;
    SetBrushOrgEx(hdc, destRect.left - phase.x, destRect.top - phase.y, &oldOrg);
    FillRect(hdc, &destRect, hBrush.get());
    SetBrushOrgEx(hdc, oldOrg.x, oldOrg.y, 0);
}

static void drawPatternSimple(HDC hdc, const RECT& destRect, const SharedBitmap* bmp, const SIZE& bmpSize, const POINT& phase)
{
    int dstY = destRect.top;
    for (int sourceY = phase.y; dstY < destRect.bottom; ) {
        int sourceH = std::min<int>(bmpSize.cy - sourceY, destRect.bottom - dstY);
        int dstX = destRect.left;
        for (int sourceX = phase.x; dstX < destRect.right; ) {
            int sourceW = std::min<int>(bmpSize.cx - sourceX, destRect.right - dstX);

            StretchDIBits(hdc, dstX, dstY, sourceW, sourceH, sourceX, sourceY, sourceW, sourceH,
                bmp->bytes(), &bmp->bitmapInfo(), DIB_RGB_COLORS, SRCCOPY);

            dstX += sourceW;
            sourceX = 0;
        }

        dstY += sourceH;
        sourceY = 0;
    }
}

static LONG normalizePhase(LONG phase, int limit)
{
    if (!phase || limit < 2)
        return 0;

    if (limit == 2)
        return phase & 1;

    if (phase < 0) {
        phase = -phase;
        if (phase > limit)
            phase = static_cast<LONG>(static_cast<unsigned>(phase) % static_cast<unsigned>(limit));
        if (phase)
            phase = limit - phase;
        return phase;
    }

    if (phase < limit)
        return phase;

    return static_cast<LONG>(static_cast<unsigned>(phase) % static_cast<unsigned>(limit));
}

void SharedBitmap::drawPattern(GraphicsContext* ctxt, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                        const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, const IntSize& origSourceSize)
{
    if (!m_pixels)
        return;
    ctxt->drawBitmapPattern(this, tileRectIn, patternTransform, phase, styleColorSpace, op, destRect, origSourceSize);
}

void SharedBitmap::drawPattern(HDC hdc, const AffineTransform& transform, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                        const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, const IntSize& origSourceSize)
{
    if (!m_pixels)
        return;

    if (tileRectIn.width() <= 0 || tileRectIn.height() <= 0)
        return;

    bool useAlpha = op == CompositeSourceOver && hasAlpha() && is32bit();

    int bmpWidth = width();
    int bmpHeight = height();

    FloatRect tileRect(tileRectIn);
    if (bmpWidth != origSourceSize.width()) {
        double rate = static_cast<double>(bmpWidth) / origSourceSize.width();
        double temp = tileRect.width() * rate;
        tileRect.setX(tileRect.x() * rate);
        tileRect.setWidth(temp);
        temp = tileRect.height() * rate;
        tileRect.setY(tileRect.y() * rate);
        tileRect.setHeight(temp);
    }

    OwnPtr<HBITMAP> clippedBmp;

    if (tileRect.x() || tileRect.y() || tileRect.width() != bmpWidth || tileRect.height() != bmpHeight) {
        BitmapInfo patternBmpInfo;
        void* patternPixels;
        clippedBmp = clipBitmap(IntRect(tileRect), useAlpha, patternBmpInfo, patternPixels);
        if (!clippedBmp)
            return;

        bmpWidth = tileRect.width();
        bmpHeight = tileRect.height();
    }

    AffineTransform tf = patternTransform * transform;

    FloatRect trRect = tf.mapRect(destRect);

    RECT clipBox;
    int clipType = GetClipBox(hdc, &clipBox);
    if (clipType == SIMPLEREGION)
        trRect.intersect(FloatRect(clipBox.left, clipBox.top, clipBox.right - clipBox.left, clipBox.bottom - clipBox.top));
    else if (clipType == COMPLEXREGION) {
        OwnPtr<HRGN> clipRgn = adoptPtr(CreateRectRgn(0, 0, 0, 0));
        if (GetClipRgn(hdc, clipRgn.get()) > 0) {
            DWORD regionDataSize = GetRegionData(clipRgn.get(), sizeof(RGNDATA), 0);
            if (regionDataSize) {
                Vector<RGNDATA> regionData(regionDataSize);
                GetRegionData(clipRgn.get(), regionDataSize, regionData.data());
                RECT* rect = reinterpret_cast<RECT*>(regionData[0].Buffer);
                for (DWORD i = 0; i < regionData[0].rdh.nCount; ++i, ++rect)
                    trRect.intersect(FloatRect(rect->left, rect->top, rect->right - rect->left, rect->bottom - rect->top));
            }
        }
    }

    if (trRect.width() <= 0 || trRect.height() <= 0)
        return;

    trRect.inflate(1);
    IntRect visibleDstRect = enclosingIntRect(tf.inverse().mapRect(trRect));
    visibleDstRect.intersect(IntRect(destRect));

    if (visibleDstRect.width() <= 0 || visibleDstRect.height() <= 0)
        return;

    trRect = tf.mapRect(visibleDstRect);
    RECT dstRectWin = {
        stableRound(trRect.x()),
        stableRound(trRect.y()),
        stableRound(trRect.maxX()),
        stableRound(trRect.maxY()),
    };
    if (dstRectWin.right <= dstRectWin.left || dstRectWin.bottom <= dstRectWin.top)
        return;

    SIZE bmpSize = { bmpWidth, bmpHeight };

    // Relative to destination, in bitmap pixels
    POINT phaseWin = { stableRound(visibleDstRect.x() - phase.x()), stableRound(visibleDstRect.y() - phase.y()) };
    phaseWin.x = normalizePhase(phaseWin.x, bmpSize.cx);
    phaseWin.y = normalizePhase(phaseWin.y, bmpSize.cy);

    RECT srcRectWin = {
        0,
        0,
        stableRound(visibleDstRect.maxX()) - stableRound(visibleDstRect.x()),
        stableRound(visibleDstRect.maxY()) - stableRound(visibleDstRect.y())
    };
    if (srcRectWin.right <= 0 || srcRectWin.bottom <= 0)
        return;

    BitmapInfo bmpInfo = BitmapInfo::createBottomUp(IntSize(srcRectWin.right, srcRectWin.bottom), useAlpha ? BitmapInfo::BitCount32 : BitmapInfo::BitCount16);
    void* pixels;
    OwnPtr<HBITMAP> hbmpTemp = adoptPtr(CreateDIBSection(0, &bmpInfo, DIB_RGB_COLORS, &pixels, 0, 0));

    if (!hbmpTemp)
        return;

    OwnPtr<HDC> hmemdc = adoptPtr(CreateCompatibleDC(hdc));
    HGDIOBJ oldBmp = SelectObject(hmemdc.get(), hbmpTemp.get());
    if (clippedBmp)
        drawPatternSimple(hmemdc.get(), srcRectWin, clippedBmp.get(), phaseWin);
    else if ((op != CompositeSourceOver || canUseDIBits()) && srcRectWin.right <= bmpSize.cx * 2 && srcRectWin.bottom <= bmpSize.cy * 2)
        drawPatternSimple(hmemdc.get(), srcRectWin, this, bmpSize, phaseWin);
    else if (ensureHandle())
        drawPatternSimple(hmemdc.get(), srcRectWin, getHandle(), phaseWin);
    else {
        void* pixels;
        BitmapInfo bmpInfo;
        OwnPtr<HBITMAP> hbmp = createHandle(&pixels, &bmpInfo, -1, false);
        if (hbmp)
            drawPatternSimple(hmemdc.get(), srcRectWin, hbmp.get(), phaseWin);
        else {
            SelectObject(hmemdc.get(), oldBmp);
            return;
        }
    }

    if (useAlpha && hasAlphaBlendSupport()) {
        static const BLENDFUNCTION blend = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
        bool success = alphaBlendIfSupported(hdc, dstRectWin.left, dstRectWin.top, dstRectWin.right - dstRectWin.left, dstRectWin.bottom - dstRectWin.top,
            hmemdc.get(), 0, 0, srcRectWin.right, srcRectWin.bottom, blend);
        ASSERT_UNUSED(success, success);
    } else if (useAlpha && !hasAlphaBlendSupport() || op == CompositeSourceOver && usesTransparentColor()) {
        TransparentBlt(hdc, dstRectWin.left, dstRectWin.top, dstRectWin.right - dstRectWin.left,
            dstRectWin.bottom - dstRectWin.top, hmemdc.get(), 0, 0, srcRectWin.right, srcRectWin.bottom, transparentColor());
    } else {
        DWORD bmpOp = op == CompositeCopy ? SRCCOPY
                    : op == CompositeSourceOver ? SRCCOPY
                    : op == CompositeXOR ? PATINVERT
                    : op == CompositeClear ? WHITENESS
                    : SRCCOPY; // FIXEME: other types?

        StretchDIBits(hdc, dstRectWin.left, dstRectWin.top, dstRectWin.right - dstRectWin.left,
            dstRectWin.bottom - dstRectWin.top, 0, 0, srcRectWin.right, srcRectWin.bottom,
            pixels, &bmpInfo, DIB_RGB_COLORS, bmpOp);
    }
    SelectObject(hmemdc.get(), oldBmp);
}

SharedBitmap::DCProvider* SharedBitmap::s_dcProvider = new SharedBitmap::DCProvider;

HDC SharedBitmap::DCProvider::getDC(SharedBitmap* bmp, unsigned* key)
{
    if (!bmp || !bmp->ensureHandle())
        return 0;

    HDC hdc = CreateCompatibleDC(0);
    if (!hdc)
        return 0;

    *key = reinterpret_cast<unsigned>(SelectObject(hdc, bmp->getHandle()));
    RECT rect = { 0, 0, bmp->width(), bmp->height() };
    OwnPtr<HRGN> clipRgn = adoptPtr(CreateRectRgnIndirect(&rect));
    SelectClipRgn(hdc, clipRgn.get());

    return hdc;
}

void SharedBitmap::DCProvider::releaseDC(SharedBitmap*, HDC hdc, unsigned key1)
{
    if (!hdc)
        return;

    SelectObject(hdc, reinterpret_cast<HGDIOBJ>(key1));
    DeleteDC(hdc);
}

void SharedBitmap::clearPixels(const IntRect& rect)
{
    if (!m_pixels)
        return;

    IntRect bmpRect(0, 0, width(), height());
    bmpRect.intersect(rect);
    if (is16bit()) {
        unsigned w = m_bmpInfo.paddedWidth();
        unsigned short* dst = static_cast<unsigned short*>(m_pixels);
        dst += bmpRect.y() * w + bmpRect.x();
        int wordsToSet = bmpRect.width();
        const unsigned short* dstEnd = dst + bmpRect.height() * w;
        while (dst < dstEnd) {
            wmemset(reinterpret_cast<wchar_t*>(dst), 0, wordsToSet);
            dst += w;
        }
        return;
    }

    unsigned w = width();
    unsigned* dst = static_cast<unsigned*>(m_pixels);
    dst += bmpRect.y() * w + bmpRect.x();
    int wordsToSet = bmpRect.width() * 2;
    const unsigned* dstEnd = dst + bmpRect.height() * w;
    while (dst < dstEnd) {
        wmemset(reinterpret_cast<wchar_t*>(dst), 0, wordsToSet);
        dst += w;
    }
}

} // namespace WebCore
