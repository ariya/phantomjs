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

#ifdef QT_HAVE_IWMMXT

#include <mmintrin.h>
#if defined(Q_OS_WINCE)
#  include "qplatformdefs.h"
#endif
#if !defined(__IWMMXT__) && !defined(Q_OS_WINCE)
#  include <xmmintrin.h>
#elif defined(Q_OS_WINCE_STD) && defined(_X86_)
#  pragma warning(disable: 4391)
#  include <xmmintrin.h>
#endif

#include <private/qdrawhelper_sse_p.h>

QT_BEGIN_NAMESPACE

#ifndef _MM_SHUFFLE
#define _MM_SHUFFLE(fp3,fp2,fp1,fp0) \
 (((fp3) << 6) | ((fp2) << 4) | ((fp1) << 2) | (fp0))
#endif

struct QIWMMXTIntrinsics : public QMMXCommonIntrinsics
{
    static inline m64 alpha(m64 x) {
        return _mm_shuffle_pi16 (x, _MM_SHUFFLE(3, 3, 3, 3));
    }

    static inline m64 _load_alpha(uint x, const m64 &mmx_0x0000) {
        m64 t = _mm_unpacklo_pi8(_mm_cvtsi32_si64(x), mmx_0x0000);
        return _mm_shuffle_pi16(t, _MM_SHUFFLE(0, 0, 0, 0));
    }

    static inline void end() {
    }
};

CompositionFunctionSolid qt_functionForModeSolid_IWMMXT[numCompositionFunctions] = {
    comp_func_solid_SourceOver<QIWMMXTIntrinsics>,
    comp_func_solid_DestinationOver<QIWMMXTIntrinsics>,
    comp_func_solid_Clear<QIWMMXTIntrinsics>,
    comp_func_solid_Source<QIWMMXTIntrinsics>,
    0,
    comp_func_solid_SourceIn<QIWMMXTIntrinsics>,
    comp_func_solid_DestinationIn<QIWMMXTIntrinsics>,
    comp_func_solid_SourceOut<QIWMMXTIntrinsics>,
    comp_func_solid_DestinationOut<QIWMMXTIntrinsics>,
    comp_func_solid_SourceAtop<QIWMMXTIntrinsics>,
    comp_func_solid_DestinationAtop<QIWMMXTIntrinsics>,
    comp_func_solid_XOR<QIWMMXTIntrinsics>,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // svg 1.2 modes
    rasterop_solid_SourceOrDestination<QIWMMXTIntrinsics>,
    rasterop_solid_SourceAndDestination<QIWMMXTIntrinsics>,
    rasterop_solid_SourceXorDestination<QIWMMXTIntrinsics>,
    rasterop_solid_NotSourceAndNotDestination<QIWMMXTIntrinsics>,
    rasterop_solid_NotSourceOrNotDestination<QIWMMXTIntrinsics>,
    rasterop_solid_NotSourceXorDestination<QIWMMXTIntrinsics>,
    rasterop_solid_NotSource<QIWMMXTIntrinsics>,
    rasterop_solid_NotSourceAndDestination<QIWMMXTIntrinsics>,
    rasterop_solid_SourceAndNotDestination<QIWMMXTIntrinsics>
};

CompositionFunction qt_functionForMode_IWMMXT[] = {
    comp_func_SourceOver<QIWMMXTIntrinsics>,
    comp_func_DestinationOver<QIWMMXTIntrinsics>,
    comp_func_Clear<QIWMMXTIntrinsics>,
    comp_func_Source<QIWMMXTIntrinsics>,
    comp_func_Destination,
    comp_func_SourceIn<QIWMMXTIntrinsics>,
    comp_func_DestinationIn<QIWMMXTIntrinsics>,
    comp_func_SourceOut<QIWMMXTIntrinsics>,
    comp_func_DestinationOut<QIWMMXTIntrinsics>,
    comp_func_SourceAtop<QIWMMXTIntrinsics>,
    comp_func_DestinationAtop<QIWMMXTIntrinsics>,
    comp_func_XOR<QIWMMXTIntrinsics>,
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

void qt_blend_color_argb_iwmmxt(int count, const QSpan *spans, void *userData)
{
    qt_blend_color_argb_x86<QIWMMXTIntrinsics>(count, spans, userData,
                                               (CompositionFunctionSolid*)qt_functionForModeSolid_IWMMXT);
}

#endif // QT_HAVE_IWMMXT

QT_END_NAMESPACE
