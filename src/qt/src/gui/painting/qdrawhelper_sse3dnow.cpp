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

#include <private/qdrawhelper_x86_p.h>

#if defined(QT_HAVE_3DNOW) && defined(QT_HAVE_SSE)

#include <private/qdrawhelper_sse_p.h>
#include <mm3dnow.h>

QT_BEGIN_NAMESPACE

struct QSSE3DNOWIntrinsics : public QSSEIntrinsics
{
    static inline void end() {
        _m_femms();
    }
};

CompositionFunctionSolid qt_functionForModeSolid_SSE3DNOW[numCompositionFunctions] = {
    comp_func_solid_SourceOver<QSSE3DNOWIntrinsics>,
    comp_func_solid_DestinationOver<QSSE3DNOWIntrinsics>,
    comp_func_solid_Clear<QSSE3DNOWIntrinsics>,
    comp_func_solid_Source<QSSE3DNOWIntrinsics>,
    0,
    comp_func_solid_SourceIn<QSSE3DNOWIntrinsics>,
    comp_func_solid_DestinationIn<QSSE3DNOWIntrinsics>,
    comp_func_solid_SourceOut<QSSE3DNOWIntrinsics>,
    comp_func_solid_DestinationOut<QSSE3DNOWIntrinsics>,
    comp_func_solid_SourceAtop<QSSE3DNOWIntrinsics>,
    comp_func_solid_DestinationAtop<QSSE3DNOWIntrinsics>,
    comp_func_solid_XOR<QSSE3DNOWIntrinsics>,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // svg 1.2 modes
    rasterop_solid_SourceOrDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_SourceAndDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_SourceXorDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_NotSourceAndNotDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_NotSourceOrNotDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_NotSourceXorDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_NotSource<QSSE3DNOWIntrinsics>,
    rasterop_solid_NotSourceAndDestination<QSSE3DNOWIntrinsics>,
    rasterop_solid_SourceAndNotDestination<QSSE3DNOWIntrinsics>
};

CompositionFunction qt_functionForMode_SSE3DNOW[numCompositionFunctions] = {
    comp_func_SourceOver<QSSE3DNOWIntrinsics>,
    comp_func_DestinationOver<QSSE3DNOWIntrinsics>,
    comp_func_Clear<QSSE3DNOWIntrinsics>,
    comp_func_Source<QSSE3DNOWIntrinsics>,
    comp_func_Destination,
    comp_func_SourceIn<QSSE3DNOWIntrinsics>,
    comp_func_DestinationIn<QSSE3DNOWIntrinsics>,
    comp_func_SourceOut<QSSE3DNOWIntrinsics>,
    comp_func_DestinationOut<QSSE3DNOWIntrinsics>,
    comp_func_SourceAtop<QSSE3DNOWIntrinsics>,
    comp_func_DestinationAtop<QSSE3DNOWIntrinsics>,
    comp_func_XOR<QSSE3DNOWIntrinsics>,
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

void qt_blend_color_argb_sse3dnow(int count, const QSpan *spans, void *userData)
{
    qt_blend_color_argb_x86<QSSE3DNOWIntrinsics>(count, spans, userData,
                                                 (CompositionFunctionSolid*)qt_functionForModeSolid_SSE3DNOW);
}

void qt_memfill32_sse3dnow(quint32 *dest, quint32 value, int count)
{
    return qt_memfill32_sse_template<QSSE3DNOWIntrinsics>(dest, value, count);
}


void qt_bitmapblit16_sse3dnow(QRasterBuffer *rasterBuffer, int x, int y,
                              quint32 color,
                              const uchar *src,
                              int width, int height, int stride)
{
    return qt_bitmapblit16_sse_template<QSSE3DNOWIntrinsics>(rasterBuffer, x,y,
                                                             color, src, width,
                                                             height, stride);
}

QT_END_NAMESPACE

#endif // QT_HAVE_3DNOW && QT_HAVE_SSE
