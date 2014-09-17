/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDRAWHELPER_SSE_P_H
#define QDRAWHELPER_SSE_P_H

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

#include <private/qdrawhelper_mmx_p.h>

#ifdef QT_HAVE_SSE

#ifdef QT_LINUXBASE
// this is an evil hack - the posix_memalign declaration in LSB
// is wrong - see http://bugs.linuxbase.org/show_bug.cgi?id=2431
#  define posix_memalign _lsb_hack_posix_memalign
#  include <xmmintrin.h>
#  undef posix_memalign
#else
#  include <xmmintrin.h>
#endif

QT_BEGIN_NAMESPACE

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) \
 (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))
#endif

struct QSSEIntrinsics : public QMMXIntrinsics
{
    static inline m64 alpha(m64 x) {
        return _mm_shuffle_pi16 (x, _MM_SHUFFLE(3, 3, 3, 3));
    }

    static inline m64 _load_alpha(uint x, const m64 &mmx_0x0000) {
        m64 t = _mm_unpacklo_pi8(_mm_cvtsi32_si64(x), mmx_0x0000);
        return _mm_shuffle_pi16 (t, _MM_SHUFFLE(0, 0, 0, 0));
    }
};

template <class MM>
inline void qt_memfill32_sse_template(quint32 *dest, quint32 value, int count)
{
    if (count < 7) {
        switch (count) {
        case 6: *dest++ = value;
        case 5: *dest++ = value;
        case 4: *dest++ = value;
        case 3: *dest++ = value;
        case 2: *dest++ = value;
        case 1: *dest   = value;
        }
        return;
    };

    __m64 *dst64 = reinterpret_cast<__m64*>(dest);
    const __m64 value64 = _mm_set_pi32(value, value);
    int count64 = count / 2;

    int n = (count64 + 3) / 4;
    switch (count64 & 0x3) {
    case 0: do { _mm_stream_pi(dst64++, value64);
    case 3:      _mm_stream_pi(dst64++, value64);
    case 2:      _mm_stream_pi(dst64++, value64);
    case 1:      _mm_stream_pi(dst64++, value64);
    } while (--n > 0);
    }

    if (count & 0x1)
        dest[count - 1] = value;

    MM::end();
}

template <class MM>
inline void qt_bitmapblit16_sse_template(QRasterBuffer *rasterBuffer,
                                         int x, int y,
                                         quint32 color,
                                         const uchar *src,
                                         int width, int height, int stride)
{
    const quint16 c = qt_colorConvert<quint16, quint32>(color, 0);
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint16);

    const __m64 c64 = _mm_set1_pi16(c);
#ifdef Q_CC_MSVC
#  pragma warning(disable: 4309) // truncation of constant value
#endif
    const __m64 maskmask1 = _mm_set_pi16(0x1010, 0x2020, 0x4040, 0x8080);
    const __m64 maskadd1 = _mm_set_pi16(0x7070, 0x6060, 0x4040, 0x0000);

    if (width > 4) {
        const __m64 maskmask2 = _mm_set_pi16(0x0101, 0x0202, 0x0404, 0x0808);
        const __m64 maskadd2 = _mm_set_pi16(0x7f7f, 0x7e7e, 0x7c7c, 0x7878);

        while (height--) {
            for (int x = 0; x < width; x += 8) {
                const quint8 s = src[x >> 3];
                if (!s)
                    continue;
                __m64 mask1 = _mm_set1_pi8(s);
                __m64 mask2 = mask1;
                mask1 = _m_pand(mask1, maskmask1);
                mask1 = _mm_add_pi16(mask1, maskadd1);
                _mm_maskmove_si64(c64, mask1, (char*)(dest + x));
                mask2 = _m_pand(mask2, maskmask2);
                mask2 = _mm_add_pi16(mask2, maskadd2);
                _mm_maskmove_si64(c64, mask2, (char*)(dest + x + 4));
            }
            dest += destStride;
            src += stride;
        }
    } else {
        while (height--) {
            const quint8 s = *src;
            if (s) {
                __m64 mask1 = _mm_set1_pi8(s);
                mask1 = _m_pand(mask1, maskmask1);
                mask1 = _mm_add_pi16(mask1, maskadd1);
                _mm_maskmove_si64(c64, mask1, (char*)(dest));
            }
            dest += destStride;
            src += stride;
        }
    }

    MM::end();
}

QT_END_NAMESPACE

#endif // QT_HAVE_SSE
#endif // QDRAWHELPER_SSE_P_H
