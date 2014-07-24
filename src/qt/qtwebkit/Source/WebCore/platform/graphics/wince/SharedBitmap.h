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

#ifndef SharedBitmap_h
#define SharedBitmap_h

#include "AffineTransform.h"
#include "BitmapInfo.h"
#include "ColorSpace.h"
#include "GraphicsTypes.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>
#include <wingdi.h>

namespace WebCore {

class FloatPoint;
class FloatRect;
class GraphicsContext;
class IntRect;
class IntSize;
class TransformationMatrix;

class SharedBitmap: public RefCounted<SharedBitmap> {
public:
    ~SharedBitmap();
    static PassRefPtr<SharedBitmap> create(const IntSize&, BitmapInfo::BitCount = BitmapInfo::BitCount32, bool initPixels = true);
    static PassRefPtr<SharedBitmap> create(const Vector<unsigned>&, const IntSize&, bool hasAlpha = true);

    const BitmapInfo& bitmapInfo() const { return m_bmpInfo; }
    void* bytes() { return m_pixels; }
    const void* bytes() const { return m_pixels; }
    unsigned width() const { return m_bmpInfo.width(); }
    unsigned height() const { return m_bmpInfo.height(); }
    unsigned validHeight() const { return m_validHeight; }
    void setValidHeight(unsigned validHeight) { m_validHeight = validHeight; }
    void resetPixels(bool black = false);
    void clearPixels(const IntRect& r);
    bool locked() const { return m_locked; }
    void lock() { m_locked = true; }
    void unlock() { m_locked = false; }
    bool freeMemory();
    bool is16bit() const { return m_bmpInfo.is16bit(); }
    bool is32bit() const { return m_bmpInfo.is32bit(); }
    bool to16bit();
    bool hasAlpha() const { return m_hasAlpha; }
    void setHasAlpha(bool alpha) { m_hasAlpha = alpha; }
    bool ensureHandle();
    HBITMAP getHandle() { return m_hbitmap.get(); }
    PassOwnPtr<HBITMAP> createHandle(void** pixels, BitmapInfo* bmpInfo, int h = -1, bool use16bit = true) const;
    bool usesTransparentColor() const { return m_usesTransparentColor; }
    COLORREF transparentColor() const { return m_transparentColor; }
    void setTransparentColor(COLORREF c)
    {
        m_usesTransparentColor = true;
        m_transparentColor = c;
    }
    bool canUseDIBits() const { return !hasAlpha() && !usesTransparentColor(); }

    PassOwnPtr<HBITMAP> clipBitmap(const IntRect& rect, bool useAlpha, BitmapInfo& bmpInfo, void*& pixels);

    PassRefPtr<SharedBitmap> clipBitmap(const IntRect& rect, bool useAlpha);

    void draw(GraphicsContext* ctxt, const IntRect& dstRect, const IntRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode);
    void drawPattern(GraphicsContext* ctxt, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                    const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, const IntSize& origSourceSize);
    void draw(HDC, const IntRect& dstRect, const IntRect& srcRect, CompositeOperator compositeOp, BlendMode blendMode);
    void drawPattern(HDC, const AffineTransform&, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                    const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRect, const IntSize& origSourceSize);

    class DCProvider {
    public:
        virtual HDC getDC(SharedBitmap*, unsigned*);
        virtual void releaseDC(SharedBitmap*, HDC, unsigned);
    };

    static DCProvider* s_dcProvider;

    HDC getDC(unsigned* key1) { return s_dcProvider->getDC(this, key1); }
    void releaseDC(HDC hdc, unsigned key1) { s_dcProvider->releaseDC(this, hdc, key1); }

    class DCHolder {
    public:
        DCHolder(SharedBitmap* bmp = 0) { setInternal(bmp); }
        ~DCHolder() { clearInternal(); }
        void set(SharedBitmap* bmp = 0)
        {
            clearInternal();
            setInternal(bmp);
        }
        HDC get() const { return m_hdc; }
    private:
        DCHolder& operator=(const DCHolder&);
        DCHolder(const DCHolder&);
        void clearInternal()
        {
            if (m_hdc)
                m_bitmap->releaseDC(m_hdc, m_key);
        }
        void setInternal(SharedBitmap* bmp)
        {
            m_bitmap = bmp;
            m_hdc = bmp ? bmp->getDC(&m_key) : 0;
        }
        SharedBitmap* m_bitmap;
        HDC m_hdc;
        unsigned m_key;
    };

private:
    SharedBitmap(const IntSize&, BitmapInfo::BitCount, bool initPixels);
    BitmapInfo m_bmpInfo;
    OwnPtr<HBITMAP> m_hbitmap;
    void* m_pixels;
    OwnArrayPtr<unsigned> m_pixelData;
    COLORREF m_transparentColor;
    int m_validHeight;
    bool m_locked;
    bool m_usesTransparentColor;
    bool m_hasAlpha;
};

} // namespace WebCore

#endif // SharedBitmap_h
