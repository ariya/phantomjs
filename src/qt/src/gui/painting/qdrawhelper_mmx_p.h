/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDRAWHELPER_MMX_P_H
#define QDRAWHELPER_MMX_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <private/qdrawhelper_p.h>
#include <private/qdrawhelper_x86_p.h>
#include <private/qpaintengine_raster_p.h>

#ifdef QT_HAVE_MMX
#include <mmintrin.h>
#endif

#define C_FF const m64 mmx_0x00ff = _mm_set1_pi16(0xff)
#define C_80 const m64 mmx_0x0080 = _mm_set1_pi16(0x80)
#define C_00 const m64 mmx_0x0000 = _mm_setzero_si64()

#ifdef Q_CC_MSVC
#  pragma warning(disable: 4799) // No EMMS at end of function
#endif

typedef __m64 m64;

QT_BEGIN_NAMESPACE

struct QMMXCommonIntrinsics
{
    static inline m64 alpha(m64 x) {
        x = _mm_unpackhi_pi16(x, x);
        x = _mm_unpackhi_pi16(x, x);
        return x;
    }

    static inline m64 _negate(const m64 &x, const m64 &mmx_0x00ff) {
        return _mm_xor_si64(x, mmx_0x00ff);
    }

    static inline m64 add(const m64 &a, const m64 &b) {
        return  _mm_adds_pu16 (a, b);
    }

    static inline m64 _byte_mul(const m64 &a, const m64 &b,
                                const m64 &mmx_0x0080)
    {
        m64 res = _mm_mullo_pi16(a, b);
        res = _mm_adds_pu16(res, mmx_0x0080);
        res = _mm_adds_pu16(res, _mm_srli_pi16 (res, 8));
        return _mm_srli_pi16(res, 8);
    }

    static inline m64 interpolate_pixel_256(const m64 &x, const m64 &a,
                                           const m64 &y, const m64 &b)
    {
        m64 res = _mm_adds_pu16(_mm_mullo_pi16(x, a), _mm_mullo_pi16(y, b));
        return _mm_srli_pi16(res, 8);
    }

    static inline m64 _interpolate_pixel_255(const m64 &x, const m64 &a,
                                             const m64 &y, const m64 &b,
                                             const m64 &mmx_0x0080)
    {
        m64 res = _mm_adds_pu16(_mm_mullo_pi16(x, a), _mm_mullo_pi16(y, b));
        res = _mm_adds_pu16(res, mmx_0x0080);
        res = _mm_adds_pu16(res, _mm_srli_pi16 (res, 8));
        return _mm_srli_pi16(res, 8);
    }

    static inline m64 _premul(m64 x, const m64 &mmx_0x0080) {
        m64 a = alpha(x);
        return _byte_mul(x, a, mmx_0x0080);
    }

    static inline m64 _load(uint x, const m64 &mmx_0x0000) {
        return _mm_unpacklo_pi8(_mm_cvtsi32_si64(x), mmx_0x0000);
    }

    static inline m64 _load_alpha(uint x, const m64 &) {
        x |= (x << 16);
        return _mm_set1_pi32(x);
    }

    static inline uint _store(const m64 &x, const m64 &mmx_0x0000) {
        return _mm_cvtsi64_si32(_mm_packs_pu16(x, mmx_0x0000));
    }
};

#define negate(x) _negate(x, mmx_0x00ff)
#define byte_mul(a, b) _byte_mul(a, b, mmx_0x0080)
#define interpolate_pixel_255(x, a, y, b) _interpolate_pixel_255(x, a, y, b, mmx_0x0080)
#define premul(x) _premul(x, mmx_0x0080)
#define load(x) _load(x, mmx_0x0000)
#define load_alpha(x) _load_alpha(x, mmx_0x0000)
#define store(x) _store(x, mmx_0x0000)

