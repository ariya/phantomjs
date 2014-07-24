/*
 *  Copyright (C) 2007-2009 Torch Mobile Inc.
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
 *
 */

#include "config.h"
#include "GraphicsContext.h"

#include "AffineTransform.h"
#include "Font.h"
#include "GDIExtras.h"
#include "GlyphBuffer.h"
#include "Gradient.h"
#include "NotImplemented.h"
#include "Path.h"
#include "PlatformPathWinCE.h"
#include "SharedBitmap.h"
#include "SimpleFontData.h"
#include <windows.h>
#include <wtf/OwnPtr.h>
#include <wtf/unicode/CharacterNames.h>

namespace WebCore {

typedef void (*FuncGradientFillRectLinear)(HDC hdc, const IntRect& r, const IntPoint& p0, const IntPoint& p1, const Vector<Gradient::ColorStop>& stops);
typedef void (*FuncGradientFillRectRadial)(HDC hdc, const IntRect& r, const IntPoint& p0, const IntPoint& p1, float r0, float r1, const Vector<Gradient::ColorStop>& stops);
FuncGradientFillRectLinear g_linearGradientFiller = 0;
FuncGradientFillRectRadial g_radialGradientFiller = 0;

static inline bool isZero(double d)
{
    return d > 0 ? d <= 1.E-10 : d >= -1.E-10;
}

// stableRound rounds -0.5 to 0, where lround rounds -0.5 to -1.
static inline int stableRound(double d)
{
    if (d > 0)
        return static_cast<int>(d + 0.5);

    int i = static_cast<int>(d);
    return i - d > 0.5 ? i - 1 : i;
}

// Unlike enclosingIntRect(), this function does strict rounding.
static inline IntRect roundRect(const FloatRect& r)
{
    return IntRect(stableRound(r.x()), stableRound(r.y()), stableRound(r.maxX()) - stableRound(r.x()), stableRound(r.maxY()) - stableRound(r.y()));
}

// Rotation transformation
class RotationTransform {
public:
    RotationTransform()
        : m_cosA(1.)
        , m_sinA(0.)
        , m_preShiftX(0)
        , m_preShiftY(0)
        , m_postShiftX(0)
        , m_postShiftY(0)
    {
    }
    RotationTransform operator-() const
    {
        RotationTransform rtn;
        rtn.m_cosA = m_cosA;
        rtn.m_sinA = -m_sinA;
        rtn.m_preShiftX = m_postShiftX;
        rtn.m_preShiftY = m_postShiftY;
        rtn.m_postShiftX = m_preShiftX;
        rtn.m_postShiftY = m_preShiftY;
        return rtn;
    }
    void map(double x1, double y1, double* x2, double* y2) const
    {
        x1 += m_preShiftX;
        y1 += m_preShiftY;
        *x2 = x1 * m_cosA + y1 * m_sinA + m_postShiftX;
        *y2 = y1 * m_cosA - x1 * m_sinA + m_postShiftY;
    }
    void map(int x1, int y1, int* x2, int* y2) const
    {
        x1 += m_preShiftX;
        y1 += m_preShiftY;
        *x2 = stableRound(x1 * m_cosA + y1 * m_sinA) + m_postShiftX;
        *y2 = stableRound(y1 * m_cosA - x1 * m_sinA) + m_postShiftY;
    }

    double m_cosA;
    double m_sinA;
    int m_preShiftX;
    int m_preShiftY;
    int m_postShiftX;
    int m_postShiftY;
};

template<class T> static inline IntPoint mapPoint(const IntPoint& p, const T& t)
{
    int x, y;
    t.map(p.x(), p.y(), &x, &y);
    return IntPoint(x, y);
}

template<class T> static inline FloatPoint mapPoint(const FloatPoint& p, const T& t)
{
    double x, y;
    t.map(p.x(), p.y(), &x, &y);
    return FloatPoint(static_cast<float>(x), static_cast<float>(y));
}

template<class Transform, class Rect, class Value> static inline Rect mapRect(const Rect& rect, const Transform& transform)
{
    Value x[4], y[4];
    Value l, t, r, b;
    r = rect.maxX() - 1;
    b = rect.maxY() - 1;
    transform.map(rect.x(), rect.y(), x, y);
    transform.map(rect.x(), b, x + 1, y + 1);
    transform.map(r, b, x + 2, y + 2);
    transform.map(r, rect.y(), x + 3, y + 3);
    l = r = x[3];
    t = b = y[3];
    for (int i = 0; i < 3; ++i) {
        if (x[i] < l)
            l = x[i];
        else if (x[i] > r)
            r = x[i];

        if (y[i] < t)
            t = y[i];
        else if (y[i] > b)
            b = y[i];
    }

    return IntRect(l, t, r - l + 1, b - t + 1);
}

template<class T> static inline IntRect mapRect(const IntRect& rect, const T& transform)
{
    return mapRect<T, IntRect, int>(rect, transform);
}

template<class T> static inline FloatRect mapRect(const FloatRect& rect, const T& transform)
{
    return mapRect<T, FloatRect, double>(rect, transform);
}

class GraphicsContextPlatformPrivateData {
public:
    GraphicsContextPlatformPrivateData()
        : m_transform()
        , m_opacity(1.0)
    {
    }

    AffineTransform m_transform;
    float m_opacity;
};

enum AlphaPaintType {
    AlphaPaintNone,
    AlphaPaintImage,
    AlphaPaintOther,
};

class GraphicsContextPlatformPrivate : public GraphicsContextPlatformPrivateData {
public:
    GraphicsContextPlatformPrivate(HDC dc)
        : m_dc(dc)
    {
    }
    ~GraphicsContextPlatformPrivate()
    {
        while (!m_backupData.isEmpty())
            restore();
    }

    void translate(float x, float y)
    {
        m_transform.translate(x, y);
    }

    void scale(const FloatSize& size)
    {
        m_transform.scaleNonUniform(size.width(), size.height());
    }

    void rotate(float radians)
    {
        m_transform.rotate(rad2deg(radians));
    }

    void concatCTM(const AffineTransform& transform)
    {
        m_transform *= transform;
    }

    void setCTM(const AffineTransform& transform)
    {
        m_transform = transform;
    }

    IntRect mapRect(const IntRect& rect) const
    {
        return m_transform.mapRect(rect);
    }

    FloatRect mapRect(const FloatRect& rect) const
    {
        return m_transform.mapRect(rect);
    }

    IntPoint mapPoint(const IntPoint& point) const
    {
        return m_transform.mapPoint(point);
    }

    FloatPoint mapPoint(const FloatPoint& point) const
    {
        return m_transform.mapPoint(point);
    }

    FloatSize mapSize(const FloatSize& size) const
    {
        double w, h;
        m_transform.map(size.width(), size.height(), w, h);
        return FloatSize(static_cast<float>(w), static_cast<float>(h));
    }

    void save()
    {
        if (m_dc)
            SaveDC(m_dc);

        m_backupData.append(*static_cast<GraphicsContextPlatformPrivateData*>(this));
    }

    void restore()
    {
        if (m_backupData.isEmpty())
            return;

        if (m_dc)
            RestoreDC(m_dc, -1);

        GraphicsContextPlatformPrivateData::operator=(m_backupData.last());
        m_backupData.removeLast();
    }

    bool hasAlpha() const { return m_bitmap && m_bitmap->hasAlpha(); }

