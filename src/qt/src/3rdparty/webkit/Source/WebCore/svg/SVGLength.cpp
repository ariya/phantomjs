/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Apple Inc. All rights reserved.
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
#include "SVGLength.h"

#include "CSSHelper.h"
#include "FloatConversion.h"
#include "FrameView.h"
#include "RenderObject.h"
#include "RenderView.h"
#include "SVGNames.h"
#include "SVGParserUtilities.h"
#include "SVGSVGElement.h"

#include <wtf/MathExtras.h>
#include <wtf/text/StringConcatenate.h>

namespace WebCore {

// Helper functions
static inline unsigned int storeUnit(SVGLengthMode mode, SVGLengthType type)
{
    return (mode << 4) | type;
}

static inline SVGLengthMode extractMode(unsigned int unit)
{
    unsigned int mode = unit >> 4;    
    return static_cast<SVGLengthMode>(mode);
}

static inline SVGLengthType extractType(unsigned int unit)
{
    unsigned int mode = unit >> 4;
    unsigned int type = unit ^ (mode << 4);
    return static_cast<SVGLengthType>(type);
}

static inline String lengthTypeToString(SVGLengthType type)
{
    switch (type) {
    case LengthTypeUnknown:
    case LengthTypeNumber:
        return "";    
    case LengthTypePercentage:
        return "%";
    case LengthTypeEMS:
        return "em";
    case LengthTypeEXS:
        return "ex";
    case LengthTypePX:
        return "px";
    case LengthTypeCM:
        return "cm";
    case LengthTypeMM:
        return "mm";
    case LengthTypeIN:
        return "in";
    case LengthTypePT:
        return "pt";
    case LengthTypePC:
        return "pc";
    }

    ASSERT_NOT_REACHED();
    return String();
}

inline SVGLengthType stringToLengthType(const UChar*& ptr, const UChar* end)
{
    if (ptr == end)
        return LengthTypeNumber;

    const UChar firstChar = *ptr;

    if (++ptr == end)
        return firstChar == '%' ? LengthTypePercentage : LengthTypeUnknown;

    const UChar secondChar = *ptr;

    if (++ptr != end)
        return LengthTypeUnknown;

    if (firstChar == 'e' && secondChar == 'm')
        return LengthTypeEMS;
    if (firstChar == 'e' && secondChar == 'x')
        return LengthTypeEXS;
    if (firstChar == 'p' && secondChar == 'x')
        return LengthTypePX;
    if (firstChar == 'c' && secondChar == 'm')
        return LengthTypeCM;
    if (firstChar == 'm' && secondChar == 'm')
        return LengthTypeMM;
    if (firstChar == 'i' && secondChar == 'n')
        return LengthTypeIN;
    if (firstChar == 'p' && secondChar == 't')
        return LengthTypePT;
    if (firstChar == 'p' && secondChar == 'c')
        return LengthTypePC;

    return LengthTypeUnknown;
}

SVGLength::SVGLength(SVGLengthMode mode, const String& valueAsString)
    : m_valueInSpecifiedUnits(0)
    , m_unit(storeUnit(mode, LengthTypeNumber))
{
    ExceptionCode ec = 0;
    setValueAsString(valueAsString, ec);
}

SVGLength::SVGLength(const SVGLength& other)
    : m_valueInSpecifiedUnits(other.m_valueInSpecifiedUnits)
    , m_unit(other.m_unit)
{
}

bool SVGLength::operator==(const SVGLength& other) const
{
    return m_unit == other.m_unit
        && m_valueInSpecifiedUnits == other.m_valueInSpecifiedUnits;
}

bool SVGLength::operator!=(const SVGLength& other) const
{
    return !operator==(other);
}

SVGLengthType SVGLength::unitType() const
{
    return extractType(m_unit);
}

float SVGLength::value(const SVGElement* context) const
{
    ExceptionCode ec = 0;
    return value(context, ec);
}

float SVGLength::value(const SVGElement* context, ExceptionCode& ec) const
{
    switch (extractType(m_unit)) {
    case LengthTypeUnknown:
        ec = NOT_SUPPORTED_ERR;
        return 0;
    case LengthTypeNumber:
        return m_valueInSpecifiedUnits;
    case LengthTypePercentage:
        return convertValueFromPercentageToUserUnits(m_valueInSpecifiedUnits / 100, context, ec);
    case LengthTypeEMS:
        return convertValueFromEMSToUserUnits(m_valueInSpecifiedUnits, context, ec);
    case LengthTypeEXS:
        return convertValueFromEXSToUserUnits(m_valueInSpecifiedUnits, context, ec);
    case LengthTypePX:
        return m_valueInSpecifiedUnits;
    case LengthTypeCM:
        return m_valueInSpecifiedUnits / 2.54f * cssPixelsPerInch;
    case LengthTypeMM:
        return m_valueInSpecifiedUnits / 25.4f * cssPixelsPerInch;
    case LengthTypeIN:
        return m_valueInSpecifiedUnits * cssPixelsPerInch;
    case LengthTypePT:
        return m_valueInSpecifiedUnits / 72 * cssPixelsPerInch;
    case LengthTypePC:
        return m_valueInSpecifiedUnits / 6 * cssPixelsPerInch;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

void SVGLength::setValue(float value, const SVGElement* context, ExceptionCode& ec)
{
    switch (extractType(m_unit)) {
    case LengthTypeUnknown:
        ec = NOT_SUPPORTED_ERR;
        break;
    case LengthTypeNumber:
        m_valueInSpecifiedUnits = value;
        break;
    case LengthTypePercentage: {
        float result = convertValueFromUserUnitsToPercentage(value, context, ec);
        if (!ec)
            m_valueInSpecifiedUnits = result; 
        break;
    }
    case LengthTypeEMS: {
        float result = convertValueFromUserUnitsToEMS(value, context, ec);
        if (!ec)
            m_valueInSpecifiedUnits = result;
        break;
    }
    case LengthTypeEXS: {
        float result = convertValueFromUserUnitsToEXS(value, context, ec);
        if (!ec)
            m_valueInSpecifiedUnits = result; 
        break;
    }
    case LengthTypePX:
        m_valueInSpecifiedUnits = value;
        break;
    case LengthTypeCM:
        m_valueInSpecifiedUnits = value * 2.54f / cssPixelsPerInch;
        break;
    case LengthTypeMM:
        m_valueInSpecifiedUnits = value * 25.4f / cssPixelsPerInch;
        break;
    case LengthTypeIN:
        m_valueInSpecifiedUnits = value / cssPixelsPerInch;
        break;
    case LengthTypePT:
        m_valueInSpecifiedUnits = value * 72 / cssPixelsPerInch;
        break;
    case LengthTypePC:
        m_valueInSpecifiedUnits = value * 6 / cssPixelsPerInch;
        break;
    }
}

float SVGLength::valueAsPercentage() const
{
    // 100% = 100.0 instead of 1.0 for historical reasons, this could eventually be changed
    if (extractType(m_unit) == LengthTypePercentage)
        return m_valueInSpecifiedUnits / 100;

    return m_valueInSpecifiedUnits;
}

void SVGLength::setValueAsString(const String& string, ExceptionCode& ec)
{
    if (string.isEmpty())
        return;

    float convertedNumber = 0;
    const UChar* ptr = string.characters();
    const UChar* end = ptr + string.length();

    if (!parseNumber(ptr, end, convertedNumber, false)) {
        ec = SYNTAX_ERR;
        return;
    }

    SVGLengthType type = stringToLengthType(ptr, end);
    ASSERT(ptr <= end);
    if (type == LengthTypeUnknown) {
        ec = SYNTAX_ERR;
        return;
    }

    m_unit = storeUnit(extractMode(m_unit), type);
    m_valueInSpecifiedUnits = convertedNumber;
}

String SVGLength::valueAsString() const
{
    return makeString(String::number(m_valueInSpecifiedUnits), lengthTypeToString(extractType(m_unit)));
}

void SVGLength::newValueSpecifiedUnits(unsigned short type, float value, ExceptionCode& ec)
{
    if (type == LengthTypeUnknown || type > LengthTypePC) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    m_unit = storeUnit(extractMode(m_unit), static_cast<SVGLengthType>(type));
    m_valueInSpecifiedUnits = value;
}

void SVGLength::convertToSpecifiedUnits(unsigned short type, const SVGElement* context, ExceptionCode& ec)
{
    if (type == LengthTypeUnknown || type > LengthTypePC) {
        ec = NOT_SUPPORTED_ERR;
        return;
    }

    float valueInUserUnits = value(context, ec);
    if (ec)
        return;

    unsigned int originalUnitAndType = m_unit;    
    m_unit = storeUnit(extractMode(m_unit), static_cast<SVGLengthType>(type));
    setValue(valueInUserUnits, context, ec);
    if (!ec)
        return;

    // Eventually restore old unit and type
    m_unit = originalUnitAndType;
}

bool SVGLength::determineViewport(const SVGElement* context, float& width, float& height) const
{
    if (!context)
        return false;

    // Take size from outermost <svg> element.
    Document* document = context->document();
    if (document->documentElement() == context) {
        if (RenderView* view = toRenderView(document->renderer())) {
            width = view->viewWidth();
            height = view->viewHeight();
            return true;
        }

        return false;
    }

    // Resolve value against nearest viewport element (common case: inner <svg> elements)
    SVGElement* viewportElement = context->viewportElement();
    if (viewportElement && viewportElement->isSVG()) {
        const SVGSVGElement* svg = static_cast<const SVGSVGElement*>(viewportElement);
        if (svg->hasAttribute(SVGNames::viewBoxAttr)) {
            width = svg->viewBox().width();
            height = svg->viewBox().height();
        } else {
            width = svg->width().value(svg);
            height = svg->height().value(svg);
        }

        return true;
    }
    
    // Resolve value against enclosing non-SVG RenderBox
    if (!context->parentNode() || context->parentNode()->isSVGElement())
        return false;

    RenderObject* renderer = context->renderer();
    if (!renderer || !renderer->isBox())
        return false;

    RenderBox* box = toRenderBox(renderer);
    width = box->width();
    height = box->height();
    return true;
}

float SVGLength::convertValueFromUserUnitsToPercentage(float value, const SVGElement* context, ExceptionCode& ec) const
{
    float width = 0;
    float height = 0;
    if (!determineViewport(context, width, height)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    switch (extractMode(m_unit)) {
    case LengthModeWidth:
        return value / width * 100;
    case LengthModeHeight:
        return value / height * 100;
    case LengthModeOther:
        return value / (sqrtf((width * width + height * height) / 2)) * 100;
    };

    ASSERT_NOT_REACHED();
    return 0;
}

float SVGLength::convertValueFromPercentageToUserUnits(float value, const SVGElement* context, ExceptionCode& ec) const
{
    float width = 0;
    float height = 0;
    if (!determineViewport(context, width, height)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    switch (extractMode(m_unit)) {
    case LengthModeWidth:
        return value * width;
    case LengthModeHeight:
        return value * height;
    case LengthModeOther:
        return value * sqrtf((width * width + height * height) / 2);
    };

    ASSERT_NOT_REACHED();
    return 0;
}

float SVGLength::convertValueFromUserUnitsToEMS(float value, const SVGElement* context, ExceptionCode& ec) const
{
    if (!context || !context->renderer() || !context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = context->renderer()->style();
    float fontSize = style->fontSize();
    if (!fontSize) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    return value / fontSize;
}

float SVGLength::convertValueFromEMSToUserUnits(float value, const SVGElement* context, ExceptionCode& ec) const
{
    if (!context || !context->renderer() || !context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = context->renderer()->style();
    return value * style->fontSize();
}

float SVGLength::convertValueFromUserUnitsToEXS(float value, const SVGElement* context, ExceptionCode& ec) const
{
    if (!context || !context->renderer() || !context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = context->renderer()->style();

    // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
    // if this causes problems in real world cases maybe it would be best to remove this
    float xHeight = ceilf(style->fontMetrics().xHeight());
    if (!xHeight) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    return value / xHeight;
}

float SVGLength::convertValueFromEXSToUserUnits(float value, const SVGElement* context, ExceptionCode& ec) const
{
    if (!context || !context->renderer() || !context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = context->renderer()->style();
    // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
    // if this causes problems in real world cases maybe it would be best to remove this
    return value * ceilf(style->fontMetrics().xHeight());
}

SVGLength SVGLength::fromCSSPrimitiveValue(CSSPrimitiveValue* value)
{
    ASSERT(value);

    SVGLengthType svgType;
    switch (value->primitiveType()) {
    case CSSPrimitiveValue::CSS_NUMBER:
        svgType = LengthTypeNumber;
        break;
    case CSSPrimitiveValue::CSS_PERCENTAGE:
        svgType = LengthTypePercentage;
        break;
    case CSSPrimitiveValue::CSS_EMS:
        svgType = LengthTypeEMS;
        break;
    case CSSPrimitiveValue::CSS_EXS:
        svgType = LengthTypeEXS;
        break;
    case CSSPrimitiveValue::CSS_PX:
        svgType = LengthTypePX;
        break;
    case CSSPrimitiveValue::CSS_CM:
        svgType = LengthTypeCM;
        break;
    case CSSPrimitiveValue::CSS_MM:
        svgType = LengthTypeMM;
        break;
    case CSSPrimitiveValue::CSS_IN:
        svgType = LengthTypeIN;
        break;
    case CSSPrimitiveValue::CSS_PT:
        svgType = LengthTypePT;
        break;
    case CSSPrimitiveValue::CSS_PC:
        svgType = LengthTypePC;
        break;
    case CSSPrimitiveValue::CSS_UNKNOWN:
    default:
        svgType = LengthTypeUnknown;
        break;
    };

    if (svgType == LengthTypeUnknown)
        return SVGLength();

    ExceptionCode ec = 0;
    SVGLength length;
    length.newValueSpecifiedUnits(svgType, value->getFloatValue(), ec);
    if (ec)    
        return SVGLength();

    return length;
}

PassRefPtr<CSSPrimitiveValue> SVGLength::toCSSPrimitiveValue(const SVGLength& length)
{
    CSSPrimitiveValue::UnitTypes cssType = CSSPrimitiveValue::CSS_UNKNOWN;
    switch (length.unitType()) {
    case LengthTypeUnknown:
        break;
    case LengthTypeNumber:
        cssType = CSSPrimitiveValue::CSS_NUMBER;
        break;
    case LengthTypePercentage:
        cssType = CSSPrimitiveValue::CSS_PERCENTAGE;
        break;
    case LengthTypeEMS:
        cssType = CSSPrimitiveValue::CSS_EMS;
        break;
    case LengthTypeEXS:
        cssType = CSSPrimitiveValue::CSS_EXS;
        break;
    case LengthTypePX:
        cssType = CSSPrimitiveValue::CSS_PX;
        break;
    case LengthTypeCM:
        cssType = CSSPrimitiveValue::CSS_CM;
        break;
    case LengthTypeMM:
        cssType = CSSPrimitiveValue::CSS_MM;
        break;
    case LengthTypeIN:
        cssType = CSSPrimitiveValue::CSS_IN;
        break;
    case LengthTypePT:
        cssType = CSSPrimitiveValue::CSS_PT;
        break;
    case LengthTypePC:
        cssType = CSSPrimitiveValue::CSS_PC;
        break;
    };

    return CSSPrimitiveValue::create(length.valueInSpecifiedUnits(), cssType);
}

}

#endif