/*
  result = 0
  d = d * cia
*/
#define comp_func_Clear_impl(dest, length, const_alpha)\
{\
    if (const_alpha == 255) {\
        qt_memfill(static_cast<quint32*>(dest), quint32(0), length);\
    } else {\
        C_FF; C_80; C_00;\
        m64 ia = MM::negate(MM::load_alpha(const_alpha));\
        for (int i = 0; i < length; ++i) {\
            dest[i] = MM::store(MM::byte_mul(MM::load(dest[i]), ia));\
        }\
        MM::end();\
    }\
}

template <class MM>
static void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

template <class MM>
static void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

/*
  result = s
  dest = s * ca + d * cia
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint src, uint const_alpha)
{
    if (const_alpha == 255) {
        qt_memfill(static_cast<quint32*>(dest), quint32(src), length);
    } else {
        C_FF; C_80; C_00;
        const m64 a = MM::load_alpha(const_alpha);
        const m64 ia = MM::negate(a);
        const m64 s = MM::byte_mul(MM::load(src), a);
        for (int i = 0; i < length; ++i) {
            dest[i] = MM::store(MM::add(s, MM::byte_mul(MM::load(dest[i]), ia)));
        }
        MM::end();
    }
}

template <class MM>
static void QT_FASTCALL comp_func_Source(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        C_FF; C_80; C_00;
        const m64 a = MM::load_alpha(const_alpha);
        const m64 ia = MM::negate(a);
        for (int i = 0; i < length; ++i)
            dest[i] = MM::store(MM::interpolate_pixel_255(MM::load(src[i]), a,
                                                        MM::load(dest[i]), ia));
    }
    MM::end();
}

/*
  result = s + d * sia
  dest = (s + d * sia) * ca + d * cia
       = s * ca + d * (sia * ca + cia)
       = s * ca + d * (1 - sa*ca)
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint src, uint const_alpha)
{
    if ((const_alpha & qAlpha(src)) == 255) {
        qt_memfill(static_cast<quint32*>(dest), quint32(src), length);
    } else {
        C_FF; C_80; C_00;
        m64 s = MM::load(src);
        if (const_alpha != 255) {
            m64 ca = MM::load_alpha(const_alpha);
            s = MM::byte_mul(s, ca);
        }
        m64 a = MM::negate(MM::alpha(s));
        for (int i = 0; i < length; ++i)
            dest[i] = MM::store(MM::add(s, MM::byte_mul(MM::load(dest[i]), a)));
        MM::end();
    }
}

template <class MM>
static void QT_FASTCALL comp_func_SourceOver(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            const uint alphaMaskedSource = 0xff000000 & src[i];
            if (alphaMaskedSource == 0)
                continue;
            if (alphaMaskedSource == 0xff000000) {
                dest[i] = src[i];
            } else {
                m64 s = MM::load(src[i]);
                m64 ia = MM::negate(MM::alpha(s));
                dest[i] = MM::store(MM::add(s, MM::byte_mul(MM::load(dest[i]), ia)));
            }
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        for (int i = 0; i < length; ++i) {
            if ((0xff000000 & src[i]) == 0)
                continue;
            m64 s = MM::byte_mul(MM::load(src[i]), ca);
            m64 ia = MM::negate(MM::alpha(s));
            dest[i] = MM::store(MM::add(s, MM::byte_mul(MM::load(dest[i]), ia)));
        }
    }
    MM::end();
}

/*
  result = d + s * dia
  dest = (d + s * dia) * ca + d * cia
       = d + s * dia * ca
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint src, uint const_alpha)
{
    C_FF; C_80; C_00;
    m64 s = MM::load(src);
    if (const_alpha != 255)
        s = MM::byte_mul(s, MM::load_alpha(const_alpha));

    for (int i = 0; i < length; ++i) {
        m64 d = MM::load(dest[i]);
        m64 dia = MM::negate(MM::alpha(d));
        dest[i] = MM::store(MM::add(d, MM::byte_mul(s, dia)));
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_DestinationOver(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            m64 ia = MM::negate(MM::alpha(d));
            dest[i] = MM::store(MM::add(d, MM::byte_mul(MM::load(src[i]), ia)));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            m64 dia = MM::negate(MM::alpha(d));
            dia = MM::byte_mul(dia, ca);
            dest[i] = MM::store(MM::add(d, MM::byte_mul(MM::load(src[i]), dia)));
        }
    }
    MM::end();
}

/*
  result = s * da
  dest = s * da * ca + d * cia
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint src, uint const_alpha)
{
    C_80; C_00;
    if (const_alpha == 255) {
        m64 s = MM::load(src);
        for (int i = 0; i < length; ++i) {
            m64 da = MM::alpha(MM::load(dest[i]));
            dest[i] = MM::store(MM::byte_mul(s, da));
        }
    } else {
        C_FF;
        m64 s = MM::load(src);
        m64 ca = MM::load_alpha(const_alpha);
        s = MM::byte_mul(s, ca);
        m64 cia = MM::negate(ca);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::alpha(d), d, cia));
        }
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_SourceIn(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 a = MM::alpha(MM::load(dest[i]));
            dest[i] = MM::store(MM::byte_mul(MM::load(src[i]), a));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        m64 cia = MM::negate(ca);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            m64 da = MM::byte_mul(MM::alpha(d), ca);
            dest[i] = MM::store(MM::interpolate_pixel_255(
                                   MM::load(src[i]), da, d, cia));
        }
    }
    MM::end();
}

/*
  result = d * sa
  dest = d * sa * ca + d * cia
       = d * (sa * ca + cia)
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint src, uint const_alpha)
{
    C_80; C_00;
    m64 a = MM::alpha(MM::load(src));
    if (const_alpha != 255) {
        C_FF;
        m64 ca = MM::load_alpha(const_alpha);
        m64 cia = MM::negate(ca);
        a = MM::byte_mul(a, ca);
        a = MM::add(a, cia);
    }
    for (int i = 0; i < length; ++i)
        dest[i] = MM::store(MM::byte_mul(MM::load(dest[i]), a));
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_DestinationIn(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 a = MM::alpha(MM::load(src[i]));
            dest[i] = MM::store(MM::byte_mul(MM::load(dest[i]), a));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        m64 cia = MM::negate(ca);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            m64 a = MM::alpha(MM::load(src[i]));
            a = MM::byte_mul(a, ca);
            a = MM::add(a, cia);
            dest[i] = MM::store(MM::byte_mul(d, a));
        }
    }
    MM::end();
}

/*
  result = s * dia
  dest = s * dia * ca + d * cia
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint src, uint const_alpha)
{
    C_FF; C_80; C_00;
    m64 s = MM::load(src);
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 dia = MM::negate(MM::alpha(MM::load(dest[i])));
            dest[i] = MM::store(MM::byte_mul(s, dia));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        m64 cia = MM::negate(ca);
        s = MM::byte_mul(s, ca);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::negate(MM::alpha(d)), d, cia));
        }
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_SourceOut(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 ia = MM::negate(MM::alpha(MM::load(dest[i])));
            dest[i] = MM::store(MM::byte_mul(MM::load(src[i]), ia));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        m64 cia = MM::negate(ca);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            m64 dia = MM::byte_mul(MM::negate(MM::alpha(d)), ca);
            dest[i] = MM::store(MM::interpolate_pixel_255(MM::load(src[i]), dia, d, cia));
        }
    }
    MM::end();
}

/*
  result = d * sia
  dest = d * sia * ca + d * cia
       = d * (sia * ca + cia)
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint src, uint const_alpha)
{
    C_FF; C_80; C_00;
    m64 a = MM::negate(MM::alpha(MM::load(src)));
    if (const_alpha != 255) {
        m64 ca = MM::load_alpha(const_alpha);
        a = MM::byte_mul(a, ca);
        a = MM::add(a, MM::negate(ca));
    }
    for (int i = 0; i < length; ++i)
        dest[i] = MM::store(MM::byte_mul(MM::load(dest[i]), a));
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_DestinationOut(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 a = MM::negate(MM::alpha(MM::load(src[i])));
            dest[i] = MM::store(MM::byte_mul(MM::load(dest[i]), a));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        m64 cia = MM::negate(ca);
        for (int i = 0; i < length; ++i) {
            m64 d = MM::load(dest[i]);
            m64 a = MM::negate(MM::alpha(MM::load(src[i])));
            a = MM::byte_mul(a, ca);
            a = MM::add(a, cia);
            dest[i] = MM::store(MM::byte_mul(d, a));
        }
    }
    MM::end();
}

/*
  result = s*da + d*sia
  dest = s*da*ca + d*sia*ca + d *cia
       = s*ca * da + d * (sia*ca + cia)
       = s*ca * da + d * (1 - sa*ca)
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint src, uint const_alpha)
{
    C_FF; C_80; C_00;
    m64 s = MM::load(src);
    if (const_alpha != 255) {
        m64 ca = MM::load_alpha(const_alpha);
        s = MM::byte_mul(s, ca);
    }
    m64 a = MM::negate(MM::alpha(s));
    for (int i = 0; i < length; ++i) {
        m64 d = MM::load(dest[i]);
        dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::alpha(d), d, a));
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_SourceAtop(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 s = MM::load(src[i]);
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::alpha(d), d,
                                                        MM::negate(MM::alpha(s))));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        for (int i = 0; i < length; ++i) {
            m64 s = MM::load(src[i]);
            s = MM::byte_mul(s, ca);
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::alpha(d), d,
                                                        MM::negate(MM::alpha(s))));
        }
    }
    MM::end();
}

/*
  result = d*sa + s*dia
  dest = d*sa*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sa*ca + cia)
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint src, uint const_alpha)
{
    C_FF; C_80; C_00;
    m64 s = MM::load(src);
    m64 a = MM::alpha(s);
    if (const_alpha != 255) {
        m64 ca = MM::load_alpha(const_alpha);
        s = MM::byte_mul(s, ca);
        a = MM::alpha(s);
        a = MM::add(a, MM::negate(ca));
    }
    for (int i = 0; i < length; ++i) {
        m64 d = MM::load(dest[i]);
        dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::negate(MM::alpha(d)), d, a));
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_DestinationAtop(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 s = MM::load(src[i]);
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(d, MM::alpha(s), s,
                                                        MM::negate(MM::alpha(d))));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        for (int i = 0; i < length; ++i) {
            m64 s = MM::load(src[i]);
            s = MM::byte_mul(s, ca);
            m64 d = MM::load(dest[i]);
            m64 a = MM::alpha(s);
            a = MM::add(a, MM::negate(ca));
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::negate(MM::alpha(d)),
                                                        d, a));
        }
    }
    MM::end();
}

/*
  result = d*sia + s*dia
  dest = d*sia*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sia*ca + cia)
       = s*ca * dia + d * (1 - sa*ca)
*/
template <class MM>
static void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint src, uint const_alpha)
{
    C_FF; C_80; C_00;
    m64 s = MM::load(src);
    if (const_alpha != 255) {
        m64 ca = MM::load_alpha(const_alpha);
        s = MM::byte_mul(s, ca);
    }
    m64 a = MM::negate(MM::alpha(s));
    for (int i = 0; i < length; ++i) {
        m64 d = MM::load(dest[i]);
        dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::negate(MM::alpha(d)),
                                                    d, a));
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL comp_func_XOR(uint *dest, const uint *src, int length, uint const_alpha)
{
    C_FF; C_80; C_00;
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            m64 s = MM::load(src[i]);
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::negate(MM::alpha(d)),
                                                        d, MM::negate(MM::alpha(s))));
        }
    } else {
        m64 ca = MM::load_alpha(const_alpha);
        for (int i = 0; i < length; ++i) {
            m64 s = MM::load(src[i]);
            s = MM::byte_mul(s, ca);
            m64 d = MM::load(dest[i]);
            dest[i] = MM::store(MM::interpolate_pixel_255(s, MM::negate(MM::alpha(d)),
                                                        d, MM::negate(MM::alpha(s))));
        }
    }
    MM::end();
}