    PassRefPtr<SharedBitmap> getTransparentLayerBitmap(IntRect& origRect, AlphaPaintType alphaPaint, RECT& bmpRect, bool checkClipBox, bool force) const
    {
        if (m_opacity <= 0)
            return 0;

        if (force || m_opacity < 1.)  {
            if (checkClipBox) {
                RECT clipBox;
                int clipType = GetClipBox(m_dc, &clipBox);
                if (clipType == SIMPLEREGION || clipType == COMPLEXREGION)
                    origRect.intersect(clipBox);
                if (origRect.isEmpty())
                    return 0;
            }

            RefPtr<SharedBitmap> bmp = SharedBitmap::create(origRect.size(), alphaPaint == AlphaPaintNone ? BitmapInfo::BitCount16 : BitmapInfo::BitCount32, false);
            SetRect(&bmpRect, 0, 0, origRect.width(), origRect.height());
            if (bmp) {
                switch (alphaPaint) {
                case AlphaPaintNone:
                case AlphaPaintImage:
                    {
                        SharedBitmap::DCHolder dc(bmp.get());
                        if (dc.get()) {
                            BitBlt(dc.get(), 0, 0, origRect.width(), origRect.height(), m_dc, origRect.x(), origRect.y(), SRCCOPY);
                            if (bmp->is32bit() && (!m_bitmap || m_bitmap->is16bit())) {
                                // Set alpha channel
                                unsigned* pixels = (unsigned*)bmp->bytes();
                                const unsigned* const pixelsEnd = pixels + bmp->bitmapInfo().numPixels();
                                while (pixels < pixelsEnd) {
                                    *pixels |= 0xFF000000;
                                    ++pixels;
                                }
                            }
                            return bmp;
                        }
                    }
                    break;
                //case AlphaPaintOther:
                default:
                    memset(bmp->bytes(), 0xFF, bmp->bitmapInfo().numPixels() * 4);
                    return bmp;
                    break;
                }
            }
        }

        bmpRect = origRect;
        return 0;
    }

    void paintBackTransparentLayerBitmap(HDC hdc, SharedBitmap* bmp, const IntRect& origRect, AlphaPaintType alphaPaint, const RECT& bmpRect)
    {
        if (hdc == m_dc)
            return;

        if (alphaPaint == AlphaPaintOther && hasAlphaBlendSupport()) {
            ASSERT(bmp && bmp->bytes() && bmp->is32bit());
            unsigned* pixels = (unsigned*)bmp->bytes();
            const unsigned* const pixelsEnd = pixels + bmp->bitmapInfo().numPixels();
            while (pixels < pixelsEnd) {
                *pixels ^= 0xFF000000;
                ++pixels;
            }
        }
        if ((m_opacity < 1. || alphaPaint == AlphaPaintOther) && hasAlphaBlendSupport()) {
            const BLENDFUNCTION blend = { AC_SRC_OVER, 0
                , m_opacity >= 1. ? 255 : (BYTE)(m_opacity * 255)
                , alphaPaint == AlphaPaintNone ? 0 : AC_SRC_ALPHA };
            bool success = alphaBlendIfSupported(m_dc, origRect.x(), origRect.y(), origRect.width(), origRect.height(), hdc, 0, 0, bmpRect.right, bmpRect.bottom, blend);
            ASSERT_UNUSED(success, success);
        } else
            StretchBlt(m_dc, origRect.x(), origRect.y(), origRect.width(), origRect.height(), hdc, 0, 0, bmpRect.right, bmpRect.bottom, SRCCOPY);
    }

    HDC m_dc;
    RefPtr<SharedBitmap> m_bitmap;
    Vector<GraphicsContextPlatformPrivateData> m_backupData;
};

static PassOwnPtr<HPEN> createPen(const Color& col, double fWidth, StrokeStyle style)
{
    int width = stableRound(fWidth);
    if (width < 1)
        width = 1;

    int penStyle = PS_NULL;
    switch (style) {
        case SolidStroke:
#if ENABLE(CSS3_TEXT)
        case DoubleStroke:
        case WavyStroke: // FIXME: https://bugs.webkit.org/show_bug.cgi?id=94114 - Needs platform support.
#endif // CSS3_TEXT
            penStyle = PS_SOLID;
            break;
        case DottedStroke:  // not supported on Windows CE
        case DashedStroke:
            penStyle = PS_DASH;
            width = 1;
            break;
        default:
            break;
    }

    return adoptPtr(CreatePen(penStyle, width, RGB(col.red(), col.green(), col.blue())));
}

static inline PassOwnPtr<HBRUSH> createBrush(const Color& col)
{
    return adoptPtr(CreateSolidBrush(RGB(col.red(), col.green(), col.blue())));
}

template <typename PixelType, bool Is16bit> static void _rotateBitmap(SharedBitmap* destBmp, const SharedBitmap* sourceBmp, const RotationTransform& transform)
{
    int destW = destBmp->width();
    int destH = destBmp->height();
    int sourceW = sourceBmp->width();
    int sourceH = sourceBmp->height();
    PixelType* dest = (PixelType*)destBmp->bytes();
    const PixelType* source = (const PixelType*)sourceBmp->bytes();
    int padding;
    int paddedSourceW;
    if (Is16bit) {
        padding = destW & 1;
        paddedSourceW = sourceW + (sourceW & 1);
    } else {
        padding = 0;
        paddedSourceW = sourceW;
    }
    if (isZero(transform.m_sinA)) {
        int cosA = transform.m_cosA > 0 ? 1 : -1;
        for (int y = 0; y < destH; ++y) {
            for (int x = 0; x < destW; ++x) {
                int x1 = x + transform.m_preShiftX;
                int y1 = y + transform.m_preShiftY;
                int srcX = x1 * cosA + transform.m_postShiftX;
                int srcY = y1 * cosA - transform.m_postShiftY;
                if (srcX >= 0 && srcX <= sourceW && srcY >= 0 && srcY <= sourceH)
                    *dest++ = source[srcY * paddedSourceW + srcX] | 0xFF000000;
                else
                    *dest++ |= 0xFF;
            }
            dest += padding;
        }
    } else if (isZero(transform.m_cosA)) {
        int sinA = transform.m_sinA > 0 ? 1 : -1;
        for (int y = 0; y < destH; ++y) {
            for (int x = 0; x < destW; ++x) {
                int x1 = x + transform.m_preShiftX;
                int y1 = y + transform.m_preShiftY;
                int srcX = y1 * sinA + transform.m_postShiftX;
                int srcY = -x1 * sinA + transform.m_postShiftY;
                if (srcX >= 0 && srcX <= sourceW && srcY >= 0 && srcY <= sourceH)
                    *dest++ = source[srcY * paddedSourceW + srcX];
            }
            dest += padding;
        }
    } else {
        for (int y = 0; y < destH; ++y) {
            for (int x = 0; x < destW; ++x) {
                // FIXME: for best quality, we should get weighted sum of four neighbours,
                // but that will be too expensive
                int srcX, srcY;
                transform.map(x, y, &srcX, &srcY);
                if (srcX >= 0 && srcX <= sourceW && srcY >= 0 && srcY <= sourceH)
                    *dest++ = source[srcY * paddedSourceW + srcX];
            }
            dest += padding;
        }
    }
}

static void rotateBitmap(SharedBitmap* destBmp, const SharedBitmap* sourceBmp, const RotationTransform& transform)
{
    ASSERT(destBmp->is16bit() == sourceBmp->is16bit());
    if (destBmp->is16bit())
        _rotateBitmap<unsigned short, true>(destBmp, sourceBmp, transform);
    else
        _rotateBitmap<unsigned, false>(destBmp, sourceBmp, transform);
}

class TransparentLayerDC {
    WTF_MAKE_NONCOPYABLE(TransparentLayerDC);
public:
    TransparentLayerDC(GraphicsContextPlatformPrivate* data, IntRect& origRect, const IntRect* rectBeforeTransform = 0, int alpha = 255, bool paintImage = false);
    ~TransparentLayerDC();

