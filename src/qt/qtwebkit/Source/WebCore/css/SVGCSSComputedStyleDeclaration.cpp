/*
    Copyright (C) 2007 Eric Seidel <eric@webkit.org>
    Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>

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
#include "CSSComputedStyleDeclaration.h"

#include "CSSPrimitiveValueMappings.h"
#include "CSSPropertyNames.h"
#include "Document.h"
#include "RenderStyle.h"
#include "SVGPaint.h"

namespace WebCore {

static PassRefPtr<CSSPrimitiveValue> glyphOrientationToCSSPrimitiveValue(EGlyphOrientation orientation)
{
    switch (orientation) {
        case GO_0DEG:
            return CSSPrimitiveValue::create(0.0f, CSSPrimitiveValue::CSS_DEG);
        case GO_90DEG:
            return CSSPrimitiveValue::create(90.0f, CSSPrimitiveValue::CSS_DEG);
        case GO_180DEG:
            return CSSPrimitiveValue::create(180.0f, CSSPrimitiveValue::CSS_DEG);
        case GO_270DEG:
            return CSSPrimitiveValue::create(270.0f, CSSPrimitiveValue::CSS_DEG);
        default:
            return 0;
    }
}

static PassRefPtr<CSSValue> strokeDashArrayToCSSValueList(const Vector<SVGLength>& dashes)
{
    if (dashes.isEmpty())
        return CSSPrimitiveValue::createIdentifier(CSSValueNone);

    RefPtr<CSSValueList> list = CSSValueList::createCommaSeparated();
    const Vector<SVGLength>::const_iterator end = dashes.end();
    for (Vector<SVGLength>::const_iterator it = dashes.begin(); it != end; ++it)
        list->append(SVGLength::toCSSPrimitiveValue(*it));

    return list.release();
}

PassRefPtr<SVGPaint> ComputedStyleExtractor::adjustSVGPaintForCurrentColor(PassRefPtr<SVGPaint> newPaint, RenderStyle* style) const
{
    RefPtr<SVGPaint> paint = newPaint;
    if (paint->paintType() == SVGPaint::SVG_PAINTTYPE_CURRENTCOLOR || paint->paintType() == SVGPaint::SVG_PAINTTYPE_URI_CURRENTCOLOR)
        paint->setColor(style->color());
    return paint.release();
}

PassRefPtr<CSSValue> ComputedStyleExtractor::svgPropertyValue(CSSPropertyID propertyID, EUpdateLayout updateLayout) const
{
    Node* node = m_node.get();
    if (!node)
        return 0;

    // Make sure our layout is up to date before we allow a query on these attributes.
    if (updateLayout)
        node->document()->updateLayout();

    RenderStyle* style = node->computedStyle();
    if (!style)
        return 0;

    const SVGRenderStyle* svgStyle = style->svgStyle();
    if (!svgStyle)
        return 0;

    switch (propertyID) {
        case CSSPropertyClipRule:
            return CSSPrimitiveValue::create(svgStyle->clipRule());
        case CSSPropertyFloodOpacity:
            return CSSPrimitiveValue::create(svgStyle->floodOpacity(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyStopOpacity:
            return CSSPrimitiveValue::create(svgStyle->stopOpacity(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyColorInterpolation:
            return CSSPrimitiveValue::create(svgStyle->colorInterpolation());
        case CSSPropertyColorInterpolationFilters:
            return CSSPrimitiveValue::create(svgStyle->colorInterpolationFilters());
        case CSSPropertyFillOpacity:
            return CSSPrimitiveValue::create(svgStyle->fillOpacity(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyFillRule:
            return CSSPrimitiveValue::create(svgStyle->fillRule());
        case CSSPropertyColorRendering:
            return CSSPrimitiveValue::create(svgStyle->colorRendering());
        case CSSPropertyShapeRendering:
            return CSSPrimitiveValue::create(svgStyle->shapeRendering());
        case CSSPropertyStrokeLinecap:
            return CSSPrimitiveValue::create(svgStyle->capStyle());
        case CSSPropertyStrokeLinejoin:
            return CSSPrimitiveValue::create(svgStyle->joinStyle());
        case CSSPropertyStrokeMiterlimit:
            return CSSPrimitiveValue::create(svgStyle->strokeMiterLimit(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyStrokeOpacity:
            return CSSPrimitiveValue::create(svgStyle->strokeOpacity(), CSSPrimitiveValue::CSS_NUMBER);
        case CSSPropertyAlignmentBaseline:
            return CSSPrimitiveValue::create(svgStyle->alignmentBaseline());
        case CSSPropertyDominantBaseline:
            return CSSPrimitiveValue::create(svgStyle->dominantBaseline());
        case CSSPropertyTextAnchor:
            return CSSPrimitiveValue::create(svgStyle->textAnchor());
        case CSSPropertyWritingMode:
            return CSSPrimitiveValue::create(svgStyle->writingMode());
        case CSSPropertyClipPath:
            if (!svgStyle->clipperResource().isEmpty())
                return CSSPrimitiveValue::create(svgStyle->clipperResource(), CSSPrimitiveValue::CSS_URI);
            return CSSPrimitiveValue::createIdentifier(CSSValueNone);
        case CSSPropertyMask:
            if (!svgStyle->maskerResource().isEmpty())
                return CSSPrimitiveValue::create(svgStyle->maskerResource(), CSSPrimitiveValue::CSS_URI);
            return CSSPrimitiveValue::createIdentifier(CSSValueNone);
        case CSSPropertyFilter:
            if (!svgStyle->filterResource().isEmpty())
                return CSSPrimitiveValue::create(svgStyle->filterResource(), CSSPrimitiveValue::CSS_URI);
            return CSSPrimitiveValue::createIdentifier(CSSValueNone);
        case CSSPropertyFloodColor:
            return currentColorOrValidColor(style, svgStyle->floodColor());
        case CSSPropertyLightingColor:
            return currentColorOrValidColor(style, svgStyle->lightingColor());
        case CSSPropertyStopColor:
            return currentColorOrValidColor(style, svgStyle->stopColor());
        case CSSPropertyFill:
            return adjustSVGPaintForCurrentColor(SVGPaint::create(svgStyle->fillPaintType(), svgStyle->fillPaintUri(), svgStyle->fillPaintColor()), style);
        case CSSPropertyKerning:
            return SVGLength::toCSSPrimitiveValue(svgStyle->kerning());
        case CSSPropertyMarkerEnd:
            if (!svgStyle->markerEndResource().isEmpty())
                return CSSPrimitiveValue::create(svgStyle->markerEndResource(), CSSPrimitiveValue::CSS_URI);
            return CSSPrimitiveValue::createIdentifier(CSSValueNone);
        case CSSPropertyMarkerMid:
            if (!svgStyle->markerMidResource().isEmpty())
                return CSSPrimitiveValue::create(svgStyle->markerMidResource(), CSSPrimitiveValue::CSS_URI);
            return CSSPrimitiveValue::createIdentifier(CSSValueNone);
        case CSSPropertyMarkerStart:
            if (!svgStyle->markerStartResource().isEmpty())
                return CSSPrimitiveValue::create(svgStyle->markerStartResource(), CSSPrimitiveValue::CSS_URI);
            return CSSPrimitiveValue::createIdentifier(CSSValueNone);
        case CSSPropertyStroke:
            return adjustSVGPaintForCurrentColor(SVGPaint::create(svgStyle->strokePaintType(), svgStyle->strokePaintUri(), svgStyle->strokePaintColor()), style);
        case CSSPropertyStrokeDasharray:
            return strokeDashArrayToCSSValueList(svgStyle->strokeDashArray());
        case CSSPropertyStrokeDashoffset:
            return SVGLength::toCSSPrimitiveValue(svgStyle->strokeDashOffset());
        case CSSPropertyStrokeWidth:
            return SVGLength::toCSSPrimitiveValue(svgStyle->strokeWidth());
        case CSSPropertyBaselineShift: {
            switch (svgStyle->baselineShift()) {
                case BS_BASELINE:
                    return CSSPrimitiveValue::createIdentifier(CSSValueBaseline);
                case BS_SUPER:
                    return CSSPrimitiveValue::createIdentifier(CSSValueSuper);
                case BS_SUB:
                    return CSSPrimitiveValue::createIdentifier(CSSValueSub);
                case BS_LENGTH:
                    return SVGLength::toCSSPrimitiveValue(svgStyle->baselineShiftValue());
            }
            ASSERT_NOT_REACHED();
            return 0;
        }
        case CSSPropertyBufferedRendering:
            return CSSPrimitiveValue::create(svgStyle->bufferedRendering());
        case CSSPropertyGlyphOrientationHorizontal:
            return glyphOrientationToCSSPrimitiveValue(svgStyle->glyphOrientationHorizontal());
        case CSSPropertyGlyphOrientationVertical: {
            if (RefPtr<CSSPrimitiveValue> value = glyphOrientationToCSSPrimitiveValue(svgStyle->glyphOrientationVertical()))
                return value.release();

            if (svgStyle->glyphOrientationVertical() == GO_AUTO)
                return CSSPrimitiveValue::createIdentifier(CSSValueAuto);

            return 0;
        }
        case CSSPropertyWebkitSvgShadow:
            return valueForShadow(svgStyle->shadow(), propertyID, style);
        case CSSPropertyVectorEffect:
            return CSSPrimitiveValue::create(svgStyle->vectorEffect());
        case CSSPropertyMaskType:
            return CSSPrimitiveValue::create(svgStyle->maskType());
        case CSSPropertyMarker:
        case CSSPropertyEnableBackground:
        case CSSPropertyColorProfile:
            // the above properties are not yet implemented in the engine
            break;
    default:
        // If you crash here, it's because you added a css property and are not handling it
        // in either this switch statement or the one in CSSComputedStyleDelcaration::getPropertyCSSValue
        ASSERT_WITH_MESSAGE(0, "unimplemented propertyID: %d", propertyID);
    }
    LOG_ERROR("unimplemented propertyID: %d", propertyID);
    return 0;
}

}

#endif // ENABLE(SVG)

// vim:ts=4:noet
