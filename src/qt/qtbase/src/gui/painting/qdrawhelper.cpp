/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <qglobal.h>
#ifdef Q_OS_IOS
// We don't build the NEON drawhelpers as they are implemented partly
// in GAS syntax assembly, which is not supported by the iOS toolchain.
#undef __ARM_NEON__
#endif

#include <qstylehints.h>
#include <qguiapplication.h>
#include <qatomic.h>
#include <private/qdrawhelper_p.h>
#include <private/qpaintengine_raster_p.h>
#include <private/qpainter_p.h>
#include <private/qdrawhelper_x86_p.h>
#include <private/qdrawhelper_neon_p.h>
#if defined(QT_COMPILER_SUPPORTS_MIPS_DSP) || defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
#include <private/qdrawhelper_mips_dsp_p.h>
#endif
#include <private/qmath_p.h>
#include <private/qguiapplication_p.h>
#include <qmath.h>

QT_BEGIN_NAMESPACE

#define MASK(src, a) src = BYTE_MUL(src, a)

/*
  constants and structures
*/

enum {
    fixed_scale = 1 << 16,
    half_point = 1 << 15
};

// must be multiple of 4 for easier SIMD implementations
static const int buffer_size = 2048;

#ifdef Q_COMPILER_CONSTEXPR

template<QImage::Format> Q_DECL_CONSTEXPR uint redWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint redShift();
template<QImage::Format> Q_DECL_CONSTEXPR uint greenWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint greenShift();
template<QImage::Format> Q_DECL_CONSTEXPR uint blueWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint blueShift();
template<QImage::Format> Q_DECL_CONSTEXPR uint alphaWidth();
template<QImage::Format> Q_DECL_CONSTEXPR uint alphaShift();