template <class MM>
static void QT_FASTCALL rasterop_solid_SourceOrDestination(uint *dest,
                                                           int length,
                                                           uint color,
                                                           uint const_alpha)
{
    Q_UNUSED(const_alpha);

    if ((quintptr)(dest) & 0x7) {
        *dest++ |= color;
        --length;
    }

    const int length64 = length / 2;
    if (length64) {
        __m64 *dst64 = reinterpret_cast<__m64*>(dest);
        const __m64 color64 = _mm_set_pi32(color, color);

        int n = (length64 + 3) / 4;
        switch (length64 & 0x3) {
        case 0: do { *dst64 = _mm_or_si64(*dst64, color64); ++dst64;
        case 3:      *dst64 = _mm_or_si64(*dst64, color64); ++dst64;
        case 2:      *dst64 = _mm_or_si64(*dst64, color64); ++dst64;
        case 1:      *dst64 = _mm_or_si64(*dst64, color64); ++dst64;
        } while (--n > 0);
        }
    }

    if (length & 0x1) {
        dest[length - 1] |= color;
    }

    MM::end();
}

template <class MM>
static void QT_FASTCALL rasterop_solid_SourceAndDestination(uint *dest,
                                                            int length,
                                                            uint color,
                                                            uint const_alpha)
{
    Q_UNUSED(const_alpha);

    color |= 0xff000000;

    if ((quintptr)(dest) & 0x7) { // align
        *dest++ &= color;
        --length;
    }

    const int length64 = length / 2;
    if (length64) {
        __m64 *dst64 = reinterpret_cast<__m64*>(dest);
        const __m64 color64 = _mm_set_pi32(color, color);

        int n = (length64 + 3) / 4;
        switch (length64 & 0x3) {
        case 0: do { *dst64 = _mm_and_si64(*dst64, color64); ++dst64;
        case 3:      *dst64 = _mm_and_si64(*dst64, color64); ++dst64;
        case 2:      *dst64 = _mm_and_si64(*dst64, color64); ++dst64;
        case 1:      *dst64 = _mm_and_si64(*dst64, color64); ++dst64;
        } while (--n > 0);
        }
    }

    if (length & 0x1) {
        dest[length - 1] &= color;
    }

    MM::end();
}

