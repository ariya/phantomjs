/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(SVG)
#include "JSSVGPathSeg.h"

#include "JSDOMBinding.h"
#include "JSSVGPathSegArcAbs.h"
#include "JSSVGPathSegArcRel.h"
#include "JSSVGPathSegClosePath.h"
#include "JSSVGPathSegCurvetoCubicAbs.h"
#include "JSSVGPathSegCurvetoCubicRel.h"
#include "JSSVGPathSegCurvetoCubicSmoothAbs.h"
#include "JSSVGPathSegCurvetoCubicSmoothRel.h"
#include "JSSVGPathSegCurvetoQuadraticAbs.h"
#include "JSSVGPathSegCurvetoQuadraticRel.h"
#include "JSSVGPathSegCurvetoQuadraticSmoothAbs.h"
#include "JSSVGPathSegCurvetoQuadraticSmoothRel.h"
#include "JSSVGPathSegLinetoAbs.h"
#include "JSSVGPathSegLinetoRel.h"
#include "JSSVGPathSegLinetoHorizontalAbs.h"
#include "JSSVGPathSegLinetoHorizontalRel.h"
#include "JSSVGPathSegLinetoVerticalAbs.h"
#include "JSSVGPathSegLinetoVerticalRel.h"
#include "JSSVGPathSegMovetoAbs.h"
#include "JSSVGPathSegMovetoRel.h"
#include "SVGPathSeg.h"
#include "SVGPathSegArc.h"
#include "SVGPathSegClosePath.h"
#include "SVGPathSegCurvetoCubic.h"
#include "SVGPathSegCurvetoCubicSmooth.h"
#include "SVGPathSegCurvetoQuadratic.h"
#include "SVGPathSegCurvetoQuadraticSmoothAbs.h"
#include "SVGPathSegCurvetoQuadraticSmoothRel.h"
#include "SVGPathSegLinetoAbs.h"
#include "SVGPathSegLinetoHorizontal.h"
#include "SVGPathSegLinetoRel.h"
#include "SVGPathSegLinetoVertical.h"
#include "SVGPathSegMovetoAbs.h"
#include "SVGPathSegMovetoRel.h"

using namespace JSC;

namespace WebCore {

JSValue toJS(ExecState* exec, JSDOMGlobalObject* globalObject, SVGPathSeg* object)
{
    if (!object)
        return jsNull();

    if (JSDOMWrapper* wrapper = getCachedWrapper(currentWorld(exec), object))
        return wrapper;

    switch (object->pathSegType()) {
    case SVGPathSeg::PATHSEG_CLOSEPATH:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegClosePath, object);
    case SVGPathSeg::PATHSEG_MOVETO_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegMovetoAbs, object);
    case SVGPathSeg::PATHSEG_MOVETO_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegMovetoRel, object);
    case SVGPathSeg::PATHSEG_LINETO_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegLinetoAbs, object);
    case SVGPathSeg::PATHSEG_LINETO_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegLinetoRel, object);
    case SVGPathSeg::PATHSEG_CURVETO_CUBIC_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoCubicAbs, object);
    case SVGPathSeg::PATHSEG_CURVETO_CUBIC_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoCubicRel, object);
    case SVGPathSeg::PATHSEG_CURVETO_QUADRATIC_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoQuadraticAbs, object);
    case SVGPathSeg::PATHSEG_CURVETO_QUADRATIC_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoQuadraticRel, object);
    case SVGPathSeg::PATHSEG_ARC_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegArcAbs, object);
    case SVGPathSeg::PATHSEG_ARC_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegArcRel, object);
    case SVGPathSeg::PATHSEG_LINETO_HORIZONTAL_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegLinetoHorizontalAbs, object);
    case SVGPathSeg::PATHSEG_LINETO_HORIZONTAL_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegLinetoHorizontalRel, object);
    case SVGPathSeg::PATHSEG_LINETO_VERTICAL_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegLinetoVerticalAbs, object);
    case SVGPathSeg::PATHSEG_LINETO_VERTICAL_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegLinetoVerticalRel, object);
    case SVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoCubicSmoothAbs, object);
    case SVGPathSeg::PATHSEG_CURVETO_CUBIC_SMOOTH_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoCubicSmoothRel, object);
    case SVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_ABS:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoQuadraticSmoothAbs, object);
    case SVGPathSeg::PATHSEG_CURVETO_QUADRATIC_SMOOTH_REL:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSegCurvetoQuadraticSmoothRel, object);
    case SVGPathSeg::PATHSEG_UNKNOWN:
    default:
        return CREATE_DOM_WRAPPER(exec, globalObject, SVGPathSeg, object);
    }
}

}

#endif // ENABLE(SVG)
