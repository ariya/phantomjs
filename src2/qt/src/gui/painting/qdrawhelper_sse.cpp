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

#include <private/qdrawhelper_p.h>

#ifdef QT_HAVE_SSE

#include <private/qdrawhelper_sse_p.h>

QT_BEGIN_NAMESPACE

CompositionFunctionSolid qt_functionForModeSolid_SSE[numCompositionFunctions] = {
    comp_func_solid_SourceOver<QSSEIntrinsics>,
    comp_func_solid_DestinationOver<QSSEIntrinsics>,
    comp_func_solid_Clear<QSSEIntrinsics>,
    comp_func_solid_Source<QSSEIntrinsics>,
    0,
    comp_func_solid_SourceIn<QSSEIntrinsics>,
    comp_func_solid_DestinationIn<QSSEIntrinsics>,
    comp_func_solid_SourceOut<QSSEIntrinsics>,
    comp_func_solid_DestinationOut<QSSEIntrinsics>,
    comp_func_solid_SourceAtop<QSSEIntrinsics>,
    comp_func_solid_DestinationAtop<QSSEIntrinsics>,
    comp_func_solid_XOR<QSSEIntrinsics>,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // svg 1.2 modes
    rasterop_solid_SourceOrDestination<QMMXIntrinsics>,
    rasterop_solid_SourceAndDestination<QMMXIntrinsics>,
    rasterop_solid_SourceXorDestination<QMMXIntrinsics>,
    rasterop_solid_NotSourceAndNotDestination<QMMXIntrinsics>,
    rasterop_solid_NotSourceOrNotDestination<QMMXIntrinsics>,
    rasterop_solid_NotSourceXorDestination<QMMXIntrinsics>,
    rasterop_solid_NotSource<QMMXIntrinsics>,
    rasterop_solid_NotSourceAndDestination<QMMXIntrinsics>,
    rasterop_solid_SourceAndNotDestination<QMMXIntrinsics>
};

CompositionFunction qt_functionForMode_SSE[numCompositionFunctions] = {
    comp_func_SourceOver<QSSEIntrinsics>,
    comp_func_DestinationOver<QSSEIntrinsics>,
    comp_func_Clear<QSSEIntrinsics>,
    comp_func_Source<QSSEIntrinsics>,
    comp_func_Destination,
    comp_func_SourceIn<QSSEIntrinsics>,
    comp_func_DestinationIn<QSSEIntrinsics>,
    comp_func_SourceOut<QSSEIntrinsics>,
    comp_func_DestinationOut<QSSEIntrinsics>,
    comp_func_SourceAtop<QSSEIntrinsics>,
    comp_func_DestinationAtop<QSSEIntrinsics>,
    comp_func_XOR<QSSEIntrinsics>,
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
    rasterop_SourceAndNotDestination
};

void qt_blend_color_argb_sse(int count, const QSpan *spans, void *userData)
{
    qt_blend_color_argb_x86<QSSEIntrinsics>(count, spans, userData,
                                            (CompositionFunctionSolid*)qt_functionForModeSolid_SSE);
}

void qt_memfill32_sse(quint32 *dest, quint32 value, int count)
{
    return qt_memfill32_sse_template<QSSEIntrinsics>(dest, value, count);
}

void qt_bitmapblit16_sse(QRasterBuffer *rasterBuffer, int x, int y,
                         quint32 color,
                         const uchar *src,
                         int width, int height, int stride)
{
    return qt_bitmapblit16_sse_template<QSSEIntrinsics>(rasterBuffer, x,y,
                                                        color, src, width,
                                                        height, stride);
}

void qt_blend_argb32_on_argb32_sse(uchar *destPixels, int dbpl,
                                   const uchar *srcPixels, int sbpl,
                                   int w, int h,
                                   int const_alpha)
{
    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;

    uint ca = const_alpha - 1;

    for (int y=0; y<h; ++y) {
        comp_func_SourceOver<QSSEIntrinsics>(dst, src, w, ca);
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        src = (const quint32 *)(((const uchar *) src) + sbpl);
    }
}

void qt_blend_rgb32_on_rgb32_sse(uchar *destPixels, int dbpl,
                                 const uchar *srcPixels, int sbpl,
                                 int w, int h,
                                 int const_alpha)
{
    const uint *src = (const uint *) srcPixels;
    uint *dst = (uint *) destPixels;

    uint ca = const_alpha - 1;

    for (int y=0; y<h; ++y) {
        comp_func_Source<QSSEIntrinsics>(dst, src, w, ca);
        dst = (quint32 *)(((uchar *) dst) + dbpl);
        src = (const quint32 *)(((const uchar *) src) + sbpl);
    }
}

QT_END_NAMESPACE

#endif // QT_HAVE_SSE
