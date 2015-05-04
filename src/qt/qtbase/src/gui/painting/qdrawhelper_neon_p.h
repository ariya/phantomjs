/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QDRAWHELPER_NEON_P_H
#define QDRAWHELPER_NEON_P_H

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

QT_BEGIN_NAMESPACE

#ifdef __ARM_NEON__

void qt_blend_argb32_on_argb32_neon(uchar *destPixels, int dbpl,
                                            const uchar *srcPixels, int sbpl,
                                            int w, int h,
                                            int const_alpha);

void qt_blend_rgb32_on_rgb32_neon(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha);

void qt_blend_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha);

void qt_blend_argb32_on_argb32_scanline_neon(uint *dest,
                                             const uint *src,
                                             int length,
                                             uint const_alpha);

void qt_blend_rgb16_on_argb32_neon(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha);

void qt_blend_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                  const uchar *srcPixels, int sbpl,
                                  int w, int h,
                                  int const_alpha);

void qt_alphamapblit_quint16_neon(QRasterBuffer *rasterBuffer,
                                  int x, int y, quint32 color,
                                  const uchar *bitmap,
                                  int mapWidth, int mapHeight, int mapStride,
                                  const QClipData *clip);

void qt_scale_image_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                         const uchar *srcPixels, int sbpl, int srch,
                                         const QRectF &targetRect,
                                         const QRectF &sourceRect,
                                         const QRect &clip,
                                         int const_alpha);

void qt_scale_image_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                        const uchar *srcPixels, int sbpl, int srch,
                                        const QRectF &targetRect,
                                        const QRectF &sourceRect,
                                        const QRect &clip,
                                        int const_alpha);

void qt_transform_image_argb32_on_rgb16_neon(uchar *destPixels, int dbpl,
                                             const uchar *srcPixels, int sbpl,
                                             const QRectF &targetRect,
                                             const QRectF &sourceRect,
                                             const QRect &clip,
                                             const QTransform &targetRectTransform,
                                             int const_alpha);

void qt_transform_image_rgb16_on_rgb16_neon(uchar *destPixels, int dbpl,
                                            const uchar *srcPixels, int sbpl,
                                            const QRectF &targetRect,
                                            const QRectF &sourceRect,
                                            const QRect &clip,
                                            const QTransform &targetRectTransform,
                                            int const_alpha);

void qt_memfill32_neon(quint32 *dest, quint32 value, int count);
void qt_memrotate90_16_neon(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl);
void qt_memrotate270_16_neon(const uchar *srcPixels, int w, int h, int sbpl, uchar *destPixels, int dbpl);

uint * QT_FASTCALL qt_destFetchRGB16_neon(uint *buffer,
                                          QRasterBuffer *rasterBuffer,
                                          int x, int y, int length);

void QT_FASTCALL qt_destStoreRGB16_neon(QRasterBuffer *rasterBuffer,
                                        int x, int y, const uint *buffer, int length);

void QT_FASTCALL comp_func_solid_SourceOver_neon(uint *destPixels, int length, uint color, uint const_alpha);
void QT_FASTCALL comp_func_Plus_neon(uint *dst, const uint *src, int length, uint const_alpha);

#endif // __ARM_NEON__

QT_END_NAMESPACE

#endif // QDRAWHELPER_NEON_P_H
