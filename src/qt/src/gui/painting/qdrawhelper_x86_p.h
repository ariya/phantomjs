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

#ifndef QDRAWHELPER_X86_P_H
#define QDRAWHELPER_X86_P_H

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

#ifdef QT_HAVE_MMX
extern CompositionFunction qt_functionForMode_MMX[];
extern CompositionFunctionSolid qt_functionForModeSolid_MMX[];
void qt_blend_color_argb_mmx(int count, const QSpan *spans, void *userData);
#endif

#ifdef QT_HAVE_MMXEXT
void qt_memfill32_mmxext(quint32 *dest, quint32 value, int count);
void qt_bitmapblit16_mmxext(QRasterBuffer *rasterBuffer, int x, int y,
                            quint32 color, const uchar *src,
                            int width, int height, int stride);
#endif

#ifdef QT_HAVE_3DNOW
#if defined(QT_HAVE_MMX) || !defined(QT_HAVE_SSE)
extern CompositionFunction qt_functionForMode_MMX3DNOW[];
extern CompositionFunctionSolid qt_functionForModeSolid_MMX3DNOW[];

void qt_blend_color_argb_mmx3dnow(int count, const QSpan *spans,
                                  void *userData);
#endif // MMX

#ifdef QT_HAVE_SSE
extern CompositionFunction qt_functionForMode_SSE3DNOW[];
extern CompositionFunctionSolid qt_functionForModeSolid_SSE3DNOW[];

void qt_memfill32_sse3dnow(quint32 *dest, quint32 value, int count);
void qt_bitmapblit16_sse3dnow(QRasterBuffer *rasterBuffer, int x, int y,
                              quint32 color,
                              const uchar *src, int width, int height,
                              int stride);
void qt_blend_color_argb_sse3dnow(int count, const QSpan *spans,
                                  void *userData);
#endif // SSE
#endif // QT_HAVE_3DNOW

#ifdef QT_HAVE_SSE
void qt_memfill32_sse(quint32 *dest, quint32 value, int count);
void qt_bitmapblit16_sse(QRasterBuffer *rasterBuffer, int x, int y,
                         quint32 color,
                         const uchar *src, int width, int height, int stride);

void qt_blend_color_argb_sse(int count, const QSpan *spans, void *userData);

extern CompositionFunction qt_functionForMode_SSE[];
extern CompositionFunctionSolid qt_functionForModeSolid_SSE[];
#endif // QT_HAVE_SSE

#ifdef QT_HAVE_SSE2
void qt_memfill32_sse2(quint32 *dest, quint32 value, int count);
void qt_memfill16_sse2(quint16 *dest, quint16 value, int count);
void qt_bitmapblit32_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          quint32 color,
                          const uchar *src, int width, int height, int stride);
void qt_bitmapblit16_sse2(QRasterBuffer *rasterBuffer, int x, int y,
                          quint32 color,
                          const uchar *src, int width, int height, int stride);
void qt_blend_argb32_on_argb32_sse2(uchar *destPixels, int dbpl,
                                    const uchar *srcPixels, int sbpl,
                                    int w, int h,
                                    int const_alpha);
void qt_blend_rgb32_on_rgb32_sse2(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha);

extern CompositionFunction qt_functionForMode_onlySSE2[];
extern CompositionFunctionSolid qt_functionForModeSolid_onlySSE2[];
#endif // QT_HAVE_SSE2

#ifdef QT_HAVE_IWMMXT
void qt_blend_color_argb_iwmmxt(int count, const QSpan *spans, void *userData);

extern CompositionFunction qt_functionForMode_IWMMXT[];
extern CompositionFunctionSolid qt_functionForModeSolid_IWMMXT[];
#endif

static const int numCompositionFunctions = 33;

QT_END_NAMESPACE

#endif // QDRAWHELPER_X86_P_H
