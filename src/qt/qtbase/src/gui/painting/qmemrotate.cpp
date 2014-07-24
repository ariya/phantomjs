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

#include "private/qmemrotate_p.h"

QT_BEGIN_NAMESPACE

#if QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
static const int tileSize = 32;
#endif

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
#if QT_ROTATION_ALGORITHM == QT_ROTATION_PACKED || QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
#error Big endian version not implemented for the transformed driver!
#endif
#endif

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate90_cachedRead(const T *src, int w, int h, int sstride, T *dest,
                                      int dstride)
{
    const char *s = reinterpret_cast<const char*>(src);
    char *d = reinterpret_cast<char*>(dest);
    for (int y = 0; y < h; ++y) {
        for (int x = w - 1; x >= 0; --x) {
            T *destline = reinterpret_cast<T *>(d + (w - x - 1) * dstride);
            destline[y] = src[x];
        }
        s += sstride;
        src = reinterpret_cast<const T*>(s);
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate270_cachedRead(const T *src, int w, int h, int sstride, T *dest,
                                       int dstride)
{
    const char *s = reinterpret_cast<const char*>(src);
    char *d = reinterpret_cast<char*>(dest);
    s += (h - 1) * sstride;
    for (int y = h - 1; y >= 0; --y) {
        src = reinterpret_cast<const T*>(s);
        for (int x = 0; x < w; ++x) {
            T *destline = reinterpret_cast<T *>(d + x * dstride);
            destline[h - y - 1] = src[x];
        }
        s -= sstride;
    }
}

#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate90_cachedWrite(const T *src, int w, int h, int sstride, T *dest,
                                       int dstride)
{
    for (int x = w - 1; x >= 0; --x) {
        T *d = dest + (w - x - 1) * dstride;
        for (int y = 0; y < h; ++y) {
            *d++ = src[y * sstride + x];
        }
    }

}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate270_cachedWrite(const T *src, int w, int h, int sstride, T *dest,
                                        int dstride)
{
    for (int x = 0; x < w; ++x) {
        T *d = dest + x * dstride;
        for (int y = h - 1; y >= 0; --y) {
            *d++ = src[y * sstride + x];
        }
    }
}

#endif // QT_ROTATION_CACHEDWRITE

#if QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING

// TODO: packing algorithms should probably be modified on 64-bit architectures

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate90_packing(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);

    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned = int((long(dest) & (sizeof(quint32)-1))) / sizeof(T);

    for (int x = w - 1; x >= 0; --x) {
        int y = 0;

        for (int i = 0; i < unaligned; ++i) {
            dest[(w - x - 1) * dstride + y] = src[y * sstride + x];
            ++y;
        }

        quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride
                                                + unaligned);
        const int rest = (h - unaligned) % pack;
        while (y < h - rest) {
            quint32 c = src[y * sstride + x];
            for (int i = 1; i < pack; ++i) {
                c |= src[(y + i) * sstride + x] << (sizeof(int) * 8 / pack * i);
            }
            *d++ = c;
            y += pack;
        }

        while (y < h) {
            dest[(w - x - 1) * dstride + y] = src[y * sstride + x];
            ++y;
        }
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate270_packing(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);

    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned = int((long(dest) & (sizeof(quint32)-1))) / sizeof(T);

    for (int x = 0; x < w; ++x) {
        int y = h - 1;

        for (int i = 0; i < unaligned; ++i) {
            dest[x * dstride + h - y - 1] = src[y * sstride + x];
            --y;
        }

        quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                                                + unaligned);
        const int rest = (h - unaligned) % pack;
        while (y > rest) {
            quint32 c = src[y * sstride + x];
            for (int i = 1; i < pack; ++i) {
                c |= src[(y - i) * sstride + x] << (sizeof(int) * 8 / pack * i);
            }
            *d++ = c;
            y -= pack;
        }
        while (y >= 0) {
            dest[x * dstride + h - y - 1] = src[y * sstride + x];
            --y;
        }
    }
}

#endif // QT_ROTATION_PACKING