template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB16>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB8565_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint redWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB16>() { return  11; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB444>() { return  8; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB555>() { return 10; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB666>() { return 12; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_RGB888>() { return 16; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB4444_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB8555_Premultiplied>() { return 18; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB8565_Premultiplied>() { return 19; }
template<> Q_DECL_CONSTEXPR uint redShift<QImage::Format_ARGB6666_Premultiplied>() { return 12; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB16>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB8565_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB16>() { return  5; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB4444_Premultiplied>() { return  4; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB8555_Premultiplied>() { return 13; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB8565_Premultiplied>() { return 13; }
template<> Q_DECL_CONSTEXPR uint greenShift<QImage::Format_ARGB6666_Premultiplied>() { return  6; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB16>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB444>() { return 4; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB555>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB666>() { return 6; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_RGB888>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB4444_Premultiplied>() { return 4; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB8555_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB8565_Premultiplied>() { return 5; }
template<> Q_DECL_CONSTEXPR uint blueWidth<QImage::Format_ARGB6666_Premultiplied>() { return 6; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB16>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB444>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB555>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB666>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_RGB888>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB4444_Premultiplied>() { return 0; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB8555_Premultiplied>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB8565_Premultiplied>() { return 8; }
template<> Q_DECL_CONSTEXPR uint blueShift<QImage::Format_ARGB6666_Premultiplied>() { return 0; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB4444_Premultiplied>() { return  4; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB8555_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB8565_Premultiplied>() { return  8; }
template<> Q_DECL_CONSTEXPR uint alphaWidth<QImage::Format_ARGB6666_Premultiplied>() { return  6; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB4444_Premultiplied>() { return 12; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB8555_Premultiplied>() { return  0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB8565_Premultiplied>() { return  0; }
template<> Q_DECL_CONSTEXPR uint alphaShift<QImage::Format_ARGB6666_Premultiplied>() { return 18; }

template<QImage::Format> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel();
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB16>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB444>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB555>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB666>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_RGB888>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB4444_Premultiplied>() { return QPixelLayout::BPP16; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB8555_Premultiplied>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB8565_Premultiplied>() { return QPixelLayout::BPP24; }
template<> Q_DECL_CONSTEXPR QPixelLayout::BPP bitsPerPixel<QImage::Format_ARGB6666_Premultiplied>() { return QPixelLayout::BPP24; }


template<QImage::Format Format>
static const uint *QT_FASTCALL convertToRGB32(uint *buffer, const uint *src, int count,
                                              const QPixelLayout *, const QRgb *)
{
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar redLeftShift = 8 - redWidth<Format>();
    Q_CONSTEXPR uchar greenLeftShift = 8 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueLeftShift = 8 - blueWidth<Format>();

    Q_CONSTEXPR uchar redRightShift = 2 * redWidth<Format>() - 8;
    Q_CONSTEXPR uchar greenRightShift = 2 * greenWidth<Format>() - 8;
    Q_CONSTEXPR uchar blueRightShift = 2 * blueWidth<Format>() - 8;

    for (int i = 0; i < count; ++i) {
        uint red = (src[i] >> redShift<Format>()) & redMask;
        uint green = (src[i] >> greenShift<Format>()) & greenMask;
        uint blue = (src[i] >> blueShift<Format>()) & blueMask;

        red = ((red << redLeftShift) | (red >> redRightShift)) << 16;
        green = ((green << greenLeftShift) | (green >> greenRightShift)) << 8;
        blue = (blue << blueLeftShift) | (blue >> blueRightShift);
        buffer[i] = 0xff000000 | red | green | blue;
    }

    return buffer;
}

template<QImage::Format Format>
static const uint *QT_FASTCALL convertARGBPMToARGB32PM(uint *buffer, const uint *src, int count,
                                                       const QPixelLayout *, const QRgb *)
{
    Q_CONSTEXPR uint alphaMask = ((1 << alphaWidth<Format>()) - 1);
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar alphaLeftShift = 8 - alphaWidth<Format>();
    Q_CONSTEXPR uchar redLeftShift = 8 - redWidth<Format>();
    Q_CONSTEXPR uchar greenLeftShift = 8 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueLeftShift = 8 - blueWidth<Format>();

    Q_CONSTEXPR uchar alphaRightShift = 2 * alphaWidth<Format>() - 8;
    Q_CONSTEXPR uchar redRightShift = 2 * redWidth<Format>() - 8;
    Q_CONSTEXPR uchar greenRightShift = 2 * greenWidth<Format>() - 8;
    Q_CONSTEXPR uchar blueRightShift = 2 * blueWidth<Format>() - 8;

    for (int i = 0; i < count; ++i) {
        uint alpha = (src[i] >> alphaShift<Format>()) & alphaMask;
        uint red = (src[i] >> redShift<Format>()) & redMask;
        uint green = (src[i] >> greenShift<Format>()) & greenMask;
        uint blue = (src[i] >> blueShift<Format>()) & blueMask;

        alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
        red = qMin(alpha, (red << redLeftShift) | (red >> redRightShift));
        green = qMin(alpha, (green << greenLeftShift) | (green >> greenRightShift));
        blue = qMin(alpha, (blue << blueLeftShift) | (blue >> blueRightShift));
        buffer[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
    }

    return buffer;
}

template<QImage::Format Format>
static const uint *QT_FASTCALL convertRGBFromARGB32PM(uint *buffer, const uint *src, int count,
                                                      const QPixelLayout *, const QRgb *)
{
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar redRightShift = 24 - redWidth<Format>();
    Q_CONSTEXPR uchar greenRightShift = 16 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueRightShift = 8 - blueWidth<Format>();

    for (int i = 0; i < count; ++i) {
        const uint color = qUnpremultiply(src[i]);
        const uint red = ((color >> redRightShift) & redMask) << redShift<Format>();
        const uint green = ((color >> greenRightShift) & greenMask) << greenShift<Format>();
        const uint blue = ((color >> blueRightShift) & blueMask) << blueShift<Format>();
        buffer[i] = red | green | blue;
    }
    return buffer;
}

template<QImage::Format Format>
static const uint *QT_FASTCALL convertRGBFromRGB32(uint *buffer, const uint *src, int count,
                                                   const QPixelLayout *, const QRgb *)
{
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar redRightShift = 24 - redWidth<Format>();
    Q_CONSTEXPR uchar greenRightShift = 16 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueRightShift = 8 - blueWidth<Format>();

    for (int i = 0; i < count; ++i) {
        const uint red = ((src[i] >> redRightShift) & redMask) << redShift<Format>();
        const uint green = ((src[i] >> greenRightShift) & greenMask) << greenShift<Format>();
        const uint blue = ((src[i] >> blueRightShift) & blueMask) << blueShift<Format>();
        buffer[i] = red | green | blue;
    }
    return buffer;
}

template<QImage::Format Format>
static const uint *QT_FASTCALL convertARGBPMFromARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QPixelLayout *, const QRgb *)
{
    Q_CONSTEXPR uint alphaMask = ((1 << alphaWidth<Format>()) - 1);
    Q_CONSTEXPR uint redMask = ((1 << redWidth<Format>()) - 1);
    Q_CONSTEXPR uint greenMask = ((1 << greenWidth<Format>()) - 1);
    Q_CONSTEXPR uint blueMask = ((1 << blueWidth<Format>()) - 1);

    Q_CONSTEXPR uchar alphaRightShift = 32 - alphaWidth<Format>();
    Q_CONSTEXPR uchar redRightShift = 24 - redWidth<Format>();
    Q_CONSTEXPR uchar greenRightShift = 16 - greenWidth<Format>();
    Q_CONSTEXPR uchar blueRightShift = 8 - blueWidth<Format>();

    for (int i = 0; i < count; ++i) {
        const uint alpha = ((src[i] >> alphaRightShift) & alphaMask) << alphaShift<Format>();
        const uint red = ((src[i] >> redRightShift) & redMask) << redShift<Format>();
        const uint green = ((src[i] >> greenRightShift) & greenMask) << greenShift<Format>();
        const uint blue = ((src[i] >> blueRightShift) & blueMask) << blueShift<Format>();
        buffer[i] = alpha | red | green | blue;
    }
    return buffer;
}

template<QImage::Format Format> Q_DECL_CONSTEXPR static inline QPixelLayout pixelLayoutRGB()
{
    return QPixelLayout{
        redWidth<Format>(), redShift<Format>(),
        greenWidth<Format>(), greenShift<Format>(),
        blueWidth<Format>(), blueShift<Format>(),
        0, 0,
        false, bitsPerPixel<Format>(),
        convertToRGB32<Format>,
        convertRGBFromARGB32PM<Format>,
        convertRGBFromRGB32<Format>
    };
}

template<QImage::Format Format> Q_DECL_CONSTEXPR static inline QPixelLayout pixelLayoutARGBPM()
{
    return QPixelLayout{
        redWidth<Format>(), redShift<Format>(),
        greenWidth<Format>(), greenShift<Format>(),
        blueWidth<Format>(), blueShift<Format>(),
        alphaWidth<Format>(), alphaShift<Format>(),
        true, bitsPerPixel<Format>(),
        convertARGBPMToARGB32PM<Format>,
        convertARGBPMFromARGB32PM<Format>,
        0
    };
}

#else // CONSTEXPR

static const uint *QT_FASTCALL convertToARGB32PM(uint *buffer, const uint *src, int count,
                                                 const QPixelLayout *layout, const QRgb *)
{
    Q_ASSERT(layout->redWidth >= 4);
    Q_ASSERT(layout->greenWidth >= 4);
    Q_ASSERT(layout->blueWidth >= 4);
    Q_ASSERT(layout->alphaWidth >= 4);

    const uint redMask = ((1 << layout->redWidth) - 1);
    const uint greenMask = ((1 << layout->greenWidth) - 1);
    const uint blueMask = ((1 << layout->blueWidth) - 1);

    const uchar redLeftShift = 8 - layout->redWidth;
    const uchar greenLeftShift = 8 - layout->greenWidth;
    const uchar blueLeftShift = 8 - layout->blueWidth;

    const uchar redRightShift = 2 * layout->redWidth - 8;
    const uchar greenRightShift = 2 * layout->greenWidth - 8;
    const uchar blueRightShift = 2 * layout->blueWidth - 8;

    const uint alphaMask = ((1 << layout->alphaWidth) - 1);
    const uchar alphaLeftShift = 8 - layout->alphaWidth;
    const uchar alphaRightShift = 2 * layout->alphaWidth - 8;

    if (layout->premultiplied) {
        for (int i = 0; i < count; ++i) {
            uint alpha = (src[i] >> layout->alphaShift) & alphaMask;
            uint red = (src[i] >> layout->redShift) & redMask;
            uint green = (src[i] >> layout->greenShift) & greenMask;
            uint blue = (src[i] >> layout->blueShift) & blueMask;

            alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
            red = qMin(alpha, (red << redLeftShift) | (red >> redRightShift));
            green = qMin(alpha, (green << greenLeftShift) | (green >> greenRightShift));
            blue = qMin(alpha, (blue << blueLeftShift) | (blue >> blueRightShift));
            buffer[i] = (alpha << 24) | (red << 16) | (green << 8) | blue;
        }
    } else {
        for (int i = 0; i < count; ++i) {
            uint alpha = (src[i] >> layout->alphaShift) & alphaMask;
            uint red = (src[i] >> layout->redShift) & redMask;
            uint green = (src[i] >> layout->greenShift) & greenMask;
            uint blue = (src[i] >> layout->blueShift) & blueMask;

            alpha = (alpha << alphaLeftShift) | (alpha >> alphaRightShift);
            red = (red << redLeftShift) | (red >> redRightShift);
            green = (green << greenLeftShift) | (green >> greenRightShift);
            blue = (blue << blueLeftShift) | (blue >> blueRightShift);
            buffer[i] = qPremultiply((alpha << 24) | (red << 16) | (green << 8) | blue);
        }
    }
    return buffer;
}

static const uint *QT_FASTCALL convertToRGB32(uint *buffer, const uint *src, int count,
                                              const QPixelLayout *layout, const QRgb *)
{
    Q_ASSERT(layout->redWidth >= 4);
    Q_ASSERT(layout->greenWidth >= 4);
    Q_ASSERT(layout->blueWidth >= 4);
    Q_ASSERT(layout->alphaWidth == 0);

    const uint redMask = ((1 << layout->redWidth) - 1);
    const uint greenMask = ((1 << layout->greenWidth) - 1);
    const uint blueMask = ((1 << layout->blueWidth) - 1);

    const uchar redLeftShift = 8 - layout->redWidth;
    const uchar greenLeftShift = 8 - layout->greenWidth;
    const uchar blueLeftShift = 8 - layout->blueWidth;

    const uchar redRightShift = 2 * layout->redWidth - 8;
    const uchar greenRightShift = 2 * layout->greenWidth - 8;
    const uchar blueRightShift = 2 * layout->blueWidth - 8;

    for (int i = 0; i < count; ++i) {
        uint red = (src[i] >> layout->redShift) & redMask;
        uint green = (src[i] >> layout->greenShift) & greenMask;
        uint blue = (src[i] >> layout->blueShift) & blueMask;

        red = (red << redLeftShift) | (red >> redRightShift);
        green = (green << greenLeftShift) | (green >> greenRightShift);
        blue = (blue << blueLeftShift) | (blue >> blueRightShift);
        buffer[i] = 0xff000000 | (red << 16) | (green << 8) | blue;
    }
    return buffer;
}

static const uint *QT_FASTCALL convertFromARGB32PM(uint *buffer, const uint *src, int count,
                                                   const QPixelLayout *layout, const QRgb *)
{
    Q_ASSERT(layout->redWidth <= 8);
    Q_ASSERT(layout->greenWidth <= 8);
    Q_ASSERT(layout->blueWidth <= 8);
    Q_ASSERT(layout->alphaWidth <= 8);

    const uint redMask = (1 << layout->redWidth) - 1;
    const uint greenMask = (1 << layout->greenWidth) - 1;
    const uint blueMask = (1 << layout->blueWidth) - 1;
    const uint alphaMask = (1 << layout->alphaWidth) - 1;

    const uchar redRightShift = 24 - layout->redWidth;
    const uchar greenRightShift = 16 - layout->greenWidth;
    const uchar blueRightShift = 8 - layout->blueWidth;
    const uchar alphaRightShift = 32 - layout->alphaWidth;

    if (!layout->premultiplied) {
        for (int i = 0; i < count; ++i)
            buffer[i] = qUnpremultiply(src[i]);
        src = buffer;
    }
    for (int i = 0; i < count; ++i) {
        uint red = ((src[i] >> redRightShift) & redMask) << layout->redShift;
        uint green = ((src[i] >> greenRightShift) & greenMask) << layout->greenShift;
        uint blue = ((src[i] >> blueRightShift) & blueMask) << layout->blueShift;
        uint alpha = ((src[i] >> alphaRightShift) & alphaMask) << layout->alphaShift;
        buffer[i] = red | green | blue | alpha;
    }
    return buffer;
}

static const uint *QT_FASTCALL convertFromRGB32(uint *buffer, const uint *src, int count,
                                               const QPixelLayout *layout, const QRgb *)
{
    Q_ASSERT(layout->redWidth <= 8);
    Q_ASSERT(layout->greenWidth <= 8);
    Q_ASSERT(layout->blueWidth <= 8);
    Q_ASSERT(layout->alphaWidth == 0);
    Q_ASSERT(!layout->premultiplied);

    const uint redMask = (1 << layout->redWidth) - 1;
    const uint greenMask = (1 << layout->greenWidth) - 1;
    const uint blueMask = (1 << layout->blueWidth) - 1;

    const uchar redRightShift = 24 - layout->redWidth;
    const uchar greenRightShift = 16 - layout->greenWidth;
    const uchar blueRightShift = 8 - layout->blueWidth;

    for (int i = 0; i < count; ++i) {
        uint red = ((src[i] >> redRightShift) & redMask) << layout->redShift;
        uint green = ((src[i] >> greenRightShift) & greenMask) << layout->greenShift;
        uint blue = ((src[i] >> blueRightShift) & blueMask) << layout->blueShift;
        buffer[i] = red | green | blue;
    }
    return buffer;
}

static const uint *QT_FASTCALL convertRGB16ToRGB32(uint *buffer, const uint *src, int count,
                                                      const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertRgb16To32(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGB16FromRGB32(uint *buffer, const uint *src, int count,
                                                     const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertRgb32To16(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGB16FromARGB32PM(uint *buffer, const uint *src, int count,
                                                        const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qConvertRgb32To16(qUnpremultiply(src[i]));
    return buffer;
}
#endif

// To convert in place, let 'dest' and 'src' be the same.
static const uint *QT_FASTCALL convertIndexedToARGB32PM(uint *buffer, const uint *src, int count,
                                                        const QPixelLayout *, const QRgb *clut)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qPremultiply(clut[src[i]]);
    return buffer;
}

static const uint *QT_FASTCALL convertPassThrough(uint *, const uint *src, int,
                                                  const QPixelLayout *, const QRgb *)
{
    return src;
}

static const uint *QT_FASTCALL convertARGB32ToARGB32PM(uint *buffer, const uint *src, int count,
                                                       const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qPremultiply(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888PMToARGB32PM(uint *buffer, const uint *src, int count,
                                                           const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = RGBA2ARGB(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888ToARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qPremultiply(RGBA2ARGB(src[i]));
    return buffer;
}

static const uint *QT_FASTCALL convertARGB32FromARGB32PM(uint *buffer, const uint *src, int count,
                                                         const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = qUnpremultiply(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888PMFromARGB32PM(uint *buffer, const uint *src, int count,
                                                             const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBA8888FromARGB32PM(uint *buffer, const uint *src, int count,
                                                             const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(qUnpremultiply(src[i]));
    return buffer;
}

static const uint *QT_FASTCALL convertRGBXFromRGB32(uint *buffer, const uint *src, int count,
                                                    const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(0xff000000 | src[i]);
    return buffer;
}

static const uint *QT_FASTCALL convertRGBXFromARGB32PM(uint *buffer, const uint *src, int count,
                                                       const QPixelLayout *, const QRgb *)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = ARGB2RGBA(0xff000000 | qUnpremultiply(src[i]));
    return buffer;
}

template <QPixelLayout::BPP bpp> static
uint QT_FASTCALL fetchPixel(const uchar *src, int index);

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP1LSB>(const uchar *src, int index)
{
    return (src[index >> 3] >> (index & 7)) & 1;
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP1MSB>(const uchar *src, int index)
{
    return (src[index >> 3] >> (~index & 7)) & 1;
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP8>(const uchar *src, int index)
{
    return src[index];
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP16>(const uchar *src, int index)
{
    return reinterpret_cast<const quint16 *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP24>(const uchar *src, int index)
{
    return reinterpret_cast<const quint24 *>(src)[index];
}

template <>
inline uint QT_FASTCALL fetchPixel<QPixelLayout::BPP32>(const uchar *src, int index)
{
    return reinterpret_cast<const uint *>(src)[index];
}

template <QPixelLayout::BPP bpp>
inline const uint *QT_FASTCALL fetchPixels(uint *buffer, const uchar *src, int index, int count)
{
    for (int i = 0; i < count; ++i)
        buffer[i] = fetchPixel<bpp>(src, index + i);
    return buffer;
}

template <>
inline const uint *QT_FASTCALL fetchPixels<QPixelLayout::BPP32>(uint *, const uchar *src, int index, int)
{
    return reinterpret_cast<const uint *>(src) + index;
}

template <QPixelLayout::BPP width> static
void QT_FASTCALL storePixel(uchar *dest, int index, uint pixel);

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP1LSB>(uchar *dest, int index, uint pixel)
{
    if (pixel)
        dest[index >> 3] |= 1 << (index & 7);
    else
        dest[index >> 3] &= ~(1 << (index & 7));
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP1MSB>(uchar *dest, int index, uint pixel)
{
    if (pixel)
        dest[index >> 3] |= 1 << (~index & 7);
    else
        dest[index >> 3] &= ~(1 << (~index & 7));
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP8>(uchar *dest, int index, uint pixel)
{
    dest[index] = uchar(pixel);
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP16>(uchar *dest, int index, uint pixel)
{
    reinterpret_cast<quint16 *>(dest)[index] = quint16(pixel);
}

template <>
inline void QT_FASTCALL storePixel<QPixelLayout::BPP24>(uchar *dest, int index, uint pixel)
{
    reinterpret_cast<quint24 *>(dest)[index] = quint24(pixel);
}

template <QPixelLayout::BPP width>
inline void QT_FASTCALL storePixels(uchar *dest, const uint *src, int index, int count)
{
    for (int i = 0; i < count; ++i)
        storePixel<width>(dest, index + i, src[i]);
}

template <>
inline void QT_FASTCALL storePixels<QPixelLayout::BPP32>(uchar *dest, const uint *src, int index, int count)
{
    memcpy(reinterpret_cast<uint *>(dest) + index, src, count * sizeof(uint));
}

// Note:
// convertToArgb32() assumes that no color channel is less than 4 bits.
// convertFromArgb32() assumes that no color channel is more than 8 bits.
// QImage::rgbSwapped() assumes that the red and blue color channels have the same number of bits.
QPixelLayout qPixelLayouts[QImage::NImageFormats] = {
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPPNone, 0, 0, 0 }, // Format_Invalid
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPP1MSB, convertIndexedToARGB32PM, 0, 0 }, // Format_Mono
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPP1LSB, convertIndexedToARGB32PM, 0, 0 }, // Format_MonoLSB
    { 0,  0, 0,  0, 0,  0, 0,  0, false, QPixelLayout::BPP8, convertIndexedToARGB32PM, 0, 0 }, // Format_Indexed8
    // Technically using convertPassThrough to convert from ARGB32PM to RGB32 is wrong,
    // but everywhere this generic conversion would be wrong is currently overloaed.
    { 8, 16, 8,  8, 8,  0, 0,  0, false, QPixelLayout::BPP32, convertPassThrough, convertPassThrough, convertPassThrough }, // Format_RGB32
    { 8, 16, 8,  8, 8,  0, 8, 24, false, QPixelLayout::BPP32, convertARGB32ToARGB32PM, convertARGB32FromARGB32PM, 0 }, // Format_ARGB32
    { 8, 16, 8,  8, 8,  0, 8, 24,  true, QPixelLayout::BPP32, convertPassThrough, convertPassThrough, 0 }, // Format_ARGB32_Premultiplied
#ifdef Q_COMPILER_CONSTEXPR
    pixelLayoutRGB<QImage::Format_RGB16>(),
    pixelLayoutARGBPM<QImage::Format_ARGB8565_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB666>(),
    pixelLayoutARGBPM<QImage::Format_ARGB6666_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB555>(),
    pixelLayoutARGBPM<QImage::Format_ARGB8555_Premultiplied>(),
    pixelLayoutRGB<QImage::Format_RGB888>(),
    pixelLayoutRGB<QImage::Format_RGB444>(),
    pixelLayoutARGBPM<QImage::Format_ARGB4444_Premultiplied>(),
#else
    { 5, 11, 6,  5, 5,  0, 0,  0, false, QPixelLayout::BPP16, convertRGB16ToRGB32, convertRGB16FromARGB32PM, convertRGB16FromRGB32 }, // Format_RGB16
    { 5, 19, 6, 13, 5,  8, 8,  0,  true, QPixelLayout::BPP24, convertToARGB32PM, convertFromARGB32PM, 0 }, // Format_ARGB8565_Premultiplied
    { 6, 12, 6,  6, 6,  0, 0,  0, false, QPixelLayout::BPP24, convertToRGB32, convertFromARGB32PM, convertFromRGB32 }, // Format_RGB666
    { 6, 12, 6,  6, 6,  0, 6, 18,  true, QPixelLayout::BPP24, convertToARGB32PM, convertFromARGB32PM, 0 }, // Format_ARGB6666_Premultiplied
    { 5, 10, 5,  5, 5,  0, 0,  0, false, QPixelLayout::BPP16, convertToRGB32, convertFromARGB32PM, convertFromRGB32 }, // Format_RGB555
    { 5, 18, 5, 13, 5,  8, 8,  0,  true, QPixelLayout::BPP24, convertToARGB32PM, convertFromARGB32PM, 0 }, // Format_ARGB8555_Premultiplied
    { 8, 16, 8,  8, 8,  0, 0,  0, false, QPixelLayout::BPP24, convertToRGB32, convertFromARGB32PM, convertFromRGB32 }, // Format_RGB888
    { 4,  8, 4,  4, 4,  0, 0,  0, false, QPixelLayout::BPP16, convertToRGB32, convertFromARGB32PM, convertFromRGB32 }, // Format_RGB444
    { 4,  8, 4,  4, 4,  0, 4, 12,  true, QPixelLayout::BPP16, convertToARGB32PM, convertFromARGB32PM, 0 }, // Format_ARGB4444_Premultiplied
#endif
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
    { 8, 24, 8, 16, 8,  8, 0,  0, false, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBXFromARGB32PM, convertRGBXFromRGB32 }, // Format_RGBX8888
    { 8, 24, 8, 16, 8,  8, 8,  0, false, QPixelLayout::BPP32, convertRGBA8888ToARGB32PM, convertRGBA8888FromARGB32PM, 0 }, // Format_RGBA8888
    { 8, 24, 8, 16, 8,  8, 8,  0,  true, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBA8888PMFromARGB32PM, 0 }, // Format_RGBA8888_Premultiplied
#else
    { 8,  0, 8,  8, 8, 16, 0, 24, false, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBXFromARGB32PM, convertRGBXFromRGB32 }, // Format_RGBX8888
    { 8,  0, 8,  8, 8, 16, 8, 24, false, QPixelLayout::BPP32, convertRGBA8888ToARGB32PM, convertRGBA8888FromARGB32PM, 0 }, // Format_RGBA8888 (ABGR32)
    { 8,  0, 8,  8, 8, 16, 8, 24,  true, QPixelLayout::BPP32, convertRGBA8888PMToARGB32PM, convertRGBA8888PMFromARGB32PM, 0 }  // Format_RGBA8888_Premultiplied
#endif
};

FetchPixelsFunc qFetchPixels[QPixelLayout::BPPCount] = {
    0, // BPPNone
    fetchPixels<QPixelLayout::BPP1MSB>, // BPP1MSB
    fetchPixels<QPixelLayout::BPP1LSB>, // BPP1LSB
    fetchPixels<QPixelLayout::BPP8>, // BPP8
    fetchPixels<QPixelLayout::BPP16>, // BPP16
    fetchPixels<QPixelLayout::BPP24>, // BPP24
    fetchPixels<QPixelLayout::BPP32> // BPP32
};

StorePixelsFunc qStorePixels[QPixelLayout::BPPCount] = {
    0, // BPPNone
    storePixels<QPixelLayout::BPP1MSB>, // BPP1MSB
    storePixels<QPixelLayout::BPP1LSB>, // BPP1LSB
    storePixels<QPixelLayout::BPP8>, // BPP8
    storePixels<QPixelLayout::BPP16>, // BPP16
    storePixels<QPixelLayout::BPP24>, // BPP24
    storePixels<QPixelLayout::BPP32> // BPP32
};

typedef uint (QT_FASTCALL *FetchPixelFunc)(const uchar *src, int index);

FetchPixelFunc qFetchPixel[QPixelLayout::BPPCount] = {
    0, // BPPNone
    fetchPixel<QPixelLayout::BPP1MSB>, // BPP1MSB
    fetchPixel<QPixelLayout::BPP1LSB>, // BPP1LSB
    fetchPixel<QPixelLayout::BPP8>, // BPP8
    fetchPixel<QPixelLayout::BPP16>, // BPP16
    fetchPixel<QPixelLayout::BPP24>, // BPP24
    fetchPixel<QPixelLayout::BPP32> // BPP32
};

/*
  Destination fetch. This is simple as we don't have to do bounds checks or
  transformations
*/

static uint * QT_FASTCALL destFetchMono(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x80 >> (x & 7)) ? rasterBuffer->destColor1 : rasterBuffer->destColor0;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchMonoLsb(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    uint *start = buffer;
    const uint *end = buffer + length;
    while (buffer < end) {
        *buffer = data[x>>3] & (0x1 << (x & 7)) ? rasterBuffer->destColor1 : rasterBuffer->destColor0;
        ++buffer;
        ++x;
    }
    return start;
}

static uint * QT_FASTCALL destFetchARGB32P(uint *, QRasterBuffer *rasterBuffer, int x, int y, int)
{
    return (uint *)rasterBuffer->scanLine(y) + x;
}

static uint * QT_FASTCALL destFetchRGB16(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const ushort *Q_DECL_RESTRICT data = (const ushort *)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        buffer[i] = qConvertRgb16To32(data[i]);
    return buffer;
}

static uint *QT_FASTCALL destFetch(uint *buffer, QRasterBuffer *rasterBuffer, int x, int y, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    const uint *ptr = qFetchPixels[layout->bpp](buffer, rasterBuffer->scanLine(y), x, length);
    return const_cast<uint *>(layout->convertToARGB32PM(buffer, ptr, length, layout, 0));
}


static DestFetchProc destFetchProc[QImage::NImageFormats] =
{
    0,                  // Format_Invalid
    destFetchMono,      // Format_Mono,
    destFetchMonoLsb,   // Format_MonoLSB
    0,                  // Format_Indexed8
    destFetchARGB32P,   // Format_RGB32
    destFetch,          // Format_ARGB32,
    destFetchARGB32P,   // Format_ARGB32_Premultiplied
    destFetchRGB16,     // Format_RGB16
    destFetch,          // Format_ARGB8565_Premultiplied
    destFetch,          // Format_RGB666
    destFetch,          // Format_ARGB6666_Premultiplied
    destFetch,          // Format_RGB555
    destFetch,          // Format_ARGB8555_Premultiplied
    destFetch,          // Format_RGB888
    destFetch,          // Format_RGB444
    destFetch,          // Format_ARGB4444_Premultiplied
    destFetch,          // Format_RGBX8888
    destFetch,          // Format_RGBA8888
    destFetch,          // Format_RGBA8888_Premultiplied
};

/*
   Returns the color in the mono destination color table
   that is the "nearest" to /color/.
*/
static inline QRgb findNearestColor(QRgb color, QRasterBuffer *rbuf)
{
    QRgb color_0 = qPremultiply(rbuf->destColor0);
    QRgb color_1 = qPremultiply(rbuf->destColor1);
    color = qPremultiply(color);

    int r = qRed(color);
    int g = qGreen(color);
    int b = qBlue(color);
    int rx, gx, bx;
    int dist_0, dist_1;

    rx = r - qRed(color_0);
    gx = g - qGreen(color_0);
    bx = b - qBlue(color_0);
    dist_0 = rx*rx + gx*gx + bx*bx;

    rx = r - qRed(color_1);
    gx = g - qGreen(color_1);
    bx = b - qBlue(color_1);
    dist_1 = rx*rx + gx*gx + bx*bx;

    if (dist_0 < dist_1)
        return color_0;
    return color_1;
}

/*
  Destination store.
*/

static void QT_FASTCALL destStoreMono(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 0x80 >> (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(0x80 >> (x & 7));
            } else {
                data[x >> 3] |= 0x80 >> (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 0x80 >> (x & 7);
            else
                data[x >> 3] &= ~(0x80 >> (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreMonoLsb(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uchar *Q_DECL_RESTRICT data = (uchar *)rasterBuffer->scanLine(y);
    if (rasterBuffer->monoDestinationWithClut) {
        for (int i = 0; i < length; ++i) {
            if (buffer[i] == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else if (buffer[i] == rasterBuffer->destColor1) {
                data[x >> 3] |= 1 << (x & 7);
            } else if (findNearestColor(buffer[i], rasterBuffer) == rasterBuffer->destColor0) {
                data[x >> 3] &= ~(1 << (x & 7));
            } else {
                data[x >> 3] |= 1 << (x & 7);
            }
            ++x;
        }
    } else {
        for (int i = 0; i < length; ++i) {
            if (qGray(buffer[i]) < int(qt_bayer_matrix[y & 15][x & 15]))
                data[x >> 3] |= 1 << (x & 7);
            else
                data[x >> 3] &= ~(1 << (x & 7));
            ++x;
        }
    }
}

static void QT_FASTCALL destStoreRGB16(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    quint16 *data = (quint16*)rasterBuffer->scanLine(y) + x;
    for (int i = 0; i < length; ++i)
        data[i] = qConvertRgb32To16(buffer[i]);
}

static void QT_FASTCALL destStore(QRasterBuffer *rasterBuffer, int x, int y, const uint *buffer, int length)
{
    uint buf[buffer_size];
    const QPixelLayout *layout = &qPixelLayouts[rasterBuffer->format];
    StorePixelsFunc store = qStorePixels[layout->bpp];
    uchar *dest = rasterBuffer->scanLine(y);
    while (length) {
        int l = qMin(length, buffer_size);
        const uint *ptr = 0;
        if (layout->convertFromRGB32) {
            Q_ASSERT(!layout->premultiplied && !layout->alphaWidth);
            ptr = layout->convertFromRGB32(buf, buffer, l, layout, 0);
        } else
            ptr = layout->convertFromARGB32PM(buf, buffer, l, layout, 0);
        store(dest, ptr, x, l);
        length -= l;
        buffer += l;
        x += l;
    }
}

static DestStoreProc destStoreProc[QImage::NImageFormats] =
{
    0,                  // Format_Invalid
    destStoreMono,      // Format_Mono,
    destStoreMonoLsb,   // Format_MonoLSB
    0,                  // Format_Indexed8
    0,                  // Format_RGB32
    destStore,          // Format_ARGB32,
    0,                  // Format_ARGB32_Premultiplied
    destStoreRGB16,     // Format_RGB16
    destStore,          // Format_ARGB8565_Premultiplied
    destStore,          // Format_RGB666
    destStore,          // Format_ARGB6666_Premultiplied
    destStore,          // Format_RGB555
    destStore,          // Format_ARGB8555_Premultiplied
    destStore,          // Format_RGB888
    destStore,          // Format_RGB444
    destStore,          // Format_ARGB4444_Premultiplied
    destStore,          // Format_RGBX8888
    destStore,          // Format_RGBA8888
    destStore           // Format_RGBA8888_Premultiplied
};

/*
  Source fetches

  This is a bit more complicated, as we need several fetch routines for every surface type

  We need 5 fetch methods per surface type:
  untransformed
  transformed (tiled and not tiled)
  transformed bilinear (tiled and not tiled)

  We don't need bounds checks for untransformed, but we need them for the other ones.

  The generic implementation does pixel by pixel fetches
*/

enum TextureBlendType {
    BlendUntransformed,
    BlendTiled,
    BlendTransformed,
    BlendTransformedTiled,
    BlendTransformedBilinear,
    BlendTransformedBilinearTiled,
    NBlendTypes
};

static const uint *QT_FASTCALL fetchUntransformed(uint *buffer, const Operator *,
                                                  const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const uint *ptr = qFetchPixels[layout->bpp](buffer, data->texture.scanLine(y), x, length);
    const QRgb *clut = data->texture.colorTable ? data->texture.colorTable->constData() : 0;
    return layout->convertToARGB32PM(buffer, ptr, length, layout, clut);
}

static const uint *QT_FASTCALL fetchUntransformedARGB32PM(uint *, const Operator *,
                                                          const QSpanData *data, int y, int x, int)
{
    const uchar *scanLine = data->texture.scanLine(y);
    return ((const uint *)scanLine) + x;
}

static const uint *QT_FASTCALL fetchUntransformedRGB16(uint *buffer, const Operator *,
                                                       const QSpanData *data, int y, int x,
                                                       int length)
{
    const quint16 *scanLine = (const quint16 *)data->texture.scanLine(y) + x;
#ifdef QT_COMPILER_SUPPORTS_MIPS_DSPR2
    qConvertRgb16To32_asm_mips_dspr2(buffer, scanLine, length);
#else
    for (int i = 0; i < length; ++i)
        buffer[i] = qConvertRgb16To32(scanLine[i]);
#endif
    return buffer;
}

// blendType is either BlendTransformed or BlendTransformedTiled
template<TextureBlendType blendType>
static const uint *QT_FASTCALL fetchTransformedARGB32PM(uint *buffer, const Operator *, const QSpanData *data,
                                                        int y, int x, int length)
{
    int image_width = data->texture.width;
    int image_height = data->texture.height;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    const uint *end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        while (b < end) {
            int px = fx >> 16;
            int py = fy >> 16;

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(0, px, image_width - 1);
                py = qBound(0, py, image_height - 1);
            }
            *b = reinterpret_cast<const uint *>(data->texture.scanLine(py))[px];

            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal tx = fx * iw;
            const qreal ty = fy * iw;
            int px = int(tx) - (tx < 0);
            int py = int(ty) - (ty < 0);

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(0, px, image_width - 1);
                py = qBound(0, py, image_height - 1);
            }
            *b = reinterpret_cast<const uint *>(data->texture.scanLine(py))[px];

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }
    return buffer;
}

template<TextureBlendType blendType>  /* either BlendTransformed or BlendTransformedTiled */
static const uint *QT_FASTCALL fetchTransformed(uint *buffer, const Operator *, const QSpanData *data,
                                         int y, int x, int length)
{
    int image_width = data->texture.width;
    int image_height = data->texture.height;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    FetchPixelFunc fetch = qFetchPixel[layout->bpp];

    const uint *end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        while (b < end) {
            int px = fx >> 16;
            int py = fy >> 16;

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(0, px, image_width - 1);
                py = qBound(0, py, image_height - 1);
            }
            *b = fetch(data->texture.scanLine(py), px);

            fx += fdx;
            fy += fdy;
            ++b;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal tx = fx * iw;
            const qreal ty = fy * iw;
            int px = int(tx) - (tx < 0);
            int py = int(ty) - (ty < 0);

            if (blendType == BlendTransformedTiled) {
                px %= image_width;
                py %= image_height;
                if (px < 0) px += image_width;
                if (py < 0) py += image_height;
            } else {
                px = qBound(0, px, image_width - 1);
                py = qBound(0, py, image_height - 1);
            }
            *b = fetch(data->texture.scanLine(py), px);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }
    const QRgb *clut = data->texture.colorTable ? data->texture.colorTable->constData() : 0;
    return layout->convertToARGB32PM(buffer, buffer, length, layout, clut);
}

/** \internal
  interpolate 4 argb pixels with the distx and disty factor.
  distx and disty bust be between 0 and 16
 */
static inline uint interpolate_4_pixels_16(uint tl, uint tr, uint bl, uint br, int distx, int disty)
{
    uint distxy = distx * disty;
    //idistx * disty = (16-distx) * disty = 16*disty - distxy
    //idistx * idisty = (16-distx) * (16-disty) = 16*16 - 16*distx -16*disty + distxy
    uint tlrb = (tl & 0x00ff00ff)         * (16*16 - 16*distx - 16*disty + distxy);
    uint tlag = ((tl & 0xff00ff00) >> 8)  * (16*16 - 16*distx - 16*disty + distxy);
    uint trrb = ((tr & 0x00ff00ff)        * (distx*16 - distxy));
    uint trag = (((tr & 0xff00ff00) >> 8) * (distx*16 - distxy));
    uint blrb = ((bl & 0x00ff00ff)        * (disty*16 - distxy));
    uint blag = (((bl & 0xff00ff00) >> 8) * (disty*16 - distxy));
    uint brrb = ((br & 0x00ff00ff)        * (distxy));
    uint brag = (((br & 0xff00ff00) >> 8) * (distxy));
    return (((tlrb + trrb + blrb + brrb) >> 8) & 0x00ff00ff) | ((tlag + trag + blag + brag) & 0xff00ff00);
}

#if defined(__SSE2__)
#define interpolate_4_pixels_16_sse2(tl, tr, bl, br, distx, disty, colorMask, v_256, b)  \
{ \
    const __m128i dxdy = _mm_mullo_epi16 (distx, disty); \
    const __m128i distx_ = _mm_slli_epi16(distx, 4); \
    const __m128i disty_ = _mm_slli_epi16(disty, 4); \
    const __m128i idxidy =  _mm_add_epi16(dxdy, _mm_sub_epi16(v_256, _mm_add_epi16(distx_, disty_))); \
    const __m128i dxidy =  _mm_sub_epi16(distx_, dxdy); \
    const __m128i idxdy =  _mm_sub_epi16(disty_, dxdy); \
 \
    __m128i tlAG = _mm_srli_epi16(tl, 8); \
    __m128i tlRB = _mm_and_si128(tl, colorMask); \
    __m128i trAG = _mm_srli_epi16(tr, 8); \
    __m128i trRB = _mm_and_si128(tr, colorMask); \
    __m128i blAG = _mm_srli_epi16(bl, 8); \
    __m128i blRB = _mm_and_si128(bl, colorMask); \
    __m128i brAG = _mm_srli_epi16(br, 8); \
    __m128i brRB = _mm_and_si128(br, colorMask); \
 \
    tlAG = _mm_mullo_epi16(tlAG, idxidy); \
    tlRB = _mm_mullo_epi16(tlRB, idxidy); \
    trAG = _mm_mullo_epi16(trAG, dxidy); \
    trRB = _mm_mullo_epi16(trRB, dxidy); \
    blAG = _mm_mullo_epi16(blAG, idxdy); \
    blRB = _mm_mullo_epi16(blRB, idxdy); \
    brAG = _mm_mullo_epi16(brAG, dxdy); \
    brRB = _mm_mullo_epi16(brRB, dxdy); \
 \
    /* Add the values, and shift to only keep 8 significant bits per colors */ \
    __m128i rAG =_mm_add_epi16(_mm_add_epi16(tlAG, trAG), _mm_add_epi16(blAG, brAG)); \
    __m128i rRB =_mm_add_epi16(_mm_add_epi16(tlRB, trRB), _mm_add_epi16(blRB, brRB)); \
    rAG = _mm_andnot_si128(colorMask, rAG); \
    rRB = _mm_srli_epi16(rRB, 8); \
    _mm_storeu_si128((__m128i*)(b), _mm_or_si128(rAG, rRB)); \
}
#endif

#if defined(__ARM_NEON__)
#define interpolate_4_pixels_16_neon(tl, tr, bl, br, distx, disty, disty_, colorMask, invColorMask, v_256, b)  \
{ \
    const int16x8_t dxdy = vmulq_s16(distx, disty); \
    const int16x8_t distx_ = vshlq_n_s16(distx, 4); \
    const int16x8_t idxidy =  vaddq_s16(dxdy, vsubq_s16(v_256, vaddq_s16(distx_, disty_))); \
    const int16x8_t dxidy =  vsubq_s16(distx_, dxdy); \
    const int16x8_t idxdy =  vsubq_s16(disty_, dxdy); \
 \
    int16x8_t tlAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(tl), 8)); \
    int16x8_t tlRB = vandq_s16(tl, colorMask); \
    int16x8_t trAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(tr), 8)); \
    int16x8_t trRB = vandq_s16(tr, colorMask); \
    int16x8_t blAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(bl), 8)); \
    int16x8_t blRB = vandq_s16(bl, colorMask); \
    int16x8_t brAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(br), 8)); \
    int16x8_t brRB = vandq_s16(br, colorMask); \
 \
    int16x8_t rAG = vmulq_s16(tlAG, idxidy); \
    int16x8_t rRB = vmulq_s16(tlRB, idxidy); \
    rAG = vmlaq_s16(rAG, trAG, dxidy); \
    rRB = vmlaq_s16(rRB, trRB, dxidy); \
    rAG = vmlaq_s16(rAG, blAG, idxdy); \
    rRB = vmlaq_s16(rRB, blRB, idxdy); \
    rAG = vmlaq_s16(rAG, brAG, dxdy); \
    rRB = vmlaq_s16(rRB, brRB, dxdy); \
 \
    rAG = vandq_s16(invColorMask, rAG); \
    rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8)); \
    vst1q_s16((int16_t*)(b), vorrq_s16(rAG, rRB)); \
}
#endif

#if defined(__SSE2__)
static inline uint interpolate_4_pixels(uint tl, uint tr, uint bl, uint br, uint distx, uint disty)
{
    // First interpolate right and left pixels in parallel.
    __m128i vl = _mm_unpacklo_epi32(_mm_cvtsi32_si128(tl), _mm_cvtsi32_si128(bl));
    __m128i vr = _mm_unpacklo_epi32(_mm_cvtsi32_si128(tr), _mm_cvtsi32_si128(br));
    vl = _mm_unpacklo_epi8(vl, _mm_setzero_si128());
    vr = _mm_unpacklo_epi8(vr, _mm_setzero_si128());
    vl = _mm_mullo_epi16(vl, _mm_set1_epi16(256 - distx));
    vr = _mm_mullo_epi16(vr, _mm_set1_epi16(distx));
    __m128i vtb = _mm_add_epi16(vl, vr);
    vtb = _mm_srli_epi16(vtb, 8);
    // vtb now contains the result of the first two interpolate calls vtb = unpacked((xbot << 64) | xtop)

    // Now the last interpolate between top and bottom interpolations.
    const __m128i vidisty = _mm_shufflelo_epi16(_mm_cvtsi32_si128(256 - disty), _MM_SHUFFLE(0, 0, 0, 0));
    const __m128i vdisty = _mm_shufflelo_epi16(_mm_cvtsi32_si128(disty), _MM_SHUFFLE(0, 0, 0, 0));
    const __m128i vmuly = _mm_unpacklo_epi16(vidisty, vdisty);
    vtb = _mm_unpacklo_epi16(vtb, _mm_srli_si128(vtb, 8));
    // vtb now contains the colors of top and bottom interleaved { ta, ba, tr, br, tg, bg, tb, bb }
    vtb = _mm_madd_epi16(vtb, vmuly); // Multiply and horizontal add.
    vtb = _mm_srli_epi32(vtb, 8);
    vtb = _mm_packs_epi32(vtb, _mm_setzero_si128());
    vtb = _mm_packus_epi16(vtb, _mm_setzero_si128());
    return _mm_cvtsi128_si32(vtb);
}
#else
static inline uint interpolate_4_pixels(uint tl, uint tr, uint bl, uint br, uint distx, uint disty)
{
    uint idistx = 256 - distx;
    uint idisty = 256 - disty;
    uint xtop = INTERPOLATE_PIXEL_256(tl, idistx, tr, distx);
    uint xbot = INTERPOLATE_PIXEL_256(bl, idistx, br, distx);
    return INTERPOLATE_PIXEL_256(xtop, idisty, xbot, disty);
}
#endif


template<TextureBlendType blendType>
void fetchTransformedBilinear_pixelBounds(int max, int l1, int l2, int &v1, int &v2);

template<>
inline void fetchTransformedBilinear_pixelBounds<BlendTransformedBilinearTiled>(int max, int, int, int &v1, int &v2)
{
    v1 %= max;
    if (v1 < 0)
        v1 += max;
    v2 = v1 + 1;
    if (v2 == max)
        v2 = 0;
    Q_ASSERT(v1 >= 0 && v1 < max);
    Q_ASSERT(v2 >= 0 && v2 < max);
}

template<>
inline void fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(int, int l1, int l2, int &v1, int &v2)
{
    if (v1 < l1)
        v2 = v1 = l1;
    else if (v1 >= l2)
        v2 = v1 = l2;
    else
        v2 = v1 + 1;
    Q_ASSERT(v1 >= l1 && v1 <= l2);
    Q_ASSERT(v2 >= l1 && v2 <= l2);
}

template<TextureBlendType blendType> /* blendType = BlendTransformedBilinear or BlendTransformedBilinearTiled */
static const uint * QT_FASTCALL fetchTransformedBilinearARGB32PM(uint *buffer, const Operator *,
                                                                 const QSpanData *data, int y, int x,
                                                                 int length)
{
    int image_width = data->texture.width;
    int image_height = data->texture.height;

    int image_x1 = data->texture.x1;
    int image_y1 = data->texture.y1;
    int image_x2 = data->texture.x2 - 1;
    int image_y2 = data->texture.y2 - 1;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    uint *end = buffer + length;
    uint *b = buffer;
    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy
                      + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy
                      + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        if (fdy == 0) { //simple scale, no rotation
            int y1 = (fy >> 16);
            int y2;
            fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
            const uint *s1 = (const uint *)data->texture.scanLine(y1);
            const uint *s2 = (const uint *)data->texture.scanLine(y2);

            if (fdx <= fixed_scale && fdx > 0) { // scale up on X
                int disty = (fy & 0x0000ffff) >> 8;
                int idisty = 256 - disty;
                int x = fx >> 16;

                // The idea is first to do the interpolation between the row s1 and the row s2
                // into an intermediate buffer, then we interpolate between two pixel of this buffer.

                // intermediate_buffer[0] is a buffer of red-blue component of the pixel, in the form 0x00RR00BB
                // intermediate_buffer[1] is the alpha-green component of the pixel, in the form 0x00AA00GG
                quint32 intermediate_buffer[2][buffer_size + 2];
                // count is the size used in the intermediate_buffer.
                int count = qCeil(length * data->m11) + 2; //+1 for the last pixel to interpolate with, and +1 for rounding errors.
                Q_ASSERT(count <= buffer_size + 2); //length is supposed to be <= buffer_size and data->m11 < 1 in this case
                int f = 0;
                int lim = count;
                if (blendType == BlendTransformedBilinearTiled) {
                    x %= image_width;
                    if (x < 0) x += image_width;
                } else {
                    lim = qMin(count, image_x2-x+1);
                    if (x < image_x1) {
                        Q_ASSERT(x <= image_x2);
                        uint t = s1[image_x1];
                        uint b = s2[image_x1];
                        quint32 rb = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        quint32 ag = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        do {
                            intermediate_buffer[0][f] = rb;
                            intermediate_buffer[1][f] = ag;
                            f++;
                            x++;
                        } while (x < image_x1 && f < lim);
                    }
                }

                if (blendType != BlendTransformedBilinearTiled) {
#if defined(__SSE2__)
                    const __m128i disty_ = _mm_set1_epi16(disty);
                    const __m128i idisty_ = _mm_set1_epi16(idisty);
                    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);

                    lim -= 3;
                    for (; f < lim; x += 4, f += 4) {
                        // Load 4 pixels from s1, and split the alpha-green and red-blue component
                        __m128i top = _mm_loadu_si128((__m128i*)((const uint *)(s1)+x));
                        __m128i topAG = _mm_srli_epi16(top, 8);
                        __m128i topRB = _mm_and_si128(top, colorMask);
                        // Multiplies each colour component by idisty
                        topAG = _mm_mullo_epi16 (topAG, idisty_);
                        topRB = _mm_mullo_epi16 (topRB, idisty_);

                        // Same for the s2 vector
                        __m128i bottom = _mm_loadu_si128((__m128i*)((const uint *)(s2)+x));
                        __m128i bottomAG = _mm_srli_epi16(bottom, 8);
                        __m128i bottomRB = _mm_and_si128(bottom, colorMask);
                        bottomAG = _mm_mullo_epi16 (bottomAG, disty_);
                        bottomRB = _mm_mullo_epi16 (bottomRB, disty_);

                        // Add the values, and shift to only keep 8 significant bits per colors
                        __m128i rAG =_mm_add_epi16(topAG, bottomAG);
                        rAG = _mm_srli_epi16(rAG, 8);
                        _mm_storeu_si128((__m128i*)(&intermediate_buffer[1][f]), rAG);
                        __m128i rRB =_mm_add_epi16(topRB, bottomRB);
                        rRB = _mm_srli_epi16(rRB, 8);
                        _mm_storeu_si128((__m128i*)(&intermediate_buffer[0][f]), rRB);
                    }
#elif defined(__ARM_NEON__)
                    const int16x8_t disty_ = vdupq_n_s16(disty);
                    const int16x8_t idisty_ = vdupq_n_s16(idisty);
                    const int16x8_t colorMask = vdupq_n_s16(0x00ff);

                    lim -= 3;
                    for (; f < lim; x += 4, f += 4) {
                        // Load 4 pixels from s1, and split the alpha-green and red-blue component
                        int16x8_t top = vld1q_s16((int16_t*)((const uint *)(s1)+x));
                        int16x8_t topAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(top), 8));
                        int16x8_t topRB = vandq_s16(top, colorMask);
                        // Multiplies each colour component by idisty
                        topAG = vmulq_s16(topAG, idisty_);
                        topRB = vmulq_s16(topRB, idisty_);

                        // Same for the s2 vector
                        int16x8_t bottom = vld1q_s16((int16_t*)((const uint *)(s2)+x));
                        int16x8_t bottomAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(bottom), 8));
                        int16x8_t bottomRB = vandq_s16(bottom, colorMask);
                        bottomAG = vmulq_s16(bottomAG, disty_);
                        bottomRB = vmulq_s16(bottomRB, disty_);

                        // Add the values, and shift to only keep 8 significant bits per colors
                        int16x8_t rAG = vaddq_s16(topAG, bottomAG);
                        rAG = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rAG), 8));
                        vst1q_s16((int16_t*)(&intermediate_buffer[1][f]), rAG);
                        int16x8_t rRB = vaddq_s16(topRB, bottomRB);
                        rRB = vreinterpretq_s16_u16(vshrq_n_u16(vreinterpretq_u16_s16(rRB), 8));
                        vst1q_s16((int16_t*)(&intermediate_buffer[0][f]), rRB);
                    }
#endif
                }
                for (; f < count; f++) { // Same as above but without sse2
                    if (blendType == BlendTransformedBilinearTiled) {
                        if (x >= image_width) x -= image_width;
                    } else {
                        x = qMin(x, image_x2);
                    }

                    uint t = s1[x];
                    uint b = s2[x];

                    intermediate_buffer[0][f] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                    intermediate_buffer[1][f] = ((((t>>8) & 0xff00ff) * idisty + ((b>>8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                    x++;
                }
                // Now interpolate the values from the intermediate_buffer to get the final result.
                fx &= fixed_scale - 1;
                Q_ASSERT((fx >> 16) == 0);
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2 = x1 + 1;
                    Q_ASSERT(x1 >= 0);
                    Q_ASSERT(x2 < count);

                    int distx = (fx & 0x0000ffff) >> 8;
                    int idistx = 256 - distx;
                    int rb = ((intermediate_buffer[0][x1] * idistx + intermediate_buffer[0][x2] * distx) >> 8) & 0xff00ff;
                    int ag = (intermediate_buffer[1][x1] * idistx + intermediate_buffer[1][x2] * distx) & 0xff00ff00;
                    *b = rb | ag;
                    b++;
                    fx += fdx;
                }
            } else if ((fdx < 0 && fdx > -(fixed_scale / 8)) || fabs(data->m22) < (1./8.)) { // scale up more than 8x
                int y1 = (fy >> 16);
                int y2;
                fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
                const uint *s1 = (const uint *)data->texture.scanLine(y1);
                const uint *s2 = (const uint *)data->texture.scanLine(y2);
                int disty = (fy & 0x0000ffff) >> 8;
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    uint tl = s1[x1];
                    uint tr = s1[x2];
                    uint bl = s2[x1];
                    uint br = s2[x2];
                    int distx = (fx & 0x0000ffff) >> 8;
                    *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

                    fx += fdx;
                    ++b;
                }
            } else { //scale down
                int y1 = (fy >> 16);
                int y2;
                fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
                const uint *s1 = (const uint *)data->texture.scanLine(y1);
                const uint *s2 = (const uint *)data->texture.scanLine(y2);
                int disty = (fy & 0x0000ffff) >> 12;

                if (blendType != BlendTransformedBilinearTiled) {
#define BILINEAR_DOWNSCALE_BOUNDS_PROLOG \
                    while (b < end) { \
                        int x1 = (fx >> 16); \
                        int x2; \
                        fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2); \
                        if (x1 != x2) \
                            break; \
                        uint tl = s1[x1]; \
                        uint tr = s1[x2]; \
                        uint bl = s2[x1]; \
                        uint br = s2[x2]; \
                        int distx = (fx & 0x0000ffff) >> 12; \
                        *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty); \
                        fx += fdx; \
                        ++b; \
                    } \
                    uint *boundedEnd; \
                    if (fdx > 0) \
                        boundedEnd = qMin(end, buffer + uint((image_x2 - (fx >> 16)) / data->m11)); \
                    else \
                        boundedEnd = qMin(end, buffer + uint((image_x1 - (fx >> 16)) / data->m11)); \
                    boundedEnd -= 3;

#if defined(__SSE2__)
                    BILINEAR_DOWNSCALE_BOUNDS_PROLOG

                    const __m128i colorMask = _mm_set1_epi32(0x00ff00ff);
                    const __m128i v_256 = _mm_set1_epi16(256);
                    const __m128i v_disty = _mm_set1_epi16(disty);
                    __m128i v_fdx = _mm_set1_epi32(fdx*4);

                    ptrdiff_t secondLine = reinterpret_cast<const uint *>(s2) - reinterpret_cast<const uint *>(s1);

                    union Vect_buffer { __m128i vect; quint32 i[4]; };
                    Vect_buffer v_fx;

                    for (int i = 0; i < 4; i++) {
                        v_fx.i[i] = fx;
                        fx += fdx;
                    }

                    while (b < boundedEnd) {

                        Vect_buffer tl, tr, bl, br;

                        for (int i = 0; i < 4; i++) {
                            int x1 = v_fx.i[i] >> 16;
                            const uint *addr_tl = reinterpret_cast<const uint *>(s1) + x1;
                            const uint *addr_tr = addr_tl + 1;
                            tl.i[i] = *addr_tl;
                            tr.i[i] = *addr_tr;
                            bl.i[i] = *(addr_tl+secondLine);
                            br.i[i] = *(addr_tr+secondLine);
                        }
                        __m128i v_distx = _mm_srli_epi16(v_fx.vect, 12);
                        v_distx = _mm_shufflehi_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));
                        v_distx = _mm_shufflelo_epi16(v_distx, _MM_SHUFFLE(2,2,0,0));

                        interpolate_4_pixels_16_sse2(tl.vect, tr.vect, bl.vect, br.vect, v_distx, v_disty, colorMask, v_256, b);
                        b+=4;
                        v_fx.vect = _mm_add_epi32(v_fx.vect, v_fdx);
                    }
                    fx = v_fx.i[0];
#elif defined(__ARM_NEON__)
                    BILINEAR_DOWNSCALE_BOUNDS_PROLOG

                    const int16x8_t colorMask = vdupq_n_s16(0x00ff);
                    const int16x8_t invColorMask = vmvnq_s16(colorMask);
                    const int16x8_t v_256 = vdupq_n_s16(256);
                    const int16x8_t v_disty = vdupq_n_s16(disty);
                    const int16x8_t v_disty_ = vshlq_n_s16(v_disty, 4);
                    int32x4_t v_fdx = vdupq_n_s32(fdx*4);

                    ptrdiff_t secondLine = reinterpret_cast<const uint *>(s2) - reinterpret_cast<const uint *>(s1);

                    union Vect_buffer { int32x4_t vect; quint32 i[4]; };
                    Vect_buffer v_fx;

                    for (int i = 0; i < 4; i++) {
                        v_fx.i[i] = fx;
                        fx += fdx;
                    }

                    const int32x4_t v_ffff_mask = vdupq_n_s32(0x0000ffff);

                    while (b < boundedEnd) {

                        Vect_buffer tl, tr, bl, br;

                        Vect_buffer v_fx_shifted;
                        v_fx_shifted.vect = vshrq_n_s32(v_fx.vect, 16);

                        int32x4_t v_distx = vshrq_n_s32(vandq_s32(v_fx.vect, v_ffff_mask), 12);

                        for (int i = 0; i < 4; i++) {
                            int x1 = v_fx_shifted.i[i];
                            const uint *addr_tl = reinterpret_cast<const uint *>(s1) + x1;
                            const uint *addr_tr = addr_tl + 1;
                            tl.i[i] = *addr_tl;
                            tr.i[i] = *addr_tr;
                            bl.i[i] = *(addr_tl+secondLine);
                            br.i[i] = *(addr_tr+secondLine);
                        }

                        v_distx = vorrq_s32(v_distx, vshlq_n_s32(v_distx, 16));

                        interpolate_4_pixels_16_neon(vreinterpretq_s16_s32(tl.vect), vreinterpretq_s16_s32(tr.vect), vreinterpretq_s16_s32(bl.vect), vreinterpretq_s16_s32(br.vect), vreinterpretq_s16_s32(v_distx), v_disty, v_disty_, colorMask, invColorMask, v_256, b);
                        b+=4;
                        v_fx.vect = vaddq_s32(v_fx.vect, v_fdx);
                    }
                    fx = v_fx.i[0];
#endif
                }

                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    uint tl = s1[x1];
                    uint tr = s1[x2];
                    uint bl = s2[x1];
                    uint br = s2[x2];
                    int distx = (fx & 0x0000ffff) >> 12;
                    *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
                    fx += fdx;
                    ++b;
                }
            }
        } else { //rotation
            if (fabs(data->m11) > 8 || fabs(data->m22) > 8) {
                //if we are zooming more than 8 times, we use 8bit precision for the position.
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    int y1 = (fy >> 16);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

                    const uint *s1 = (const uint *)data->texture.scanLine(y1);
                    const uint *s2 = (const uint *)data->texture.scanLine(y2);

                    uint tl = s1[x1];
                    uint tr = s1[x2];
                    uint bl = s2[x1];
                    uint br = s2[x2];

                    int distx = (fx & 0x0000ffff) >> 8;
                    int disty = (fy & 0x0000ffff) >> 8;

                    *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

                    fx += fdx;
                    fy += fdy;
                    ++b;
                }
            } else {
                //we are zooming less than 8x, use 4bit precision
                while (b < end) {
                    int x1 = (fx >> 16);
                    int x2;
                    int y1 = (fy >> 16);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

                    const uint *s1 = (const uint *)data->texture.scanLine(y1);
                    const uint *s2 = (const uint *)data->texture.scanLine(y2);

                    uint tl = s1[x1];
                    uint tr = s1[x2];
                    uint bl = s2[x1];
                    uint br = s2[x2];

                    int distx = (fx & 0x0000ffff) >> 12;
                    int disty = (fy & 0x0000ffff) >> 12;

                    *b = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);

                    fx += fdx;
                    fy += fdy;
                    ++b;
                }
            }
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        while (b < end) {
            const qreal iw = fw == 0 ? 1 : 1 / fw;
            const qreal px = fx * iw - qreal(0.5);
            const qreal py = fy * iw - qreal(0.5);

            int x1 = int(px) - (px < 0);
            int x2;
            int y1 = int(py) - (py < 0);
            int y2;

            int distx = int((px - x1) * 256);
            int disty = int((py - y1) * 256);

            fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
            fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

            const uint *s1 = (const uint *)data->texture.scanLine(y1);
            const uint *s2 = (const uint *)data->texture.scanLine(y2);

            uint tl = s1[x1];
            uint tr = s1[x2];
            uint bl = s2[x1];
            uint br = s2[x2];

            *b = interpolate_4_pixels(tl, tr, bl, br, distx, disty);

            fx += fdx;
            fy += fdy;
            fw += fdw;
            //force increment to avoid /0
            if (!fw) {
                fw += fdw;
            }
            ++b;
        }
    }

    return buffer;
}

// blendType = BlendTransformedBilinear or BlendTransformedBilinearTiled
template<TextureBlendType blendType>
static const uint *QT_FASTCALL fetchTransformedBilinear(uint *buffer, const Operator *,
                                                        const QSpanData *data, int y, int x, int length)
{
    const QPixelLayout *layout = &qPixelLayouts[data->texture.format];
    const QRgb *clut = data->texture.colorTable ? data->texture.colorTable->constData() : 0;

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    int image_x1 = data->texture.x1;
    int image_y1 = data->texture.y1;
    int image_x2 = data->texture.x2 - 1;
    int image_y2 = data->texture.y2 - 1;

    const qreal cx = x + qreal(0.5);
    const qreal cy = y + qreal(0.5);

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        int fx = int((data->m21 * cy + data->m11 * cx + data->dx) * fixed_scale);
        int fy = int((data->m22 * cy + data->m12 * cx + data->dy) * fixed_scale);

        fx -= half_point;
        fy -= half_point;

        if (fdy == 0) { //simple scale, no rotation
            int y1 = (fy >> 16);
            int y2;
            fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);
            const uchar *s1 = data->texture.scanLine(y1);
            const uchar *s2 = data->texture.scanLine(y2);

            if (fdx <= fixed_scale && fdx > 0) { // scale up on X
                int disty = (fy & 0x0000ffff) >> 8;
                int idisty = 256 - disty;
                int x = fx >> 16;

                // The idea is first to do the interpolation between the row s1 and the row s2
                // into an intermediate buffer, then we interpolate between two pixel of this buffer.
                FetchPixelsFunc fetch = qFetchPixels[layout->bpp];
                uint buf1[buffer_size + 2];
                uint buf2[buffer_size + 2];
                const uint *ptr1;
                const uint *ptr2;

                int count = qCeil(length * data->m11) + 2; //+1 for the last pixel to interpolate with, and +1 for rounding errors.
                Q_ASSERT(count <= buffer_size + 2); //length is supposed to be <= buffer_size and data->m11 < 1 in this case

                if (blendType == BlendTransformedBilinearTiled) {
                    x %= image_width;
                    if (x < 0)
                        x += image_width;
                    int len1 = qMin(count, image_width - x);
                    int len2 = qMin(x, count - len1);

                    ptr1 = fetch(buf1, s1, x, len1);
                    ptr1 = layout->convertToARGB32PM(buf1, ptr1, len1, layout, clut);
                    ptr2 = fetch(buf2, s2, x, len1);
                    ptr2 = layout->convertToARGB32PM(buf2, ptr2, len1, layout, clut);
                    for (int i = 0; i < len1; ++i) {
                        uint t = ptr1[i];
                        uint b = ptr2[i];
                        buf1[i] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        buf2[i] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                    }

                    if (len2) {
                        ptr1 = fetch(buf1 + len1, s1, 0, len2);
                        ptr1 = layout->convertToARGB32PM(buf1 + len1, ptr1, len2, layout, clut);
                        ptr2 = fetch(buf2 + len1, s2, 0, len2);
                        ptr2 = layout->convertToARGB32PM(buf2 + len1, ptr2, len2, layout, clut);
                        for (int i = 0; i < len2; ++i) {
                            uint t = ptr1[i];
                            uint b = ptr2[i];
                            buf1[i + len1] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                            buf2[i + len1] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        }
                    }
                    for (int i = image_width; i < count; ++i) {
                        buf1[i] = buf1[i - image_width];
                        buf2[i] = buf2[i - image_width];
                    }
                } else {
                    int start = qMax(x, image_x1);
                    int end = qMin(x + count, image_x2 + 1);
                    int len = qMax(1, end - start);
                    int leading = start - x;

                    ptr1 = fetch(buf1 + leading, s1, start, len);
                    ptr1 = layout->convertToARGB32PM(buf1 + leading, ptr1, len, layout, clut);
                    ptr2 = fetch(buf2 + leading, s2, start, len);
                    ptr2 = layout->convertToARGB32PM(buf2 + leading, ptr2, len, layout, clut);

                    for (int i = 0; i < len; ++i) {
                        uint t = ptr1[i];
                        uint b = ptr2[i];
                        buf1[i + leading] = (((t & 0xff00ff) * idisty + (b & 0xff00ff) * disty) >> 8) & 0xff00ff;
                        buf2[i + leading] = ((((t >> 8) & 0xff00ff) * idisty + ((b >> 8) & 0xff00ff) * disty) >> 8) & 0xff00ff;
                    }

                    for (int i = 0; i < leading; ++i) {
                        buf1[i] = buf1[leading];
                        buf2[i] = buf2[leading];
                    }
                    for (int i = leading + len; i < count; ++i) {
                        buf1[i] = buf1[i - 1];
                        buf2[i] = buf2[i - 1];
                    }
                }

                // Now interpolate the values from the intermediate_buffer to get the final result.
                fx &= fixed_scale - 1;
                Q_ASSERT((fx >> 16) == 0);
                for (int i = 0; i < length; ++i) {
                    int x1 = (fx >> 16);
                    int x2 = x1 + 1;
                    Q_ASSERT(x1 >= 0);
                    Q_ASSERT(x2 < count);

                    int distx = (fx & 0x0000ffff) >> 8;
                    int idistx = 256 - distx;
                    int rb = ((buf1[x1] * idistx + buf1[x2] * distx) >> 8) & 0xff00ff;
                    int ag = (buf2[x1] * idistx + buf2[x2] * distx) & 0xff00ff00;
                    buffer[i] = rb | ag;
                    fx += fdx;
                }
            } else {
                FetchPixelFunc fetch = qFetchPixel[layout->bpp];
                uint buf1[buffer_size];
                uint buf2[buffer_size];
                uint *b = buffer;
                while (length) {
                    int len = qMin(length, buffer_size / 2);
                    int fracX = fx;
                    for (int i = 0; i < len; ++i) {
                        int x1 = (fx >> 16);
                        int x2;
                        fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);

                        buf1[i * 2 + 0] = fetch(s1, x1);
                        buf1[i * 2 + 1] = fetch(s1, x2);
                        buf2[i * 2 + 0] = fetch(s2, x1);
                        buf2[i * 2 + 1] = fetch(s2, x2);

                        fx += fdx;
                    }
                    layout->convertToARGB32PM(buf1, buf1, len * 2, layout, clut);
                    layout->convertToARGB32PM(buf2, buf2, len * 2, layout, clut);

                    if ((fdx < 0 && fdx > -(fixed_scale / 8)) || fabs(data->m22) < (1./8.)) { // scale up more than 8x
                        int disty = (fy & 0x0000ffff) >> 8;
                        for (int i = 0; i < len; ++i) {
                            uint tl = buf1[i * 2 + 0];
                            uint tr = buf1[i * 2 + 1];
                            uint bl = buf2[i * 2 + 0];
                            uint br = buf2[i * 2 + 1];
                            int distx = (fracX & 0x0000ffff) >> 8;
                            b[i] = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
                            fracX += fdx;
                        }
                    } else { //scale down
                        int disty = (fy & 0x0000ffff) >> 12;
                        for (int i = 0; i < len; ++i) {
                            uint tl = buf1[i * 2 + 0];
                            uint tr = buf1[i * 2 + 1];
                            uint bl = buf2[i * 2 + 0];
                            uint br = buf2[i * 2 + 1];
                            int distx = (fracX & 0x0000ffff) >> 12;
                            b[i] = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
                            fracX += fdx;
                        }
                    }
                    length -= len;
                    b += len;
                }
            }
        } else { //rotation
            FetchPixelFunc fetch = qFetchPixel[layout->bpp];
            uint buf1[buffer_size];
            uint buf2[buffer_size];
            uint *b = buffer;

            while (length) {
                int len = qMin(length, buffer_size / 2);
                int fracX = fx;
                int fracY = fy;
                for (int i = 0; i < len; ++i) {
                    int x1 = (fx >> 16);
                    int x2;
                    int y1 = (fy >> 16);
                    int y2;
                    fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                    fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

                    const uchar *s1 = data->texture.scanLine(y1);
                    const uchar *s2 = data->texture.scanLine(y2);

                    buf1[i * 2 + 0] = fetch(s1, x1);
                    buf1[i * 2 + 1] = fetch(s1, x2);
                    buf2[i * 2 + 0] = fetch(s2, x1);
                    buf2[i * 2 + 1] = fetch(s2, x2);

                    fx += fdx;
                    fy += fdy;
                }
                layout->convertToARGB32PM(buf1, buf1, len * 2, layout, clut);
                layout->convertToARGB32PM(buf2, buf2, len * 2, layout, clut);

                if (fabs(data->m11) > 8 || fabs(data->m22) > 8) {
                    //if we are zooming more than 8 times, we use 8bit precision for the position.
                    for (int i = 0; i < len; ++i) {
                        uint tl = buf1[i * 2 + 0];
                        uint tr = buf1[i * 2 + 1];
                        uint bl = buf2[i * 2 + 0];
                        uint br = buf2[i * 2 + 1];

                        int distx = (fracX & 0x0000ffff) >> 8;
                        int disty = (fracY & 0x0000ffff) >> 8;

                        b[i] = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
                        fracX += fdx;
                        fracY += fdy;
                    }
                } else {
                    //we are zooming less than 8x, use 4bit precision
                    for (int i = 0; i < len; ++i) {
                        uint tl = buf1[i * 2 + 0];
                        uint tr = buf1[i * 2 + 1];
                        uint bl = buf2[i * 2 + 0];
                        uint br = buf2[i * 2 + 1];

                        int distx = (fracX & 0x0000ffff) >> 12;
                        int disty = (fracY & 0x0000ffff) >> 12;

                        b[i] = interpolate_4_pixels_16(tl, tr, bl, br, distx, disty);
                        fracX += fdx;
                        fracY += fdy;
                    }
                }

                length -= len;
                b += len;
            }
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        qreal fx = data->m21 * cy + data->m11 * cx + data->dx;
        qreal fy = data->m22 * cy + data->m12 * cx + data->dy;
        qreal fw = data->m23 * cy + data->m13 * cx + data->m33;

        FetchPixelFunc fetch = qFetchPixel[layout->bpp];
        uint buf1[buffer_size];
        uint buf2[buffer_size];
        uint *b = buffer;

        int distxs[buffer_size / 2];
        int distys[buffer_size / 2];

        while (length) {
            int len = qMin(length, buffer_size / 2);
            for (int i = 0; i < len; ++i) {
                const qreal iw = fw == 0 ? 1 : 1 / fw;
                const qreal px = fx * iw - qreal(0.5);
                const qreal py = fy * iw - qreal(0.5);

                int x1 = int(px) - (px < 0);
                int x2;
                int y1 = int(py) - (py < 0);
                int y2;

                distxs[i] = int((px - x1) * 256);
                distys[i] = int((py - y1) * 256);

                fetchTransformedBilinear_pixelBounds<blendType>(image_width, image_x1, image_x2, x1, x2);
                fetchTransformedBilinear_pixelBounds<blendType>(image_height, image_y1, image_y2, y1, y2);

                const uchar *s1 = data->texture.scanLine(y1);
                const uchar *s2 = data->texture.scanLine(y2);

                buf1[i * 2 + 0] = fetch(s1, x1);
                buf1[i * 2 + 1] = fetch(s1, x2);
                buf2[i * 2 + 0] = fetch(s2, x1);
                buf2[i * 2 + 1] = fetch(s2, x2);

                fx += fdx;
                fy += fdy;
                fw += fdw;
                //force increment to avoid /0
                if (!fw)
                    fw += fdw;
            }

            layout->convertToARGB32PM(buf1, buf1, len * 2, layout, clut);
            layout->convertToARGB32PM(buf2, buf2, len * 2, layout, clut);

            for (int i = 0; i < len; ++i) {
                int distx = distxs[i];
                int disty = distys[i];

                uint tl = buf1[i * 2 + 0];
                uint tr = buf1[i * 2 + 1];
                uint bl = buf2[i * 2 + 0];
                uint br = buf2[i * 2 + 1];

                b[i] = interpolate_4_pixels(tl, tr, bl, br, distx, disty);
            }
            length -= len;
            b += len;
        }
    }

    return buffer;
}

static SourceFetchProc sourceFetch[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        fetchUntransformed,         // Mono
        fetchUntransformed,         // MonoLsb
        fetchUntransformed,         // Indexed8
        fetchUntransformedARGB32PM, // RGB32
        fetchUntransformed,         // ARGB32
        fetchUntransformedARGB32PM, // ARGB32_Premultiplied
        fetchUntransformedRGB16,    // RGB16
        fetchUntransformed,         // ARGB8565_Premultiplied
        fetchUntransformed,         // RGB666
        fetchUntransformed,         // ARGB6666_Premultiplied
        fetchUntransformed,         // RGB555
        fetchUntransformed,         // ARGB8555_Premultiplied
        fetchUntransformed,         // RGB888
        fetchUntransformed,         // RGB444
        fetchUntransformed,         // ARGB4444_Premultiplied
        fetchUntransformed,         // RGBX8888
        fetchUntransformed,         // RGBA8888
        fetchUntransformed          // RGBA8888_Premultiplied
    },
    // Tiled
    {
        0, // Invalid
        fetchUntransformed,         // Mono
        fetchUntransformed,         // MonoLsb
        fetchUntransformed,         // Indexed8
        fetchUntransformedARGB32PM, // RGB32
        fetchUntransformed,         // ARGB32
        fetchUntransformedARGB32PM, // ARGB32_Premultiplied
        fetchUntransformedRGB16,    // RGB16
        fetchUntransformed,         // ARGB8565_Premultiplied
        fetchUntransformed,         // RGB666
        fetchUntransformed,         // ARGB6666_Premultiplied
        fetchUntransformed,         // RGB555
        fetchUntransformed,         // ARGB8555_Premultiplied
        fetchUntransformed,         // RGB888
        fetchUntransformed,         // RGB444
        fetchUntransformed,         // ARGB4444_Premultiplied
        fetchUntransformed,         // RGBX8888
        fetchUntransformed,         // RGBA8888
        fetchUntransformed          // RGBA8888_Premultiplied
    },
    // Transformed
    {
        0, // Invalid
        fetchTransformed<BlendTransformed>,         // Mono
        fetchTransformed<BlendTransformed>,         // MonoLsb
        fetchTransformed<BlendTransformed>,         // Indexed8
        fetchTransformedARGB32PM<BlendTransformed>, // RGB32
        fetchTransformed<BlendTransformed>,         // ARGB32
        fetchTransformedARGB32PM<BlendTransformed>, // ARGB32_Premultiplied
        fetchTransformed<BlendTransformed>,         // RGB16
        fetchTransformed<BlendTransformed>,         // ARGB8565_Premultiplied
        fetchTransformed<BlendTransformed>,         // RGB666
        fetchTransformed<BlendTransformed>,         // ARGB6666_Premultiplied
        fetchTransformed<BlendTransformed>,         // RGB555
        fetchTransformed<BlendTransformed>,         // ARGB8555_Premultiplied
        fetchTransformed<BlendTransformed>,         // RGB888
        fetchTransformed<BlendTransformed>,         // RGB444
        fetchTransformed<BlendTransformed>,         // ARGB4444_Premultiplied
        fetchTransformed<BlendTransformed>,         // RGBX8888
        fetchTransformed<BlendTransformed>,         // RGBA8888
        fetchTransformed<BlendTransformed>,         // RGBA8888_Premultiplied
    },
    {
        0, // TransformedTiled
        fetchTransformed<BlendTransformedTiled>,            // Mono
        fetchTransformed<BlendTransformedTiled>,            // MonoLsb
        fetchTransformed<BlendTransformedTiled>,            // Indexed8
        fetchTransformedARGB32PM<BlendTransformedTiled>,    // RGB32
        fetchTransformed<BlendTransformedTiled>,            // ARGB32
        fetchTransformedARGB32PM<BlendTransformedTiled>,    // ARGB32_Premultiplied
        fetchTransformed<BlendTransformedTiled>,            // RGB16
        fetchTransformed<BlendTransformedTiled>,            // ARGB8565_Premultiplied
        fetchTransformed<BlendTransformedTiled>,            // RGB666
        fetchTransformed<BlendTransformedTiled>,            // ARGB6666_Premultiplied
        fetchTransformed<BlendTransformedTiled>,            // RGB555
        fetchTransformed<BlendTransformedTiled>,            // ARGB8555_Premultiplied
        fetchTransformed<BlendTransformedTiled>,            // RGB888
        fetchTransformed<BlendTransformedTiled>,            // RGB444
        fetchTransformed<BlendTransformedTiled>,            // ARGB4444_Premultiplied
        fetchTransformed<BlendTransformedTiled>,            // RGBX8888
        fetchTransformed<BlendTransformedTiled>,            // RGBA8888
        fetchTransformed<BlendTransformedTiled>,            // RGBA8888_Premultiplied
    },
    {
        0, // Bilinear
        fetchTransformedBilinear<BlendTransformedBilinear>,         // Mono
        fetchTransformedBilinear<BlendTransformedBilinear>,         // MonoLsb
        fetchTransformedBilinear<BlendTransformedBilinear>,         // Indexed8
        fetchTransformedBilinearARGB32PM<BlendTransformedBilinear>, // RGB32
        fetchTransformedBilinear<BlendTransformedBilinear>,         // ARGB32
        fetchTransformedBilinearARGB32PM<BlendTransformedBilinear>, // ARGB32_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGB16
        fetchTransformedBilinear<BlendTransformedBilinear>,         // ARGB8565_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGB666
        fetchTransformedBilinear<BlendTransformedBilinear>,         // ARGB6666_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGB555
        fetchTransformedBilinear<BlendTransformedBilinear>,         // ARGB8555_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGB888
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGB444
        fetchTransformedBilinear<BlendTransformedBilinear>,         // ARGB4444_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGBX8888
        fetchTransformedBilinear<BlendTransformedBilinear>,         // RGBA8888
        fetchTransformedBilinear<BlendTransformedBilinear>          // RGBA8888_Premultiplied
    },
    {
        0, // BilinearTiled
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // Mono
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // MonoLsb
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // Indexed8
        fetchTransformedBilinearARGB32PM<BlendTransformedBilinearTiled>,    // RGB32
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // ARGB32
        fetchTransformedBilinearARGB32PM<BlendTransformedBilinearTiled>,    // ARGB32_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGB16
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // ARGB8565_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGB666
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // ARGB6666_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGB555
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // ARGB8555_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGB888
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGB444
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // ARGB4444_Premultiplied
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGBX8888
        fetchTransformedBilinear<BlendTransformedBilinearTiled>,            // RGBA8888
        fetchTransformedBilinear<BlendTransformedBilinearTiled>             // RGBA8888_Premultiplied
    },
};

#define FIXPT_BITS 8
#define FIXPT_SIZE (1<<FIXPT_BITS)

static uint qt_gradient_pixel_fixed(const QGradientData *data, int fixed_pos)
{
    int ipos = (fixed_pos + (FIXPT_SIZE / 2)) >> FIXPT_BITS;
    return data->colorTable[qt_gradient_clamp(data, ipos)];
}

static void QT_FASTCALL getLinearGradientValues(LinearGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.linear.end.x - data->gradient.linear.origin.x;
    v->dy = data->gradient.linear.end.y - data->gradient.linear.origin.y;
    v->l = v->dx * v->dx + v->dy * v->dy;
    v->off = 0;
    if (v->l != 0) {
        v->dx /= v->l;
        v->dy /= v->l;
        v->off = -v->dx * data->gradient.linear.origin.x - v->dy * data->gradient.linear.origin.y;
    }
}

static const uint * QT_FASTCALL qt_fetch_linear_gradient(uint *buffer, const Operator *op, const QSpanData *data,
                                                         int y, int x, int length)
{
    const uint *b = buffer;
    qreal t, inc;

    bool affine = true;
    qreal rx=0, ry=0;
    if (op->linear.l == 0) {
        t = inc = 0;
    } else {
        rx = data->m21 * (y + qreal(0.5)) + data->m11 * (x + qreal(0.5)) + data->dx;
        ry = data->m22 * (y + qreal(0.5)) + data->m12 * (x + qreal(0.5)) + data->dy;
        t = op->linear.dx*rx + op->linear.dy*ry + op->linear.off;
        inc = op->linear.dx * data->m11 + op->linear.dy * data->m12;
        affine = !data->m13 && !data->m23;

        if (affine) {
            t *= (GRADIENT_STOPTABLE_SIZE - 1);
            inc *= (GRADIENT_STOPTABLE_SIZE - 1);
        }
    }

    const uint *end = buffer + length;
    if (affine) {
        if (inc > qreal(-1e-5) && inc < qreal(1e-5)) {
            QT_MEMFILL_UINT(buffer, length, qt_gradient_pixel_fixed(&data->gradient, int(t * FIXPT_SIZE)));
        } else {
            if (t+inc*length < qreal(INT_MAX >> (FIXPT_BITS + 1)) &&
                t+inc*length > qreal(INT_MIN >> (FIXPT_BITS + 1))) {
                // we can use fixed point math
                int t_fixed = int(t * FIXPT_SIZE);
                int inc_fixed = int(inc * FIXPT_SIZE);
                while (buffer < end) {
                    *buffer = qt_gradient_pixel_fixed(&data->gradient, t_fixed);
                    t_fixed += inc_fixed;
                    ++buffer;
                }
            } else {
                // we have to fall back to float math
                while (buffer < end) {
                    *buffer = qt_gradient_pixel(&data->gradient, t/GRADIENT_STOPTABLE_SIZE);
                    t += inc;
                    ++buffer;
                }
            }
        }
    } else { // fall back to float math here as well
        qreal rw = data->m23 * (y + qreal(0.5)) + data->m13 * (x + qreal(0.5)) + data->m33;
        while (buffer < end) {
            qreal x = rx/rw;
            qreal y = ry/rw;
            t = (op->linear.dx*x + op->linear.dy *y) + op->linear.off;

            *buffer = qt_gradient_pixel(&data->gradient, t);
            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }

    return b;
}

static void QT_FASTCALL getRadialGradientValues(RadialGradientValues *v, const QSpanData *data)
{
    v->dx = data->gradient.radial.center.x - data->gradient.radial.focal.x;
    v->dy = data->gradient.radial.center.y - data->gradient.radial.focal.y;

    v->dr = data->gradient.radial.center.radius - data->gradient.radial.focal.radius;
    v->sqrfr = data->gradient.radial.focal.radius * data->gradient.radial.focal.radius;

    v->a = v->dr * v->dr - v->dx*v->dx - v->dy*v->dy;
    v->inv2a = 1 / (2 * v->a);

    v->extended = !qFuzzyIsNull(data->gradient.radial.focal.radius) || v->a <= 0;
}

class RadialFetchPlain
{
public:
    static inline void fetch(uint *buffer, uint *end, const Operator *op, const QSpanData *data, qreal det,
                             qreal delta_det, qreal delta_delta_det, qreal b, qreal delta_b)
    {
        if (op->radial.extended) {
            while (buffer < end) {
                quint32 result = 0;
                if (det >= 0) {
                    qreal w = qSqrt(det) - b;
                    if (data->gradient.radial.focal.radius + op->radial.dr * w >= 0)
                        result = qt_gradient_pixel(&data->gradient, w);
                }

                *buffer = result;

                det += delta_det;
                delta_det += delta_delta_det;
                b += delta_b;

                ++buffer;
            }
        } else {
            while (buffer < end) {
                *buffer++ = qt_gradient_pixel(&data->gradient, qSqrt(det) - b);

                det += delta_det;
                delta_det += delta_delta_det;
                b += delta_b;
            }
        }
    }
};

const uint * QT_FASTCALL qt_fetch_radial_gradient_plain(uint *buffer, const Operator *op, const QSpanData *data,
                                                        int y, int x, int length)
{
    return qt_fetch_radial_gradient_template<RadialFetchPlain>(buffer, op, data, y, x, length);
}

static SourceFetchProc qt_fetch_radial_gradient = qt_fetch_radial_gradient_plain;

static const uint * QT_FASTCALL qt_fetch_conical_gradient(uint *buffer, const Operator *, const QSpanData *data,
                                                          int y, int x, int length)
{
    const uint *b = buffer;
    qreal rx = data->m21 * (y + qreal(0.5))
               + data->dx + data->m11 * (x + qreal(0.5));
    qreal ry = data->m22 * (y + qreal(0.5))
               + data->dy + data->m12 * (x + qreal(0.5));
    bool affine = !data->m13 && !data->m23;

    const uint *end = buffer + length;
    if (affine) {
        rx -= data->gradient.conical.center.x;
        ry -= data->gradient.conical.center.y;
        while (buffer < end) {
            qreal angle = qAtan2(ry, rx) + data->gradient.conical.angle;

            *buffer = qt_gradient_pixel(&data->gradient, 1 - angle / (2*Q_PI));

            rx += data->m11;
            ry += data->m12;
            ++buffer;
        }
    } else {
        qreal rw = data->m23 * (y + qreal(0.5))
                   + data->m33 + data->m13 * (x + qreal(0.5));
        if (!rw)
            rw = 1;
        while (buffer < end) {
            qreal angle = qAtan2(ry/rw - data->gradient.conical.center.x,
                                rx/rw - data->gradient.conical.center.y)
                          + data->gradient.conical.angle;

            *buffer = qt_gradient_pixel(&data->gradient, 1. - angle / (2*Q_PI));

            rx += data->m11;
            ry += data->m12;
            rw += data->m13;
            if (!rw) {
                rw += data->m13;
            }
            ++buffer;
        }
    }
    return b;
}

#if defined(Q_CC_RVCT)
// Force ARM code generation for comp_func_* -methods
#  pragma push
#  pragma arm
#  if defined(Q_PROCESSOR_ARM_V6)
static __forceinline void preload(const uint *start)
{
    asm( "pld [start]" );
}
static const uint L2CacheLineLength = 32;
static const uint L2CacheLineLengthInInts = L2CacheLineLength/sizeof(uint);
#    define PRELOAD_INIT(x) preload(x);
#    define PRELOAD_INIT2(x,y) PRELOAD_INIT(x) PRELOAD_INIT(y)
#    define PRELOAD_COND(x) if (((uint)&x[i])%L2CacheLineLength == 0) preload(&x[i] + L2CacheLineLengthInInts);
// Two consecutive preloads stall, so space them out a bit by using different modulus.
#    define PRELOAD_COND2(x,y) if (((uint)&x[i])%L2CacheLineLength == 0) preload(&x[i] + L2CacheLineLengthInInts); \
         if (((uint)&y[i])%L2CacheLineLength == 16) preload(&y[i] + L2CacheLineLengthInInts);
#  endif // Q_PROCESSOR_ARM_V6
#endif // Q_CC_RVCT

#if !defined(Q_CC_RVCT) || !defined(Q_PROCESSOR_ARM_V6)
#    define PRELOAD_INIT(x)
#    define PRELOAD_INIT2(x,y)
#    define PRELOAD_COND(x)
#    define PRELOAD_COND2(x,y)
#endif

/* The constant alpha factor describes an alpha factor that gets applied
   to the result of the composition operation combining it with the destination.

   The intent is that if const_alpha == 0. we get back dest, and if const_alpha == 1.
   we get the unmodified operation

   result = src op dest
   dest = result * const_alpha + dest * (1. - const_alpha)

   This means that in the comments below, the first line is the const_alpha==255 case, the
   second line the general one.

   In the lines below:
   s == src, sa == alpha(src), sia = 1 - alpha(src)
   d == dest, da == alpha(dest), dia = 1 - alpha(dest)
   ca = const_alpha, cia = 1 - const_alpha

   The methods exist in two variants. One where we have a constant source, the other
   where the source is an array of pixels.
*/

/*
  result = 0
  d = d * cia
*/
#define comp_func_Clear_impl(dest, length, const_alpha)\
{\
    if (const_alpha == 255) {\
        QT_MEMFILL_UINT(dest, length, 0);\
    } else {\
        int ialpha = 255 - const_alpha;\
        PRELOAD_INIT(dest)\
        for (int i = 0; i < length; ++i) {\
            PRELOAD_COND(dest)\
            dest[i] = BYTE_MUL(dest[i], ialpha);\
        }\
    }\
}

void QT_FASTCALL comp_func_solid_Clear(uint *dest, int length, uint, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

void QT_FASTCALL comp_func_Clear(uint *dest, const uint *, int length, uint const_alpha)
{
    comp_func_Clear_impl(dest, length, const_alpha);
}

/*
  result = s
  dest = s * ca + d * cia
*/
void QT_FASTCALL comp_func_solid_Source(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        int ialpha = 255 - const_alpha;
        color = BYTE_MUL(color, const_alpha);
        PRELOAD_INIT(dest)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = color + BYTE_MUL(dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_Source(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        int ialpha = 255 - const_alpha;
        PRELOAD_INIT2(dest, src)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = INTERPOLATE_PIXEL_255(src[i], const_alpha, dest[i], ialpha);
        }
    }
}

void QT_FASTCALL comp_func_solid_Destination(uint *, int, uint, uint)
{
}

void QT_FASTCALL comp_func_Destination(uint *, const uint *, int, uint)
{
}

/*
  result = s + d * sia
  dest = (s + d * sia) * ca + d * cia
       = s * ca + d * (sia * ca + cia)
       = s * ca + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_SourceOver(uint *dest, int length, uint color, uint const_alpha)
{
    if ((const_alpha & qAlpha(color)) == 255) {
        QT_MEMFILL_UINT(dest, length, color);
    } else {
        if (const_alpha != 255)
            color = BYTE_MUL(color, const_alpha);
        PRELOAD_INIT(dest)
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = color + BYTE_MUL(dest[i], qAlpha(~color));
        }
    }
}

void QT_FASTCALL comp_func_SourceOver(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            if (s >= 0xff000000)
                dest[i] = s;
            else if (s != 0)
                dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = s + BYTE_MUL(dest[i], qAlpha(~s));
        }
    }
}

/*
  result = d + s * dia
  dest = (d + s * dia) * ca + d * cia
       = d + s * dia * ca
*/
void QT_FASTCALL comp_func_solid_DestinationOver(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = d + BYTE_MUL(color, qAlpha(~d));
    }
}

void QT_FASTCALL comp_func_DestinationOver(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            dest[i] = d + BYTE_MUL(src[i], qAlpha(~d));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = d + BYTE_MUL(s, qAlpha(~d));
        }
    }
}

/*
  result = s * da
  dest = s * da * ca + d * cia
*/
void QT_FASTCALL comp_func_solid_SourceIn(uint *dest, int length, uint color, uint const_alpha)
{
    PRELOAD_INIT(dest)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = BYTE_MUL(color, qAlpha(dest[i]));
        }
    } else {
        color = BYTE_MUL(color, const_alpha);
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceIn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(src[i], qAlpha(dest[i]));
        }
    } else {
        uint cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, cia);
        }
    }
}

/*
  result = d * sa
  dest = d * sa * ca + d * cia
       = d * (sa * ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationIn(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    }
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void QT_FASTCALL comp_func_DestinationIn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(dest[i], qAlpha(src[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint a = BYTE_MUL(qAlpha(src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], a);
        }
    }
}

/*
  result = s * dia
  dest = s * dia * ca + d * cia
*/

void QT_FASTCALL comp_func_solid_SourceOut(uint *dest, int length, uint color, uint const_alpha)
{
    PRELOAD_INIT(dest)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            dest[i] = BYTE_MUL(color, qAlpha(~dest[i]));
        }
    } else {
        color = BYTE_MUL(color, const_alpha);
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND(dest)
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, cia);
        }
    }
}

void QT_FASTCALL comp_func_SourceOut(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(src[i], qAlpha(~dest[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, cia);
        }
    }
}

/*
  result = d * sia
  dest = d * sia * ca + d * cia
       = d * (sia * ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationOut(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(~color);
    if (const_alpha != 255)
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = BYTE_MUL(dest[i], a);
    }
}

void QT_FASTCALL comp_func_DestinationOut(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            dest[i] = BYTE_MUL(dest[i], qAlpha(~src[i]));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint sia = BYTE_MUL(qAlpha(~src[i]), const_alpha) + cia;
            dest[i] = BYTE_MUL(dest[i], sia);
        }
    }
}

/*
  result = s*da + d*sia
  dest = s*da*ca + d*sia*ca + d *cia
       = s*ca * da + d * (sia*ca + cia)
       = s*ca * da + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_SourceAtop(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
    }
    uint sia = qAlpha(~color);
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(dest[i]), dest[i], sia);
    }
}

void QT_FASTCALL comp_func_SourceAtop(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
    }
}

/*
  result = d*sa + s*dia
  dest = d*sa*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sa*ca + cia)
*/
void QT_FASTCALL comp_func_solid_DestinationAtop(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
        a = qAlpha(color) + 255 - const_alpha;
    }
    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(d, a, color, qAlpha(~d));
    }
}

