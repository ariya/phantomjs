/****************************************************************************
**
** Copyright (C) 2013 Imagination Technologies Limited, www.imgtec.com
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

#include <private/qdrawhelper_p.h>
#include <private/qdrawhelper_mips_dsp_p.h>
#include <private/qpaintengine_raster_p.h>

QT_BEGIN_NAMESPACE

void qt_blend_argb32_on_argb32_mips_dsp(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha)

{
#ifdef QT_DEBUG_DRAW
    fprintf(stdout,
            "qt_blend_argb32_on_argb32: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
            destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    fflush(stdout);
#endif

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    if (const_alpha == 256) {
        for (int y=0; y<h; ++y) {
            qt_blend_argb32_on_argb32_const_alpha_256_mips_dsp_asm(dst, src, w);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    } else if (const_alpha != 0) {
        const_alpha = (const_alpha * 255) >> 8;
        for (int y=0; y<h; ++y) {
            if (h%2 > 0) {
                uint s = BYTE_MUL(src[0], const_alpha);
                dst[0] = s + BYTE_MUL(dst[0], qAlpha(~s));
                h--;
                dst++;
                src++;
            }
            qt_blend_argb32_on_argb32_mips_dsp_asm_x2(dst, src, h, const_alpha);
            dst = (quint32 *)(((uchar *) dst) + dbpl);
            src = (const quint32 *)(((const uchar *) src) + sbpl);
        }
    }
}

void qt_blend_rgb32_on_rgb32_mips_dsp(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha)
{
#ifdef QT_DEBUG_DRAW
    fprintf(stdout,
            "qt_blend_rgb32_on_rgb32: dst=(%p, %d), src=(%p, %d), dim=(%d, %d) alpha=%d\n",
            destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
    fflush(stdout);
#endif

    if (const_alpha != 256) {
        qt_blend_argb32_on_argb32_mips_dsp(destPixels, dbpl, srcPixels, sbpl, w, h, const_alpha);
        return;
    }

    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;
    int len = w * 4;
    for (int y=0; y<h; ++y) {
        memcpy(dst, src, len);
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        src = (const quint32 *)(((const uchar *) src) + sbpl);
    }
}

#if defined QT_COMPILER_SUPPORTS_MIPS_DSPR2
void qt_blend_rgb16_on_rgb16_mips_dspr2(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl,
                                        int w, int h,
                                        int const_alpha)
{
    if (const_alpha == 256) {
        if (w < 256) {
            const quint16 *src = (const quint16*) srcPixels;
            quint16 *dst = (quint16*) destPixels;
            for (int y = 0; y < h; ++y) {
                qt_blend_rgb16_on_rgb16_const_alpha_256_mips_dsp_asm(dst, src, w);
                dst = (quint16*) (((uchar*) dst) + dbpl);
                src = (quint16*) (((uchar*) src) + sbpl);
            }
        }
        else {
            int length = w << 1;
            while (h--) {
                memcpy(destPixels, srcPixels, length);
                destPixels += dbpl;
                srcPixels += sbpl;
            }
        }
    }
    else if (const_alpha != 0) {
        const quint16 *src = (const quint16*) srcPixels;
        quint16 *dst = (quint16*) destPixels;
        for (int y = 0; y < h; ++y) {
            qt_blend_rgb16_on_rgb16_mips_dspr2_asm(dst, src, w, const_alpha);
            dst = (quint16*) (((uchar*) dst) + dbpl);
            src = (quint16*) (((uchar*) src) + sbpl);
        }
    }
}
#else
void qt_blend_rgb16_on_rgb16_mips_dsp(uchar *destPixels, int dbpl,
                                      const uchar *srcPixels, int sbpl,
                                      int w, int h,
                                      int const_alpha)
{
    if (const_alpha == 256) {
        if (w < 256) {
            const quint16 *src = (const quint16*) srcPixels;
            quint16 *dst = (quint16*) destPixels;
            for (int y = 0; y < h; ++y) {
                qt_blend_rgb16_on_rgb16_const_alpha_256_mips_dsp_asm(dst, src, w);
                dst = (quint16*) (((uchar*) dst) + dbpl);
                src = (quint16*) (((uchar*) src) + sbpl);
            }
        }
        else {
            int length = w << 1;
            while (h--) {
                memcpy(destPixels, srcPixels, length);
                destPixels += dbpl;
                srcPixels += sbpl;
            }
        }
    }
    else if (const_alpha != 0) {
        const quint16 *src = (const quint16*) srcPixels;
        quint16 *dst = (quint16*) destPixels;
        for (int y = 0; y < h; ++y) {
            qt_blend_rgb16_on_rgb16_mips_dsp_asm(dst, src, w, const_alpha);
            dst = (quint16*) (((uchar*) dst) + dbpl);
            src = (quint16*) (((uchar*) src) + sbpl);
        }
    }
}
#endif

void comp_func_Source_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (const_alpha == 255) {
        ::memcpy(dest, src, length * sizeof(uint));
    } else {
        int ialpha = 255 - const_alpha;
        if (length%2 > 0) {
            dest[0] = INTERPOLATE_PIXEL_255(src[0], const_alpha, dest[0], ialpha);
            length--;
            dest++;
            src++;
        }
        comp_func_Source_dsp_asm_x2(dest, src, length, const_alpha);
    }
}

uint * QT_FASTCALL qt_destFetchARGB32_mips_dsp(uint *buffer,
                                          QRasterBuffer *rasterBuffer,
                                          int x, int y, int length)
{
    const uint *data = (const uint *)rasterBuffer->scanLine(y) + x;
    buffer = destfetchARGB32_asm_mips_dsp(buffer, data, length);
    return buffer;
}

void QT_FASTCALL qt_destStoreARGB32_mips_dsp(QRasterBuffer *rasterBuffer, int x, int y,
                                             const uint *buffer, int length)
{
    uint *data = (uint *)rasterBuffer->scanLine(y) + x;
    qt_destStoreARGB32_asm_mips_dsp(data, buffer, length);
}

void QT_FASTCALL comp_func_solid_SourceOver_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    if (length%2 > 0) {
        dest[0] = color + BYTE_MUL(dest[0], qAlpha(~color));
        length--;
        dest++;
    }
    comp_func_solid_Source_dsp_asm_x2(dest, length, color, qAlpha(~color));
}

void QT_FASTCALL comp_func_solid_DestinationOver_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    if (length%2 > 0) {
        uint d = dest[0];
        dest[0] = d + BYTE_MUL(color, qAlpha(~d));
        length--;
        dest++;
    }
    comp_func_solid_DestinationOver_dsp_asm_x2(dest, length, color);
}

void QT_FASTCALL comp_func_DestinationOver_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            uint d = dest[0];
            dest[0] = d + BYTE_MUL(src[0], qAlpha(~d));
        } else {
            uint d = dest[0];
            uint s = BYTE_MUL(src[0], const_alpha);
            dest[0] = d + BYTE_MUL(s, qAlpha(~d));
        }
        length--;
        dest++;
        src++;
    }
    comp_func_DestinationOver_dsp_asm_x2(dest, src, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_SourceIn_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            dest[0] = BYTE_MUL(color, qAlpha(dest[0]));
        } else {
            uint tmp_color = BYTE_MUL(color, const_alpha);
            uint cia = 255 - const_alpha;
            uint d = dest[0];
            dest[0] = INTERPOLATE_PIXEL_255(tmp_color, qAlpha(d), d, cia);
        }
        length--;
        dest++;
    }
    comp_func_solid_SourceIn_dsp_asm_x2(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceIn_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            dest[0] = BYTE_MUL(src[0], qAlpha(dest[0]));
        } else {
            uint cia = 255 - const_alpha;
            uint d = dest[0];
            uint s = BYTE_MUL(src[0], const_alpha);
            dest[0] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, cia);
        }
        length--;
        dest++;
        src++;
    }
    comp_func_SourceIn_dsp_asm_x2(dest, src, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_DestinationIn_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    }
    if (length%2 > 0) {
        dest[0] = BYTE_MUL(dest[0], a);
        length--;
        dest++;
    }
    comp_func_solid_DestinationIn_dsp_asm_x2(dest, length, a);
}

void QT_FASTCALL comp_func_DestinationIn_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            dest[0] = BYTE_MUL(dest[0], qAlpha(src[0]));
        } else {
            int cia = 255 - const_alpha;
            uint a = BYTE_MUL(qAlpha(src[0]), const_alpha) + cia;
            dest[0] = BYTE_MUL(dest[0], a);
        }
    length--;
    src++;
    dest++;
    }
    comp_func_DestinationIn_dsp_asm_x2(dest, src, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_DestinationOut_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(~color);
    if (const_alpha != 255) {
        a = BYTE_MUL(a, const_alpha) + 255 - const_alpha;
    }
    if (length%2 > 0) {
        dest[0] = BYTE_MUL(dest[0], a);
        length--;
        dest++;
    }
    comp_func_solid_DestinationIn_dsp_asm_x2(dest, length, a);
}

void QT_FASTCALL comp_func_DestinationOut_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            dest[0] = BYTE_MUL(dest[0], qAlpha(~src[0]));
        } else {
            int cia = 255 - const_alpha;
            uint sia = BYTE_MUL(qAlpha(~src[0]), const_alpha) + cia;
            dest[0] = BYTE_MUL(dest[0], sia);
        }
        length--;
        dest++;
        src++;
    }
    comp_func_DestinationOut_dsp_asm_x2(dest, src, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_SourceAtop_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
    }
    uint sia = qAlpha(~color);
    if (length%2 > 0) {
        dest[0] = INTERPOLATE_PIXEL_255(color, qAlpha(dest[0]), dest[0], sia);
        length--;
        dest++;
    }
    comp_func_solid_SourceAtop_dsp_asm_x2(dest, length, color, sia);
}

void QT_FASTCALL comp_func_SourceAtop_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            uint s = src[0];
            uint d = dest[0];
            dest[0] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        } else {
            uint s = BYTE_MUL(src[0], const_alpha);
            uint d = dest[0];
            dest[0] = INTERPOLATE_PIXEL_255(s, qAlpha(d), d, qAlpha(~s));
        }
        length--;
        dest++;
        src++;
    }
    comp_func_SourceAtop_dsp_asm_x2(dest, src, length, const_alpha);
}


void QT_FASTCALL comp_func_solid_DestinationAtop_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    uint a = qAlpha(color);
    if (const_alpha != 255) {
        color = BYTE_MUL(color, const_alpha);
        a = qAlpha(color) + 255 - const_alpha;
    }
    if (length%2 > 0) {
        uint d = dest[0];
        dest[0] = INTERPOLATE_PIXEL_255(d, a, color, qAlpha(~d));
        length--;
        dest++;
    }
    comp_func_solid_DestinationAtop_dsp_asm_x2(dest, length, color, a);
}

void QT_FASTCALL comp_func_DestinationAtop_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            uint s = src[0];
            uint d = dest[0];
            dest[0] = INTERPOLATE_PIXEL_255(d, qAlpha(s), s, qAlpha(~d));
        } else {
            int cia = 255 - const_alpha;
            uint s = BYTE_MUL(src[0], const_alpha);
            uint d = dest[0];
            uint a = qAlpha(s) + cia;
            dest[0] = INTERPOLATE_PIXEL_255(d, a, s, qAlpha(~d));
        }
        length--;
        dest++;
        src++;
    }
    comp_func_DestinationAtop_dsp_asm_x2(dest, src, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_XOR_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    if (const_alpha != 255)
        color = BYTE_MUL(color, const_alpha);
    uint sia = qAlpha(~color);

     if (length%2 > 0) {
        uint d = dest[0];
        dest[0] = INTERPOLATE_PIXEL_255(color, qAlpha(~d), d, sia);
        length--;
        dest++;
    }
    comp_func_solid_XOR_dsp_asm_x2(dest, length, color, sia);
}

void QT_FASTCALL comp_func_XOR_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            uint d = dest[0];
            uint s = src[0];
            dest[0] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        } else {
            uint d = dest[0];
            uint s = BYTE_MUL(src[0], const_alpha);
            dest[0] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, qAlpha(~s));
        }
        length--;
        dest++;
        src++;
    }
    comp_func_XOR_dsp_asm_x2(dest, src, length, const_alpha);
}

void QT_FASTCALL comp_func_solid_SourceOut_mips_dsp(uint *dest, int length, uint color, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            dest[0] = BYTE_MUL(color, qAlpha(~dest[0]));
        } else {
            uint tmp_color = BYTE_MUL(color, const_alpha);
            int cia = 255 - const_alpha;
            uint d = dest[0];
            dest[0] = INTERPOLATE_PIXEL_255(tmp_color, qAlpha(~d), d, cia);
        }
        length--;
        dest++;
    }
    comp_func_solid_SourceOut_dsp_asm_x2(dest, length, color, const_alpha);
}

void QT_FASTCALL comp_func_SourceOut_mips_dsp(uint *dest, const uint *src, int length, uint const_alpha)
{
    if (length%2 > 0) {
        if (const_alpha == 255) {
            dest[0] = BYTE_MUL(src[0], qAlpha(~dest[0]));
        } else {
            int cia = 255 - const_alpha;
            uint s = BYTE_MUL(src[0], const_alpha);
            uint d = dest[0];
            dest[0] = INTERPOLATE_PIXEL_255(s, qAlpha(~d), d, cia);
        }
        length--;
        dest++;
        src++;
    }
    comp_func_SourceOut_dsp_asm_x2(dest, src, length, const_alpha);
}

const uint * QT_FASTCALL qt_fetchUntransformed_888_mips_dsp (uint *buffer, const Operator *, const QSpanData *data,
                                             int y, int x, int length)
{
    uchar *line = (uchar *)data->texture.scanLine(y) + x;
    fetchUntransformed_888_asm_mips_dsp(buffer, line, length);
    return buffer;
}

const uint * QT_FASTCALL qt_fetchUntransformed_444_mips_dsp (uint *buffer, const Operator *, const QSpanData *data,
                                             int y, int x, int length)
{
    uchar *line = (uchar *)data->texture.scanLine(y) + x;
    fetchUntransformed_444_asm_mips_dsp(buffer, line, length);
    return buffer;
}

const uint * QT_FASTCALL qt_fetchUntransformed_argb8565_premultiplied_mips_dsp (uint *buffer, const Operator *, const QSpanData *data,
                                             int y, int x, int length)
{
    uchar *line = (uchar *)data->texture.scanLine(y) + x;
    fetchUntransformed_argb8565_premultiplied_asm_mips_dsp(buffer, line, length);
    return buffer;
}

QT_END_NAMESPACE