#if QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate90_tiled(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);

    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned =
        qMin(uint((quintptr(dest) & (sizeof(quint32)-1)) / sizeof(T)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        if (unaligned) {
            for (int x = startx; x >= stopx; --x) {
                T *d = dest + (w - x - 1) * dstride;
                for (int y = 0; y < unaligned; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize + unaligned;
            const int stopy = qMin(starty + tileSize, h - unoptimizedY);

            for (int x = startx; x >= stopx; --x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + (w - x - 1) * dstride + starty);
                for (int y = starty; y < stopy; y += pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(int) * 8 / pack * i);
                        const T color = src[(y + i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }

        if (unoptimizedY) {
            const int starty = h - unoptimizedY;
            for (int x = startx; x >= stopx; --x) {
                T *d = dest + (w - x - 1) * dstride + starty;
                for (int y = starty; y < h; ++y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate90_tiled_unpacked(const T *src, int w, int h, int sstride, T *dest,
                                          int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = w - tx * tileSize - 1;
        const int stopx = qMax(startx - tileSize, 0);

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = ty * tileSize;
            const int stopy = qMin(starty + tileSize, h);

            for (int x = startx; x >= stopx; --x) {
                T *d = (T *)((char*)dest + (w - x - 1) * dstride) + starty;
                const char *s = (const char*)(src + x) + starty * sstride;
                for (int y = starty; y < stopy; ++y) {
                    *d++ = *(const T *)(s);
                    s += sstride;
                }
            }
        }
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate270_tiled(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    sstride /= sizeof(T);
    dstride /= sizeof(T);

    const int pack = sizeof(quint32) / sizeof(T);
    const int unaligned =
        qMin(uint((long(dest) & (sizeof(quint32)-1)) / sizeof(T)), uint(h));
    const int restX = w % tileSize;
    const int restY = (h - unaligned) % tileSize;
    const int unoptimizedY = restY % pack;
    const int numTilesX = w / tileSize + (restX > 0);
    const int numTilesY = (h - unaligned) / tileSize + (restY >= pack);

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        if (unaligned) {
            for (int x = startx; x < stopx; ++x) {
                T *d = dest + x * dstride;
                for (int y = h - 1; y >= h - unaligned; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - unaligned - ty * tileSize;
            const int stopy = qMax(starty - tileSize, unoptimizedY);

            for (int x = startx; x < stopx; ++x) {
                quint32 *d = reinterpret_cast<quint32*>(dest + x * dstride
                                                        + h - 1 - starty);
                for (int y = starty; y > stopy; y -= pack) {
                    quint32 c = src[y * sstride + x];
                    for (int i = 1; i < pack; ++i) {
                        const int shift = (sizeof(int) * 8 / pack * i);
                        const T color = src[(y - i) * sstride + x];
                        c |= color << shift;
                    }
                    *d++ = c;
                }
            }
        }
        if (unoptimizedY) {
            const int starty = unoptimizedY - 1;
            for (int x = startx; x < stopx; ++x) {
                T *d = dest + x * dstride + h - 1 - starty;
                for (int y = starty; y >= 0; --y) {
                    *d++ = src[y * sstride + x];
                }
            }
        }
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate270_tiled_unpacked(const T *src, int w, int h, int sstride, T *dest,
                                           int dstride)
{
    const int numTilesX = (w + tileSize - 1) / tileSize;
    const int numTilesY = (h + tileSize - 1) / tileSize;

    for (int tx = 0; tx < numTilesX; ++tx) {
        const int startx = tx * tileSize;
        const int stopx = qMin(startx + tileSize, w);

        for (int ty = 0; ty < numTilesY; ++ty) {
            const int starty = h - 1 - ty * tileSize;
            const int stopy = qMax(starty - tileSize, 0);

            for (int x = startx; x < stopx; ++x) {
                T *d = (T*)((char*)dest + x * dstride) + h - 1 - starty;
                const char *s = (const char*)(src + x) + starty * sstride;
                for (int y = starty; y >= stopy; --y) {
                    *d++ = *(const T*)s;
                    s -= sstride;
                }
            }
        }
    }
}

#endif // QT_ROTATION_ALGORITHM

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate90_template(const T *src, int srcWidth, int srcHeight, int srcStride,
                                    T *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate90_cachedRead<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate90_cachedWrite<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    qt_memrotate90_packing<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    qt_memrotate90_tiled<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#endif
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate180_template(const T *src, int w, int h, int sstride, T *dest, int dstride)
{
    const char *s = (const char*)(src) + (h - 1) * sstride;
    for (int y = h - 1; y >= 0; --y) {
        T *d = reinterpret_cast<T*>((char *)(dest) + (h - y - 1) * dstride);
        src = reinterpret_cast<const T*>(s);
        for (int x = w - 1; x >= 0; --x) {
            d[w - x - 1] = src[x];
        }
        s -= sstride;
    }
}

template <class T>
Q_STATIC_TEMPLATE_FUNCTION
inline void qt_memrotate270_template(const T *src, int srcWidth, int srcHeight, int srcStride,
                                     T *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate270_cachedRead<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate270_cachedWrite<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    qt_memrotate270_packing<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    qt_memrotate270_tiled_unpacked<T>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#endif
}

template <>
inline void qt_memrotate90_template<quint24>(const quint24 *src, int srcWidth, int srcHeight,
                                             int srcStride, quint24 *dest, int dstStride)
{
#if QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDREAD
    qt_memrotate90_cachedRead<quint24>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_CACHEDWRITE
    qt_memrotate90_cachedWrite<quint24>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_PACKING
    // packed algorithm not implemented
    qt_memrotate90_cachedRead<quint24>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#elif QT_ROTATION_ALGORITHM == QT_ROTATION_TILED
    // packed algorithm not implemented
    qt_memrotate90_tiled_unpacked<quint24>(src, srcWidth, srcHeight, srcStride, dest, dstStride);
#endif
}

#define QT_IMPL_MEMROTATE(type)                                     \
Q_GUI_EXPORT void qt_memrotate90(const type *src, int w, int h, int sstride, \
                                 type *dest, int dstride)           \
{                                                                   \
    qt_memrotate90_template(src, w, h, sstride, dest, dstride);     \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate180(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate270(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate270_template(src, w, h, sstride, dest, dstride);    \
}

#define QT_IMPL_SIMPLE_MEMROTATE(type)                              \
Q_GUI_EXPORT void qt_memrotate90(const type *src, int w, int h, int sstride,  \
                                 type *dest, int dstride)           \
{                                                                   \
    qt_memrotate90_tiled_unpacked<type>(src, w, h, sstride, dest, dstride); \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate180(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate180_template(src, w, h, sstride, dest, dstride);    \
}                                                                   \
Q_GUI_EXPORT void qt_memrotate270(const type *src, int w, int h, int sstride, \
                                  type *dest, int dstride)          \
{                                                                   \
    qt_memrotate270_tiled_unpacked<type>(src, w, h, sstride, dest, dstride); \
}




QT_IMPL_MEMROTATE(quint32)
QT_IMPL_MEMROTATE(quint16)
QT_IMPL_MEMROTATE(quint24)
QT_IMPL_MEMROTATE(quint8)

void qt_memrotate90_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}

void qt_memrotate180_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}

void qt_memrotate270_16(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const ushort *)srcPixels, w, h, sbpl, (ushort *)destPixels, dbpl);
}

void qt_memrotate90_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate90((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}

void qt_memrotate180_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate180((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}

void qt_memrotate270_32(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl)
{
    qt_memrotate270((const uint *)srcPixels, w, h, sbpl, (uint *)destPixels, dbpl);
}

MemRotateFunc qMemRotateFunctions[QImage::NImageFormats][3] =
// 90, 180, 270
{
    { 0, 0, 0 },      // Format_Invalid,
    { 0, 0, 0 },      // Format_Mono,
    { 0, 0, 0 },      // Format_MonoLSB,
    { 0, 0, 0 },      // Format_Indexed8,
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 },      // Format_RGB32,
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 },      // Format_ARGB32,
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 },      // Format_ARGB32_Premultiplied,
    { qt_memrotate90_16, qt_memrotate180_16, qt_memrotate270_16 },      // Format_RGB16,
    { 0, 0, 0 },      // Format_ARGB8565_Premultiplied,
    { 0, 0, 0 },      // Format_RGB666,
    { 0, 0, 0 },      // Format_ARGB6666_Premultiplied,
    { 0, 0, 0 },      // Format_RGB555,
    { 0, 0, 0 },      // Format_ARGB8555_Premultiplied,
    { 0, 0, 0 },      // Format_RGB888,
    { 0, 0, 0 },      // Format_RGB444,
    { 0, 0, 0 },      // Format_ARGB4444_Premultiplied,
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 },      // Format_RGBX8888,
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 },      // Format_RGBA8888,
    { qt_memrotate90_32, qt_memrotate180_32, qt_memrotate270_32 }       // Format_RGBA8888_Premultiplied,
};

QT_END_NAMESPACE