void QT_FASTCALL comp_func_DestinationAtop(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = src[i];
            uint d = dest[i];
            dest[i] = INTERPOLATE_PIXEL_255(d, qAlpha(s), s, qAlpha(~d));
        }
    } else {
        int cia = 255 - const_alpha;
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint s = BYTE_MUL(src[i], const_alpha);
            uint d = dest[i];
            uint a = qAlpha(s) + cia;
            dest[i] = INTERPOLATE_PIXEL_255(d, a, s, qAlpha(~d));
        }
    }
}

/*
  result = d*sia + s*dia
  dest = d*sia*ca + s*dia*ca + d *cia
       = s*ca * dia + d * (sia*ca + cia)
       = s*ca * dia + d * (1 - sa*ca)
*/
void QT_FASTCALL comp_func_solid_XOR(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    uint sia = qAlpha(~color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        dest[i] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, sia);
    }
}

void QT_FASTCALL comp_func_XOR(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    PRELOAD_INIT2(dest, src)
    if (const_alpha == 255) {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = src[i];
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    } else {
        for (int i = 0; i < length; ++i) {
            PRELOAD_COND2(dest, src)
            uint d = dest[i];
            uint s = BYTE_MUL(src[i], const_alpha);
            dest[i] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
    }
}

struct QFullCoverage {
    inline void store(uint *dest, const uint src) const
    {
        *dest = src;
    }
};

struct QPartialCoverage {
    inline QPartialCoverage(uint const_alpha)
        : ca(const_alpha)
        , ica(255 - const_alpha)
    {
    }

    inline void store(uint *dest, const uint src) const
    {
        *dest = INTERPOLATE_PIXEL_255(src, ca, *dest, ica);
    }

private:
    const uint ca;
    const uint ica;
};

static inline int mix_alpha(int da, int sa)
{
    return 255 - ((255 - sa) * (255 - da) >> 8);
}

/*
    Dca' = Sca.Da + Dca.Sa + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Plus_impl(uint *dest, int length, uint color, const T &coverage)
{
    uint s = color;

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        d = comp_func_Plus_one_pixel(d, s);
        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_solid_Plus(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Plus_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Plus_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Plus_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        d = comp_func_Plus_one_pixel(d, s);

        coverage.store(&dest[i], d);
    }
}

void QT_FASTCALL comp_func_Plus(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Plus_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Plus_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int multiply_op(int dst, int src, int da, int sa)
{
    return qt_div_255(src * dst + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Multiply_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Multiply(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Multiply_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Multiply_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Multiply_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) multiply_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Multiply(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Multiply_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Multiply_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
         = Sca + Dca - Sca.Dca
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Screen_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) 255 - qt_div_255((255-a) * (255-b))
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Screen(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Screen_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Screen_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Screen_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) 255 - (((255-a) * (255-b)) >> 8)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Screen(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Screen_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Screen_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Dca < Da
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int overlay_op(int dst, int src, int da, int sa)
{
    const int temp = src * (255 - da) + dst * (255 - sa);
    if (2 * dst < da)
        return qt_div_255(2 * src * dst + temp);
    else
        return qt_div_255(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Overlay_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Overlay(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Overlay_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Overlay_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Overlay_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) overlay_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Overlay(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Overlay_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Overlay_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = min(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
    Da'  = Sa + Da - Sa.Da
*/
static inline int darken_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMin(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Darken_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) darken_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Darken(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Darken_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Darken_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Darken_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) darken_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Darken(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Darken_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Darken_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   Dca' = max(Sca.Da, Dca.Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
   Da'  = Sa + Da - Sa.Da
*/
static inline int lighten_op(int dst, int src, int da, int sa)
{
    return qt_div_255(qMax(src * da, dst * sa) + src * (255 - da) + dst * (255 - sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Lighten_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Lighten(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Lighten_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Lighten_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Lighten_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) lighten_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Lighten(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Lighten_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Lighten_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa >= Sa.Da
       Dca' = Sa.Da + Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Dca.Sa/(1-Sca/Sa) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_dodge_op(int dst, int src, int da, int sa)
{
    const int sa_da = sa * da;
    const int dst_sa = dst * sa;
    const int src_da = src * da;

    const int temp = src * (255 - da) + dst * (255 - sa);
    if (src_da + dst_sa >= sa_da)
        return qt_div_255(sa_da + temp);
    else
        return qt_div_255(255 * dst_sa / (255 - 255 * src / sa) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_ColorDodge_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a,b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d), sr);
        int b = OP( qBlue(d), sb);
        int g = OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorDodge(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorDodge_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorDodge_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_ColorDodge_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_dodge_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorDodge(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorDodge_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorDodge_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   if Sca.Da + Dca.Sa <= Sa.Da
       Dca' = Sca.(1 - Da) + Dca.(1 - Sa)
   otherwise
       Dca' = Sa.(Sca.Da + Dca.Sa - Sa.Da)/Sca + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int color_burn_op(int dst, int src, int da, int sa)
{
    const int src_da = src * da;
    const int dst_sa = dst * sa;
    const int sa_da = sa * da;

    const int temp = src * (255 - da) + dst * (255 - sa);

    if (src == 0 || src_da + dst_sa <= sa_da)
        return qt_div_255(temp);
    return qt_div_255(sa * (src_da + dst_sa - sa_da) / src + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_ColorBurn_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_ColorBurn(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_ColorBurn_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_ColorBurn_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_ColorBurn_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) color_burn_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_ColorBurn(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_ColorBurn_impl(dest, src, length, QFullCoverage());
    else
        comp_func_ColorBurn_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Sca < Sa
        Dca' = 2.Sca.Dca + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise
        Dca' = Sa.Da - 2.(Da - Dca).(Sa - Sca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline uint hardlight_op(int dst, int src, int da, int sa)
{
    const uint temp = src * (255 - da) + dst * (255 - sa);

    if (2 * src < sa)
        return qt_div_255(2 * src * dst + temp);
    else
        return qt_div_255(sa * da - 2 * (da - dst) * (sa - src) + temp);
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_HardLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_HardLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_HardLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_HardLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_HardLight_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) hardlight_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_HardLight(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_HardLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_HardLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    if 2.Sca <= Sa
        Dca' = Dca.(Sa + (2.Sca - Sa).(1 - Dca/Da)) + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise if 2.Sca > Sa and 4.Dca <= Da
        Dca' = Dca.Sa + Da.(2.Sca - Sa).(4.Dca/Da.(4.Dca/Da + 1).(Dca/Da - 1) + 7.Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
    otherwise if 2.Sca > Sa and 4.Dca > Da
        Dca' = Dca.Sa + Da.(2.Sca - Sa).((Dca/Da)^0.5 - Dca/Da) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
static inline int soft_light_op(int dst, int src, int da, int sa)
{
    const int src2 = src << 1;
    const int dst_np = da != 0 ? (255 * dst) / da : 0;
    const int temp = (src * (255 - da) + dst * (255 - sa)) * 255;

    if (src2 < sa)
        return (dst * (sa * 255 + (src2 - sa) * (255 - dst_np)) + temp) / 65025;
    else if (4 * dst <= da)
        return (dst * sa * 255 + da * (src2 - sa) * ((((16 * dst_np - 12 * 255) * dst_np + 3 * 65025) * dst_np) / 65025) + temp) / 65025;
    else {
#   ifdef Q_CC_RVCT // needed to avoid compiler crash in RVCT 2.2
        return (dst * sa * 255 + da * (src2 - sa) * (qIntSqrtInt(dst_np * 255) - dst_np) + temp) / 65025;
#   else
        return (dst * sa * 255 + da * (src2 - sa) * (int(qSqrt(qreal(dst_np * 255))) - dst_np) + temp) / 65025;
#   endif
    }
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_SoftLight_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_SoftLight(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_SoftLight_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_SoftLight_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_SoftLight_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) soft_light_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_SoftLight(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_SoftLight_impl(dest, src, length, QFullCoverage());
    else
        comp_func_SoftLight_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
   Dca' = abs(Dca.Sa - Sca.Da) + Sca.(1 - Da) + Dca.(1 - Sa)
        = Sca + Dca - 2.min(Sca.Da, Dca.Sa)
*/
static inline int difference_op(int dst, int src, int da, int sa)
{
    return src + dst - qt_div_255(2 * qMin(src * da, dst * sa));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_solid_Difference_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) difference_op(a, b, da, sa)
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Difference(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Difference_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Difference_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Difference_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) difference_op(a, b, da, sa)
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Difference(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Difference_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Difference_impl(dest, src, length, QPartialCoverage(const_alpha));
}

/*
    Dca' = (Sca.Da + Dca.Sa - 2.Sca.Dca) + Sca.(1 - Da) + Dca.(1 - Sa)
*/
template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void QT_FASTCALL comp_func_solid_Exclusion_impl(uint *dest, int length, uint color, const T &coverage)
{
    int sa = qAlpha(color);
    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);

    PRELOAD_INIT(dest)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND(dest)
        uint d = dest[i];
        int da = qAlpha(d);

#define OP(a, b) (a + b - qt_div_255(2*(a*b)))
        int r =  OP(  qRed(d), sr);
        int b =  OP( qBlue(d), sb);
        int g =  OP(qGreen(d), sg);
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_solid_Exclusion(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_solid_Exclusion_impl(dest, length, color, QFullCoverage());
    else
        comp_func_solid_Exclusion_impl(dest, length, color, QPartialCoverage(const_alpha));
}

template <typename T>
Q_STATIC_TEMPLATE_FUNCTION inline void comp_func_Exclusion_impl(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, const T &coverage)
{
    PRELOAD_INIT2(dest, src)
    for (int i = 0; i < length; ++i) {
        PRELOAD_COND2(dest, src)
        uint d = dest[i];
        uint s = src[i];

        int da = qAlpha(d);
        int sa = qAlpha(s);

#define OP(a, b) (a + b - ((a*b) >> 7))
        int r = OP(  qRed(d),   qRed(s));
        int b = OP( qBlue(d),  qBlue(s));
        int g = OP(qGreen(d), qGreen(s));
        int a = mix_alpha(da, sa);
#undef OP

        coverage.store(&dest[i], qRgba(r, g, b, a));
    }
}

void QT_FASTCALL comp_func_Exclusion(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src, int length, uint const_alpha)
{
    if (const_alpha == 255)
        comp_func_Exclusion_impl(dest, src, length, QFullCoverage());
    else
        comp_func_Exclusion_impl(dest, src, length, QPartialCoverage(const_alpha));
}

#if defined(Q_CC_RVCT)
// Restore pragma state from previous #pragma arm
#  pragma pop
#endif

void QT_FASTCALL rasterop_solid_SourceOrDestination(uint *dest,
                                                    int length,
                                                    uint color,
                                                    uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ |= color;
}

void QT_FASTCALL rasterop_SourceOrDestination(uint *Q_DECL_RESTRICT dest,
                                              const uint *Q_DECL_RESTRICT src,
                                              int length,
                                              uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ |= *src++;
}

void QT_FASTCALL rasterop_solid_SourceAndDestination(uint *dest,
                                                     int length,
                                                     uint color,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color |= 0xff000000;
    while (length--)
        *dest++ &= color;
}

void QT_FASTCALL rasterop_SourceAndDestination(uint *Q_DECL_RESTRICT dest,
                                               const uint *Q_DECL_RESTRICT src,
                                               int length,
                                               uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src & *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceXorDestination(uint *dest,
                                                     int length,
                                                     uint color,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color &= 0x00ffffff;
    while (length--)
        *dest++ ^= color;
}

void QT_FASTCALL rasterop_SourceXorDestination(uint *Q_DECL_RESTRICT dest,
                                               const uint *Q_DECL_RESTRICT src,
                                               int length,
                                               uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src ^ *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceAndNotDestination(uint *dest,
                                                           int length,
                                                           uint color,
                                                           uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color;
    while (length--) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceAndNotDestination(uint *Q_DECL_RESTRICT dest,
                                                     const uint *Q_DECL_RESTRICT src,
                                                     int length,
                                                     uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) & ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceOrNotDestination(uint *dest,
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

void QT_FASTCALL rasterop_NotSourceOrNotDestination(uint *Q_DECL_RESTRICT dest,
                                                    const uint *Q_DECL_RESTRICT src,
                                                    int length,
                                                    uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = ~(*src) | ~(*dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceXorDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color & 0x00ffffff;
    while (length--) {
        *dest = color ^ (*dest);
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceXorDestination(uint *Q_DECL_RESTRICT dest,
                                                  const uint *Q_DECL_RESTRICT src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = ((~(*src)) ^ (*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSource(uint *dest, int length,
                                          uint color, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    qt_memfill(dest, ~color | 0xff000000, length);
}

void QT_FASTCALL rasterop_NotSource(uint *Q_DECL_RESTRICT dest, const uint *Q_DECL_RESTRICT src,
                                    int length, uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--)
        *dest++ = ~(*src++) | 0xff000000;
}

void QT_FASTCALL rasterop_solid_NotSourceAndDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--) {
        *dest = color & *dest;
        ++dest;
    }
}

void QT_FASTCALL rasterop_NotSourceAndDestination(uint *Q_DECL_RESTRICT dest,
                                                  const uint *Q_DECL_RESTRICT src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) & *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceAndNotDestination(uint *dest,
                                                        int length,
                                                        uint color,
                                                        uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (color & ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_SourceAndNotDestination(uint *Q_DECL_RESTRICT dest,
                                                  const uint *Q_DECL_RESTRICT src,
                                                  int length,
                                                  uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src & ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_NotSourceOrDestination(uint *Q_DECL_RESTRICT dest,
                                                 const uint *Q_DECL_RESTRICT src,
                                                 int length,
                                                 uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (~(*src) | *dest) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_NotSourceOrDestination(uint *Q_DECL_RESTRICT dest,
                                                       int length,
                                                       uint color,
                                                       uint const_alpha)
{
    Q_UNUSED(const_alpha);
    color = ~color | 0xff000000;
    while (length--)
        *dest++ |= color;
}

void QT_FASTCALL rasterop_SourceOrNotDestination(uint *Q_DECL_RESTRICT dest,
                                                 const uint *Q_DECL_RESTRICT src,
                                                 int length,
                                                 uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (*src | ~(*dest)) | 0xff000000;
        ++dest; ++src;
    }
}

void QT_FASTCALL rasterop_solid_SourceOrNotDestination(uint *Q_DECL_RESTRICT dest,
                                                       int length,
                                                       uint color,
                                                       uint const_alpha)
{
    Q_UNUSED(const_alpha);
    while (length--) {
        *dest = (color | ~(*dest)) | 0xff000000;
        ++dest;
    }
}

void QT_FASTCALL rasterop_ClearDestination(uint *Q_DECL_RESTRICT dest,
                                           const uint *Q_DECL_RESTRICT src,
                                           int length,
                                           uint const_alpha)
{
    Q_UNUSED(src);
    comp_func_solid_SourceOver (dest, length, 0xff000000, const_alpha);
}

void QT_FASTCALL rasterop_solid_ClearDestination(uint *Q_DECL_RESTRICT dest,
                                                 int length,
                                                 uint color,
                                                 uint const_alpha)
{
    Q_UNUSED(color);
    comp_func_solid_SourceOver (dest, length, 0xff000000, const_alpha);
}

void QT_FASTCALL rasterop_SetDestination(uint *Q_DECL_RESTRICT dest,
                                         const uint *Q_DECL_RESTRICT src,
                                         int length,
                                         uint const_alpha)
{
    Q_UNUSED(src);
    comp_func_solid_SourceOver (dest, length, 0xffffffff, const_alpha);
}

void QT_FASTCALL rasterop_solid_SetDestination(uint *Q_DECL_RESTRICT dest,
                                               int length,
                                               uint color,
                                               uint const_alpha)
{
    Q_UNUSED(color);
    comp_func_solid_SourceOver (dest, length, 0xffffffff, const_alpha);
}

void QT_FASTCALL rasterop_NotDestination(uint *Q_DECL_RESTRICT dest,
                                         const uint *Q_DECL_RESTRICT src,
                                         int length,
                                         uint const_alpha)
{
    Q_UNUSED(src);
    rasterop_solid_SourceXorDestination (dest, length, 0x00ffffff, const_alpha);
}

void QT_FASTCALL rasterop_solid_NotDestination(uint *Q_DECL_RESTRICT dest,
                                               int length,
                                               uint color,
                                               uint const_alpha)
{
    Q_UNUSED(color);
    rasterop_solid_SourceXorDestination (dest, length, 0x00ffffff, const_alpha);
}

static CompositionFunctionSolid functionForModeSolid_C[] = {
        comp_func_solid_SourceOver,
        comp_func_solid_DestinationOver,
        comp_func_solid_Clear,
        comp_func_solid_Source,
        comp_func_solid_Destination,
        comp_func_solid_SourceIn,
        comp_func_solid_DestinationIn,
        comp_func_solid_SourceOut,
        comp_func_solid_DestinationOut,
        comp_func_solid_SourceAtop,
        comp_func_solid_DestinationAtop,
        comp_func_solid_XOR,
        comp_func_solid_Plus,
        comp_func_solid_Multiply,
        comp_func_solid_Screen,
        comp_func_solid_Overlay,
        comp_func_solid_Darken,
        comp_func_solid_Lighten,
        comp_func_solid_ColorDodge,
        comp_func_solid_ColorBurn,
        comp_func_solid_HardLight,
        comp_func_solid_SoftLight,
        comp_func_solid_Difference,
        comp_func_solid_Exclusion,
        rasterop_solid_SourceOrDestination,
        rasterop_solid_SourceAndDestination,
        rasterop_solid_SourceXorDestination,
        rasterop_solid_NotSourceAndNotDestination,
        rasterop_solid_NotSourceOrNotDestination,
        rasterop_solid_NotSourceXorDestination,
        rasterop_solid_NotSource,
        rasterop_solid_NotSourceAndDestination,
        rasterop_solid_SourceAndNotDestination,
        rasterop_solid_SourceAndNotDestination,
        rasterop_solid_NotSourceOrDestination,
        rasterop_solid_SourceOrNotDestination,
        rasterop_solid_ClearDestination,
        rasterop_solid_SetDestination,
        rasterop_solid_NotDestination
};

static const CompositionFunctionSolid *functionForModeSolid = functionForModeSolid_C;

static CompositionFunction functionForMode_C[] = {
        comp_func_SourceOver,
        comp_func_DestinationOver,
        comp_func_Clear,
        comp_func_Source,
        comp_func_Destination,
        comp_func_SourceIn,
        comp_func_DestinationIn,
        comp_func_SourceOut,
        comp_func_DestinationOut,
        comp_func_SourceAtop,
        comp_func_DestinationAtop,
        comp_func_XOR,
        comp_func_Plus,
        comp_func_Multiply,
        comp_func_Screen,
        comp_func_Overlay,
        comp_func_Darken,
        comp_func_Lighten,
        comp_func_ColorDodge,
        comp_func_ColorBurn,
        comp_func_HardLight,
        comp_func_SoftLight,
        comp_func_Difference,
        comp_func_Exclusion,
        rasterop_SourceOrDestination,
        rasterop_SourceAndDestination,
        rasterop_SourceXorDestination,
        rasterop_NotSourceAndNotDestination,
        rasterop_NotSourceOrNotDestination,
        rasterop_NotSourceXorDestination,
        rasterop_NotSource,
        rasterop_NotSourceAndDestination,
        rasterop_SourceAndNotDestination,
        rasterop_SourceAndNotDestination,
        rasterop_NotSourceOrDestination,
        rasterop_SourceOrNotDestination,
        rasterop_ClearDestination,
        rasterop_SetDestination,
        rasterop_NotDestination
};

static const CompositionFunction *functionForMode = functionForMode_C;

static TextureBlendType getBlendType(const QSpanData *data)
{
    TextureBlendType ft;
    if (data->txop <= QTransform::TxTranslate)
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTiled;
        else
            ft = BlendUntransformed;
    else if (data->bilinear)
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTransformedBilinearTiled;
        else
            ft = BlendTransformedBilinear;
    else
        if (data->texture.type == QTextureData::Tiled)
            ft = BlendTransformedTiled;
        else
            ft = BlendTransformed;
    return ft;
}

static inline Operator getOperator(const QSpanData *data, const QSpan *spans, int spanCount)
{
    Operator op;
    bool solidSource = false;

    switch(data->type) {
    case QSpanData::Solid:
        solidSource = (qAlpha(data->solid.color) == 255);
        break;
    case QSpanData::LinearGradient:
        solidSource = !data->gradient.alphaColor;
        getLinearGradientValues(&op.linear, data);
        op.src_fetch = qt_fetch_linear_gradient;
        break;
    case QSpanData::RadialGradient:
        solidSource = !data->gradient.alphaColor;
        getRadialGradientValues(&op.radial, data);
        op.src_fetch = qt_fetch_radial_gradient;
        break;
    case QSpanData::ConicalGradient:
        solidSource = !data->gradient.alphaColor;
        op.src_fetch = qt_fetch_conical_gradient;
        break;
    case QSpanData::Texture:
        op.src_fetch = sourceFetch[getBlendType(data)][data->texture.format];
        solidSource = !data->texture.hasAlpha;
    default:
        break;
    }

    op.mode = data->rasterBuffer->compositionMode;
    if (op.mode == QPainter::CompositionMode_SourceOver && solidSource)
        op.mode = QPainter::CompositionMode_Source;

    op.dest_fetch = destFetchProc[data->rasterBuffer->format];
    if (op.mode == QPainter::CompositionMode_Source) {
        switch (data->rasterBuffer->format) {
        case QImage::Format_RGB32:
        case QImage::Format_ARGB32_Premultiplied:
            // don't clear dest_fetch as it sets up the pointer correctly to save one copy
            break;
        default: {
            if (data->type == QSpanData::Texture && data->texture.const_alpha != 256)
                break;
            const QSpan *lastSpan = spans + spanCount;
            bool alphaSpans = false;
            while (spans < lastSpan) {
                if (spans->coverage != 255) {
                    alphaSpans = true;
                    break;
                }
                ++spans;
            }
            if (!alphaSpans)
                op.dest_fetch = 0;
        }
        }
    }

    op.dest_store = destStoreProc[data->rasterBuffer->format];

    op.funcSolid = functionForModeSolid[op.mode];
    op.func = functionForMode[op.mode];

    return op;
}



// -------------------- blend methods ---------------------

#if !defined(Q_CC_SUN)
static
#endif
void blend_color_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    uint buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        while (length) {
            int l = qMin(buffer_size, length);
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.funcSolid(dest, l, data->solid.color, spans->coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            length -= l;
            x += l;
        }
        ++spans;
    }
}

static void blend_color_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    Operator op = getOperator(data, spans, count);

    if (op.mode == QPainter::CompositionMode_Source) {
        // inline for performance
        while (count--) {
            uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_UINT(target, spans->len, data->solid.color);
            } else {
                uint c = BYTE_MUL(data->solid.color, spans->coverage);
                int ialpha = 255 - spans->coverage;
                for (int i = 0; i < spans->len; ++i)
                    target[i] = c + BYTE_MUL(target[i], ialpha);
            }
            ++spans;
        }
        return;
    }

    while (count--) {
        uint *target = ((uint *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
        op.funcSolid(target, spans->len, data->solid.color, spans->coverage);
        ++spans;
    }
}

static void blend_color_rgb16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    /*
        We duplicate a little logic from getOperator() and calculate the
        composition mode directly.  This allows blend_color_rgb16 to be used
        from qt_gradient_quint16 with minimal overhead.
     */
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;
    if (mode == QPainter::CompositionMode_SourceOver &&
        qAlpha(data->solid.color) == 255)
        mode = QPainter::CompositionMode_Source;

    if (mode == QPainter::CompositionMode_Source) {
        // inline for performance
        ushort c = qConvertRgb32To16(data->solid.color);
        while (count--) {
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            if (spans->coverage == 255) {
                QT_MEMFILL_USHORT(target, spans->len, c);
            } else {
                ushort color = BYTE_MUL_RGB16(c, spans->coverage);
                int ialpha = 255 - spans->coverage;
                const ushort *end = target + spans->len;
                while (target < end) {
                    *target = color + BYTE_MUL_RGB16(*target, ialpha);
                    ++target;
                }
            }
            ++spans;
        }
        return;
    }

    if (mode == QPainter::CompositionMode_SourceOver) {
        while (count--) {
            uint color = BYTE_MUL(data->solid.color, spans->coverage);
            int ialpha = qAlpha(~color);
            ushort c = qConvertRgb32To16(color);
            ushort *target = ((ushort *)data->rasterBuffer->scanLine(spans->y)) + spans->x;
            int len = spans->len;
            bool pre = (((quintptr)target) & 0x3) != 0;
            bool post = false;
            if (pre) {
                // skip to word boundary
                *target = c + BYTE_MUL_RGB16(*target, ialpha);
                ++target;
                --len;
            }
            if (len & 0x1) {
                post = true;
                --len;
            }
            uint *target32 = (uint*)target;
            uint c32 = c | (c<<16);
            len >>= 1;
            uint salpha = (ialpha+1) >> 3; // calculate here rather than in loop
            while (len--) {
                // blend full words
                *target32 = c32 + BYTE_MUL_RGB16_32(*target32, salpha);
                ++target32;
                target += 2;
            }
            if (post) {
                // one last pixel beyond a full word
                *target = c + BYTE_MUL_RGB16(*target, ialpha);
            }
            ++spans;
        }
        return;
    }

    blend_color_generic(count, spans, userData);
}

template <typename T>
void handleSpans(int count, const QSpan *spans, const QSpanData *data, T &handler)
{
    uint const_alpha = 256;
    if (data->type == QSpanData::Texture)
        const_alpha = data->texture.const_alpha;

    int coverage = 0;
    while (count) {
        int x = spans->x;
        const int y = spans->y;
        int right = x + spans->len;

        // compute length of adjacent spans
        for (int i = 1; i < count && spans[i].y == y && spans[i].x == right; ++i)
            right += spans[i].len;
        int length = right - x;

        while (length) {
            int l = qMin(buffer_size, length);
            length -= l;

            int process_length = l;
            int process_x = x;

            const uint *src = handler.fetch(process_x, y, process_length);
            int offset = 0;
            while (l > 0) {
                if (x == spans->x) // new span?
                    coverage = (spans->coverage * const_alpha) >> 8;

                int right = spans->x + spans->len;
                int len = qMin(l, right - x);

                handler.process(x, y, len, coverage, src, offset);

                l -= len;
                x += len;
                offset += len;

                if (x == right) { // done with current span?
                    ++spans;
                    --count;
                }
            }
            handler.store(process_x, y, process_length);
        }
    }
}

struct QBlendBase
{
    QBlendBase(QSpanData *d, Operator o)
        : data(d)
        , op(o)
        , dest(0)
    {
    }

    QSpanData *data;
    Operator op;

    uint *dest;

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
};

class BlendSrcGeneric : public QBlendBase
{
public:
    BlendSrcGeneric(QSpanData *d, Operator o)
        : QBlendBase(d, o)
    {
    }

    const uint *fetch(int x, int y, int len)
    {
        dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, y, len) : buffer;
        return op.src_fetch(src_buffer, &op, data, y, x, len);
    }

    void process(int, int, int len, int coverage, const uint *src, int offset)
    {
        op.func(dest + offset, src + offset, len, coverage);
    }

    void store(int x, int y, int len)
    {
        if (op.dest_store)
            op.dest_store(data->rasterBuffer, x, y, dest, len);
    }
};

static void blend_src_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    BlendSrcGeneric blend(data, getOperator(data, spans, count));
    handleSpans(count, spans, data, blend);
}

static void blend_untransformed_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                while (length) {
                    int l = qMin(buffer_size, length);
                    const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
                    uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
                    op.func(dest, src, l, coverage);
                    if (op.dest_store)
                        op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
                    x += l;
                    sx += l;
                    length -= l;
                }
            }
        }
        ++spans;
    }
}

static void blend_untransformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
                const uint *src = (uint *)data->texture.scanLine(sy) + sx;
                uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
                op.func(dest, src, length, coverage);
            }
        }
        ++spans;
    }
}

static inline quint16 interpolate_pixel_rgb16_255(quint16 x, quint8 a,
                                                  quint16 y, quint8 b)
{
    quint16 t = ((((x & 0x07e0) * a) + ((y & 0x07e0) * b)) >> 5) & 0x07e0;
    t |= ((((x & 0xf81f) * a) + ((y & 0xf81f) * b)) >> 5) & 0xf81f;

    return t;
}

static inline quint32 interpolate_pixel_rgb16x2_255(quint32 x, quint8 a,
                                                    quint32 y, quint8 b)
{
    uint t;
    t = ((((x & 0xf81f07e0) >> 5) * a) + (((y & 0xf81f07e0) >> 5) * b)) & 0xf81f07e0;
    t |= ((((x & 0x07e0f81f) * a) + ((y & 0x07e0f81f) * b)) >> 5) & 0x07e0f81f;
    return t;
}

static inline void blend_sourceOver_rgb16_rgb16(quint16 *Q_DECL_RESTRICT dest,
                                                const quint16 *Q_DECL_RESTRICT src,
                                                int length,
                                                const quint8 alpha,
                                                const quint8 ialpha)
{
    const int dstAlign = ((quintptr)dest) & 0x3;
    if (dstAlign) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha, *dest, ialpha);
        ++dest;
        ++src;
        --length;
    }
    const int srcAlign = ((quintptr)src) & 0x3;
    int length32 = length >> 1;
    if (length32 && srcAlign == 0) {
        while (length32--) {
            const quint32 *src32 = reinterpret_cast<const quint32*>(src);
            quint32 *dest32 = reinterpret_cast<quint32*>(dest);
            *dest32 = interpolate_pixel_rgb16x2_255(*src32, alpha,
                                                    *dest32, ialpha);
            dest += 2;
            src += 2;
        }
        length &= 0x1;
    }
    while (length--) {
        *dest = interpolate_pixel_rgb16_255(*src, alpha, *dest, ialpha);
        ++dest;
        ++src;
    }
}

static void blend_untransformed_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_untransformed_generic(count, spans, userData);
        return;
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx);
    int yoff = -qRound(-data->dy);

    while (count--) {
        const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
        if (coverage == 0) {
            ++spans;
            continue;
        }

        int x = spans->x;
        int length = spans->len;
        int sx = xoff + x;
        int sy = yoff + spans->y;
        if (sy >= 0 && sy < image_height && sx < image_width) {
            if (sx < 0) {
                x -= sx;
                length += sx;
                sx = 0;
            }
            if (sx + length > image_width)
                length = image_width - sx;
            if (length > 0) {
                quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + x;
                const quint16 *src = (quint16 *)data->texture.scanLine(sy) + sx;
                if (coverage == 255) {
                    memcpy(dest, src, length * sizeof(quint16));
                } else {
                    const quint8 alpha = (coverage + 1) >> 3;
                    const quint8 ialpha = 0x20 - alpha;
                    if (alpha > 0)
                        blend_sourceOver_rgb16_rgb16(dest, src, length, alpha, ialpha);
                }
            }
        }
        ++spans;
    }
}

static void blend_tiled_generic(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    uint buffer[buffer_size];
    uint src_buffer[buffer_size];
    Operator op = getOperator(data, spans, count);

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = op.src_fetch(src_buffer, &op, data, sy, sx, l);
            uint *dest = op.dest_fetch ? op.dest_fetch(buffer, data->rasterBuffer, x, spans->y, l) : buffer;
            op.func(dest, src, l, coverage);
            if (op.dest_store)
                op.dest_store(data->rasterBuffer, x, spans->y, dest, l);
            x += l;
            sx += l;
            length -= l;
            if (sx >= image_width)
                sx = 0;
        }
        ++spans;
    }
}

static void blend_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    Operator op = getOperator(data, spans, count);

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
        while (length) {
            int l = qMin(image_width - sx, length);
            if (buffer_size < l)
                l = buffer_size;
            const uint *src = (uint *)data->texture.scanLine(sy) + sx;
            uint *dest = ((uint *)data->rasterBuffer->scanLine(spans->y)) + x;
            op.func(dest, src, l, coverage);
            x += l;
            length -= l;
            sx = 0;
        }
        ++spans;
    }
}

static void blend_tiled_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_tiled_generic(count, spans, userData);
        return;
    }

    const int image_width = data->texture.width;
    const int image_height = data->texture.height;
    int xoff = -qRound(-data->dx) % image_width;
    int yoff = -qRound(-data->dy) % image_height;

    if (xoff < 0)
        xoff += image_width;
    if (yoff < 0)
        yoff += image_height;

    while (count--) {
        const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
        if (coverage == 0) {
            ++spans;
            continue;
        }

        int x = spans->x;
        int length = spans->len;
        int sx = (xoff + spans->x) % image_width;
        int sy = (spans->y + yoff) % image_height;
        if (sx < 0)
            sx += image_width;
        if (sy < 0)
            sy += image_height;

        if (coverage == 255) {
            // Copy the first texture block
            length = qMin(image_width,length);
            int tx = x;
            while (length) {
                int l = qMin(image_width - sx, length);
                if (buffer_size < l)
                    l = buffer_size;
                quint16 *dest = ((quint16 *)data->rasterBuffer->scanLine(spans->y)) + tx;
                const quint16 *src = (quint16 *)data->texture.scanLine(sy) + sx;
                memcpy(dest, src, l * sizeof(quint16));
                length -= l;
                tx += l;
                sx = 0;
            }

            // Now use the rasterBuffer as the source of the texture,
            // We can now progressively copy larger blocks
            // - Less cpu time in code figuring out what to copy
            // We are dealing with one block of data
            // - More likely to fit in the cache
            // - can use memcpy
            int copy_image_width = qMin(image_width, int(spans->len));
            length = spans->len - copy_image_width;
            quint16 *src = ((quint16 *)data->rasterBuffer->scanLine(spans->y)) + x;
            quint16 *dest = src + copy_image_width;
            while (copy_image_width < length) {
                memcpy(dest, src, copy_image_width * sizeof(quint16));
                dest += copy_image_width;
                length -= copy_image_width;
                copy_image_width *= 2;
            }
            if (length > 0)
                memcpy(dest, src, length * sizeof(quint16));
        } else {
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha > 0) {
                while (length) {
                    int l = qMin(image_width - sx, length);
                    if (buffer_size < l)
                        l = buffer_size;
                    quint16 *dest = ((quint16 *)data->rasterBuffer->scanLine(spans->y)) + x;
                    const quint16 *src = (quint16 *)data->texture.scanLine(sy) + sx;
                    blend_sourceOver_rgb16_rgb16(dest, src, l, alpha, ialpha);
                    x += l;
                    length -= l;
                    sx = 0;
                }
            }
        }
        ++spans;
    }
}

static void blend_transformed_bilinear_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_src_generic(count, spans, userData);
        return;
    }

    quint16 buffer[buffer_size];

    const int src_minx = data->texture.x1;
    const int src_miny = data->texture.y1;
    const int src_maxx = data->texture.x2 - 1;
    const int src_maxy = data->texture.y2 - 1;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale) - half_point;
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale) - half_point;
            int length = spans->len;

            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    int x1 = (x >> 16);
                    int x2;
                    int y1 = (y >> 16);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_minx, src_maxx, x1, x2);
                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_miny, src_maxy, y1, y2);

                    const quint16 *src1 = (quint16*)data->texture.scanLine(y1);
                    const quint16 *src2 = (quint16*)data->texture.scanLine(y2);
                    quint16 tl = src1[x1];
                    const quint16 tr = src1[x2];
                    quint16 bl = src2[x1];
                    const quint16 br = src2[x2];

                    const uint distxsl8 = x & 0xff00;
                    const uint distysl8 = y & 0xff00;
                    const uint distx = distxsl8 >> 8;
                    const uint disty = distysl8 >> 8;
                    const uint distxy = distx * disty;

                    const uint tlw = 0x10000 - distxsl8 - distysl8 + distxy; // (256 - distx) * (256 - disty)
                    const uint trw = distxsl8 - distxy; // distx * (256 - disty)
                    const uint blw = distysl8 - distxy; // (256 - distx) * disty
                    const uint brw = distxy; // distx * disty
                    uint red = ((tl & 0xf800) * tlw + (tr & 0xf800) * trw
                            + (bl & 0xf800) * blw + (br & 0xf800) * brw) & 0xf8000000;
                    uint green = ((tl & 0x07e0) * tlw + (tr & 0x07e0) * trw
                            + (bl & 0x07e0) * blw + (br & 0x07e0) * brw) & 0x07e00000;
                    uint blue = ((tl & 0x001f) * tlw + (tr & 0x001f) * trw
                            + (bl & 0x001f) * blw + (br & 0x001f) * brw);
                    *b = quint16((red | green | blue) >> 16);

                    ++b;
                    x += fdx;
                    y += fdy;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal px = x * iw - qreal(0.5);
                    const qreal py = y * iw - qreal(0.5);

                    int x1 = int(px) - (px < 0);
                    int x2;
                    int y1 = int(py) - (py < 0);
                    int y2;

                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_minx, src_maxx, x1, x2);
                    fetchTransformedBilinear_pixelBounds<BlendTransformedBilinear>(0, src_miny, src_maxy, y1, y2);

                    const quint16 *src1 = (quint16 *)data->texture.scanLine(y1);
                    const quint16 *src2 = (quint16 *)data->texture.scanLine(y2);
                    quint16 tl = src1[x1];
                    const quint16 tr = src1[x2];
                    quint16 bl = src2[x1];
                    const quint16 br = src2[x2];

                    const uint distx = uint((px - x1) * 256);
                    const uint disty = uint((py - y1) * 256);
                    const uint distxsl8 = distx << 8;
                    const uint distysl8 = disty << 8;
                    const uint distxy = distx * disty;

                    const uint tlw = 0x10000 - distxsl8 - distysl8 + distxy; // (256 - distx) * (256 - disty)
                    const uint trw = distxsl8 - distxy; // distx * (256 - disty)
                    const uint blw = distysl8 - distxy; // (256 - distx) * disty
                    const uint brw = distxy; // distx * disty
                    uint red = ((tl & 0xf800) * tlw + (tr & 0xf800) * trw
                            + (bl & 0xf800) * blw + (br & 0xf800) * brw) & 0xf8000000;
                    uint green = ((tl & 0x07e0) * tlw + (tr & 0x07e0) * trw
                            + (bl & 0x07e0) * blw + (br & 0x07e0) * brw) & 0x07e00000;
                    uint blue = ((tl & 0x001f) * tlw + (tr & 0x001f) * trw
                            + (bl & 0x001f) * blw + (br & 0x001f) * brw);
                    *b = quint16((red | green | blue) >> 16);

                    ++b;
                    x += fdx;
                    y += fdy;
                    w += fdw;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    int px = qBound(0, x >> 16, image_width - 1);
                    int py = qBound(0, y >> 16, image_height - 1);
                    *b = reinterpret_cast<const uint *>(data->texture.scanLine(py))[px];

                    x += fdx;
                    y += fdy;
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;
                    const int px = qBound(0, int(tx) - (tx < 0), image_width - 1);
                    const int py = qBound(0, int(ty) - (ty < 0), image_height - 1);

                    *b = reinterpret_cast<const uint *>(data->texture.scanLine(py))[px];
                    x += fdx;
                    y += fdy;
                    w += fdw;

                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_src_generic(count, spans, userData);
        return;
    }

    quint16 buffer[buffer_size];
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);
            int length = spans->len;

            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const int px = qBound(0, x >> 16, image_width - 1);
                    const int py = qBound(0, y >> 16, image_height - 1);

                    *b = ((quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;

                    const int px = qBound(0, int(tx) - (tx < 0), image_width - 1);
                    const int py = qBound(0, int(ty) - (ty < 0), image_height - 1);

                    *b = ((quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_tiled_argb(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    if (data->texture.format != QImage::Format_ARGB32_Premultiplied
        && data->texture.format != QImage::Format_RGB32) {
        blend_src_generic(count, spans, userData);
        return;
    }

    CompositionFunction func = functionForMode[data->rasterBuffer->compositionMode];
    uint buffer[buffer_size];

    int image_width = data->texture.width;
    int image_height = data->texture.height;
    const int scanline_offset = data->texture.bytesPerLine / 4;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        int fdx = (int)(data->m11 * fixed_scale);
        int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                int px16 = x % (image_width << 16);
                int py16 = y % (image_height << 16);
                int px_delta = fdx % (image_width << 16);
                int py_delta = fdy % (image_height << 16);
                while (b < end) {
                    if (px16 < 0) px16 += image_width << 16;
                    if (py16 < 0) py16 += image_height << 16;
                    int px = px16 >> 16;
                    int py = py16 >> 16;
                    int y_offset = py * scanline_offset;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    px16 += px_delta;
                    if (px16 >= image_width << 16)
                        px16 -= image_width << 16;
                    py16 += py_delta;
                    if (py16 >= image_height << 16)
                        py16 -= image_height << 16;
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;
        while (count--) {
            void *t = data->rasterBuffer->scanLine(spans->y);

            uint *target = ((uint *)t) + spans->x;
            uint *image_bits = (uint *)data->texture.imageData;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            const int coverage = (spans->coverage * data->texture.const_alpha) >> 8;
            int length = spans->len;
            while (length) {
                int l = qMin(length, buffer_size);
                const uint *end = buffer + l;
                uint *b = buffer;
                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;
                    int px = int(tx) - (tx < 0);
                    int py = int(ty) - (ty < 0);

                    px %= image_width;
                    py %= image_height;
                    if (px < 0) px += image_width;
                    if (py < 0) py += image_height;
                    int y_offset = py * scanline_offset;

                    Q_ASSERT(px >= 0 && px < image_width);
                    Q_ASSERT(py >= 0 && py < image_height);

                    *b = image_bits[y_offset + px];
                    x += fdx;
                    y += fdy;
                    w += fdw;
                    //force increment to avoid /0
                    if (!w) {
                        w += fdw;
                    }
                    ++b;
                }
                func(target, buffer, l, coverage);
                target += l;
                length -= l;
            }
            ++spans;
        }
    }
}

static void blend_transformed_tiled_rgb565(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData*>(userData);
    QPainter::CompositionMode mode = data->rasterBuffer->compositionMode;

    if (data->texture.format != QImage::Format_RGB16
            || (mode != QPainter::CompositionMode_SourceOver
                && mode != QPainter::CompositionMode_Source))
    {
        blend_src_generic(count, spans, userData);
        return;
    }

    quint16 buffer[buffer_size];
    const int image_width = data->texture.width;
    const int image_height = data->texture.height;

    if (data->fast_matrix) {
        // The increment pr x in the scanline
        const int fdx = (int)(data->m11 * fixed_scale);
        const int fdy = (int)(data->m12 * fixed_scale);

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;
            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);
            int x = int((data->m21 * cy
                         + data->m11 * cx + data->dx) * fixed_scale);
            int y = int((data->m22 * cy
                         + data->m12 * cx + data->dy) * fixed_scale);
            int length = spans->len;

            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    int px = (x >> 16) % image_width;
                    int py = (y >> 16) % image_height;

                    if (px < 0)
                        px += image_width;
                    if (py < 0)
                        py += image_height;

                    *b = ((quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    } else {
        const qreal fdx = data->m11;
        const qreal fdy = data->m12;
        const qreal fdw = data->m13;

        while (count--) {
            const quint8 coverage = (data->texture.const_alpha * spans->coverage) >> 8;
            const quint8 alpha = (coverage + 1) >> 3;
            const quint8 ialpha = 0x20 - alpha;
            if (alpha == 0) {
                ++spans;
                continue;
            }

            quint16 *dest = (quint16 *)data->rasterBuffer->scanLine(spans->y) + spans->x;

            const qreal cx = spans->x + qreal(0.5);
            const qreal cy = spans->y + qreal(0.5);

            qreal x = data->m21 * cy + data->m11 * cx + data->dx;
            qreal y = data->m22 * cy + data->m12 * cx + data->dy;
            qreal w = data->m23 * cy + data->m13 * cx + data->m33;

            int length = spans->len;
            while (length) {
                int l;
                quint16 *b;
                if (ialpha == 0) {
                    l = length;
                    b = dest;
                } else {
                    l = qMin(length, buffer_size);
                    b = buffer;
                }
                const quint16 *end = b + l;

                while (b < end) {
                    const qreal iw = w == 0 ? 1 : 1 / w;
                    const qreal tx = x * iw;
                    const qreal ty = y * iw;

                    int px = int(tx) - (tx < 0);
                    int py = int(ty) - (ty < 0);

                    px %= image_width;
                    py %= image_height;
                    if (px < 0)
                        px += image_width;
                    if (py < 0)
                        py += image_height;

                    *b = ((quint16 *)data->texture.scanLine(py))[px];
                    ++b;

                    x += fdx;
                    y += fdy;
                    w += fdw;
                    // force increment to avoid /0
                    if (!w)
                        w += fdw;
                }

                if (ialpha != 0)
                    blend_sourceOver_rgb16_rgb16(dest, buffer, l, alpha, ialpha);

                dest += l;
                length -= l;
            }
            ++spans;
        }
    }
}


/* Image formats here are target formats */
static const ProcessSpans processTextureSpans[NBlendTypes][QImage::NImageFormats] = {
    // Untransformed
    {
        0, // Invalid
        blend_untransformed_generic, // Mono
        blend_untransformed_generic, // MonoLsb
        blend_untransformed_generic, // Indexed8
        blend_untransformed_generic, // RGB32
        blend_untransformed_generic, // ARGB32
        blend_untransformed_argb, // ARGB32_Premultiplied
        blend_untransformed_rgb565,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
        blend_untransformed_generic,
    },
    // Tiled
    {
        0, // Invalid
        blend_tiled_generic, // Mono
        blend_tiled_generic, // MonoLsb
        blend_tiled_generic, // Indexed8
        blend_tiled_generic, // RGB32
        blend_tiled_generic, // ARGB32
        blend_tiled_argb, // ARGB32_Premultiplied
        blend_tiled_rgb565,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
        blend_tiled_generic,
    },
    // Transformed
    {
        0, // Invalid
        blend_src_generic, // Mono
        blend_src_generic, // MonoLsb
        blend_src_generic, // Indexed8
        blend_src_generic, // RGB32
        blend_src_generic, // ARGB32
        blend_transformed_argb, // ARGB32_Premultiplied
        blend_transformed_rgb565,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
    },
     // TransformedTiled
    {
        0,
        blend_src_generic, // Mono
        blend_src_generic, // MonoLsb
        blend_src_generic, // Indexed8
        blend_src_generic, // RGB32
        blend_src_generic, // ARGB32
        blend_transformed_tiled_argb, // ARGB32_Premultiplied
        blend_transformed_tiled_rgb565,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic
    },
    // Bilinear
    {
        0,
        blend_src_generic, // Mono
        blend_src_generic, // MonoLsb
        blend_src_generic, // Indexed8
        blend_src_generic, // RGB32
        blend_src_generic, // ARGB32
        blend_src_generic, // ARGB32_Premultiplied
        blend_transformed_bilinear_rgb565,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
        blend_src_generic,
    },
    // BilinearTiled
    {
        0,
        blend_src_generic, // Mono
        blend_src_generic, // MonoLsb
        blend_src_generic, // Indexed8
        blend_src_generic, // RGB32
        blend_src_generic, // ARGB32
        blend_src_generic, // ARGB32_Premultiplied
        blend_src_generic, // RGB16
        blend_src_generic, // ARGB8565_Premultiplied
        blend_src_generic, // RGB666
        blend_src_generic, // ARGB6666_Premultiplied
        blend_src_generic, // RGB555
        blend_src_generic, // ARGB8555_Premultiplied
        blend_src_generic, // RGB888
        blend_src_generic, // RGB444
        blend_src_generic, // ARGB4444_Premultiplied
        blend_src_generic, // RGBX8888
        blend_src_generic, // RGBA8888
        blend_src_generic, // RGBA8888_Premultiplied
    }
};

void qBlendTexture(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);
    ProcessSpans proc = processTextureSpans[getBlendType(data)][data->rasterBuffer->format];
    proc(count, spans, userData);
}

template <class DST> Q_STATIC_TEMPLATE_FUNCTION
inline void qt_bitmapblit_template(QRasterBuffer *rasterBuffer,
                                   int x, int y, DST color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    DST *dest = reinterpret_cast<DST *>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(DST);

    if (mapWidth > 8) {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (int x = 0; x < mapWidth; x += 8) {
                uchar s = map[x >> 3];
                for (int i = 0; i < 8; ++i) {
                    if (s & 0x80) {
                        ++n;
                    } else {
                        if (n) {
                            qt_memfill(dest + x0, color, n);
                            x0 += n + 1;
                            n = 0;
                        } else {
                            ++x0;
                        }
                        if (!s) {
                            x0 += 8 - 1 - i;
                            break;
                        }
                    }
                    s <<= 1;
                }
            }
            if (n)
                qt_memfill(dest + x0, color, n);
            dest += destStride;
            map += mapStride;
        }
    } else {
        while (mapHeight--) {
            int x0 = 0;
            int n = 0;
            for (uchar s = *map; s; s <<= 1) {
                if (s & 0x80) {
                    ++n;
                } else if (n) {
                    qt_memfill(dest + x0, color, n);
                    x0 += n + 1;
                    n = 0;
                } else {
                    ++x0;
                }
            }
            if (n)
                qt_memfill(dest + x0, color, n);
            dest += destStride;
            map += mapStride;
        }
    }
}

static void qt_gradient_argb32(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;

    if (isVerticalGradient) {
        LinearGradientValues linear;
        getLinearGradientValues(&linear, data);

        CompositionFunctionSolid funcSolid =
            functionForModeSolid[data->rasterBuffer->compositionMode];

        /*
            The logic for vertical gradient calculations is a mathematically
            reduced copy of that in fetchLinearGradient() - which is basically:

                qreal ry = data->m22 * (y + 0.5) + data->dy;
                qreal t = linear.dy*ry + linear.off;
                t *= (GRADIENT_STOPTABLE_SIZE - 1);
                quint32 color =
                    qt_gradient_pixel_fixed(&data->gradient,
                                            int(t * FIXPT_SIZE));

            This has then been converted to fixed point to improve performance.
         */
        const int gss = GRADIENT_STOPTABLE_SIZE - 1;
        int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
        int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

        while (count--) {
            int y = spans->y;
            int x = spans->x;

            quint32 *dst = (quint32 *)(data->rasterBuffer->scanLine(y)) + x;
            quint32 color =
                qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);

            funcSolid(dst, spans->len, color, spans->coverage);
            ++spans;
        }

    } else {
        blend_src_generic(count, spans, userData);
    }
}

static void qt_gradient_quint16(int count, const QSpan *spans, void *userData)
{
    QSpanData *data = reinterpret_cast<QSpanData *>(userData);

    bool isVerticalGradient =
        data->txop <= QTransform::TxScale &&
        data->type == QSpanData::LinearGradient &&
        data->gradient.linear.end.x == data->gradient.linear.origin.x;

    if (isVerticalGradient) {

        LinearGradientValues linear;
        getLinearGradientValues(&linear, data);

        /*
            The logic for vertical gradient calculations is a mathematically
            reduced copy of that in fetchLinearGradient() - which is basically:

                qreal ry = data->m22 * (y + 0.5) + data->dy;
                qreal t = linear.dy*ry + linear.off;
                t *= (GRADIENT_STOPTABLE_SIZE - 1);
                quint32 color =
                    qt_gradient_pixel_fixed(&data->gradient,
                                            int(t * FIXPT_SIZE));

            This has then been converted to fixed point to improve performance.
         */
        const int gss = GRADIENT_STOPTABLE_SIZE - 1;
        int yinc = int((linear.dy * data->m22 * gss) * FIXPT_SIZE);
        int off = int((((linear.dy * (data->m22 * qreal(0.5) + data->dy) + linear.off) * gss) * FIXPT_SIZE));

        uint oldColor = data->solid.color;
        while (count--) {
            int y = spans->y;

            quint32 color = qt_gradient_pixel_fixed(&data->gradient, yinc * y + off);

            data->solid.color = color;
            blend_color_rgb16(1, spans, userData);
            ++spans;
        }
        data->solid.color = oldColor;

    } else {
        blend_src_generic(count, spans, userData);
    }
}

inline static void qt_bitmapblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x,  y,  color,
                                    map, mapWidth, mapHeight, mapStride);
}

