/*
    Copyright (C) 2008 Eric Seidel <eric@webkit.org>
    Copyright (C) 2004, 2005, 2007 Nikolas Zimmermann <zimmermann@kde.org>
                  2004, 2005, 2007, 2010 Rob Buis <buis@kde.org>
    Copyright (C) 2005, 2006 Apple Computer, Inc.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#if ENABLE(SVG)
#include "CSSInheritedValue.h"
#include "CSSInitialValue.h"
#include "CSSParser.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSQuirkPrimitiveValue.h"
#include "CSSValueKeywords.h"
#include "CSSValueList.h"
#include "RenderTheme.h"
#include "SVGPaint.h"

using namespace std;

namespace WebCore {

bool CSSParser::parseSVGValue(int propId, bool important)
{
    CSSParserValue* value = m_valueList->current();
    if (!value)
        return false;

    int id = value->id;

    bool valid_primitive = false;
    RefPtr<CSSValue> parsedValue;

    switch (propId) {
    /* The comment to the right defines all valid value of these
     * properties as defined in SVG 1.1, Appendix N. Property index */
    case CSSPropertyAlignmentBaseline:
    // auto | baseline | before-edge | text-before-edge | middle |
    // central | after-edge | text-after-edge | ideographic | alphabetic |
    // hanging | mathematical | inherit
        if (id == CSSValueAuto || id == CSSValueBaseline || id == CSSValueMiddle ||
          (id >= CSSValueBeforeEdge && id <= CSSValueMathematical))
            valid_primitive = true;
        break;

    case CSSPropertyBaselineShift:
    // baseline | super | sub | <percentage> | <length> | inherit
        if (id == CSSValueBaseline || id == CSSValueSub ||
           id >= CSSValueSuper)
            valid_primitive = true;
        else
            valid_primitive = validUnit(value, FLength|FPercent, false);
        break;

    case CSSPropertyDominantBaseline:
    // auto | use-script | no-change | reset-size | ideographic |
    // alphabetic | hanging | mathematical | central | middle |
    // text-after-edge | text-before-edge | inherit
        if (id == CSSValueAuto || id == CSSValueMiddle ||
          (id >= CSSValueUseScript && id <= CSSValueResetSize) ||
          (id >= CSSValueCentral && id <= CSSValueMathematical))
            valid_primitive = true;
        break;

    case CSSPropertyEnableBackground:
    // accumulate | new [x] [y] [width] [height] | inherit
        if (id == CSSValueAccumulate) // TODO : new
            valid_primitive = true;
        break;

    case CSSPropertyMarkerStart:
    case CSSPropertyMarkerMid:
    case CSSPropertyMarkerEnd:
    case CSSPropertyMask:
        if (id == CSSValueNone)
            valid_primitive = true;
        else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = CSSPrimitiveValue::create(value->string, CSSPrimitiveValue::CSS_URI);
            if (parsedValue)
                m_valueList->next();
        }
        break;

    case CSSPropertyClipRule:            // nonzero | evenodd | inherit
    case CSSPropertyFillRule:
        if (id == CSSValueNonzero || id == CSSValueEvenodd)
            valid_primitive = true;
        break;

    case CSSPropertyStrokeMiterlimit:   // <miterlimit> | inherit
        valid_primitive = validUnit(value, FNumber|FNonNeg, false);
        break;

    case CSSPropertyStrokeLinejoin:   // miter | round | bevel | inherit
        if (id == CSSValueMiter || id == CSSValueRound || id == CSSValueBevel)
            valid_primitive = true;
        break;

    case CSSPropertyStrokeLinecap:    // butt | round | square | inherit
        if (id == CSSValueButt || id == CSSValueRound || id == CSSValueSquare)
            valid_primitive = true;
        break;

    case CSSPropertyStrokeOpacity:   // <opacity-value> | inherit
    case CSSPropertyFillOpacity:
    case CSSPropertyStopOpacity:
    case CSSPropertyFloodOpacity:
        valid_primitive = (!id && validUnit(value, FNumber|FPercent, false));
        break;

    case CSSPropertyShapeRendering:
    // auto | optimizeSpeed | crispEdges | geometricPrecision | inherit
        if (id == CSSValueAuto || id == CSSValueOptimizespeed ||
            id == CSSValueCrispedges || id == CSSValueGeometricprecision)
            valid_primitive = true;
        break;

    case CSSPropertyImageRendering:  // auto | optimizeSpeed |
    case CSSPropertyColorRendering:  // optimizeQuality | inherit
        if (id == CSSValueAuto || id == CSSValueOptimizespeed ||
            id == CSSValueOptimizequality)
            valid_primitive = true;
        break;

    case CSSPropertyColorProfile: // auto | sRGB | <name> | <uri> inherit
        if (id == CSSValueAuto || id == CSSValueSrgb)
            valid_primitive = true;
        break;

    case CSSPropertyColorInterpolation:   // auto | sRGB | linearRGB | inherit
    case CSSPropertyColorInterpolationFilters:  
        if (id == CSSValueAuto || id == CSSValueSrgb || id == CSSValueLinearrgb)
            valid_primitive = true;
        break;

    /* Start of supported CSS properties with validation. This is needed for parseShortHand to work
     * correctly and allows optimization in applyRule(..)
     */

    case CSSPropertyTextAnchor:    // start | middle | end | inherit
        if (id == CSSValueStart || id == CSSValueMiddle || id == CSSValueEnd)
            valid_primitive = true;
        break;

    case CSSPropertyGlyphOrientationVertical: // auto | <angle> | inherit
        if (id == CSSValueAuto) {
            valid_primitive = true;
            break;
        }
    /* fallthrough intentional */
    case CSSPropertyGlyphOrientationHorizontal: // <angle> (restricted to _deg_ per SVG 1.1 spec) | inherit
        if (value->unit == CSSPrimitiveValue::CSS_DEG || value->unit == CSSPrimitiveValue::CSS_NUMBER) {
            parsedValue = CSSPrimitiveValue::create(value->fValue, CSSPrimitiveValue::CSS_DEG);

            if (parsedValue)
                m_valueList->next();
        }
        break;

    case CSSPropertyFill:                 // <paint> | inherit
    case CSSPropertyStroke:               // <paint> | inherit
        {
            if (id == CSSValueNone)
                parsedValue = SVGPaint::createNone();
            else if (id == CSSValueCurrentcolor)
                parsedValue = SVGPaint::createCurrentColor();
            else if ((id >= CSSValueActiveborder && id <= CSSValueWindowtext) || id == CSSValueMenu)
                parsedValue = SVGPaint::createColor(RenderTheme::defaultTheme()->systemColor(id));
            else if (value->unit == CSSPrimitiveValue::CSS_URI) {
                RGBA32 c = Color::transparent;
                if (m_valueList->next() && parseColorFromValue(m_valueList->current(), c)) {
                    parsedValue = SVGPaint::createURIAndColor(value->string, c);
                } else
                    parsedValue = SVGPaint::createURI(value->string);
            } else
                parsedValue = parseSVGPaint();

            if (parsedValue)
                m_valueList->next();
        }
        break;

    case CSSPropertyColor:                // <color> | inherit
        if ((id >= CSSValueAqua && id <= CSSValueWindowtext) ||
           (id >= CSSValueAliceblue && id <= CSSValueYellowgreen))
            parsedValue = SVGColor::createFromString(value->string);
        else
            parsedValue = parseSVGColor();

        if (parsedValue)
            m_valueList->next();
        break;

    case CSSPropertyStopColor: // TODO : icccolor
    case CSSPropertyFloodColor:
    case CSSPropertyLightingColor:
        if ((id >= CSSValueAqua && id <= CSSValueWindowtext) ||
           (id >= CSSValueAliceblue && id <= CSSValueYellowgreen))
            parsedValue = SVGColor::createFromString(value->string);
        else if (id == CSSValueCurrentcolor)
            parsedValue = SVGColor::createCurrentColor();
        else // TODO : svgcolor (iccColor)
            parsedValue = parseSVGColor();

        if (parsedValue)
            m_valueList->next();

        break;
        
    case CSSPropertyVectorEffect: // none | non-scaling-stroke | inherit
        if (id == CSSValueNone || id == CSSValueNonScalingStroke)
            valid_primitive = true;
        break;

    case CSSPropertyWritingMode:
    // lr-tb | rl_tb | tb-rl | lr | rl | tb | inherit
        if (id == CSSValueLrTb || id == CSSValueRlTb || id == CSSValueTbRl || id == CSSValueLr || id == CSSValueRl || id == CSSValueTb)
            valid_primitive = true;
        break;

    case CSSPropertyStrokeWidth:         // <length> | inherit
    case CSSPropertyStrokeDashoffset:
        valid_primitive = validUnit(value, FLength | FPercent, false);
        break;
    case CSSPropertyStrokeDasharray:     // none | <dasharray> | inherit
        if (id == CSSValueNone)
            valid_primitive = true;
        else
            parsedValue = parseSVGStrokeDasharray();

        break;

    case CSSPropertyKerning:              // auto | normal | <length> | inherit
        if (id == CSSValueAuto || id == CSSValueNormal)
            valid_primitive = true;
        else
            valid_primitive = validUnit(value, FLength, false);
        break;

    case CSSPropertyClipPath:    // <uri> | none | inherit
    case CSSPropertyFilter:
        if (id == CSSValueNone)
            valid_primitive = true;
        else if (value->unit == CSSPrimitiveValue::CSS_URI) {
            parsedValue = CSSPrimitiveValue::create(value->string, (CSSPrimitiveValue::UnitTypes) value->unit);
            if (parsedValue)
                m_valueList->next();
        }
        break;
    case CSSPropertyWebkitSvgShadow:
        if (id == CSSValueNone)
            valid_primitive = true;
        else
            return parseShadow(propId, important);

    /* shorthand properties */
    case CSSPropertyMarker:
    {
        ShorthandScope scope(this, propId);
        m_implicitShorthand = true;
        if (!parseValue(CSSPropertyMarkerStart, important))
            return false;
        if (m_valueList->current()) {
            rollbackLastProperties(1);
            return false;
        }
        CSSValue* value = m_parsedProperties[m_numParsedProperties - 1]->value();
        addProperty(CSSPropertyMarkerMid, value, important);
        addProperty(CSSPropertyMarkerEnd, value, important);
        m_implicitShorthand = false;
        return true;
    }
    default:
        // If you crash here, it's because you added a css property and are not handling it
        // in either this switch statement or the one in CSSParser::parseValue
        ASSERT_WITH_MESSAGE(0, "unimplemented propertyID: %d", propId);
        return false;
    }

    if (valid_primitive) {
        if (id != 0)
            parsedValue = CSSPrimitiveValue::createIdentifier(id);
        else if (value->unit == CSSPrimitiveValue::CSS_STRING)
            parsedValue = CSSPrimitiveValue::create(value->string, (CSSPrimitiveValue::UnitTypes) value->unit);
        else if (value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
            parsedValue = CSSPrimitiveValue::create(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit);
        else if (value->unit >= CSSParserValue::Q_EMS)
            parsedValue = CSSQuirkPrimitiveValue::create(value->fValue, CSSPrimitiveValue::CSS_EMS);
        m_valueList->next();
    }
    if (!parsedValue || (m_valueList->current() && !inShorthand()))
        return false;

    addProperty(propId, parsedValue.release(), important);
    return true;
}