template <class MM>
static void QT_FASTCALL rasterop_solid_SourceXorDestination(uint *dest,
                                                            int length,
                                                            uint color,
                                                            uint const_alpha)
{
    Q_UNUSED(const_alpha);

    color &= 0x00ffffff;

    if ((quintptr)(dest) & 0x7) {
        *dest++ ^= color;
        --length;
    }

    const int length64 = length / 2;
    if (length64) {
        __m64 *dst64 = reinterpret_cast<__m64*>(dest);
        const __m64 color64 = _mm_set_pi32(color, color);

        int n = (length64 + 3) / 4;
        switch (length64 & 0x3) {
        case 0: do { *dst64 = _mm_xor_si64(*dst64, color64); ++dst64;
        case 3:      *dst64 = _mm_xor_si64(*dst64, color64); ++dst64;
        case 2:      *dst64 = _mm_xor_si64(*dst64, color64); ++dst64;
        case 1:      *dst64 = _mm_xor_si64(*dst64, color64); ++dst64;
        } while (--n > 0);
        }
    }

    if (length & 0x1) {
        dest[length - 1] ^= color;
    }

    MM::end();
}

template <class MM>
static void QT_FASTCALL rasterop_solid_SourceAndNotDestination(uint *dest,
                                                               int length,
                                                               uint color,
                                                               uint const_alpha)
{

    Q_UNUSED(const_alpha);

    if ((quintptr)(dest) & 0x7) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
        --length;
    }

    const int length64 = length / 2;
    if (length64) {
        __m64 *dst64 = reinterpret_cast<__m64*>(dest);
        const __m64 color64 = _mm_set_pi32(color, color);
        const m64 mmx_0xff000000 = _mm_set1_pi32(0xff000000);
        __m64 tmp1, tmp2, tmp3, tmp4;

        int n = (length64 + 3) / 4;
        switch (length64 & 0x3) {
        case 0: do { tmp1 = _mm_andnot_si64(*dst64, color64);
                     *dst64++ = _mm_or_si64(tmp1, mmx_0xff000000);
        case 3:      tmp2 = _mm_andnot_si64(*dst64, color64);
                     *dst64++ = _mm_or_si64(tmp2, mmx_0xff000000);
        case 2:      tmp3 = _mm_andnot_si64(*dst64, color64);
                     *dst64++ = _mm_or_si64(tmp3, mmx_0xff000000);
        case 1:      tmp4 = _mm_andnot_si64(*dst64, color64);
                     *dst64++ = _mm_or_si64(tmp4, mmx_0xff000000);
        } while (--n > 0);
        }
    }

    if (length & 0x1) {
        dest[length - 1] = (color & ~(dest[length - 1])) | 0xff000000;
    }

    MM::end();
}