inline static void qt_bitmapblit_rgba8888(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint32>(rasterBuffer, x, y, ARGB2RGBA(color),
                                    map, mapWidth, mapHeight, mapStride);
}

inline static void qt_bitmapblit_quint16(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride)
{
    qt_bitmapblit_template<quint16>(rasterBuffer, x,  y,  qConvertRgb32To16(color),
                                    map, mapWidth, mapHeight, mapStride);
}

static void qt_alphamapblit_quint16(QRasterBuffer *rasterBuffer,
                                    int x, int y, quint32 color,
                                    const uchar *map,
                                    int mapWidth, int mapHeight, int mapStride,
                                    const QClipData *)
{
    const quint16 c = qConvertRgb32To16(color);
    quint16 *dest = reinterpret_cast<quint16*>(rasterBuffer->scanLine(y)) + x;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint16);

    while (mapHeight--) {
        for (int i = 0; i < mapWidth; ++i) {
            const int coverage = map[i];

            if (coverage == 0) {
                // nothing
            } else if (coverage == 255) {
                dest[i] = c;
            } else {
                int ialpha = 255 - coverage;
                dest[i] = BYTE_MUL_RGB16(c, coverage)
                          + BYTE_MUL_RGB16(dest[i], ialpha);
            }
        }
        dest += destStride;
        map += mapStride;
    }
}

