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

#ifdef QT_HAVE_3DNOW

#include <private/qdrawhelper_mmx_p.h>
#include <mm3dnow.h>

QT_BEGIN_NAMESPACE

struct QMMX3DNOWIntrinsics : public QMMXCommonIntrinsics
{
    static inline void end() {
        _m_femms();
    }
};

CompositionFunctionSolid qt_functionForModeSolid_MMX3DNOW[numCompositionFunctions] = {
    comp_func_solid_SourceOver<QMMX3DNOWIntrinsics>,
    comp_func_solid_DestinationOver<QMMX3DNOWIntrinsics>,
    comp_func_solid_Clear<QMMX3DNOWIntrinsics>,
    comp_func_solid_Source<QMMX3DNOWIntrinsics>,
    0,
    comp_func_solid_SourceIn<QMMX3DNOWIntrinsics>,
    comp_func_solid_DestinationIn<QMMX3DNOWIntrinsics>,
    comp_func_solid_SourceOut<QMMX3DNOWIntrinsics>,
    comp_func_solid_DestinationOut<QMMX3DNOWIntrinsics>,
    comp_func_solid_SourceAtop<QMMX3DNOWIntrinsics>,
    comp_func_solid_DestinationAtop<QMMX3DNOWIntrinsics>,
    comp_func_solid_XOR<QMMX3DNOWIntrinsics>,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // svg 1.2 modes
    rasterop_solid_SourceOrDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_SourceAndDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_SourceXorDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_NotSourceAndNotDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_NotSourceOrNotDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_NotSourceXorDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_NotSource<QMMX3DNOWIntrinsics>,
    rasterop_solid_NotSourceAndDestination<QMMX3DNOWIntrinsics>,
    rasterop_solid_SourceAndNotDestination<QMMX3DNOWIntrinsics>
};

CompositionFunction qt_functionForMode_MMX3DNOW[numCompositionFunctions] = {
    comp_func_SourceOver<QMMX3DNOWIntrinsics>,
    comp_func_DestinationOver<QMMX3DNOWIntrinsics>,
    comp_func_Clear<QMMX3DNOWIntrinsics>,
    comp_func_Source<QMMX3DNOWIntrinsics>,
    comp_func_Destination,
    comp_func_SourceIn<QMMX3DNOWIntrinsics>,
    comp_func_DestinationIn<QMMX3DNOWIntrinsics>,
    comp_func_SourceOut<QMMX3DNOWIntrinsics>,
    comp_func_DestinationOut<QMMX3DNOWIntrinsics>,
    comp_func_SourceAtop<QMMX3DNOWIntrinsics>,
    comp_func_DestinationAtop<QMMX3DNOWIntrinsics>,
    comp_func_XOR<QMMX3DNOWIntrinsics>,
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

void qt_blend_color_argb_mmx3dnow(int count, const QSpan *spans, void *userData)
{
    qt_blend_color_argb_x86<QMMX3DNOWIntrinsics>(count, spans, userData,
                                                 (CompositionFunctionSolid*)qt_functionForModeSolid_MMX3DNOW);
}

QT_END_NAMESPACE

#endif // QT_HAVE_3DNOW