template <class MM>
static void QT_FASTCALL rasterop_solid_NotSourceAndNotDestination(uint *dest,
                                                                  int length,
                                                                  uint color,
                                                                  uint const_alpha)
{
    rasterop_solid_SourceAndNotDestination<MM>(dest, length,
                                               ~color, const_alpha);
}

template <class MM>
static void QT_FASTCALL rasterop_solid_NotSourceOrNotDestination(uint *dest,
                                                                 int length,
                                                                 uint color,
                                                                 uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--) {
        *dest = color | ~(*dest);
        ++dest;
    }
}

template <class MM>
static void QT_FASTCALL rasterop_solid_NotSourceXorDestination(uint *dest,
                                                               int length,
                                                               uint color,
                                                               uint const_alpha)
{
    rasterop_solid_SourceXorDestination<MM>(dest, length, ~color, const_alpha);
}

template <class MM>
static void QT_FASTCALL rasterop_solid_NotSource(uint *dest, int length,
                                                 uint color, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    qt_memfill((quint32*)dest, ~color | 0xff000000, length);
}

template <class MM>
static void QT_FASTCALL rasterop_solid_NotSourceAndDestination(uint *dest,
                                                               int length,
                                                               uint color,
                                                               uint const_alpha)
{
    rasterop_solid_SourceAndDestination<MM>(dest, length,
                                            ~color, const_alpha);
}