static inline void rgbBlendPixel(quint32 *dst, int coverage, int sr, int sg, int sb, const uchar *gamma, const uchar *invgamma)
{
    // Do a gray alphablend...
    int da = qAlpha(*dst);
    int dr = qRed(*dst);
    int dg = qGreen(*dst);
    int db = qBlue(*dst);

    if (da != 255
        ) {

        int a = qGray(coverage);
        sr = qt_div_255(invgamma[sr] * a);
        sg = qt_div_255(invgamma[sg] * a);
        sb = qt_div_255(invgamma[sb] * a);

        int ia = 255 - a;
        dr = qt_div_255(dr * ia);
        dg = qt_div_255(dg * ia);
        db = qt_div_255(db * ia);

        *dst = ((a + qt_div_255((255 - a) * da)) << 24)
            |  ((sr + dr) << 16)
            |  ((sg + dg) << 8)
            |  ((sb + db));
        return;
    }

    int mr = qRed(coverage);
    int mg = qGreen(coverage);
    int mb = qBlue(coverage);

    dr = gamma[dr];
    dg = gamma[dg];
    db = gamma[db];

    int nr = qt_div_255((sr - dr) * mr) + dr;
    int ng = qt_div_255((sg - dg) * mg) + dg;
    int nb = qt_div_255((sb - db) * mb) + db;

    nr = invgamma[nr];
    ng = invgamma[ng];
    nb = invgamma[nb];

    *dst = qRgb(nr, ng, nb);
}

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
static inline void grayBlendPixel(quint32 *dst, int coverage, int sr, int sg, int sb, const uint *gamma, const uchar *invgamma)
{
    // Do a gammacorrected gray alphablend...
    int dr = qRed(*dst);
    int dg = qGreen(*dst);
    int db = qBlue(*dst);

    dr = gamma[dr];
    dg = gamma[dg];
    db = gamma[db];

    int alpha = coverage;
    int ialpha = 255 - alpha;
    int nr = (sr * alpha + ialpha * dr) / 255;
    int ng = (sg * alpha + ialpha * dg) / 255;
    int nb = (sb * alpha + ialpha * db) / 255;

    nr = invgamma[nr];
    ng = invgamma[ng];
    nb = invgamma[nb];

    *dst = qRgb(nr, ng, nb);
}
#endif