    HDC hdc() const { return m_memDc; }
    const RECT& rect() const { return m_bmpRect; }
    IntSize toShift() const { return IntSize(m_bmpRect.left - m_origRect.x(), m_bmpRect.top - m_origRect.y()); }
    void fillAlphaChannel();

private:
    GraphicsContextPlatformPrivate* m_data;
    IntRect m_origRect;
    IntRect m_rotatedOrigRect;
    HDC m_memDc;
    RefPtr<SharedBitmap> m_bitmap;
    RefPtr<SharedBitmap> m_rotatedBitmap;
    RECT m_bmpRect;
    unsigned m_key;
    RotationTransform m_rotation;
    float m_oldOpacity;
    AlphaPaintType m_alphaPaintType;
};

TransparentLayerDC::TransparentLayerDC(GraphicsContextPlatformPrivate* data, IntRect& origRect, const IntRect* rectBeforeTransform, int alpha, bool paintImage)
: m_data(data)
, m_origRect(origRect)
, m_oldOpacity(data->m_opacity)
// m_key1 and m_key2 are not initalized here. They are used only in the case that
// SharedBitmap::getDC() is called, I.E., when m_bitmap is not null.
{
    m_data->m_opacity *= alpha / 255.;
    bool mustCreateLayer;
    if (!m_data->hasAlpha()) {
        mustCreateLayer = false;
        m_alphaPaintType = AlphaPaintNone;
    } else {
        mustCreateLayer = true;
        m_alphaPaintType = paintImage ? AlphaPaintImage : AlphaPaintOther;
    }
    if (rectBeforeTransform && !isZero(m_data->m_transform.b())) {
        m_rotatedOrigRect = origRect;
        m_rotatedBitmap = m_data->getTransparentLayerBitmap(m_rotatedOrigRect, m_alphaPaintType, m_bmpRect, false, true);
        if (m_rotatedBitmap) {
            double a = m_data->m_transform.a();
            double b = m_data->m_transform.b();
            double c = _hypot(a, b);
            m_rotation.m_cosA = a / c;
            m_rotation.m_sinA = b / c;

            int centerX = origRect.x() + origRect.width() / 2;
            int centerY = origRect.y() + origRect.height() / 2;
            m_rotation.m_preShiftX = -centerX;
            m_rotation.m_preShiftY = -centerY;
            m_rotation.m_postShiftX = centerX;
            m_rotation.m_postShiftY = centerY;

            m_origRect = mapRect(m_rotatedOrigRect, m_rotation);

            m_rotation.m_preShiftX += m_rotatedOrigRect.x();
            m_rotation.m_preShiftY += m_rotatedOrigRect.y();
            m_rotation.m_postShiftX -= m_origRect.x();
            m_rotation.m_postShiftY -= m_origRect.y();

            FloatPoint topLeft = m_data->m_transform.mapPoint(FloatPoint(rectBeforeTransform->location()));
            FloatPoint topRight(rectBeforeTransform->maxX() - 1, rectBeforeTransform->y());
            topRight = m_data->m_transform.mapPoint(topRight);
            FloatPoint bottomLeft(rectBeforeTransform->x(), rectBeforeTransform->maxY() - 1);
            bottomLeft = m_data->m_transform.mapPoint(bottomLeft);
            FloatSize sideTop = topRight - topLeft;
            FloatSize sideLeft = bottomLeft - topLeft;
            float width = _hypot(sideTop.width() + 1, sideTop.height() + 1);
            float height = _hypot(sideLeft.width() + 1, sideLeft.height() + 1);

            origRect.inflateX(stableRound((width - origRect.width()) * 0.5));
            origRect.inflateY(stableRound((height - origRect.height()) * 0.5));

            m_bitmap = SharedBitmap::create(m_origRect.size(), m_rotatedBitmap->is16bit() ? BitmapInfo::BitCount16 : BitmapInfo::BitCount32, true);
            if (m_bitmap)
                rotateBitmap(m_bitmap.get(), m_rotatedBitmap.get(), -m_rotation);
            else
                m_rotatedBitmap = 0;
        }
    } else
        m_bitmap = m_data->getTransparentLayerBitmap(m_origRect, m_alphaPaintType, m_bmpRect, true, mustCreateLayer);
    if (m_bitmap)
        m_memDc = m_bitmap->getDC(&m_key);
    else
        m_memDc = m_data->m_dc;
}

TransparentLayerDC::~TransparentLayerDC()
{
    if (m_rotatedBitmap) {
        m_bitmap->releaseDC(m_memDc, m_key);
        m_key = 0;
        rotateBitmap(m_rotatedBitmap.get(), m_bitmap.get(), m_rotation);
        m_memDc = m_rotatedBitmap->getDC(&m_key);
        m_data->paintBackTransparentLayerBitmap(m_memDc, m_rotatedBitmap.get(), m_rotatedOrigRect, m_alphaPaintType, m_bmpRect);
        m_rotatedBitmap->releaseDC(m_memDc, m_key);
    } else if (m_bitmap) {
        m_data->paintBackTransparentLayerBitmap(m_memDc, m_bitmap.get(), m_origRect, m_alphaPaintType, m_bmpRect);
        m_bitmap->releaseDC(m_memDc, m_key);
    }
    m_data->m_opacity = m_oldOpacity;
}

void TransparentLayerDC::fillAlphaChannel()
{
    if (!m_bitmap || !m_bitmap->is32bit())
        return;

    unsigned* pixels = (unsigned*)m_bitmap->bytes();
    const unsigned* const pixelsEnd = pixels + m_bitmap->bitmapInfo().numPixels();
    while (pixels < pixelsEnd) {
        *pixels |= 0xFF000000;
        ++pixels;
    }
}

class ScopeDCProvider {
    WTF_MAKE_NONCOPYABLE(ScopeDCProvider);
public:
    explicit ScopeDCProvider(GraphicsContextPlatformPrivate* data)
        : m_data(data)
    {
        if (m_data->m_bitmap)
            m_data->m_dc = m_data->m_bitmap->getDC(&m_key);
    }
    ~ScopeDCProvider()
    {
        if (m_data->m_bitmap) {
            m_data->m_bitmap->releaseDC(m_data->m_dc, m_key);
            m_data->m_dc = 0;
        }
    }
private:
    GraphicsContextPlatformPrivate* m_data;
    unsigned m_key;
};


void GraphicsContext::platformInit(PlatformGraphicsContext* dc)
{
    m_data = new GraphicsContextPlatformPrivate(dc);
}

void GraphicsContext::platformDestroy()
{
    delete m_data;
}

void GraphicsContext::setBitmap(PassRefPtr<SharedBitmap> bmp)
{
    ASSERT(!m_data->m_dc);
    m_data->m_bitmap = bmp;
}

HDC GraphicsContext::getWindowsContext(const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // FIXME: Add support for AlphaBlend.
    ASSERT(!supportAlphaBlend);
    return m_data->m_dc;
}

void GraphicsContext::releaseWindowsContext(HDC hdc, const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
}

void GraphicsContext::savePlatformState()
{
    m_data->save();
}

void GraphicsContext::restorePlatformState()
{
    m_data->restore();
}

void GraphicsContext::drawRect(const IntRect& rect)
{
    if (!m_data->m_opacity || paintingDisabled() || rect.isEmpty())
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect trRect = m_data->mapRect(rect);
    TransparentLayerDC transparentDC(m_data, trRect, &rect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    OwnPtr<HBRUSH> brush;
    HGDIOBJ oldBrush;
    if (fillColor().alpha()) {
        brush = createBrush(fillColor());
        oldBrush = SelectObject(dc, brush.get());
    } else
        oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

    OwnPtr<HPEN> pen;
    HGDIOBJ oldPen;
    if (strokeStyle() != NoStroke) {
        pen = createPen(strokeColor(), strokeThickness(), strokeStyle());
        oldPen = SelectObject(dc, pen.get());
    } else
        oldPen = SelectObject(dc, GetStockObject(NULL_PEN));

    if (brush || pen) {
        if (trRect.width() <= 0)
            trRect.setWidth(1);
        if (trRect.height() <= 0)
            trRect.setHeight(1);

        Rectangle(dc, trRect.x(), trRect.y(), trRect.maxX(), trRect.maxY());
    }

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
}

void GraphicsContext::drawLine(const IntPoint& point1, const IntPoint& point2)
{
    if (!m_data->m_opacity || paintingDisabled() || strokeStyle() == NoStroke || !strokeColor().alpha())
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntPoint trPoint1 = m_data->mapPoint(point1);
    IntPoint trPoint2 = m_data->mapPoint(point2);

    IntRect lineRect(trPoint1, trPoint2 - trPoint1);
    lineRect.setHeight(lineRect.height() + strokeThickness());
    TransparentLayerDC transparentDC(m_data, lineRect, 0, strokeColor().alpha());
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trPoint1 += transparentDC.toShift();
    trPoint2 += transparentDC.toShift();

    OwnPtr<HPEN> pen = createPen(strokeColor(), strokeThickness(), strokeStyle());
    HGDIOBJ oldPen = SelectObject(dc, pen.get());

    MoveToEx(dc, trPoint1.x(), trPoint1.y(), 0);
    LineTo(dc, trPoint2.x(), trPoint2.y());

    SelectObject(dc, oldPen);
}

void GraphicsContext::drawEllipse(const IntRect& rect)
{
    if (!m_data->m_opacity || paintingDisabled() || (!fillColor().alpha() && strokeStyle() == NoStroke))
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect trRect = m_data->mapRect(rect);
    TransparentLayerDC transparentDC(m_data, trRect, &rect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    OwnPtr<HBRUSH> brush;
    HGDIOBJ oldBrush;
    if (fillColor().alpha()) {
        brush = createBrush(fillColor());
        oldBrush = SelectObject(dc, brush.get());
    } else
        oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

    OwnPtr<HPEN> pen;
    HGDIOBJ oldPen = 0;
    if (strokeStyle() != NoStroke) {
        pen = createPen(strokeColor(), strokeThickness(), strokeStyle());
        oldPen = SelectObject(dc, pen.get());
    } else
        oldPen = SelectObject(dc, GetStockObject(NULL_PEN));

    if (brush || pen)
        Ellipse(dc, trRect.x(), trRect.y(), trRect.maxX(), trRect.maxY());

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
}

static inline bool equalAngle(double a, double b) 
{
    return fabs(a - b) < 1E-5;
}

void getEllipsePointByAngle(double angle, double a, double b, float& x, float& y)
{
    while (angle < 0)
        angle += 2 * piDouble;
    while (angle >= 2 * piDouble)
        angle -= 2 * piDouble;

    if (equalAngle(angle, 0) || equalAngle(angle, 2 * piDouble)) {
        x = a;
        y = 0;
    } else if (equalAngle(angle, piDouble)) {
        x = -a;
        y = 0;
    } else if (equalAngle(angle, .5 * piDouble)) {
        x = 0;
        y = b;
    } else if (equalAngle(angle, 1.5 * piDouble)) {
        x = 0;
        y = -b;
    } else {
        double k = tan(angle);
        double sqA = a * a;
        double sqB = b * b;
        double tmp = 1. / (1. / sqA + (k * k) / sqB);
        tmp = tmp <= 0 ? 0 : sqrt(tmp);
        if (angle > .5 * piDouble && angle < 1.5 * piDouble)
            tmp = -tmp;
        x = tmp;

        k = tan(.5 * piDouble - angle);
        tmp = 1. / ((k * k) / sqA + 1 / sqB);
        tmp = tmp <= 0 ? 0 : sqrt(tmp);
        if (angle > piDouble)
            tmp = -tmp;
        y = tmp;
    }
}

void GraphicsContext::drawConvexPolygon(size_t npoints, const FloatPoint* points, bool shouldAntialias)
{
    if (!m_data->m_opacity || paintingDisabled() || npoints <= 1 || !points)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    Vector<POINT, 20> winPoints(npoints);
    FloatPoint trPoint = m_data->mapPoint(points[0]);
    winPoints[0].x = stableRound(trPoint.x());
    winPoints[0].y = stableRound(trPoint.y());
    RECT rect = { winPoints[0].x, winPoints[0].y, winPoints[0].x, winPoints[0].y };
    for (size_t i = 1; i < npoints; ++i) {
        trPoint = m_data->mapPoint(points[i]);
        winPoints[i].x = stableRound(trPoint.x());
        winPoints[i].y = stableRound(trPoint.y());
        if (rect.left > winPoints[i].x)
            rect.left = winPoints[i].x;
        else if (rect.right < winPoints[i].x)
            rect.right = winPoints[i].x;
        if (rect.top > winPoints[i].y)
            rect.top = winPoints[i].y;
        else if (rect.bottom < winPoints[i].y)
            rect.bottom = winPoints[i].y;
    }
    rect.bottom += 1;
    rect.right += 1;

    IntRect intRect(rect);
    TransparentLayerDC transparentDC(m_data, intRect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;

    for (size_t i = 0; i < npoints; ++i) {
        winPoints[i].x += transparentDC.toShift().width();
        winPoints[i].y += transparentDC.toShift().height();
    }

    OwnPtr<HBRUSH> brush;
    HGDIOBJ oldBrush;
    if (fillColor().alpha()) {
        brush = createBrush(fillColor());
        oldBrush = SelectObject(dc, brush.get());
    } else
        oldBrush = SelectObject(dc, GetStockObject(NULL_BRUSH));

    OwnPtr<HPEN> pen;
    HGDIOBJ oldPen;
    if (strokeStyle() != NoStroke) {
        pen = createPen(strokeColor(), strokeThickness(), strokeStyle());
        oldPen = SelectObject(dc, pen.get());
    } else
        oldPen = SelectObject(dc, GetStockObject(NULL_PEN));

    if (brush || pen)
        Polygon(dc, winPoints.data(), npoints);

    SelectObject(dc, oldPen);
    SelectObject(dc, oldBrush);
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool antialiased)
{
    if (paintingDisabled())
        return;

    if (numPoints <= 1)
        return;
    
    // FIXME: IMPLEMENT!!
}

void GraphicsContext::fillRect(const FloatRect& rect, const Color& color, ColorSpace colorSpace)
{
    if (paintingDisabled() || !m_data->m_opacity)
        return;

    int alpha = color.alpha();
    if (!alpha)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect intRect = enclosingIntRect(rect);
    TransparentLayerDC transparentDC(m_data, m_data->mapRect(intRect), &intRect, alpha);

    if (!transparentDC.hdc())
        return;

    OwnPtr<HBRUSH> hbrush = adoptPtr(CreateSolidBrush(RGB(color.red(), color.green(), color.blue())));
    FillRect(transparentDC.hdc(), &transparentDC.rect(), hbrush.get());
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    if (!m_data->m_dc)
        return;

    IntRect trRect = enclosingIntRect(m_data->mapRect(rect));

    OwnPtr<HRGN> clipRgn = adoptPtr(CreateRectRgn(0, 0, 0, 0));
    if (GetClipRgn(m_data->m_dc, clipRgn.get()) > 0)
        IntersectClipRect(m_data->m_dc, trRect.x(), trRect.y(), trRect.maxX(), trRect.maxY());
    else {
        clipRgn = adoptPtr(CreateRectRgn(trRect.x(), trRect.y(), trRect.maxX(), trRect.maxY()));
        SelectClipRgn(m_data->m_dc, clipRgn.get());
    }
}

void GraphicsContext::clipOut(const IntRect& rect)
{
    if (paintingDisabled())
        return;

    if (!m_data->m_dc)
        return;

    IntRect trRect = m_data->mapRect(rect);

    ExcludeClipRect(m_data->m_dc, trRect.x(), trRect.y(), trRect.maxX(), trRect.maxY());
}

void GraphicsContext::drawFocusRing(const Path& path, int width, int offset, const Color& color)
{
    // FIXME: implement
}

void GraphicsContext::drawFocusRing(const Vector<IntRect>& rects, int width, int offset, const Color& color)
{
    if (!m_data->m_opacity || paintingDisabled())
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    int radius = (width - 1) / 2;
    offset += radius;

    unsigned rectCount = rects.size();
    IntRect finalFocusRect;
    for (unsigned i = 0; i < rectCount; i++) {
        IntRect focusRect = rects[i];
        focusRect.inflate(offset);
        finalFocusRect.unite(focusRect);
    }

    IntRect intRect = finalFocusRect;
    IntRect trRect = m_data->mapRect(finalFocusRect);
    TransparentLayerDC transparentDC(m_data, trRect, &intRect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    RECT rect = trRect;
    DrawFocusRect(dc, &rect);
}

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool printing)
{
    if (paintingDisabled())
        return;

    StrokeStyle oldStyle = strokeStyle();
    setStrokeStyle(SolidStroke);
    drawLine(roundedIntPoint(origin), roundedIntPoint(origin + FloatSize(width, 0)));
    setStrokeStyle(oldStyle);
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint&, float width, DocumentMarkerLineStyle style)
{
    notImplemented();
}

void GraphicsContext::setPlatformFillColor(const Color& col, ColorSpace colorSpace)
{
    notImplemented();
}

void GraphicsContext::setPlatformStrokeColor(const Color& col, ColorSpace colorSpace)
{
    notImplemented();
}

void GraphicsContext::setPlatformStrokeThickness(float strokeThickness)
{
    notImplemented();
}

void GraphicsContext::setURLForRect(const KURL& link, const IntRect& destRect)
{
    notImplemented();
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    if (m_data->hasAlpha()) {
        IntRect trRect = enclosingIntRect(m_data->mapRect(rect));
        m_data->m_bitmap->clearPixels(trRect);
        return;
    } 

    fillRect(rect, Color(Color::white), ColorSpaceDeviceRGB);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float width)
{
    if (!m_data->m_opacity || paintingDisabled() || strokeStyle() == NoStroke)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect intRect = enclosingIntRect(rect);
    IntRect trRect = m_data->mapRect(intRect);
    TransparentLayerDC transparentDC(m_data, trRect, &intRect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    OwnPtr<HPEN> pen = createPen(strokeColor(), strokeThickness(), strokeStyle());
    HGDIOBJ oldPen = SelectObject(dc, pen.get());

    int right = trRect.maxX() - 1;
    int bottom = trRect.maxY() - 1;
    const POINT intPoints[5] =
    {
        { trRect.x(), trRect.y() },
        { right, trRect.y() },
        { right, bottom },
        { trRect.x(), bottom },
        { trRect.x(), trRect.y() }
    };

    Polyline(dc, intPoints, 5);

    SelectObject(dc, oldPen);
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    m_data->save();
    m_data->m_opacity *= opacity;
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    m_data->restore();
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    m_data->concatCTM(transform);
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    m_data->setCTM(transform);
}

AffineTransform& GraphicsContext::affineTransform()
{
    return m_data->m_transform;
}

const AffineTransform& GraphicsContext::affineTransform() const
{
    return m_data->m_transform;
}

void GraphicsContext::resetAffineTransform()
{
    m_data->m_transform.makeIdentity();
}

void GraphicsContext::translate(float x, float y)
{
    m_data->translate(x, y);
}

void GraphicsContext::rotate(float radians)
{
    m_data->rotate(radians);
}

void GraphicsContext::scale(const FloatSize& size)
{
    m_data->scale(size);
}

void GraphicsContext::setLineCap(LineCap lineCap)
{
    notImplemented();
}

void GraphicsContext::setLineJoin(LineJoin lineJoin)
{
    notImplemented();
}

void GraphicsContext::setMiterLimit(float miter)
{
    notImplemented();
}

void GraphicsContext::setAlpha(float alpha)
{
    m_data->m_opacity = alpha;
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blendMode)
{
    notImplemented();
}

void GraphicsContext::clip(const Path& path, WindRule)
{
    notImplemented();
}

void GraphicsContext::canvasClip(const Path& path, WindRule fillRule)
{
    clip(path, fillRule);
}

void GraphicsContext::clipOut(const Path&)
{
    notImplemented();
}

static inline IntPoint rectCenterPoint(const RECT& rect)
{
    return IntPoint(rect.left + (rect.right - rect.left) / 2, rect.top + (rect.bottom - rect.top) / 2);
}
void GraphicsContext::fillRoundedRect(const IntRect& fillRect, const IntSize& topLeft, const IntSize& topRight, const IntSize& bottomLeft, const IntSize& bottomRight, const Color& c, ColorSpace colorSpace)
{
    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    FloatSize shadowOffset;
    float shadowBlur = 0;
    Color shadowColor;
    ColorSpace shadowColorSpace;
        
    getShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace);
    
    IntRect dstRect = fillRect;
    
    dstRect.move(stableRound(shadowOffset.width()), stableRound(shadowOffset.height()));
    dstRect.inflate(stableRound(shadowBlur));
    dstRect = m_data->mapRect(dstRect);
  
    FloatSize newTopLeft(m_data->mapSize(topLeft));
    FloatSize newTopRight(m_data->mapSize(topRight));
    FloatSize newBottomLeft(m_data->mapSize(bottomLeft));
    FloatSize newBottomRight(m_data->mapSize(bottomRight));

    TransparentLayerDC transparentDc(m_data, dstRect, &fillRect);
    HDC dc = transparentDc.hdc();
    if (!dc)
        return;

    dstRect.move(transparentDc.toShift());

    RECT rectWin = dstRect;

    OwnPtr<HBRUSH> brush = createBrush(shadowColor);
    HGDIOBJ oldBrush = SelectObject(dc, brush.get());

    SelectObject(dc, GetStockObject(NULL_PEN));

    IntPoint centerPoint = rectCenterPoint(rectWin);
    // Draw top left half
    RECT clipRect(rectWin);
    clipRect.right = centerPoint.x();
    clipRect.bottom = centerPoint.y();

    OwnPtr<HRGN> clipRgn = adoptPtr(CreateRectRgn(0, 0, 0, 0));
    bool needsNewClip = (GetClipRgn(dc, clipRgn.get()) <= 0);
    
    drawRoundCorner(needsNewClip, clipRect, rectWin, dc, stableRound(newTopLeft.width() * 2), stableRound(newTopLeft.height() * 2));

    // Draw top right
    clipRect = rectWin;
    clipRect.left = centerPoint.x();
    clipRect.bottom = centerPoint.y();

    drawRoundCorner(needsNewClip, clipRect, rectWin, dc, stableRound(newTopRight.width() * 2), stableRound(newTopRight.height() * 2));

     // Draw bottom left
    clipRect = rectWin;
    clipRect.right = centerPoint.x();
    clipRect.top = centerPoint.y();

    drawRoundCorner(needsNewClip, clipRect, rectWin, dc, stableRound(newBottomLeft.width() * 2), stableRound(newBottomLeft.height() * 2));

    // Draw bottom right
    clipRect = rectWin;
    clipRect.left = centerPoint.x();
    clipRect.top = centerPoint.y();

    drawRoundCorner(needsNewClip, clipRect, rectWin, dc, stableRound(newBottomRight.width() * 2), stableRound(newBottomRight.height() * 2));

    SelectObject(dc, oldBrush);
}


void GraphicsContext::drawRoundCorner(bool needsNewClip, RECT clipRect, RECT rectWin, HDC dc, int width, int height)
{
    if (!dc)
        return;

    OwnPtr<HRGN> clipRgn = adoptPtr(CreateRectRgn(0, 0, 0, 0));
    if (needsNewClip) {
        clipRgn = adoptPtr(CreateRectRgn(clipRect.left, clipRect.top, clipRect.right, clipRect.bottom));
        SelectClipRgn(dc, clipRgn.get());
    } else
        IntersectClipRect(dc, clipRect.left, clipRect.top, clipRect.right, clipRect.bottom);

    ::RoundRect(dc, rectWin.left , rectWin.top , rectWin.right , rectWin.bottom , width, height);

    SelectClipRgn(dc, needsNewClip ? 0 : clipRgn.get());
}


FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& frect, RoundingMode)
{
    notImplemented();
    return frect;
}

Color gradientAverageColor(const Gradient* gradient)
{
    const Vector<Gradient::ColorStop>& stops = gradient->getStops();
    if (stops.isEmpty())
        return Color();

    const Gradient::ColorStop& stop = stops.first();
    if (stops.size() == 1)
        return Color(stop.red, stop.green, stop.blue, stop.alpha);

    const Gradient::ColorStop& lastStop = stops.last();
    return Color((stop.red + lastStop.red) * 0.5f
        , (stop.green + lastStop.green) * 0.5f
        , (stop.blue + lastStop.blue) * 0.5f
        , (stop.alpha + lastStop.alpha) * 0.5f);
}

void GraphicsContext::fillPath(const Path& path)
{
    if (path.isNull())
        return;

    Color c = m_state.fillGradient
        ? gradientAverageColor(m_state.fillGradient.get())
        : fillColor();

    if (!c.alpha() || !m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    OwnPtr<HBRUSH> brush = createBrush(c);

    if (m_data->m_opacity < 1.0f || m_data->hasAlpha()) {
        IntRect trRect = enclosingIntRect(m_data->mapRect(path.boundingRect()));
        trRect.inflate(1);
        TransparentLayerDC transparentDC(m_data, trRect);
        HDC dc = transparentDC.hdc();
        if (!dc)
            return;

        AffineTransform tr = m_data->m_transform;
        tr.translate(transparentDC.toShift().width(), transparentDC.toShift().height());

        SelectObject(dc, GetStockObject(NULL_PEN));
        HGDIOBJ oldBrush = SelectObject(dc, brush.get());
        path.platformPath()->fillPath(dc, &tr);
        SelectObject(dc, oldBrush);
    } else {
        SelectObject(m_data->m_dc, GetStockObject(NULL_PEN));
        HGDIOBJ oldBrush = SelectObject(m_data->m_dc, brush.get());
        path.platformPath()->fillPath(m_data->m_dc, &m_data->m_transform);
        SelectObject(m_data->m_dc, oldBrush);
    }
}


void GraphicsContext::strokePath(const Path& path)
{
    if (path.isNull() || !m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    OwnPtr<HPEN> pen = createPen(strokeColor(), strokeThickness(), strokeStyle());

    if (m_data->m_opacity < 1.0f || m_data->hasAlpha()) {
        IntRect trRect = enclosingIntRect(m_data->mapRect(path.boundingRect()));
        trRect.inflate(1);
        TransparentLayerDC transparentDC(m_data, trRect);
        HDC dc = transparentDC.hdc();
        if (!dc)
            return;

        AffineTransform tr = m_data->m_transform;
        tr.translate(transparentDC.toShift().width(), transparentDC.toShift().height());

        SelectObject(dc, GetStockObject(NULL_BRUSH));
        HGDIOBJ oldPen = SelectObject(dc, pen.get());
        path.platformPath()->strokePath(dc, &tr);
        SelectObject(dc, oldPen);
    } else {
        SelectObject(m_data->m_dc, GetStockObject(NULL_BRUSH));
        HGDIOBJ oldPen = SelectObject(m_data->m_dc, pen.get());
        path.platformPath()->strokePath(m_data->m_dc, &m_data->m_transform);
        SelectObject(m_data->m_dc, oldPen);
    }
}

void GraphicsContext::fillRect(const FloatRect& r, const Gradient* gradient)
{
    if (!m_data->m_opacity)
        return;

    const Vector<Gradient::ColorStop>& stops = gradient->getStops();
    if (stops.isEmpty())
        return;

    size_t numStops = stops.size();
    if (numStops == 1) {
        const Gradient::ColorStop& stop = stops.first();
        Color color(stop.red, stop.green, stop.blue, stop.alpha);
        fillRect(r, color, ColorSpaceDeviceRGB);
        return;
    } 
    
    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect intRect = enclosingIntRect(r);
    IntRect rect = m_data->mapRect(intRect);
    TransparentLayerDC transparentDC(m_data, rect, &intRect, 255, true);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;

    rect.move(transparentDC.toShift());
    FloatPoint fp0 = m_data->mapPoint(gradient->p0());
    FloatPoint fp1 = m_data->mapPoint(gradient->p1());
    IntPoint p0(stableRound(fp0.x()), stableRound(fp0.y()));
    IntPoint p1(stableRound(fp1.x()), stableRound(fp1.y()));
    p0 += transparentDC.toShift();
    p1 += transparentDC.toShift();

    if (gradient->isRadial()) {
        if (g_radialGradientFiller) {
            // FIXME: don't support 2D scaling at this time
            double scale = (m_data->m_transform.a() + m_data->m_transform.d()) * 0.5;
            float r0 = gradient->startRadius() * scale;
            float r1 = gradient->endRadius() * scale;
            g_radialGradientFiller(dc, rect, p0, p1, r0, r1, gradient->getStops());
            return;
        }
    } else if (g_linearGradientFiller) {
        g_linearGradientFiller(dc, rect, p0, p1, gradient->getStops());
        return;
    }

    // Simple 1D linear solution that assumes p0 is on the top or left side, and p1 is on the right or bottom side
    size_t numRects = (numStops - 1);
    Vector<TRIVERTEX, 20> tv;
    tv.resize(numRects * 2);
    Vector<GRADIENT_RECT, 10> mesh;
    mesh.resize(numRects);
    int x = rect.x();
    int y = rect.y();
    int width = rect.width();
    int height = rect.height();
    FloatSize d = gradient->p1() - gradient->p0();
    bool vertical = fabs(d.height()) > fabs(d.width());
    for (size_t i = 0; i < numStops; ++i) {
        const Gradient::ColorStop& stop = stops[i];
        int iTv = i ? 2 * i - 1 : 0;
        tv[iTv].Red = stop.red * 0xFFFF;
        tv[iTv].Green = stop.green * 0xFFFF;
        tv[iTv].Blue = stop.blue * 0xFFFF;
        tv[iTv].Alpha = stop.alpha * 0xFFFF;
        if (i) {
            tv[iTv].x = vertical ? x + width: x + width * stop.stop;
            tv[iTv].y = vertical ? y + height * stop.stop : y + height;
            mesh[i - 1].UpperLeft = iTv - 1;
            mesh[i - 1].LowerRight = iTv;
        } else {
            tv[iTv].x = x;
            tv[iTv].y = y;
        }

        if (i && i < numRects) {
            tv[iTv + 1] = tv[iTv];
            if (vertical)
                tv[iTv + 1].x = x;
            else
                tv[iTv + 1].y = y;
        }
    }

    GradientFill(dc, tv.data(), tv.size(), mesh.data(), mesh.size(), vertical ? GRADIENT_FILL_RECT_V : GRADIENT_FILL_RECT_H);
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    return m_data->m_transform;
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    savePlatformState();

    if (m_state.fillGradient)
        fillRect(rect, m_state.fillGradient.get());
    else
        fillRect(rect, fillColor(), ColorSpaceDeviceRGB);

    restorePlatformState();
}

void GraphicsContext::setPlatformShadow(const FloatSize&, float, const Color&, ColorSpace)
{
    notImplemented();
}

void GraphicsContext::clearPlatformShadow()
{
    notImplemented();
}

InterpolationQuality GraphicsContext::imageInterpolationQuality() const
{
    notImplemented();
    return InterpolationDefault;
}

void GraphicsContext::setImageInterpolationQuality(InterpolationQuality)
{
    notImplemented();
}

static inline bool isCharVisible(UChar c)
{
    return c && c != zeroWidthSpace;
}

void GraphicsContext::drawText(const Font& font, const TextRun& run, const FloatPoint& point, int from, int to)
{
    if (paintingDisabled() || !fillColor().alpha() || !m_data->m_opacity)
        return;

    bool mustSupportAlpha = m_data->hasAlpha();

    if (!mustSupportAlpha && fillColor().alpha() == 0xFF && m_data->m_opacity >= 1.0) {
        font.drawText(this, run, point, from, to);
        return;
    }

    float oldOpacity = m_data->m_opacity;
    m_data->m_opacity *= fillColor().alpha() / 255.0;

    FloatRect textRect = font.selectionRectForText(run, point, font.fontMetrics().height(), from, to);
    textRect.setY(textRect.y() - font.fontMetrics().ascent());
    IntRect trRect = enclosingIntRect(m_data->mapRect(textRect));
    RECT bmpRect;
    AlphaPaintType alphaPaintType = mustSupportAlpha ? AlphaPaintOther : AlphaPaintNone;
    if (RefPtr<SharedBitmap> bmp = m_data->getTransparentLayerBitmap(trRect, alphaPaintType, bmpRect, true, mustSupportAlpha)) {
        {
            GraphicsContext gc(0);
            gc.setBitmap(bmp);
            gc.scale(FloatSize(m_data->m_transform.a(), m_data->m_transform.d()));
            font.drawText(&gc, run, IntPoint(0, font.fontMetrics().ascent()), from, to);
        }
        unsigned key1;
        HDC memDC = bmp->getDC(&key1);
        if (memDC) {
            m_data->paintBackTransparentLayerBitmap(memDC, bmp.get(), trRect, alphaPaintType, bmpRect);
            bmp->releaseDC(memDC, key1);
        }
    }

    m_data->m_opacity = oldOpacity;
}

void GraphicsContext::drawText(const SimpleFontData* fontData, const GlyphBuffer& glyphBuffer,
                      int from, int numGlyphs, const FloatPoint& point)
{
    if (!m_data->m_opacity)
        return;

    for (;;) {
        if (!numGlyphs)
            return;
        if (isCharVisible(*glyphBuffer.glyphs(from)))
            break;
        ++from;
        --numGlyphs;
    }

    double scaleX = m_data->m_transform.a();
    double scaleY = m_data->m_transform.d();

    int height = fontData->platformData().size() * scaleY;
    int width = fontData->avgCharWidth() * scaleX;

    if (!height || !width)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    HFONT hFont = height > 1
        ? fontData->platformData().getScaledFontHandle(height, scaleX == scaleY ? 0 : width)
        : 0;

    FloatPoint startPoint(point.x(), point.y() - fontData->fontMetrics().ascent());
    FloatPoint trPoint = m_data->mapPoint(startPoint);
    int y = stableRound(trPoint.y());

    Color color = fillColor();
    if (!color.alpha())
        return;

    COLORREF fontColor = RGB(color.red(), color.green(), color.blue());

    if (!hFont) {
        double offset = trPoint.x();
        const GlyphBufferAdvance* advance = glyphBuffer.advances(from);
        if (scaleX == 1.)
            for (int i = 1; i < numGlyphs; ++i)
                offset += (*advance++).width();
        else
            for (int i = 1; i < numGlyphs; ++i)
                offset += (*advance++).width() * scaleX;

        offset += width;

        OwnPtr<HPEN> hPen = adoptPtr(CreatePen(PS_DASH, 1, fontColor));
        HGDIOBJ oldPen = SelectObject(m_data->m_dc, hPen.get());

        MoveToEx(m_data->m_dc, stableRound(trPoint.x()), y, 0);
        LineTo(m_data->m_dc, stableRound(offset), y);

        SelectObject(m_data->m_dc, oldPen);
        return;
    }

    FloatSize shadowOffset;
    float shadowBlur = 0;
    Color shadowColor;
    ColorSpace shadowColorSpace;
    bool hasShadow = textDrawingMode() == TextModeFill
        && getShadow(shadowOffset, shadowBlur, shadowColor, shadowColorSpace)
        && shadowColor.alpha();
    COLORREF shadowRGBColor;
    FloatPoint trShadowPoint;
    if (hasShadow) {
        shadowRGBColor = RGB(shadowColor.red(), shadowColor.green(), shadowColor.blue());
        trShadowPoint = m_data->mapPoint(startPoint + shadowOffset);
    }

    HGDIOBJ hOldFont = SelectObject(m_data->m_dc, hFont);
    COLORREF oldTextColor = GetTextColor(m_data->m_dc);
    int oldTextAlign = GetTextAlign(m_data->m_dc);
    SetTextAlign(m_data->m_dc, 0);

    int oldBkMode = GetBkMode(m_data->m_dc);
    SetBkMode(m_data->m_dc, TRANSPARENT);

    if (numGlyphs > 1) {
        double offset = trPoint.x();
        Vector<int, 256> glyphSpace(numGlyphs);
        Vector<UChar, 256> text(numGlyphs);
        int* curSpace = glyphSpace.data();
        UChar* curChar = text.data();
        const UChar* srcChar = glyphBuffer.glyphs(from);
        const UChar* const srcCharEnd = srcChar + numGlyphs;
        *curChar++ = *srcChar++;
        int firstOffset = stableRound(offset);
        int lastOffset = firstOffset;
        const GlyphBufferAdvance* advance = glyphBuffer.advances(from);
        // FIXME: ExtTextOut() can flip over each word for RTL languages, even when TA_RTLREADING is off.
        // (this can be GDI bug or font driver bug?)
        // We are not clear how it processes characters and handles specified spaces. On the other side,
        // our glyph buffer is already in the correct order for rendering. So, the solution is that we
        // call ExtTextOut() for each single character when the text contains any RTL character.
        // This solution is not perfect as it is slower than calling ExtTextOut() one time for all characters.
        // Drawing characters one by one may be too slow.
        bool drawOneByOne = false;
        if (scaleX == 1.) {
            for (; srcChar < srcCharEnd; ++srcChar) {
                offset += (*advance++).width();
                int offsetInt = stableRound(offset);
                if (isCharVisible(*srcChar)) {
                    if (!drawOneByOne && WTF::Unicode::direction(*srcChar) == WTF::Unicode::RightToLeft)
                        drawOneByOne = true;
                    *curChar++ = *srcChar;
                    *curSpace++ = offsetInt - lastOffset;
                    lastOffset = offsetInt;
                }
            }
        } else {
            for (; srcChar < srcCharEnd; ++srcChar) {
                offset += (*advance++).width() * scaleX;
                int offsetInt = stableRound(offset);
                if (isCharVisible(*srcChar)) {
                    if (!drawOneByOne && WTF::Unicode::direction(*srcChar) == WTF::Unicode::RightToLeft)
                        drawOneByOne = true;
                    *curChar++ = *srcChar;
                    *curSpace++ = offsetInt - lastOffset;
                    lastOffset = offsetInt;
                }
            }
        }
        numGlyphs = curChar - text.data();
        if (hasShadow) {
            SetTextColor(m_data->m_dc, shadowRGBColor);
            if (drawOneByOne) {
                int xShadow = firstOffset + stableRound(trShadowPoint.x() - trPoint.x());
                int yShadow = stableRound(trShadowPoint.y());
                for (int i = 0; i < numGlyphs; ++i) {
                    ExtTextOut(m_data->m_dc, xShadow, yShadow, 0, NULL, text.data() + i, 1, 0);
                    xShadow += glyphSpace[i];
                }
            } else
                ExtTextOut(m_data->m_dc, firstOffset + stableRound(trShadowPoint.x() - trPoint.x()), stableRound(trShadowPoint.y()), 0, NULL, text.data(), numGlyphs, glyphSpace.data());
        }
        SetTextColor(m_data->m_dc, fontColor);
        if (drawOneByOne) {
            int x = firstOffset;
            for (int i = 0; i < numGlyphs; ++i) {
                ExtTextOut(m_data->m_dc, x, y, 0, NULL, text.data() + i, 1, 0);
                x += glyphSpace[i];
            }
        } else
            ExtTextOut(m_data->m_dc, firstOffset, y, 0, NULL, text.data(), numGlyphs, glyphSpace.data());
    } else {
        UChar c = *glyphBuffer.glyphs(from);
        if (hasShadow) {
            SetTextColor(m_data->m_dc, shadowRGBColor);
            ExtTextOut(m_data->m_dc, stableRound(trShadowPoint.x()), stableRound(trShadowPoint.y()), 0, NULL, &c, 1, 0);
        }
        SetTextColor(m_data->m_dc, fontColor);
        ExtTextOut(m_data->m_dc, stableRound(trPoint.x()), y, 0, NULL, &c, 1, 0);
    }

    SetTextAlign(m_data->m_dc, oldTextAlign);
    SetTextColor(m_data->m_dc, oldTextColor);
    SetBkMode(m_data->m_dc, oldBkMode);
    SelectObject(m_data->m_dc, hOldFont);
}

void GraphicsContext::drawFrameControl(const IntRect& rect, unsigned type, unsigned state)
{
    if (!m_data->m_opacity)
        return;

    const int boxWidthBest = 8;
    const int boxHeightBest = 8;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect trRect = m_data->mapRect(rect);
    TransparentLayerDC transparentDC(m_data, trRect, &rect, 255, true);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    RECT rectWin = trRect;

    if ((rectWin.right - rectWin.left) < boxWidthBest) {
        RefPtr<SharedBitmap> bmp = SharedBitmap::create(IntSize(boxWidthBest, boxHeightBest), BitmapInfo::BitCount16, true);
        SharedBitmap::DCHolder memDC(bmp.get());
        if (memDC.get()) {
            RECT tempRect = {0, 0, boxWidthBest, boxHeightBest};
            DrawFrameControl(memDC.get(), &tempRect, type, state);

            ::StretchBlt(dc, rectWin.left, rectWin.top, rectWin.right - rectWin.left, rectWin.bottom - rectWin.top, memDC.get(), 0, 0, boxWidthBest, boxHeightBest, SRCCOPY);
            return;
        }
    }

    DrawFrameControl(dc, &rectWin, type, state);
}

void GraphicsContext::drawFocusRect(const IntRect& rect)
{
    if (!m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect trRect = m_data->mapRect(rect);
    TransparentLayerDC transparentDC(m_data, trRect, &rect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    RECT rectWin = trRect;
    DrawFocusRect(dc, &rectWin);
}

void GraphicsContext::paintTextField(const IntRect& rect, unsigned state)
{
    if (!m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect trRect = m_data->mapRect(rect);
    TransparentLayerDC transparentDC(m_data, trRect, &rect);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());

    RECT rectWin = trRect;
    DrawEdge(dc, &rectWin, EDGE_ETCHED, BF_RECT | BF_ADJUST);
    FillRect(dc, &rectWin, reinterpret_cast<HBRUSH>(((state & DFCS_INACTIVE) ? COLOR_BTNFACE : COLOR_WINDOW) + 1));
}

void GraphicsContext::drawBitmap(SharedBitmap* bmp, const IntRect& dstRectIn, const IntRect& srcRect, ColorSpace styleColorSpace, CompositeOperator compositeOp, BlendMode blendMode)
{
    if (!m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect dstRect = m_data->mapRect(dstRectIn);
    TransparentLayerDC transparentDC(m_data, dstRect, &dstRectIn, 255, true);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    dstRect.move(transparentDC.toShift());

    bmp->draw(dc, dstRect, srcRect, compositeOp, blendMode);

    if (bmp->is16bit())
        transparentDC.fillAlphaChannel();
}

void GraphicsContext::drawBitmapPattern(SharedBitmap* bmp, const FloatRect& tileRectIn, const AffineTransform& patternTransform,
                const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator op, const FloatRect& destRectIn, const IntSize& origSourceSize)
{
    if (!m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect intDstRect = enclosingIntRect(destRectIn);
    IntRect trRect = m_data->mapRect(intDstRect);
    TransparentLayerDC transparentDC(m_data, trRect, &intDstRect, 255, true);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    trRect.move(transparentDC.toShift());
    FloatRect movedDstRect = m_data->m_transform.inverse().mapRect(FloatRect(trRect));
    FloatSize moved(movedDstRect.location() - destRectIn.location());
    AffineTransform transform = m_data->m_transform;
    transform.translate(moved.width(), moved.height());

    bmp->drawPattern(dc, transform, tileRectIn, patternTransform, phase, styleColorSpace, op, destRectIn, origSourceSize);

    if (!bmp->hasAlpha())
        transparentDC.fillAlphaChannel();
}

void GraphicsContext::drawIcon(HICON icon, const IntRect& dstRectIn, UINT flags)
{
    if (!m_data->m_opacity)
        return;

    ScopeDCProvider dcProvider(m_data);
    if (!m_data->m_dc)
        return;

    IntRect dstRect = m_data->mapRect(dstRectIn);
    TransparentLayerDC transparentDC(m_data, dstRect, &dstRectIn, 255, true);
    HDC dc = transparentDC.hdc();
    if (!dc)
        return;
    dstRect.move(transparentDC.toShift());

    DrawIconEx(dc, dstRect.x(), dstRect.y(), icon, dstRect.width(), dstRect.height(), 0, NULL, flags);
}

void GraphicsContext::setPlatformShouldAntialias(bool)
{
    notImplemented();
}

void GraphicsContext::setLineDash(const DashArray&, float)
{
    notImplemented();
}

void GraphicsContext::clipPath(const Path&, WindRule)
{
    notImplemented();
}

} // namespace WebCore