template <class MM>
static inline void qt_blend_color_argb_x86(int count, const QSpan *spans,
                                           void *userData,
                                           CompositionFunctionSolid *solidFunc)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->rasterBuffer->compositionMode == QPainter::CompositionMode_Source
        || (data->rasterBuffer->compositionMode == QPainter::CompositionMode_SourceOver
            && qAlpha(data->solid.color) == 255)) {
        // inline for performance
        C_FF; C_80; C_00;
        while (count--) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                qt_memfill(static_cast<quint32*>(target), quint32(data->solid.color), spans->len);
            } else {
                // dest = s * ca + d * (1 - sa*ca) --> dest = s * ca + d * (1-ca)
                m64 ca = MM::load_alpha(spans->coverage);
                m64 s = MM::byte_mul(MM::load(data->solid.color), ca);
                m64 ica = MM::negate(ca);
                for (int i = 0; i < spans->len; ++i)
                    target[i] = MM::store(MM::add(s, MM::byte_mul(MM::load(target[i]), ica)));
            }
            ++spans;
        }
        MM::end();
        return;
    }
    CompositionFunctionSolid func = solidFunc[data->rasterBuffer->compositionMode];
    while (count--) {
        uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
        func(target, spans->len, data->solid.color, spans->coverage);
        ++spans;
    }
}

#ifdef QT_HAVE_MMX
struct QMMXIntrinsics : public QMMXCommonIntrinsics
{
    static inline void end() {
#if !defined(Q_OS_WINCE) || defined(_X86_)
       _mm_empty();
#endif
    }
};
#endif // QT_HAVE_MMX

QT_END_NAMESPACE

#endif // QDRAWHELPER_MMX_P_H