static void qt_alphamapblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uchar *map,
                                   int mapWidth, int mapHeight, int mapStride,
                                   const QClipData *clip)
{
    const quint32 c = color;
    const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint32);

#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
    const QDrawHelperGammaTables *tables = QGuiApplicationPrivate::instance()->gammaTables();
    if (!tables)
        return;

    const uint *gamma = tables->qt_pow_gamma;
    const uchar *invgamma = tables->qt_pow_invgamma;

    int sr = gamma[qRed(color)];
    int sg = gamma[qGreen(color)];
    int sb = gamma[qBlue(color)];

    bool opaque_src = (qAlpha(color) == 255);
#endif

    if (!clip) {
        quint32 *dest = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i) {
                const int coverage = map[i];

                if (coverage == 0) {
                    // nothing
                } else if (coverage == 255) {
                    dest[i] = c;
                } else {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
                    if (QSysInfo::WindowsVersion >= QSysInfo::WV_XP && opaque_src
                        && qAlpha(dest[i]) == 255) {
                        grayBlendPixel(dest+i, coverage, sr, sg, sb, gamma, invgamma);
                    } else
#endif
                    {
                        int ialpha = 255 - coverage;
                        dest[i] = INTERPOLATE_PIXEL_255(c, coverage, dest[i], ialpha);
                    }
                }
            }
            dest += destStride;
            map += mapStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        map += (top - y) * mapStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint32 *dest = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp) {
                    const int coverage = map[xp - x];

                    if (coverage == 0) {
                        // nothing
                    } else if (coverage == 255) {
                        dest[xp] = c;
                    } else {
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE)
                        if (QSysInfo::WindowsVersion >= QSysInfo::WV_XP && opaque_src
                            && qAlpha(dest[xp]) == 255) {
                            grayBlendPixel(dest+xp, coverage, sr, sg, sb, gamma, invgamma);
                        } else
#endif
                        {
                            int ialpha = 255 - coverage;
                            dest[xp] = INTERPOLATE_PIXEL_255(c, coverage, dest[xp], ialpha);
                        }
                    }

                } // for (i -> line.count)
            } // for (yp -> bottom)
            map += mapStride;
        }
    }
}