PassRefPtr<CSSValue> CSSParser::parseSVGStrokeDasharray()
{
    RefPtr<CSSValueList> ret = CSSValueList::createCommaSeparated();
    CSSParserValue* value = m_valueList->current();
    bool valid_primitive = true;
    while (value) {
        valid_primitive = validUnit(value, FLength | FPercent |FNonNeg, false);
        if (!valid_primitive)
            break;
        if (value->id != 0)
            ret->append(CSSPrimitiveValue::createIdentifier(value->id));
        else if (value->unit >= CSSPrimitiveValue::CSS_NUMBER && value->unit <= CSSPrimitiveValue::CSS_KHZ)
            ret->append(CSSPrimitiveValue::create(value->fValue, (CSSPrimitiveValue::UnitTypes) value->unit));
        value = m_valueList->next();
        if (value && value->unit == CSSParserValue::Operator && value->iValue == ',')
            value = m_valueList->next();
    }
    if (!valid_primitive)
        return 0;
    return ret.release();
}

PassRefPtr<CSSValue> CSSParser::parseSVGPaint()
{
    RGBA32 c = Color::transparent;
    if (!parseColorFromValue(m_valueList->current(), c))
        return SVGPaint::createUnknown();
    return SVGPaint::createColor(Color(c));
}

PassRefPtr<CSSValue> CSSParser::parseSVGColor()
{
    RGBA32 c = Color::transparent;
    if (!parseColorFromValue(m_valueList->current(), c))
        return 0;
    return SVGColor::createFromColor(Color(c));
}

}

#endif // ENABLE(SVG)