static void qt_alphamapblit_rgba8888(QRasterBuffer *rasterBuffer,
                                     int x, int y, quint32 color,
                                     const uchar *map,
                                     int mapWidth, int mapHeight, int mapStride,
                                     const QClipData *clip)
{
    qt_alphamapblit_argb32(rasterBuffer, x, y, ARGB2RGBA(color), map, mapWidth, mapHeight, mapStride, clip);
}

static void qt_alphargbblit_argb32(QRasterBuffer *rasterBuffer,
                                   int x, int y, quint32 color,
                                   const uint *src, int mapWidth, int mapHeight, int srcStride,
                                   const QClipData *clip)
{
    const quint32 c = color;

    int sr = qRed(color);
    int sg = qGreen(color);
    int sb = qBlue(color);
    int sa = qAlpha(color);

    const QDrawHelperGammaTables *tables = QGuiApplicationPrivate::instance()->gammaTables();
    if (!tables)
        return;

    const uchar *gamma = tables->qt_pow_rgb_gamma;
    const uchar *invgamma = tables->qt_pow_rgb_invgamma;

    sr = gamma[sr];
    sg = gamma[sg];
    sb = gamma[sb];

    if (sa == 0)
        return;

    if (!clip) {
        quint32 *dst = reinterpret_cast<quint32*>(rasterBuffer->scanLine(y)) + x;
        const int destStride = rasterBuffer->bytesPerLine() / sizeof(quint32);
        while (mapHeight--) {
            for (int i = 0; i < mapWidth; ++i) {
                const uint coverage = src[i];
                if (coverage == 0xffffffff) {
                    dst[i] = c;
                } else if (coverage != 0xff000000) {
                    rgbBlendPixel(dst+i, coverage, sr, sg, sb, gamma, invgamma);
                }
            }

            dst += destStride;
            src += srcStride;
        }
    } else {
        int bottom = qMin(y + mapHeight, rasterBuffer->height());

        int top = qMax(y, 0);
        src += (top - y) * srcStride;

        const_cast<QClipData *>(clip)->initialize();
        for (int yp = top; yp<bottom; ++yp) {
            const QClipData::ClipLine &line = clip->m_clipLines[yp];

            quint32 *dst = reinterpret_cast<quint32 *>(rasterBuffer->scanLine(yp));

            for (int i=0; i<line.count; ++i) {
                const QSpan &clip = line.spans[i];

                int start = qMax<int>(x, clip.x);
                int end = qMin<int>(x + mapWidth, clip.x + clip.len);

                for (int xp=start; xp<end; ++xp) {
                    const uint coverage = src[xp - x];
                    if (coverage == 0xffffffff) {
                        dst[xp] = c;
                    } else if (coverage != 0xff000000) {
                        rgbBlendPixel(dst+xp, coverage, sr, sg, sb, gamma, invgamma);
                    }
                }
            } // for (i -> line.count)
            src += srcStride;
        } // for (yp -> bottom)

    }
}

static void qt_rectfill_argb32(QRasterBuffer *rasterBuffer,
                               int x, int y, int width, int height,
                               quint32 color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         color, x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_quint16(QRasterBuffer *rasterBuffer,
                                int x, int y, int width, int height,
                                quint32 color)
{
    qt_rectfill<quint16>(reinterpret_cast<quint16 *>(rasterBuffer->buffer()),
                         qConvertRgb32To16(color), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_nonpremul_argb32(QRasterBuffer *rasterBuffer,
                                         int x, int y, int width, int height,
                                         quint32 color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         qUnpremultiply(color), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_rgba(QRasterBuffer *rasterBuffer,
                             int x, int y, int width, int height,
                             quint32 color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         ARGB2RGBA(color), x, y, width, height, rasterBuffer->bytesPerLine());
}

static void qt_rectfill_nonpremul_rgba(QRasterBuffer *rasterBuffer,
                                       int x, int y, int width, int height,
                                       quint32 color)
{
    qt_rectfill<quint32>(reinterpret_cast<quint32 *>(rasterBuffer->buffer()),
                         ARGB2RGBA(qUnpremultiply(color)), x, y, width, height, rasterBuffer->bytesPerLine());
}


// Map table for destination image format. Contains function pointers
// for blends of various types unto the destination

DrawHelper qDrawHelper[QImage::NImageFormats] =
{
    // Format_Invalid,
    { 0, 0, 0, 0, 0, 0 },
    // Format_Mono,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_MonoLSB,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_Indexed8,
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGB32,
    {
        blend_color_argb,
        qt_gradient_argb32,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_argb32
    },
    // Format_ARGB32,
    {
        blend_color_generic,
        qt_gradient_argb32,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_nonpremul_argb32
    },
    // Format_ARGB32_Premultiplied
    {
        blend_color_argb,
        qt_gradient_argb32,
        qt_bitmapblit_argb32,
        qt_alphamapblit_argb32,
        qt_alphargbblit_argb32,
        qt_rectfill_argb32
    },
    // Format_RGB16
    {
        blend_color_rgb16,
        qt_gradient_quint16,
        qt_bitmapblit_quint16,
        qt_alphamapblit_quint16,
        0,
        qt_rectfill_quint16
    },
    // Format_ARGB8565_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGB666
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_ARGB6666_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGB555
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_ARGB8555_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGB888
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGB444
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_ARGB4444_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        0, 0, 0, 0
    },
    // Format_RGBX8888
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_rgba8888,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_alphamapblit_rgba8888,
#else
        0,
#endif
        0,
        qt_rectfill_rgba
    },
    // Format_RGBA8888
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_rgba8888,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_alphamapblit_rgba8888,
#else
        0,
#endif
        0,
        qt_rectfill_nonpremul_rgba
    },
    // Format_RGB8888_Premultiplied
    {
        blend_color_generic,
        blend_src_generic,
        qt_bitmapblit_rgba8888,
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        qt_alphamapblit_rgba8888,
#else
        0,
#endif
        0,
        qt_rectfill_rgba
    }
};

#if defined(Q_CC_MSVC) && !defined(_MIPS_)
template <class T>
inline void qt_memfill_template(T *dest, T color, int count)
{
    while (count--)
        *dest++ = color;
}

#else

template <class T>
inline void qt_memfill_template(T *dest, T color, int count)
{
    int n = (count + 7) / 8;
    switch (count & 0x07)
    {
    case 0: do { *dest++ = color;
    case 7:      *dest++ = color;
    case 6:      *dest++ = color;
    case 5:      *dest++ = color;
    case 4:      *dest++ = color;
    case 3:      *dest++ = color;
    case 2:      *dest++ = color;
    case 1:      *dest++ = color;
    } while (--n > 0);
    }
}

template <>
inline void qt_memfill_template(quint16 *dest, quint16 value, int count)
{
    if (count < 3) {
        switch (count) {
        case 2: *dest++ = value;
        case 1: *dest = value;
        }
        return;
    }

    const int align = (quintptr)(dest) & 0x3;
    switch (align) {
    case 2: *dest++ = value; --count;
    }

    const quint32 value32 = (value << 16) | value;
    qt_memfill(reinterpret_cast<quint32*>(dest), value32, count / 2);
    if (count & 0x1)
        dest[count - 1] = value;
}
#endif

#if !defined(__SSE2__)
void qt_memfill16(quint16 *dest, quint16 color, int count)
{
    qt_memfill_template<quint16>(dest, color, count);
}
#endif
#if !defined(__SSE2__) && !defined(__ARM_NEON__)
void qt_memfill32(quint32 *dest, quint32 color, int count)
{
#  ifdef QT_COMPILER_SUPPORTS_MIPS_DSP
    extern "C" qt_memfill32_asm_mips_dsp(quint32 *, quint32, int);
    qt_memfill32_asm_mips_dsp(dest, color, count);
#  else
    qt_memfill_template<quint32>(dest, color, count);
#  endif
}
#endif

void qInitDrawhelperAsm()
{
    CompositionFunction *functionForModeAsm = 0;
    CompositionFunctionSolid *functionForModeSolidAsm = 0;

    const uint features = qCpuFeatures();
    Q_UNUSED(features);
#ifdef __SSE2__
    qDrawHelper[QImage::Format_RGB32].bitmapBlit = qt_bitmapblit32_sse2;
    qDrawHelper[QImage::Format_ARGB32].bitmapBlit = qt_bitmapblit32_sse2;
    qDrawHelper[QImage::Format_ARGB32_Premultiplied].bitmapBlit = qt_bitmapblit32_sse2;
    qDrawHelper[QImage::Format_RGB16].bitmapBlit = qt_bitmapblit16_sse2;
    qDrawHelper[QImage::Format_RGBX8888].bitmapBlit = qt_bitmapblit8888_sse2;
    qDrawHelper[QImage::Format_RGBA8888].bitmapBlit = qt_bitmapblit8888_sse2;
    qDrawHelper[QImage::Format_RGBA8888_Premultiplied].bitmapBlit = qt_bitmapblit8888_sse2;

    extern void qt_scale_image_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                                     const uchar *srcPixels, int sbpl, int srch,
                                                     const QRectF &targetRect,
                                                     const QRectF &sourceRect,
                                                     const QRect &clip,
                                                     int const_alpha);
    qScaleFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;
    qScaleFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;
    qScaleFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;
    qScaleFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_scale_image_argb32_on_argb32_sse2;

    extern void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                             const uchar *srcPixels, int sbpl,
                                             int w, int h,
                                             int const_alpha);
    extern void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                               const uchar *srcPixels, int sbpl,
                                               int w, int h,
                                               int const_alpha);

    qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_sse2;
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_sse2;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_sse2;

    extern const uint * QT_FASTCALL qt_fetch_radial_gradient_sse2(uint *buffer, const Operator *op, const QSpanData *data,
                                                                  int y, int x, int length);

    qt_fetch_radial_gradient = qt_fetch_radial_gradient_sse2;

#ifdef QT_COMPILER_SUPPORTS_SSSE3
    if (features & SSSE3) {
        extern void qt_blend_argb32_on_argb32_ssse3(uchar *destPixels, int dbpl,
                                                    const uchar *srcPixels, int sbpl,
                                                    int w, int h,
                                                    int const_alpha);

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
        qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_ssse3;
    }
#endif // SSSE3

    functionForModeAsm = qt_functionForMode_SSE2;
    functionForModeSolidAsm = qt_functionForModeSolid_SSE2;
#endif // SSE2

#ifdef QT_COMPILER_SUPPORTS_IWMMXT
    if (features & IWMMXT) {
        functionForModeAsm = qt_functionForMode_IWMMXT;
        functionForModeSolidAsm = qt_functionForModeSolid_IWMMXT;
        qDrawHelper[QImage::Format_ARGB32_Premultiplied].blendColor = qt_blend_color_argb_iwmmxt;
    }
#endif // IWMMXT

#if defined(__ARM_NEON__) && !defined(Q_OS_IOS)
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_neon;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_neon;
    qBlendFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_rgb16_neon;
    qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB16] = qt_blend_rgb16_on_argb32_neon;
    qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_neon;
#if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBX8888] = qt_blend_rgb32_on_rgb32_neon;
    qBlendFunctions[QImage::Format_RGBX8888][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_neon;
    qBlendFunctions[QImage::Format_RGBA8888_Premultiplied][QImage::Format_RGBA8888_Premultiplied] = qt_blend_argb32_on_argb32_neon;
#endif

    qScaleFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_scale_image_argb32_on_rgb16_neon;
    qScaleFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_scale_image_rgb16_on_rgb16_neon;

    qTransformFunctions[QImage::Format_RGB16][QImage::Format_ARGB32_Premultiplied] = qt_transform_image_argb32_on_rgb16_neon;
    qTransformFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_transform_image_rgb16_on_rgb16_neon;

    qDrawHelper[QImage::Format_RGB16].alphamapBlit = qt_alphamapblit_quint16_neon;

    functionForMode_C[QPainter::CompositionMode_SourceOver] = qt_blend_argb32_on_argb32_scanline_neon;
    functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_neon;
    functionForMode_C[QPainter::CompositionMode_Plus] = comp_func_Plus_neon;
    destFetchProc[QImage::Format_RGB16] = qt_destFetchRGB16_neon;
    destStoreProc[QImage::Format_RGB16] = qt_destStoreRGB16_neon;

    qMemRotateFunctions[QImage::Format_RGB16][0] = qt_memrotate90_16_neon;
    qMemRotateFunctions[QImage::Format_RGB16][2] = qt_memrotate270_16_neon;

    extern const uint * QT_FASTCALL qt_fetch_radial_gradient_neon(uint *buffer, const Operator *op, const QSpanData *data,
                                                                  int y, int x, int length);

    qt_fetch_radial_gradient = qt_fetch_radial_gradient_neon;
#endif

#if defined(QT_COMPILER_SUPPORTS_MIPS_DSP)
        functionForMode_C[QPainter::CompositionMode_SourceOver] = comp_func_SourceOver_asm_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_Source] = comp_func_Source_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_DestinationOver] = comp_func_DestinationOver_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_SourceIn] = comp_func_SourceIn_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_DestinationIn] = comp_func_DestinationIn_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_DestinationOut] = comp_func_DestinationOut_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_SourceAtop] = comp_func_SourceAtop_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_DestinationAtop] = comp_func_DestinationAtop_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_Xor] = comp_func_XOR_mips_dsp;
        functionForMode_C[QPainter::CompositionMode_SourceOut] = comp_func_SourceOut_mips_dsp;

        functionForModeSolid_C[QPainter::CompositionMode_SourceOver] = comp_func_solid_SourceOver_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_DestinationOver] = comp_func_solid_DestinationOver_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_SourceIn] = comp_func_solid_SourceIn_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_DestinationIn] = comp_func_solid_DestinationIn_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_SourceAtop] = comp_func_solid_SourceAtop_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_DestinationAtop] = comp_func_solid_DestinationAtop_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_Xor] = comp_func_solid_XOR_mips_dsp;
        functionForModeSolid_C[QPainter::CompositionMode_SourceOut] = comp_func_solid_SourceOut_mips_dsp;

        qBlendFunctions[QImage::Format_RGB32][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_mips_dsp;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_RGB32] = qt_blend_rgb32_on_rgb32_mips_dsp;
        qBlendFunctions[QImage::Format_RGB32][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_mips_dsp;
        qBlendFunctions[QImage::Format_ARGB32_Premultiplied][QImage::Format_ARGB32_Premultiplied] = qt_blend_argb32_on_argb32_mips_dsp;

        destFetchProc[QImage::Format_ARGB32] = qt_destFetchARGB32_mips_dsp;

        destStoreProc[QImage::Format_ARGB32] = qt_destStoreARGB32_mips_dsp;

        sourceFetch[BlendUntransformed][QImage::Format_RGB888] = qt_fetchUntransformed_888_mips_dsp;
        sourceFetch[BlendTiled][QImage::Format_RGB888] = qt_fetchUntransformed_888_mips_dsp;

        sourceFetch[BlendUntransformed][QImage::Format_RGB444] = qt_fetchUntransformed_444_mips_dsp;
        sourceFetch[BlendTiled][QImage::Format_RGB444] = qt_fetchUntransformed_444_mips_dsp;

        sourceFetch[BlendUntransformed][QImage::Format_ARGB8565_Premultiplied] = qt_fetchUntransformed_argb8565_premultiplied_mips_dsp;
        sourceFetch[BlendTiled][QImage::Format_ARGB8565_Premultiplied] = qt_fetchUntransformed_argb8565_premultiplied_mips_dsp;

#if defined(QT_COMPILER_SUPPORTS_MIPS_DSPR2)
        qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_mips_dspr2;
#else
        qBlendFunctions[QImage::Format_RGB16][QImage::Format_RGB16] = qt_blend_rgb16_on_rgb16_mips_dsp;
#endif // QT_COMPILER_SUPPORTS_MIPS_DSPR2

#endif // QT_COMPILER_SUPPORTS_MIPS_DSP
    if (functionForModeSolidAsm) {
        const int destinationMode = QPainter::CompositionMode_Destination;
        functionForModeSolidAsm[destinationMode] = functionForModeSolid_C[destinationMode];

        // use the default qdrawhelper implementation for the
        // extended composition modes
        for (int mode = 12; mode < 24; ++mode)
            functionForModeSolidAsm[mode] = functionForModeSolid_C[mode];

        functionForModeSolid = functionForModeSolidAsm;
    }
    if (functionForModeAsm)
        functionForMode = functionForModeAsm;
}

QT_END_NAMESPACE
